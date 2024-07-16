#include "collectstatusmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include "Utility/ImPath.h"

CollectStatusManager::CollectStatusManager()
{
    load();
}

CollectStatusManager* CollectStatusManager::getInstance()
{
    static CollectStatusManager* instance = new CollectStatusManager();
    return instance;
}

void CollectStatusManager::save()
{
    QJsonObject root;
    root["next_task_index"] = m_nextTaskIndex;
    root["next_page_index"] = m_nextPageIndex;

    QJsonArray tasksJson;
    for (const auto& task : m_collectTasks)
    {
        QJsonObject taskJson;
        taskJson["goods_info"] = task.m_goodsInfo;
        taskJson["order_id"] = task.m_orderId;
        taskJson["begin_time"] = task.m_beginTime;
        taskJson["end_time"] = task.m_endTime;
        taskJson["shop_id"] = task.m_shopId;
        tasksJson.append(taskJson);
    }
    root["tasks"] = tasksJson;

    QJsonArray datasJson;
    for (const auto& data : m_collectDatas)
    {
        QJsonObject dataJson;
        dataJson["content"] = data.m_commentContent;
        dataJson["level"] = data.m_commentLevel;
        dataJson["time"] = data.m_commentTime;
        dataJson["goods_id"] = data.m_goodsId;
        dataJson["goods_info"] = data.m_goodsInfo;
        dataJson["order_id"] = data.m_orderId;
        dataJson["shop_name"] = data.m_shopName;
        datasJson.append(dataJson);
    }
    root["datas"] = datasJson;

    QJsonDocument jsonDocument(root);
    QByteArray jsonData = jsonDocument.toJson(QJsonDocument::Indented);
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"collect_status.json";
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCritical("failed to save the collecting status");
        return;
    }
    file.write(jsonData);
    file.close();
}

void CollectStatusManager::startNewTasks(const QVector<CollectTaskItem>& tasks)
{
    reset();
    m_collectTasks = tasks;
    save();
}

CollectTaskItem CollectStatusManager::getNextTask()
{
    if (m_nextTaskIndex >= m_collectTasks.size())
    {
        return CollectTaskItem();
    }

    return m_collectTasks[m_nextTaskIndex];
}

void CollectStatusManager::finishCurrentPage(const QVector<Comment>& dataModel)
{
    m_collectDatas.append(dataModel);
    m_nextPageIndex++;
    save();
}

void CollectStatusManager::finishCurrentTask(const QVector<Comment>& dataModel)
{
    m_collectDatas.append(dataModel);
    m_nextTaskIndex++;
    m_nextPageIndex = 0;
    save();
}

void CollectStatusManager::reset()
{
    m_nextTaskIndex = 0;
    m_nextPageIndex = 0;
    m_collectTasks.clear();
    m_collectDatas.clear();
    save();
}

void CollectStatusManager::load()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"collect_status.json";
    if (!QFileInfo(QString::fromStdWString(strConfFilePath)).exists())
    {
        return;
    }

    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical("failed to load the collecting status configure file");
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    QJsonObject root = jsonDocument.object();
    m_nextTaskIndex = root["next_task_index"].toInt();
    m_nextPageIndex = root["next_page_index"].toInt();

    m_collectTasks.clear();
    QJsonArray tasksJson = root["tasks"].toArray();
    for (int i=0; i < tasksJson.size(); i++)
    {
        CollectTaskItem task;
        task.m_goodsInfo = tasksJson.at(i)["goods_info"].toString();
        task.m_orderId = tasksJson.at(i)["order_id"].toString();
        task.m_beginTime = tasksJson.at(i)["begin_time"].toInt();
        task.m_endTime = tasksJson.at(i)["end_time"].toInt();
        task.m_shopId = tasksJson.at(i)["shop_id"].toString();
        m_collectTasks.push_back(task);
    }

    m_collectDatas.clear();
    QJsonArray datasJson = root["datas"].toArray();
    for (int i=0; i < datasJson.size(); i++)
    {
        auto dataJson = datasJson.at(i);
        Comment data;
        data.m_commentContent = dataJson["content"].toString();
        data.m_commentLevel = dataJson["level"].toString();
        data.m_commentTime = dataJson["time"].toString();
        data.m_goodsId = dataJson["goods_id"].toString();
        data.m_goodsInfo = dataJson["goods_info"].toString();
        data.m_orderId = dataJson["order_id"].toString();
        data.m_shopName = dataJson["shop_name"].toString();
        m_collectDatas.append(data);
    }
}
