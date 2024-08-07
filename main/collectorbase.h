#ifndef COLLECTORBASE_H
#define COLLECTORBASE_H

#include <QObject>
#include "datamodel.h"
#include "collectstatusmanager.h"

// 采集失败原因
#define COLLECT_SUCCESS                 0   // 采集成功，没有数据
#define COLLECT_SUCCESS_MORE_DATA       1   // 采集成功，还有数据
#define COLLECT_ERROR                   2
#define COLLECT_ERROR_NOT_LOGIN         3  // 未登录
#define COLLECT_ERROR_CONNECTION_FAILED 4  // 连接失败

class CollectorBase : public QObject
{
    Q_OBJECT
public:
    explicit CollectorBase(QObject *parent = nullptr);

public:
    void setTask(const CollectTaskItem& task, const Shop& shop) { m_task = task; m_shop = shop; }

    virtual bool run() = 0;

    QVector<QVector<QString>>& getDataModel() { return m_dataModel; }

signals:
    // 运行结束
    void runFinish(int errorCode);

    // 输出日志
    void collectLog(QString logContent);

protected:
    QVector<QVector<QString>> m_dataModel;

    CollectTaskItem m_task;

    Shop m_shop;
};

#endif // COLLECTORBASE_H
