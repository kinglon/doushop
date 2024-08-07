#include "filedownloader.h"
#include <QDebug>

FileDownloader::FileDownloader(QObject *parent)
    : HttpThread{parent}
{

}

CURL* FileDownloader::makeDownloadRequest()
{
    CURL* request = makeRequest(m_url, QMap<QString,QString>(), QMap<QString, QString>(), ProxyServer());
    if (request == nullptr)
    {
        qCritical() << "failed to make the file downloading request";
        return nullptr;
    }

    curl_easy_setopt(request, CURLOPT_HTTP_CONTENT_DECODING, 0L);
    curl_easy_setopt(request, CURLOPT_COOKIE, m_cookie.toStdString().c_str());
    return request;
}

void FileDownloader::run()
{
    std::string* data = new std::string();
    doDownload(*data);
    if (data->length() == 0)
    {
        delete data;
        emit downloadFinish(false, nullptr);
    }
    else
    {
        emit downloadFinish(true, data);
    }
}

void FileDownloader::doDownload(std::string& data)
{
    CURLM* multiHandle = curl_multi_init();
    if (multiHandle == nullptr)
    {
        qCritical("failed to init a multi handle");
        return;
    }

    CURL* curl = makeDownloadRequest();
    if (curl == nullptr)
    {
        curl_multi_cleanup(multiHandle);
        return;
    }
    curl_multi_add_handle(multiHandle, curl);

    while (!m_requestStop)
    {
        QThread::msleep(100);

        int stillRunning = 0;
        CURLMcode mc = curl_multi_perform(multiHandle, &stillRunning);
        if (mc)
        {
            qCritical("curl_multi_perform return error: %d", mc);
            break;
        }

        int msgs_left = 0;
        CURLMsg *m = curl_multi_info_read(multiHandle, &msgs_left);
        if (m == nullptr)
        {
            continue;
        }

        if (m->msg != CURLMSG_DONE)
        {
            continue;
        }

        if (m->data.result == CURLE_OK)
        {
            long statusCode = 0;
            getResponse(m->easy_handle, statusCode, data);
            if (statusCode != 200)
            {
                qCritical() << "failed to download file, status code is " << statusCode;
                data = "";
            }
        }
        else
        {
            qCritical() << "failed to download file, result is " << m->data.result;
        }

        break;
    }

    curl_multi_remove_handle(multiHandle, curl);
    freeRequest(curl);
    curl_multi_cleanup(multiHandle);
}


QMap<QString,QString> FileDownloader::getCommonHeaders()
{
    QMap<QString,QString> headers;
    return headers;
}
