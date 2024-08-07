#ifndef FILEDOWNLOADER_H
#define FILEDOWNLOADER_H

#include <QThread>
#include <QObject>
#include "httpthread.h"

class FileDownloader : public HttpThread
{
    Q_OBJECT

public:
    explicit FileDownloader(QObject *parent = nullptr);

public:
    // 请求退出
    void requestStop() { m_requestStop = true; }

    // 设置URL
    void setUrl(const QString& url) { m_url = url; }

    // 设置Cookie
    void setCookie(const QString& cookie) { m_cookie = cookie; }

    // 设置超时时间
    void setTimeoutSeconds(int seconds) { m_timeOutSeconds = seconds; }

protected:
    void run() override;

    virtual int getTimeOutSeconds() override { return m_timeOutSeconds; }

    void doDownload(std::string& data);

    QMap<QString,QString> getCommonHeaders() override;

private:
    CURL* makeDownloadRequest();

signals:
    // success为True时，data是下载的数据，用完要释放
    void downloadFinish(bool success, std::string* data);

private:
    bool m_requestStop = false;

    QString m_url;

    QString m_cookie;

    int m_timeOutSeconds = 60;
};

#endif // FILEDOWNLOADER_H
