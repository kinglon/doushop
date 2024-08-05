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
};

#endif // AFTERSELLDATACOLLECTOR_H
