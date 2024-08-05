#include "commentdatacollector.h"

#include <QJsonArray>
#include <QJsonObject>

CommentDataCollector::CommentDataCollector(QObject *parent)
    : DataCollector{parent}
{

}

void CommentDataCollector::httpGetData1()
{
    if (DataCollector::m_networkAccessManager == nullptr)
    {
        return;
    }

    QString url = "https://fxg.jinritemai.com/product/tcomment/commentList?rank=0&content_search=0&reply_search=0&appeal_search=0&random=0.6537696171371234&appid=1&_lid=";
    QMap<QString, QString> otherQuery;
    if (!m_task.m_goodsInfo.isEmpty())
    {
        otherQuery["id"] = m_task.m_goodsInfo;
    }
    if (!m_task.m_orderId.isEmpty())
    {
        otherQuery["order_id"] = m_task.m_orderId;
    }
    if (m_task.m_beginTime != 0)
    {
        otherQuery["comment_time_from"] = QString::number(m_task.m_beginTime);
    }
    if (m_task.m_endTime != 0)
    {
        otherQuery["comment_time_to"] = QString::number(m_task.m_endTime);
    }
    otherQuery["page"] = QString::number(m_page);
    otherQuery["pageSize"] = QString::number(m_pageSize);
    otherQuery["__token"] = m_shop.m_token;
    otherQuery["_bid"] = m_shop.m_bid;
    otherQuery["aid"] = m_shop.m_aid;
    otherQuery["aftersale_platform_source"] = m_shop.m_platformSource;

    QString otherQueryString;
    for (auto it = otherQuery.begin(); it!=otherQuery.end(); it++)
    {
        otherQueryString += "&" + it.key() + "=" + it.value();
    }

    url += otherQueryString;

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    addCommonHeader(request);
    DataCollector::m_networkAccessManager->get(request);
}

void CommentDataCollector::parseData1Array(const QJsonValue& datasJson, QVector<QVector<QString>>& datas)
{
    for (auto dataJson : datasJson.toArray())
    {
        QJsonObject dataItemJson = dataJson.toObject();
        QVector<QString> data;

        // 店铺名称
        data.append(m_shop.m_name);

        // 订单编号
        if (dataItemJson.contains("order_id"))
        {
            data.append(dataItemJson["order_id"].toString());
        }
        else
        {
            data.append("");
        }

        // 商品信息
        if (dataItemJson.contains("product") && dataItemJson["product"].toObject().contains("name"))
        {
            data.append(dataItemJson["product"].toObject()["name"].toString());
        }
        else
        {
            data.append("");
        }

        // 商品ID
        if (dataItemJson.contains("product_id"))
        {
            data.append(dataItemJson["product_id"].toString());
        }
        else
        {
            data.append("");
        }

        // 评价时间
        if (dataItemJson.contains("comment_time"))
        {
            int commentTime = dataItemJson["comment_time"].toInt();
            data.append(QDateTime::fromSecsSinceEpoch(commentTime, Qt::LocalTime).toString("yyyy/MM/dd HH:mm:ss"));
        }
        else
        {
            data.append("");
        }

        // 评价等级
        if (dataItemJson.contains("tags") && dataItemJson["tags"].toObject().contains("rank_info")
                && dataItemJson["tags"].toObject()["rank_info"].toObject().contains("name"))
        {
            data.append(dataItemJson["tags"].toObject()["rank_info"].toObject()["name"].toString());
        }
        else
        {
            data.append("");
        }

        // 评价内容
        if (dataItemJson.contains("content"))
        {
            data.append(dataItemJson["content"].toString());
        }
        else
        {
            data.append("");
        }

        datas.append(data);
    }
}
