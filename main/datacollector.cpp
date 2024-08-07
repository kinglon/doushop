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

QNetworkAccessManager *DataCollector::m_networkAccessManager = nullptr;

DataCollector::DataCollector(QObject *parent)
    : CollectorBase{parent}
{

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

void DataCollector::getData2Finish(int result)
{
    if (result != COLLECT_SUCCESS)
    {
        emit runFinish(result);
    }
    else
    {
        if (m_dataModel.size() < m_pageSize)
        {
            emit runFinish(COLLECT_SUCCESS);
        }
        else
        {
            emit runFinish(COLLECT_SUCCESS_MORE_DATA);
        }
    }
}

void DataCollector::onHttpFinished(QNetworkReply *reply)
{
    if (m_currentStep == COLLECT_STEP_GET_DATA1)
    {
        processHttpReply1(reply);
    }
    else if (m_currentStep == COLLECT_STEP_GET_DATA2)
    {
        processHttpReply2(reply);
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

    parseData1Array(root["data"], m_dataModel);

    m_retryCount = 0;
    m_currentStep = COLLECT_STEP_GET_DATA2;
    if (!doGetData2())
    {
        getData2Finish(COLLECT_SUCCESS);
    }

    return true;
}
