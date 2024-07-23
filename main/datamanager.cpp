#include "datamanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include "Utility/ImPath.h"

// 数据最多保留天数
#define MAX_DAYS 15

DataManager::DataManager()
{
    load();
}

DataManager* DataManager::getInstance()
{
    static DataManager* instance = new DataManager();
    return instance;
}

void DataManager::addComments(const QVector<Comment>& comments)
{
    removeOldData();

    for (auto& comment : comments)
    {
        bool found = false;
        for (auto& orderComment : m_orderComments)
        {
            if (orderComment.m_comment.m_orderId == comment.m_orderId)
            {
                orderComment.m_comment = comment;
                found = true;
                break;
            }
        }

        if (!found)
        {
            OrderComment orderComment;
            orderComment.m_comment = comment;
            m_orderComments.append(orderComment);
        }
    }

    save();
}

void DataManager::addOrders(const QVector<Order>& orders)
{
    removeOldData();

    for (auto& order : orders)
    {
        for (auto& orderComment : m_orderComments)
        {
            if (orderComment.m_comment.m_orderId == order.m_orderId)
            {
                orderComment.m_order = order;
                break;
            }
        }
    }

    save();
}

void DataManager::load()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"comment_order.json";
    if (!QFileInfo(QString::fromStdWString(strConfFilePath)).exists())
    {
        return;
    }

    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical("failed to load the comment order configure file");
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    QJsonObject root = jsonDocument.object();
    QJsonArray datas = root["data"].toArray();
    for (auto data : datas)
    {
        QJsonObject dataJson = data.toObject();
        OrderComment orderComment;
        orderComment.m_comment.m_commentContent = dataJson["content"].toString();
        orderComment.m_comment.m_commentLevel = dataJson["level"].toString();
        orderComment.m_comment.m_commentTime = dataJson["ctime"].toString();
        orderComment.m_comment.m_goodsId = dataJson["goodsid"].toString();
        orderComment.m_comment.m_goodsInfo = dataJson["goodsinfo"].toString();
        orderComment.m_comment.m_id = dataJson["id"].toString();
        orderComment.m_comment.m_orderId = dataJson["orderid"].toString();
        orderComment.m_comment.m_shopName = dataJson["shopname"].toString();
        orderComment.m_order.m_orderId = dataJson["orderid"].toString();
        orderComment.m_order.m_darenName = dataJson["daren"].toString();
        orderComment.m_order.m_goodsName = dataJson["goodsname"].toString();
        orderComment.m_order.m_shippingDate = dataJson["shippingdate"].toString();
        orderComment.m_order.m_shippingWareHouse = dataJson["warehouse"].toString();
        m_orderComments.append(orderComment);
    }
}

void DataManager::save()
{
    QJsonObject root;
    QJsonArray datasJson;
    for (const auto& orderComment : m_orderComments)
    {
        QJsonObject dataJson;
        dataJson["content"] = orderComment.m_comment.m_commentContent;
        dataJson["level"] = orderComment.m_comment.m_commentLevel;
        dataJson["ctime"] = orderComment.m_comment.m_commentTime;
        dataJson["goodsid"] = orderComment.m_comment.m_goodsId;
        dataJson["goodsinfo"] = orderComment.m_comment.m_goodsInfo;
        dataJson["id"] = orderComment.m_comment.m_id;
        dataJson["orderid"] = orderComment.m_comment.m_orderId;
        dataJson["shopname"] = orderComment.m_comment.m_shopName;
        dataJson["daren"] = orderComment.m_order.m_darenName;
        dataJson["goodsname"] = orderComment.m_order.m_goodsName;
        dataJson["shippingdate"] = orderComment.m_order.m_shippingDate;
        dataJson["warehouse"] = orderComment.m_order.m_shippingWareHouse;
        datasJson.append(dataJson);
    }
    root["data"] = datasJson;

    QJsonDocument jsonDocument(root);
    QByteArray jsonData = jsonDocument.toJson(QJsonDocument::Indented);
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"comment_order.json";
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCritical("failed to save the comment order data");
        return;
    }
    file.write(jsonData);
    file.close();
}

void DataManager::removeOldData()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime baseDateTime = now.addDays(MAX_DAYS * -1);
    qint64 utc = baseDateTime.toSecsSinceEpoch();
    for (auto it = m_orderComments.begin(); it != m_orderComments.end();)
    {
        if (it->m_comment.getCommentTimeUtc() < utc)
        {
            it = m_orderComments.erase(it);
        }
        else
        {
            it++;
        }
    }
}
