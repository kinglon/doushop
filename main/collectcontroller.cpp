#include "collectcontroller.h"
#include "collectstatusmanager.h"
#include "shopmanager.h"
#include "commentdatacollector.h"
#include "afterselldatacollector.h"
#include "Utility/ImPath.h"
#include "settingmanager.h"

#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"
#include <QDesktopServices>
#include "paycollector.h"

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

QString CollectController::saveCollectResult()
{
    QString srcExcelFilePath;
    QString destExcelFilePath;
    QString now = QDateTime::currentDateTime().toString("yyyyMMdd_hhmm");
    int taskType = CollectStatusManager::getInstance()->getTaskType();
    if (taskType == TASK_TYPE_COMMENT)
    {
        srcExcelFilePath = QString::fromStdWString(CImPath::GetConfPath()) + QString::fromWCharArray(L"商品评论表格模板.xlsx");
        destExcelFilePath = QString::fromStdWString(CImPath::GetDataPath()) + QString::fromWCharArray(L"商品评论-") + now + ".xlsx";
    }
    else if (taskType == TASK_TYPE_AFTERSELL)
    {
        srcExcelFilePath = QString::fromStdWString(CImPath::GetConfPath()) + QString::fromWCharArray(L"售后单表格模板.xlsx");
        destExcelFilePath = QString::fromStdWString(CImPath::GetDataPath()) + QString::fromWCharArray(L"售后单-") + now + ".xlsx";
    }
    else if (taskType == TASK_TYPE_PAY)
    {
        srcExcelFilePath = QString::fromStdWString(CImPath::GetConfPath()) + QString::fromWCharArray(L"小额打款记录模板.xlsx");
        destExcelFilePath = QString::fromStdWString(CImPath::GetDataPath()) + QString::fromWCharArray(L"小额打款-") + now + ".xlsx";
    }
    else
    {
        qCritical("unknown task type");
        return "";
    }

    // 拷贝模板文件作为初始文件
    ::DeleteFile(destExcelFilePath.toStdWString().c_str());
    if (!::CopyFile(srcExcelFilePath.toStdWString().c_str(), destExcelFilePath.toStdWString().c_str(), TRUE))
    {
        qCritical("failed to copy the result excel file");
        return "";
    }

    // 从第2行开始写
    Document xlsx(destExcelFilePath);
    if (!xlsx.load())
    {
        qCritical("failed to load the result excel file");
        return "";
    }

    auto& datas = CollectStatusManager::getInstance()->getCollectDatas();
    int row = 2;
    for (const auto& data : datas)
    {
        for (int column=1; column<=data.length(); column++)
        {
            xlsx.write(row, column, data[column-1]);
        }
        row++;
    }

    if (!xlsx.save())
    {
        qCritical("failed to save the result excel file");
        return "";
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdWString(CImPath::GetDataPath())));
    return destExcelFilePath;
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
    QString logContent = QString::fromWCharArray(L"采集店铺(%1)第%2页").arg(shop->m_name, QString::number(nextPageIndex+1));
    emit printLog(logContent);

    CollectorBase* collector = nullptr;
    if (task.m_type == TASK_TYPE_COMMENT)
    {
        collector = new CommentDataCollector(this);
        ((DataCollector*)collector)->setPage(nextPageIndex);
    }
    else if (task.m_type == TASK_TYPE_AFTERSELL)
    {
        collector = new AfterSellDataCollector(this);
        ((DataCollector*)collector)->setPage(nextPageIndex);
    }
    else if (task.m_type == TASK_TYPE_PAY)
    {
        collector = new PayCollector(this);
    }
    else
    {
        emit printLog(QString::fromWCharArray(L"采集的数据类型不支持"));
        emit runFinish(false);
        return;
    }

    collector->setTask(task, *shop);
    connect(collector, &CollectorBase::collectLog, this, &CollectController::onPrintLog);
    connect(collector, &CollectorBase::runFinish, [collector, this](int errorCode) {
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

void CollectController::finishCurrentTask(const QVector<QVector<QString>>& dataModel, bool hasMoreData)
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
        QTimer::singleShot(CSettingManager::GetInstance()->m_request_interval_ms, [this] () {
            emit collectNextTask();
        });
    }
}
