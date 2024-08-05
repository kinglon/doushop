#include "loginutil.h"
#include "browserwindow.h"
#include "settingmanager.h"
#include "Utility/ImPath.h"
#include <QDir>

// 加载URL最大重试次数
#define LOAD_URL_MAX_RETRY_COUNT 1

// 等待就绪状态最大重试次数
#define WAIT_READY_MAX_RETRY_COUNT 60

// 获取登录信息最大次数
#define GET_LOGIN_INFO_MAX_RETRY_COUNT 20

LoginUtil::LoginUtil(QObject *parent)
    : QObject{parent},
      m_runJsCodeTimer(new QTimer(this)),
      m_stepTimer(new QTimer(this))
{
    connect(BrowserWindow::getInstance(), &BrowserWindow::runJsCodeFinished,
            this, &LoginUtil::onRunJsCodeFinish);

    m_runJsCodeTimer->setInterval(m_runJsCodeTimeout);
    connect(m_runJsCodeTimer, &QTimer::timeout, this, &LoginUtil::onRunJsCodeTimeout);
    connect(m_stepTimer, &QTimer::timeout, this, &LoginUtil::onStepTimeout);
    connect(BrowserWindow::getInstance(), &BrowserWindow::loadFinished, this, &LoginUtil::onLoadUrlFinished);
}

void LoginUtil::run()
{
    if (m_shop.m_id.isEmpty())
    {
        qCritical("the shop id is empty");
        return;
    }

    BrowserWindow::getInstance()->setWebViewSize(QSize(CSettingManager::GetInstance()->m_browserWidth,
                                                       CSettingManager::GetInstance()->m_browserHeight));
    BrowserWindow::getInstance()->showMaximized();
    BrowserWindow::getInstance()->setProfileName(m_shop.m_id);

    m_currentStep = STEP_LOADURL;
    m_stepRetryCount = 0;
    doStepLoadUrl();
}

bool LoginUtil::runJsCode(const QString& jsCode)
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

bool LoginUtil::runJsCodeFile(const QString& jsFileName)
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

void LoginUtil::runJsCodeFinish(bool ok, const QMap<QString, QString>& result)
{
    if (!ok)
    {
        return;
    }

    QString fun;
    if (result.contains("fun"))
    {
        fun = result["fun"];
    }
    if (fun.isEmpty())
    {
        qCritical("js result not have fun");
        return;
    }

    if (m_currentStep == STEP_WAIT_READY)
    {
        if (isReady(result))
        {
            stepWaitReadyFinish(true);
        }
    }
    else if (m_currentStep == STEP_GET_LOGIN_INFO)
    {
        if (fun == "get_login_info")
        {
            if (result.contains("token"))
            {
                m_shop.m_token = result["token"];
            }

            if (result.contains("cookie"))
            {
                m_shop.m_cookies = result["cookie"];
            }

            m_shop.m_aid = "4272";
            m_shop.m_bid = "ffa_aftersale";
            m_shop.m_platformSource = "fxg";
        }

        if (m_shop.isLogin())
        {            
            stepGetLoginInfoFinish(true);
        }
    }
}

void LoginUtil::onRunJsCodeFinish(const QVariant& result)
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

void LoginUtil::onRunJsCodeTimeout()
{
    qCritical("running js code time out");
    m_runJsCodeTimer->stop();
    m_currentJsSessionId = 0;
    runJsCodeFinish(false, QMap<QString, QString>());
}

bool LoginUtil::isReady(const QMap<QString, QString>& result)
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
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    return false;
}

void LoginUtil::doStepLoadUrl()
{
    m_stepTimer->setInterval(20000);
    m_stepTimer->start();
    QString url = "https://fxg.jinritemai.com/login/common?extra=%7B%22target_url%22%3A%22https%3A%2F%2Ffxg.jinritemai.com%2Fffa%2Fmshop%2Fhomepage%2Findex%22%7D";
    BrowserWindow::getInstance()->load(QUrl(url));
}

void LoginUtil::stepLoadUrlFinish(bool ok)
{
    m_stepTimer->stop();
    if (ok)
    {
        m_currentStep = STEP_WAIT_READY;
        m_stepRetryCount = 0;
        emit printLog(QString::fromWCharArray(L"等待登录"));
        doStepWaitReady();
    }
    else
    {
        emit printLog(QString::fromWCharArray(L"加载链接失败"));
        emit runFinish(false);
    }
}

void LoginUtil::doStepWaitReady()
{
    if (!runJsCodeFile(getWaitReadyJsFile()))
    {
        stepWaitReadyFinish(false);
        return;
    }

    m_stepTimer->setInterval(2000);
    m_stepTimer->start();
}

void LoginUtil::stepWaitReadyFinish(bool ok)
{
    m_stepTimer->stop();

    if (!ok)
    {        
        emit printLog(QString::fromWCharArray(L"登录超时"));
        runFinish(false);
    }
    else
    {
        emit printLog(QString::fromWCharArray(L"获取登录信息"));
        m_currentStep = STEP_GET_LOGIN_INFO;
        m_stepRetryCount = 0;
        doStepGetLoginInfo();
    }
}

void LoginUtil::doStepGetLoginInfo()
{
    if (!runJsCodeFile("doushop_get_login_info"))
    {
        stepGetLoginInfoFinish(false);
        return;
    }

    m_stepTimer->setInterval(1000);
    m_stepTimer->start();
}

void LoginUtil::onLoadUrlFinished(bool ok)
{
    onSubClassLoadUrlFinished(ok);

    if (m_currentStep == STEP_LOADURL)
    {
        stepLoadUrlFinish(ok);
    }
}

void LoginUtil::onStepTimeout()
{
    m_stepTimer->stop();

    if (m_currentStep == STEP_LOADURL)
    {
        if (m_stepRetryCount < LOAD_URL_MAX_RETRY_COUNT)
        {
            emit printLog(QString::fromWCharArray(L"加载链接超时，重试"));
            doStepLoadUrl();
            m_stepRetryCount++;
        }
        else
        {
            stepLoadUrlFinish(false);
        }
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
            stepWaitReadyFinish(false);
        }
    }
    else if (m_currentStep == STEP_GET_LOGIN_INFO)
    {
        if (m_stepRetryCount < GET_LOGIN_INFO_MAX_RETRY_COUNT)
        {
            doStepGetLoginInfo();
            m_stepRetryCount++;
        }
        else
        {
            stepGetLoginInfoFinish(false);
        }
    }
}

void LoginUtil::stepGetLoginInfoFinish(bool ok)
{
    m_stepTimer->stop();
    if (!ok)
    {
        emit printLog(QString::fromWCharArray(L"获取登录信息失败"));
        emit runFinish(false);
    }
    else
    {
        emit runFinish(true);
    }
}
