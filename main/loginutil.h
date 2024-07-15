#ifndef LOGINUTIL_H
#define LOGINUTIL_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include "datamodel.h"

// 步骤
#define STEP_INIT           0
#define STEP_LOADURL        1
#define STEP_WAIT_READY     2
#define STEP_GET_LOGIN_INFO 3
#define STEP_FINISH         4

// 失败原因
#define LOGIN_UTIL_ERROR                   1

class LoginUtil : public QObject
{
    Q_OBJECT
public:
    explicit LoginUtil(QObject *parent = nullptr);

public:
    void setShopId(QString shopId) { m_shop.m_id = shopId; }

    Shop& getShop() { return m_shop;}

    // 运行
    void run();

    // 获取失败原因
    int getError() { return m_error; }

signals:
    // 运行结束
    void runFinish(bool ok);

    // 打印日志
    void printLog(const QString& log);

private slots:
    void onRunJsCodeFinish(const QVariant& result);

    void onRunJsCodeTimeout();

    void onStepTimeout();

    void onLoadUrlFinished(bool ok);

protected:
    // 执行JS脚本
    bool runJsCode(const QString& jsCode);

    // 执行JS脚本文件
    bool runJsCodeFile(const QString& jsFileName);

    // JS脚本执行结果
    virtual void runJsCodeFinish(bool ok, const QMap<QString, QString>& result);

    // 子类重载，URL加载完成后调用
    virtual void onSubClassLoadUrlFinished(bool ok) {(void)ok;}

    // 获取等待准备就绪的JS脚本文件
    virtual QString getWaitReadyJsFile() { return "doushop_check_ready"; }

    // 检查是否已准备就绪
    // result, JS代码运行的结果
    virtual bool isReady(const QMap<QString, QString>& result);

protected:
    void doStepLoadUrl();

    void stepLoadUrlFinish(bool ok);

    void doStepWaitReady();

    void stepWaitReadyFinish(bool ok);    

    void doStepGetLoginInfo();

    void stepGetLoginInfoFinish(bool ok);

protected:
    Shop m_shop;

    // 当前采集步骤
    int m_currentStep = STEP_INIT;

    // 采集失败原因
    int m_error = LOGIN_UTIL_ERROR;

private:
    // 当前JS执行会话ID，0表示没有JS在执行
    int m_currentJsSessionId = 0;

    // JS执行超时定时器
    QTimer* m_runJsCodeTimer = nullptr;

    // JS执行超时毫秒数
    int m_runJsCodeTimeout = 5000;    

    // 步骤定时器，用于控制超时
    QTimer* m_stepTimer = nullptr;

    // 步骤失败重试次数
    int m_stepRetryCount = 0;
};

#endif // LOGINUTIL_H
