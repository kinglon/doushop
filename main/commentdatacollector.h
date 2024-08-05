#ifndef COMMENTDATACOLLECTOR_H
#define COMMENTDATACOLLECTOR_H

#include "datacollector.h"

class CommentDataCollector : public DataCollector
{
    Q_OBJECT

public:
    explicit CommentDataCollector(QObject *parent = nullptr);

protected:
    // 准备获取数据的请求对象
    virtual void prepareGetData1Request(QNetworkRequest& request) override;

    // 解析最终数据
    virtual void parseData1Array(const QJsonArray& dataJson, QVector<QVector<QString>>& datas) override;
};

#endif // COMMENTDATACOLLECTOR_H
