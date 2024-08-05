#include "datacollector.h"
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QNetworkProxy>
#include "browserwindow.h"
#include "brotli/decode.h"

#define MAX_RETRY_COUNT 3

#define PAGE_SIZE 20

QNetworkAccessManager *DataCollector::m_networkAccessManager = nullptr;

DataCollector::DataCollector(QObject *parent)
    : QObject{parent}
{

}

void DataCollector::setTask(const CollectTaskItem& task, const Shop& shop, int page)
{
    m_task = task;
    m_shop = shop;
    m_page = page;
}

bool DataCollector::run()
{
    // 初始化网络请求
    if (m_networkAccessManager == nullptr)
    {
        m_networkAccessManager = new QNetworkAccessManager();
        m_networkAccessManager->setProxy(QNetworkProxy());        
    }
    m_networkAccessManager->setTransferTimeout(m_networkTimeout*1000);
    connect(m_networkAccessManager, &QNetworkAccessManager::finished, this, &DataCollector::onHttpFinished);

    m_currentStep = COLLECT_STEP_GET_DATA1;
    m_retryCount = 0;
    httpGetData1();

    return true;
}

QByteArray DataCollector::intArrayToByteArray(int datas[], int size)
{
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    for (int i = 0; i < size; i++)
    {
        stream << datas[i];
    }
    return byteArray;
}

void DataCollector::httpGetData1()
{
    if (m_networkAccessManager == nullptr)
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
    otherQuery["pageSize"] = QString::number(PAGE_SIZE);
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
    m_networkAccessManager->get(request);
}

void DataCollector::addCommonHeader(QNetworkRequest& request)
{
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Accept-Encoding", "gzip, deflate, br, zstd");
    request.setRawHeader("Origin", "https://fxg.jinritemai.com");
    request.setRawHeader("Referer", "https://fxg.jinritemai.com");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Safari/537.36");
    request.setRawHeader("Cookie", m_shop.m_cookies.toUtf8());
}

void DataCollector::processHttpReply1(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCritical("failed to send the http request for data1, error: %d", reply->error());
        if (m_retryCount >= MAX_RETRY_COUNT)
        {
            emit runFinish(COLLECT_ERROR_CONNECTION_FAILED);
        }
        else
        {
            QTimer::singleShot(1000, [this]() {
                m_retryCount++;
                httpGetData1();
            });
        }
    }
    else
    {
        QByteArray rawData = reply->readAll();
        if (reply->rawHeader("Content-Encoding").toStdString() == "br")
        {
            size_t outBufferSize = 10*1024*1024;
            static char* outBuffer = new char[outBufferSize];
            auto result = BrotliDecoderDecompress((size_t)rawData.length(), (const uint8_t*)rawData.data(), &outBufferSize, (uint8_t*)outBuffer);
            if (result != BROTLI_DECODER_RESULT_SUCCESS)
            {
                qCritical("failed to decompress the brotli response data, error is %d", result);
                emit runFinish(COLLECT_ERROR);
                return;
            }
            rawData.setRawData((const char*)outBuffer, outBufferSize);
        }       

        QJsonDocument jsonDocument = QJsonDocument::fromJson(rawData);
        if (jsonDocument.isNull() || jsonDocument.isEmpty())
        {
            qCritical("failed to parse the json data");
            emit runFinish(COLLECT_ERROR);
            return;
        }

        parseData1(jsonDocument.object());
    }
}

void DataCollector::onHttpFinished(QNetworkReply *reply)
{
    if (m_currentStep == COLLECT_STEP_GET_DATA1)
    {
        processHttpReply1(reply);
    }
    reply->deleteLater();
}

bool DataCollector::parseData1(const QJsonObject& root)
{
    if (!root.contains("code"))
    {
        qCritical("response error, there is not code field");
        emit runFinish(COLLECT_ERROR);
        return false;
    }

    QString code = root["code"].toString();
    if (root["code"].isDouble())
    {
        code = QString::number(root["code"].toInt());
    }

    if (code == "10008")
    {
        qCritical("response error, relogin");
        emit runFinish(COLLECT_ERROR_NOT_LOGIN);
        return false;
    }
    else if (code != "0")
    {
        qCritical("response error, code is %s", code.toStdString().c_str());
        emit runFinish(COLLECT_ERROR);
        return false;
    }

    if (!root.contains("data"))
    {
        qCritical("response error, not have data field");
        emit runFinish(COLLECT_ERROR);
        return false;
    }

    QJsonArray datasJson = root["data"].toArray();
    for (auto dataJson : datasJson)
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

        m_dataModel.append(data);
    }

    if (m_dataModel.size() < PAGE_SIZE)
    {
        emit runFinish(COLLECT_SUCCESS);
    }
    else
    {
        emit runFinish(COLLECT_SUCCESS_MORE_DATA);
    }

    return true;
}
