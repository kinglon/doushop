#include "afterselldatacollector.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include "Utility/ImPath.h"
#include "Utility/LogMacro.h"
#include "brotli/decode.h"

int MAX_RETRY_COUNT = 2;

static QJsonObject defaultBodyJson;

void loadDefaultBodyJson()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"aftersell_default_body.json";
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_ERROR(L"failed to open the default body configure file : %s", strConfFilePath.c_str());
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    defaultBodyJson = jsonDocument.object();
}

AfterSellDataCollector::AfterSellDataCollector(QObject *parent)
    : DataCollector{parent}
{
    if (defaultBodyJson.empty())
    {
        loadDefaultBodyJson();
    }
}

void AfterSellDataCollector::httpGetData1()
{
    if (DataCollector::m_networkAccessManager == nullptr)
    {
        return;
    }

    QString url = "https://fxg.jinritemai.com/after_sale/pc/list?appid=1&_lid=";
    QMap<QString, QString> otherQuery;
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

    defaultBodyJson["pageSize"] = m_pageSize;
    defaultBodyJson["page"] = m_page;
    defaultBodyJson["final_time_start"] = m_task.m_beginTime;
    defaultBodyJson["final_time_end"] = m_task.m_endTime;
    defaultBodyJson["_bid"] = m_shop.m_bid;
    defaultBodyJson["aid"] = m_shop.m_aid.toInt();
    defaultBodyJson["aftersale_platform_source"] = m_shop.m_platformSource;
    QJsonDocument jsonDocument(defaultBodyJson);
    QByteArray bodyData = jsonDocument.toJson();

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    addCommonHeader(request);
    DataCollector::m_networkAccessManager->post(request, bodyData);
}

void AfterSellDataCollector::parseData1Array(const QJsonValue& datasJson, QVector<QVector<QString>>& datas)
{
    if (!datasJson.toObject().contains("items"))
    {
        return;
    }

    QJsonArray itemsJson = datasJson.toObject()["items"].toArray();
    for (auto dataJson : itemsJson)
    {
        QJsonObject dataItemJson = dataJson.toObject();
        QVector<QString> data;

        // 店铺名称
        data.append(m_shop.m_name);

        // 订单编号
        if (dataItemJson.contains("order_info")
                && dataItemJson["order_info"].toObject().contains("shop_order_id"))
        {
            data.append(dataItemJson["order_info"].toObject()["shop_order_id"].toString());
        }
        else
        {
            data.append("");
        }

        // 商品信息
        if (dataItemJson.contains("order_info")
                && dataItemJson["order_info"].toObject().contains("related_order_info")
                && dataItemJson["order_info"].toObject()["related_order_info"].toArray().size() > 0
                && dataItemJson["order_info"].toObject()["related_order_info"].toArray()[0].toObject().contains("product_name"))
        {
            data.append(dataItemJson["order_info"].toObject()["related_order_info"].toArray()[0].toObject()["product_name"].toString());
        }
        else
        {
            data.append("");
        }

        // 应付金额（元）
        if (dataItemJson.contains("order_info")
                && dataItemJson["order_info"].toObject().contains("related_order_info")
                && dataItemJson["order_info"].toObject()["related_order_info"].toArray().size() > 0
                && dataItemJson["order_info"].toObject()["related_order_info"].toArray()[0].toObject().contains("pay_amount"))
        {
            int pay_amount = dataItemJson["order_info"].toObject()["related_order_info"].toArray()[0].toObject()["pay_amount"].toInt();
            data.append(QString::number(pay_amount/100.0f, 'f', 2));
        }
        else
        {
            data.append("");
        }

        // 商品发货状态
        QString shippedStatus;
        if (dataItemJson.contains("text_part")
                && dataItemJson["text_part"].toObject().contains("logistics_shipped_text"))
        {
            shippedStatus = dataItemJson["text_part"].toObject()["logistics_shipped_text"].toString();
            data.append(shippedStatus);
        }
        else
        {
            data.append("");
        }

        // 售后类型
        if (dataItemJson.contains("text_part")
                && dataItemJson["text_part"].toObject().contains("after_sale_type_text"))
        {
            data.append(dataItemJson["text_part"].toObject()["after_sale_type_text"].toString());
        }
        else
        {
            data.append("");
        }

        // 退商品金额（元）
        if (dataItemJson.contains("order_info")
                && dataItemJson["order_info"].toObject().contains("related_order_info")
                && dataItemJson["order_info"].toObject()["related_order_info"].toArray().size() > 0
                && dataItemJson["order_info"].toObject()["related_order_info"].toArray()[0].toObject().contains("refund_amount"))
        {
            int refund_amount = dataItemJson["order_info"].toObject()["related_order_info"].toArray()[0].toObject()["refund_amount"].toInt();
            data.append(QString::number(refund_amount/100.0f, 'f', 2));
        }
        else
        {
            data.append("");
        }

        // 售后状态
        if (dataItemJson.contains("text_part")
                && dataItemJson["text_part"].toObject().contains("after_sale_status_text"))
        {
            data.append(dataItemJson["text_part"].toObject()["after_sale_status_text"].toString());
        }
        else
        {
            data.append("");
        }

        // 售后申请时间
        if (dataItemJson.contains("after_sale_info")
                && dataItemJson["after_sale_info"].toObject().contains("apply_time"))
        {
            int applyTime = dataItemJson["after_sale_info"].toObject()["apply_time"].toInt();
            data.append(QDateTime::fromSecsSinceEpoch(applyTime, Qt::LocalTime).toString("yyyy/MM/dd HH:mm:ss"));
        }
        else
        {
            data.append("");
        }

        // 售后ID，获取快递单号使用，已发货才需要获取
        static const QString SHIPPED_STATUS = QString::fromWCharArray(L"已发货");
        if (shippedStatus.indexOf(SHIPPED_STATUS) != -1
                && dataItemJson.contains("after_sale_info")
                && dataItemJson["after_sale_info"].toObject().contains("after_sale_id"))
        {
            m_afterSaleIds.append(dataItemJson["after_sale_info"].toObject()["after_sale_id"].toString());
        }
        else
        {
            m_afterSaleIds.append("");
        }

        datas.append(data);
    }
}

bool AfterSellDataCollector::doGetData2()
{
    if (m_afterSaleIds.empty())
    {
        return false;
    }

    getNextDeliveryId();

    return true;
}

void AfterSellDataCollector::getNextDeliveryId()
{
    if (m_nextIndex >= m_afterSaleIds.size())
    {
        getData2Finish(COLLECT_SUCCESS);
        return;
    }

    if (m_afterSaleIds[m_nextIndex].isEmpty())
    {
        // 无需获取快递号
        if (m_nextIndex < m_dataModel.size())
        {
            // 物流公司+物流单号
            m_dataModel[m_nextIndex].append("");
            m_dataModel[m_nextIndex].append("");
        }
        m_nextIndex++;
        getNextDeliveryId();
        return;
    }

    m_retryCount = 0;
    sendDeliveryHttpRequest();
}

void AfterSellDataCollector::sendDeliveryHttpRequest()
{
    QString url = "https://fxg.jinritemai.com/v1/aftersale/pc/detail?order_id=&_lid=";
    QMap<QString, QString> otherQuery;
    otherQuery["after_sale_id"] = m_afterSaleIds[m_nextIndex];
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

void AfterSellDataCollector::processHttpReply2(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCritical("failed to send the http request for delivery, error: %d", reply->error());
        if (m_retryCount >= MAX_RETRY_COUNT)
        {
            getData2Finish(COLLECT_ERROR_CONNECTION_FAILED);
        }
        else
        {
            QTimer::singleShot(1000, [this]() {
                m_retryCount++;
                sendDeliveryHttpRequest();
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
                getData2Finish(COLLECT_ERROR);
                return;
            }
            rawData.setRawData((const char*)outBuffer, outBufferSize);
        }

        QJsonDocument jsonDocument = QJsonDocument::fromJson(rawData);
        if (jsonDocument.isNull() || jsonDocument.isEmpty())
        {
            qCritical("failed to parse the json data");
            getData2Finish(COLLECT_ERROR);
            return;
        }

        QString logisticsName = "";
        QString logisticsCode = "";
        if (jsonDocument.object().contains("data")
                && jsonDocument.object()["data"].toObject().contains("order_info")
                && jsonDocument.object()["data"].toObject()["order_info"].toObject().contains("order_logistics_info")
                && jsonDocument.object()["data"].toObject()["order_info"].toObject()["order_logistics_info"].toObject().contains("logistics"))
        {
            QJsonArray logistics = jsonDocument.object()["data"].toObject()["order_info"].toObject()["order_logistics_info"].toObject()["logistics"].toArray();
            if (logistics.size() > 0 && logistics[0].toObject().contains("logistics_code"))
            {
                logisticsCode = logistics[0].toObject()["logistics_code"].toString();
            }

            if (logistics.size() > 0 && logistics[0].toObject().contains("logistics_name"))
            {
                logisticsName = logistics[0].toObject()["logistics_name"].toString();
            }
        }

        if (!logisticsCode.isEmpty())
        {
            if (m_nextIndex < m_dataModel.size())
            {
                // 物流公司 + 物流单号
                m_dataModel[m_nextIndex].append(logisticsName);
                m_dataModel[m_nextIndex].append(logisticsCode);
            }
            m_nextIndex++;
            getNextDeliveryId();
            return;
        }
        else
        {
            qCritical("the delivery id returned by server is empty");
            getData2Finish(COLLECT_ERROR);
            return;
        }
    }
}
