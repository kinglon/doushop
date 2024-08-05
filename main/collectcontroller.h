#ifndef COLLECTCONTROLLER_H
#define COLLECTCONTROLLER_H

#include <QObject>
#include "datamodel.h"

class CollectController : public QObject
{
    Q_OBJECT
public:
    explicit CollectController(QObject *parent = nullptr);

public:
    void run();

    // 保存采集结果并打开目录，返回保存路径，空表示保存失败
    static QString saveCollectResult();

private:
    void finishCurrentTask(const QVector<QVector<QString>>& dataModel, bool hasMoreData);

private slots:
    void onPrintLog(QString content);

    void onCollectNextTask();

signals:
    void collectNextTask();

    void runFinish(bool success);

    void printLog(QString content);
};

#endif // COLLECTCONTROLLER_H
