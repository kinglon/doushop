#ifndef COLLECTSTATUSMANAGER_H
#define COLLECTSTATUSMANAGER_H

#include <QString>
#include <QVector>
#include "datamodel.h"

// 任务类型
#define TASK_TYPE_UNKNOWN   0   // 未知
#define TASK_TYPE_COMMENT   1   // 商品评论
#define TASK_TYPE_AFTERSELL 2   // 售后单
#define TASK_TYPE_PAY       3   // 打款记录

class CollectTaskItem
{
public:
    // 商品信息
    QString m_goodsInfo;

    // 订单编号
    QString m_orderId;

    // 开始时间，UTC，秒
    int m_beginTime = 0;

    // 结束时间，UTC，秒
    int m_endTime = 0;

    // 店铺ID
    QString m_shopId;

    // 任务类型
    int m_type = TASK_TYPE_UNKNOWN;
};

class CollectStatusManager
{
public:
    CollectStatusManager();

public:
    static CollectStatusManager* getInstance();

public:
    // 查询是否有任务待采集
    bool hasTaskCollecting() { return m_collectTasks.size() > 0; }

    // 启动新任务采集
    void startNewTasks(const QVector<CollectTaskItem>& tasks);

    CollectTaskItem getNextTask();

    int getNextPageIndex() { return m_nextPageIndex; }

    void finishCurrentPage(const QVector<QVector<QString>>& dataModel);

    QVector<QVector<QString>>& getCollectDatas() { return m_collectDatas; }

    void finishCurrentTask(const QVector<QVector<QString>>& dataModel);

    bool isFinish() { return m_nextTaskIndex >= m_collectTasks.size(); }

    int getTaskType();

    void reset();

    // 设置id key索引，用于采集结果去重，-1表示不去重
    void setKeyIndex(int keyIndex) { m_idKeyIndex = keyIndex; }

private:
    // 检查采集结果是否已经存在
    bool isExist(const QVector<QString>& data);

private:
    // 采集任务列表
    QVector<CollectTaskItem> m_collectTasks;

    // 下一次采集任务索引
    int m_nextTaskIndex = 0;

    // 下一次采集的页面索引
    int m_nextPageIndex = 0;

    // 采集的结果
    QVector<QVector<QString>> m_collectDatas;

    // id key索引，用于采集结果去重
    int m_idKeyIndex = -1;
};

#endif // COLLECTSTATUSMANAGER_H
