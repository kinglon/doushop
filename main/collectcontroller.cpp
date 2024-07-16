#include "collectcontroller.h"
#include "collectstatusmanager.h"
#include "shopmanager.h"
#include "datacollector.h"
#include "Utility/ImPath.h"

#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"

using namespace QXlsx;

CollectController::CollectController(QObject *parent)
    : QObject{parent}
{
    // 异步调用
    connect(this, &CollectController::collectNextTask, this, &CollectController::onCollectNextTask, Qt::QueuedConnection);
}

void CollectController::run()
{
    emit collectNextTask();
}

bool CollectController::saveCollectResult()
{
    // 拷贝默认采集结果输出表格到保存目录
    QString excelFileName = QString::fromWCharArray(L"采集结果.xlsx");
    QString srcExcelFilePath = QString::fromStdWString(CImPath::GetConfPath()) + excelFileName;
    QString destExcelFileName = QDateTime::currentDateTime().toString("yyyyMMdd_hhmm.xlsx");
    QString destExcelFilePath = QString::fromStdWString(CImPath::GetDataPath()) + destExcelFileName;
    ::DeleteFile(destExcelFilePath.toStdWString().c_str());
    if (!::CopyFile(srcExcelFilePath.toStdWString().c_str(), destExcelFilePath.toStdWString().c_str(), TRUE))
    {
        qCritical("failed to copy the result excel file");
        return false;
    }

    // 从第2行开始写
    Document xlsx(destExcelFilePath);
    if (!xlsx.load())
    {
        qCritical("failed to load the result excel file");
        return false;
    }

    auto& datas = CollectStatusManager::getInstance()->getCollectDatas();
    int row = 2;
    for (const auto& data : datas)
    {
        xlsx.write(row, 1, data.m_shopName);
        xlsx.write(row, 2, data.m_orderId);
        xlsx.write(row, 3, data.m_goodsInfo);
        xlsx.write(row, 4, data.m_goodsId);
        xlsx.write(row, 5, data.m_commentTime);
        xlsx.write(row, 6, data.m_commentLevel);
        xlsx.write(row, 7, data.m_commentContent);
        row++;
    }

    if (!xlsx.save())
    {
        qCritical("failed to save the result excel file");
        return false;
    }

    return true;
}

void CollectController::onPrintLog(QString content)
{
    emit printLog(content);
}

void CollectController::onCollectNextTask()
{
    CollectTaskItem task = CollectStatusManager::getInstance()->getNextTask();
    if (task.m_shopId.isEmpty())
    {
        emit runFinish(true);
        return;
    }

    Shop* shop = ShopManager::getInstance()->getShopById(task.m_shopId);
    if (shop == nullptr)
    {
        emit printLog(QString::fromWCharArray(L"采集的店铺已被删除"));
        emit runFinish(false);
        return;
    }

    int nextPageIndex = CollectStatusManager::getInstance()->getNextPageIndex();
    QString logContent = QString::fromWCharArray(L"采集店铺%1第%2页").arg(shop->m_name.toStdString().c_str(), nextPageIndex+1);
    emit printLog(logContent);

    DataCollector* collector = new DataCollector(this);
    collector->setTask(task, *shop, nextPageIndex);
    connect(collector, &DataCollector::runFinish, [collector, this](int errorCode) {
        if (errorCode == COLLECT_SUCCESS)
        {
            finishCurrentTask(collector->getDataModel(), false);
        }
        else if (errorCode == COLLECT_SUCCESS_MORE_DATA)
        {
            finishCurrentTask(collector->getDataModel(), true);
        }
        else if (errorCode == COLLECT_ERROR_NOT_LOGIN)
        {
            emit printLog(QString::fromWCharArray(L"登录已过期，请重新登录"));
            emit runFinish(false);
        }
        else
        {
            emit printLog(QString::fromWCharArray(L"采集失败"));
            emit runFinish(false);
        }
        collector->deleteLater();
    });
    collector->run();
}

void CollectController::finishCurrentTask(const QVector<Comment>& dataModel, bool hasMoreData)
{
    if (hasMoreData)
    {
        CollectStatusManager::getInstance()->finishCurrentPage(dataModel);
    }
    else
    {
        CollectStatusManager::getInstance()->finishCurrentTask(dataModel);
    }

    if (CollectStatusManager::getInstance()->isFinish())
    {
        emit runFinish(true);
    }
    else
    {
        // 异步调用
        emit collectNextTask();
    }
}
