#ifndef DATACOLLECTOR_H
#define DATACOLLECTOR_H

#include <QObject>
#include <QVector>
#include <QNetworkReply>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "collectorbase.h"
#include "datamodel.h"
#include "collectstatusmanager.h"

// 采集步骤
#define COLLECT_STEP_INIT               0
#define COLLECT_STEP_GET_DATA1          1
#define COLLECT_STEP_GET_DATA2          2

class DataCollector : public CollectorBase
{
    Q_OBJECT
public:
    explicit DataCollector(QObject *parent = nullptr);

public:
    void setPage(int page) { m_page = page; }

    virtual bool run() override;

    // 设置网络超时，单位秒
    void setNetworkTimeout(int timeout) { m_networkTimeout = timeout;}

    static QByteArray intArrayToByteArray(int datas[], int size);

protected:
    virtual void httpGetData1() = 0;

    void addCommonHeader(QNetworkRequest& request);

    void processHttpReply1(QNetworkReply *reply);

    bool parseData1(const QJsonObject& dataJson);

    // 解析最终数据
    virtual void parseData1Array(const QJsonValue& dataJson, QVector<QVector<QString>>& datas) = 0;

    // 采集更多数据，返回false表示已采集完毕，返回true表示还有数据需要采集
    virtual bool doGetData2() { return false; }

    virtual void processHttpReply2(QNetworkReply *reply) { (void)reply; }

    void getData2Finish(int result);

private slots:
    void onHttpFinished(QNetworkReply *reply);

protected:
    int m_page = 0;

    int m_pageSize = 20;

    static QNetworkAccessManager *m_networkAccessManager;

    int m_retryCount = 0;

    int m_currentStep = COLLECT_STEP_INIT;

private:

    // 网络请求超时时长，单位秒
    int m_networkTimeout = 5;
};

#endif // DATACOLLECTOR_H
