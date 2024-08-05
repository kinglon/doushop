#ifndef DATACOLLECTOR_H
#define DATACOLLECTOR_H

#include <QObject>
#include <QVector>
#include <QNetworkReply>
#include <QTimer>
#include <QNetworkAccessManager>
#include "datamodel.h"
#include "collectstatusmanager.h"

// 采集失败原因
#define COLLECT_SUCCESS                 0   // 采集成功，没有数据
#define COLLECT_SUCCESS_MORE_DATA       1   // 采集成功，还有数据
#define COLLECT_ERROR                   2
#define COLLECT_ERROR_NOT_LOGIN         3  // 未登录
#define COLLECT_ERROR_CONNECTION_FAILED 4  // 连接失败

// 采集步骤
#define COLLECT_STEP_INIT               0
#define COLLECT_STEP_GET_DATA1          1

class DataCollector : public QObject
{
    Q_OBJECT
public:
    explicit DataCollector(QObject *parent = nullptr);

public:
    void setTask(const CollectTaskItem& task, const Shop& shop, int page);

    // 设置网络超时，单位秒
    void setNetworkTimeout(int timeout) { m_networkTimeout = timeout;}

    bool run();

    QVector<QVector<QString>>& getDataModel() { return m_dataModel; }

    static QByteArray intArrayToByteArray(int datas[], int size);

protected:
    virtual void httpGetData1() = 0;

    void addCommonHeader(QNetworkRequest& request);

    void processHttpReply1(QNetworkReply *reply);

    bool parseData1(const QJsonObject& dataJson);

    // 解析最终数据
    virtual void parseData1Array(const QJsonValue& dataJson, QVector<QVector<QString>>& datas) = 0;

private slots:
    void onHttpFinished(QNetworkReply *reply);

signals:
    // 运行结束
    void runFinish(int errorCode);

protected:
    CollectTaskItem m_task;

    int m_page = 0;

    int m_pageSize = 20;

    Shop m_shop;

    static QNetworkAccessManager *m_networkAccessManager;

private:
    QVector<QVector<QString>> m_dataModel;

    int m_retryCount = 0;

    int m_currentStep = COLLECT_STEP_INIT;    

    // 网络请求超时时长，单位秒
    int m_networkTimeout = 5;
};

#endif // DATACOLLECTOR_H
