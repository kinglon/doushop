#include "paycollector.h"
#include "browserwindow.h"
#include "settingmanager.h"
#include "Utility/ImPath.h"
#include <QDir>
#include <QBuffer>
#include "filedownloader.h"

#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"

using namespace QXlsx;

// 加载URL最大重试次数
#define LOAD_URL_MAX_RETRY_COUNT 1

// 等待就绪状态最大重试次数
#define WAIT_READY_MAX_RETRY_COUNT 20

// 等待导出数据最大秒数
#define EXPORT_DATA_MAX_SECONDS     120

// 下载重试次数
#define DOWNLOAD_DATA_MAX_RETRY_COUNT 1

void PayRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    if (info.resourceType() != QWebEngineUrlRequestInfo::ResourceTypeXhr)
    {
        return;
    }

    QString url = info.requestUrl().toString();
    if (url.indexOf("/shopremit/record/export/tasks") != -1)
    {
        qInfo("exporting paying data");
    }
    else if (url.indexOf("/shopremit/record/export/download") != -1)
    {
        m_downloadUrl = url;
        qInfo("download url: %s", url.toStdString().c_str());
    }
}

PayRequestInterceptor* PayCollector::m_requestInterceptor = nullptr;

PayCollector::PayCollector(QObject *parent)
    : CollectorBase{parent},
      m_runJsCodeTimer(new QTimer(this)),
      m_stepTimer(new QTimer(this))
{
    connect(BrowserWindow::getInstance(), &BrowserWindow::runJsCodeFinished,
            this, &PayCollector::onRunJsCodeFinish);

    m_runJsCodeTimer->setInterval(m_runJsCodeTimeout);
    connect(m_runJsCodeTimer, &QTimer::timeout, this, &PayCollector::onRunJsCodeTimeout);
    connect(m_stepTimer, &QTimer::timeout, this, &PayCollector::onStepTimeout);
    connect(BrowserWindow::getInstance(), &BrowserWindow::loadFinished, this, &PayCollector::onLoadUrlFinished);
}

bool PayCollector::run()
{
    if (m_requestInterceptor == nullptr)
    {
        m_requestInterceptor = new PayRequestInterceptor(nullptr);
        BrowserWindow::getInstance()->setRequestInterceptor(m_requestInterceptor);
    }
    BrowserWindow::getInstance()->setProfileName(m_shop.m_id);

    m_currentStep = STEP_LOADURL;
    m_stepRetryCount = 0;
    emit collectLog(QString::fromWCharArray(L"加载小额打款页面"));
    doStepLoadUrl();
    return true;
}

bool PayCollector::runJsCode(const QString& jsCode)
{
    if (jsCode.isEmpty())
    {
        qCritical("the js code is empty");
        return false;
    }

    // 上一次还没执行完，打下日志
    if (m_currentJsSessionId > 0)
    {
        qWarning("the last time of running js code not finish");
        m_runJsCodeTimer->stop();
    }

    // 返回结果变量固定为jsResult，把id添加到返回的结果，用于匹配
    static int sessionId = 1;
    ++sessionId;
    m_currentJsSessionId = sessionId;
    QString sessionJsCode = QString("; jsResult['sessionId']='%1'; jsResult;").arg(m_currentJsSessionId);
    QString newJsCode = jsCode + sessionJsCode;
    BrowserWindow::getInstance()->runJsCode(newJsCode);
    m_runJsCodeTimer->start();

    return true;
}

bool PayCollector::runJsCodeFile(const QString& jsFileName)
{
    static QMap<QString, QString> jsCodes; // 缓存JS代码
    QString jsCode;
    if (!CSettingManager::GetInstance()->m_cacheJsCode || !jsCodes.contains(jsFileName))
    {
        // 从文件加载
        std::wstring jsFilePath = CImPath::GetSoftInstallPath() + L"\\Js\\" + jsFileName.toStdWString();
        QFile file(QString::fromStdWString(jsFilePath));
        if (file.open(QIODevice::ReadOnly))
        {
            QByteArray jsonData = file.readAll();
            file.close();
            jsCode = QString::fromUtf8(jsonData);
        }
    }
    else
    {
        jsCode = jsCodes[jsFileName];
    }

    if (jsCode.isEmpty())
    {
        qCritical("the js file (%s) not have content", jsFileName.toStdString().c_str());
        return false;
    }

    if (CSettingManager::GetInstance()->m_cacheJsCode)
    {
        jsCodes[jsFileName] = jsCode;
    }

    return runJsCode(jsCode);
}

void PayCollector::runJsCodeFinish(bool ok, const QMap<QString, QString>& result)
{
    if (m_currentStep == STEP_WAIT_READY)
    {
        if (ok && result.contains("ready"))
        {
            if (result["ready"] == "2") // 未登录
            {
                stepWaitReadyFinish(false, COLLECT_ERROR_NOT_LOGIN);
            }
            else if (result["ready"] == "1") // 就绪
            {
                stepWaitReadyFinish(true, COLLECT_SUCCESS);
            }
        }
    }
}

void PayCollector::onRunJsCodeFinish(const QVariant& result)
{
    if (m_currentJsSessionId == 0)
    {
        return;
    }

    if (result.type() != QVariant::Map)
    {
        qCritical("the js result is not a map");
        return;
    }

    QMap map = result.toMap();
    if (!map.contains("sessionId") || map["sessionId"].type() != QVariant::String)
    {
        qCritical("the js result map not have sessionId member");
        return;
    }

    if (map["sessionId"].toString() != QString::number(m_currentJsSessionId))
    {
        qWarning("the session id is old");
        return;
    }

    m_runJsCodeTimer->stop();
    m_currentJsSessionId = 0;

    QMap<QString, QString> newMap;
    QList<QString> keys = map.keys();
    QString jsResultStr;
    for (QString key : keys)
    {
        newMap[key] = map[key].toString();
        if (!jsResultStr.isEmpty())
        {
            jsResultStr += ", ";
        }
        jsResultStr += key + "=" + newMap[key];
    }
    qInfo("js result: %s", jsResultStr.toStdString().c_str());
    runJsCodeFinish(true, newMap);
}

void PayCollector::onRunJsCodeTimeout()
{
    qCritical("running js code time out");
    m_runJsCodeTimer->stop();
    m_currentJsSessionId = 0;
    runJsCodeFinish(false, QMap<QString, QString>());
}

bool PayCollector::isReady(const QMap<QString, QString>& result, bool& validLink)
{
    QString fun;
    if (result.contains("fun"))
    {
        fun = result["fun"];
    }
    if (fun.isEmpty())
    {
        qCritical("js result not have fun");
        return false;
    }

    if (fun == "check_ready")
    {
        if (result.contains("ready"))
        {
            if (result["ready"] == "1") // 就绪
            {
                validLink = true;
                return true;
            }
            else if (result["ready"] == "2") // 无效链接
            {
                validLink = false;
                return true;
            }
        }
    }

    return false;
}

void PayCollector::doStepLoadUrl()
{
    m_stepTimer->setInterval(60000);
    m_stepTimer->start();
    BrowserWindow::getInstance()->load(QUrl("https://fxg.jinritemai.com/ffa/maftersale/aftersale/part-pay?type=record"));
}

void PayCollector::stepLoadUrlFinish(bool ok)
{
    m_stepTimer->stop();
    if (!ok)
    {
        if (m_stepRetryCount < LOAD_URL_MAX_RETRY_COUNT)
        {
            emit collectLog(QString::fromWCharArray(L"加载页面失败，重试"));
            doStepLoadUrl();
            m_stepRetryCount++;
        }
        else
        {
            emit collectLog(QString::fromWCharArray(L"加载页面失败"));
            if (BrowserWindow::getInstance()->getUrl().indexOf("/login/common") != -1)
            {
                emit runFinish(COLLECT_ERROR_NOT_LOGIN);
            }
            else
            {
                emit runFinish(COLLECT_ERROR_CONNECTION_FAILED);
            }
        }
    }
    else
    {
        m_currentStep = STEP_WAIT_READY;
        m_stepRetryCount = 0;
        emit collectLog(QString::fromWCharArray(L"等待页面就绪"));
        doStepWaitReady();
    }
}

void PayCollector::doStepWaitReady()
{
    if (!runJsCodeFile(getWaitReadyJsFile()))
    {
        stepWaitReadyFinish(false, COLLECT_ERROR);
        return;
    }

    m_stepTimer->setInterval(2000);
    m_stepTimer->start();
}

void PayCollector::stepWaitReadyFinish(bool ok, int errorCode)
{
    m_stepTimer->stop();

    if (!ok)
    {
        if (BrowserWindow::getInstance()->getUrl().indexOf("/login/common") != -1)
        {
            emit runFinish(COLLECT_ERROR_NOT_LOGIN);
        }
        else
        {
            emit runFinish(errorCode);
        }
    }
    else
    {
        m_currentStep = STEP_EXPORT_DATA;
        m_stepRetryCount = 0;
        if (m_requestInterceptor)
        {
            m_requestInterceptor->m_downloadUrl = "";
        }
        emit collectLog(QString::fromWCharArray(L"导出数据"));
        doStepExportData();
    }
}

void PayCollector::doStepExportData()
{
    m_stepTimer->setInterval(1000);
    m_stepTimer->start();
}

void PayCollector::stepExportDataFinish()
{
    m_stepTimer->stop();
    if (m_requestInterceptor && !m_requestInterceptor->m_downloadUrl.isEmpty())
    {
        m_downloadUrl = m_requestInterceptor->m_downloadUrl;
        m_currentStep = STEP_DOWNLOAD_DATA;
        m_stepRetryCount = 0;
        emit collectLog(QString::fromWCharArray(L"下载数据"));
        doStepDownloadData();
    }
    else
    {
        if (m_stepRetryCount < EXPORT_DATA_MAX_SECONDS)
        {
            m_stepRetryCount++;
            doStepExportData();
        }
        else
        {
            BrowserWindow::getInstance()->setRequestInterceptor(nullptr);
            emit collectLog(QString::fromWCharArray(L"导出数据超时"));
            emit runFinish(COLLECT_ERROR);
        }
    }
}

void PayCollector::doStepDownloadData()
{
    FileDownloader* fileDownloader = new FileDownloader();
    fileDownloader->setUrl(m_downloadUrl);
    fileDownloader->setCookie(m_shop.m_cookies);
    fileDownloader->setTimeoutSeconds(60);

    connect(fileDownloader, &FileDownloader::downloadFinish, this, &PayCollector::onDownloadFinish);
    connect(fileDownloader, &FileDownloader::finished, fileDownloader, &QObject::deleteLater);
    fileDownloader->start();
}

void PayCollector::stepDownloadDataFinish(bool ok, std::string* data)
{
    if (!ok)
    {
        if (m_stepRetryCount < DOWNLOAD_DATA_MAX_RETRY_COUNT)
        {
            m_stepRetryCount++;
            doStepDownloadData();
        }
        else
        {
            emit collectLog(QString::fromWCharArray(L"下载数据失败"));
            qInfo("download url : %s", m_downloadUrl.toStdString().c_str());
            qInfo("cookie : %s", m_shop.m_cookies.toStdString().c_str());
            emit runFinish(COLLECT_ERROR);
        }

        return;
    }

    // 解析excel数据
    QByteArray byteArray(data->c_str(), static_cast<int>(data->length()));
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::ReadOnly);
    Document excel(&buffer);
    if (!excel.load())
    {
        emit collectLog(QString::fromWCharArray(L"下载数据不是excel表格"));
        emit runFinish(COLLECT_ERROR);
        return;
    }

    CellRange excelRange = excel.dimension();
    for (int row=2; row <= excelRange.lastRow(); row++)
    {
        QVector<QString> rowContents;

        // 店铺名字
        rowContents.push_back(m_shop.m_name);

        // 订单号
        Cell* cell = excel.cellAt(row, 2);
        if (cell)
        {
            QVariant rawValue = cell->value();
            QString value = rawValue.toString();
            if (rawValue.type() != QVariant::String)
            {
                value = cell->readValue().toString();
            }
            rowContents.push_back(value);
        }
        else
        {
            rowContents.push_back("");
        }

        // 从第5列开始，全部获取
        for (int column=5; column <=excelRange.lastColumn(); column++)
        {
            Cell* cell = excel.cellAt(row, column);
            if (cell)
            {
                QVariant rawValue = cell->value();
                QString value = rawValue.toString();
                if (rawValue.type() != QVariant::String)
                {
                    value = cell->readValue().toString();
                }
                rowContents.push_back(value);
            }
            else
            {
                rowContents.push_back("");
            }
        }

        // 筛选满足打款申请时间的数据
        if (rowContents.length() < 6)
        {
            continue;
        }
        QString applyTime = rowContents[5];
        QDateTime dateTime = QDateTime::fromString(applyTime, "yyyy-MM-dd HH:mm:ss");
        if (!dateTime.isValid())
        {
            qCritical("the apply time (%s) is invalid", applyTime.toStdString().c_str());
            continue;
        }
        qint64 utcTime = dateTime.toSecsSinceEpoch();
        if (utcTime >= m_task.m_beginTime && utcTime <= m_task.m_endTime)
        {
            m_dataModel.push_back(rowContents);
        }
    }

    emit runFinish(COLLECT_SUCCESS);
}

void PayCollector::onLoadUrlFinished(bool ok)
{
    onSubClassLoadUrlFinished(ok);

    if (m_currentStep == STEP_LOADURL)
    {
        stepLoadUrlFinish(ok);
    }
}

void PayCollector::onDownloadFinish(bool success, std::string* data)
{
    stepDownloadDataFinish(success, data);
    if (success)
    {
        delete data;
    }
}

void PayCollector::onStepTimeout()
{
    m_stepTimer->stop();

    if (m_currentStep == STEP_LOADURL)
    {
        stepLoadUrlFinish(false);
    }
    else if (m_currentStep == STEP_WAIT_READY)
    {
        if (m_stepRetryCount < WAIT_READY_MAX_RETRY_COUNT)
        {
            doStepWaitReady();
            m_stepRetryCount++;
        }
        else
        {
            stepWaitReadyFinish(false, COLLECT_ERROR);
        }
    }
    else if (m_currentStep == STEP_EXPORT_DATA)
    {
        stepExportDataFinish();
    }
}
