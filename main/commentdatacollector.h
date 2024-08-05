﻿#ifndef COMMENTDATACOLLECTOR_H
#define COMMENTDATACOLLECTOR_H

#include "datacollector.h"

class CommentDataCollector : public DataCollector
{
    Q_OBJECT

public:
    explicit CommentDataCollector(QObject *parent = nullptr);

protected:
    virtual void httpGetData1() override;

    // 解析最终数据
    virtual void parseData1Array(const QJsonValue& dataJson, QVector<QVector<QString>>& datas) override;
};

#endif // COMMENTDATACOLLECTOR_H
