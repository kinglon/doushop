#ifndef PAYCOLLECTOR_H
#define PAYCOLLECTOR_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QWebEngineUrlRequestInterceptor>
#include "collectorbase.h"

// 步骤
#define STEP_INIT           0
#define STEP_LOADURL        1
#define STEP_WAIT_READY     2
#define STEP_EXPORT_DATA    3
#define STEP_DOWNLOAD_DATA  4
#define STEP_FINISH         5

class PayRequestInterceptor : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT

public:
    PayRequestInterceptor(QObject *parent) : QWebEngineUrlRequestInterceptor(parent) {}

    void interceptRequest(QWebEngineUrlRequestInfo &info);

public:
    QString m_downloadUrl;
};


class PayCollector : public CollectorBase
{
    Q_OBJECT

    friend class PayRequestInterceptor;

public:
    explicit PayCollector(QObject *parent = nullptr);

public:
    // 运行
    virtual bool run() override;


private slots:
    void onRunJsCodeFinish(const QVariant& result);

    void onRunJsCodeTimeout();

    void onStepTimeout();

    void onLoadUrlFinished(bool ok);

    void onDownloadFinish(bool success, std::string* data);

protected:
    // 执行JS脚本
    bool runJsCode(const QString& jsCode);

    // 执行JS脚本文件
    bool runJsCodeFile(const QString& jsFileName);

    // JS脚本执行结果
    virtual void runJsCodeFinish(bool ok, const QMap<QString, QString>& result);

    // 子类重载，URL加载完成后除非
    virtual void onSubClassLoadUrlFinished(bool ok) {(void)ok;}

    // 获取等待准备就绪的JS脚本文件
    virtual QString getWaitReadyJsFile() { return "pay_check_ready"; }

    // 检查是否已准备就绪
    // result, JS代码运行的结果
    // validLink, 标志是否未有效链接
    // return， true表示就绪，就绪结果要看validLink
    virtual bool isReady(const QMap<QString, QString>& result, bool& validLink);

protected:
    void doStepLoadUrl();

    void stepLoadUrlFinish(bool ok);

    void doStepWaitReady();

    void stepWaitReadyFinish(bool ok, int errorCode);

    void doStepExportData();

    void stepExportDataFinish();

    void doStepDownloadData();

    void stepDownloadDataFinish(bool ok, std::string* data);

protected:
    // 当前采集步骤
    int m_currentStep = STEP_INIT;

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

    QString m_downloadUrl;

    static PayRequestInterceptor* m_requestInterceptor;
};

#endif // PAYCOLLECTOR_H
