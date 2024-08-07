#ifndef AFTERSELLDATACOLLECTOR_H
#define AFTERSELLDATACOLLECTOR_H

#include "datacollector.h"

class AfterSellDataCollector : public DataCollector
{
public:
    explicit AfterSellDataCollector(QObject *parent = nullptr);

protected:
    virtual void httpGetData1() override;

    // 解析最终数据
    virtual void parseData1Array(const QJsonValue& dataJson, QVector<QVector<QString>>& datas) override;

    virtual bool doGetData2() override;

    // 获取下一个快递单号
    void getNextDeliveryId();

    // 发送请求获取快递单号
    void sendDeliveryHttpRequest();

    virtual void processHttpReply2(QNetworkReply *reply) override;

    void getDeliveryIdFinish(bool ok);

private:
    // 售后单ID，与最终输出data是对应的
    QVector<QString> m_afterSaleIds;

    int m_nextIndex = 0;
};

#endif // AFTERSELLDATACOLLECTOR_H
