#include "collectstatusmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include "Utility/ImPath.h"

CollectStatusManager::CollectStatusManager()
{

}

CollectStatusManager* CollectStatusManager::getInstance()
{
    static CollectStatusManager* instance = new CollectStatusManager();
    return instance;
}

void CollectStatusManager::startNewTasks(const QVector<CollectTaskItem>& tasks)
{
    reset();
    m_collectTasks = tasks;
}

CollectTaskItem CollectStatusManager::getNextTask()
{
    if (m_nextTaskIndex >= m_collectTasks.size())
    {
        return CollectTaskItem();
    }

    return m_collectTasks[m_nextTaskIndex];
}

void CollectStatusManager::finishCurrentPage(const QVector<QVector<QString>>& dataModel)
{
    for (auto& data : dataModel)
    {
        if (!isExist(data))
        {
            m_collectDatas.append(data);
        }
    }

    m_nextPageIndex++;
}

void CollectStatusManager::finishCurrentTask(const QVector<QVector<QString>>& dataModel)
{
    for (auto& data : dataModel)
    {
        if (!isExist(data))
        {
            m_collectDatas.append(data);
        }
    }

    m_nextTaskIndex++;
    m_nextPageIndex = 0;
}

int CollectStatusManager::getTaskType()
{
    if (m_collectTasks.length() == 0)
    {
        return TASK_TYPE_UNKNOWN;
    }

    return m_collectTasks[0].m_type;
}

void CollectStatusManager::reset()
{
    m_idKeyIndex = -1;
    m_nextTaskIndex = 0;
    m_nextPageIndex = 0;
    m_collectTasks.clear();
    m_collectDatas.clear();
}

bool CollectStatusManager::isExist(const QVector<QString>& data)
{
    if (m_idKeyIndex < 0)
    {
        return false;
    }

    if (m_idKeyIndex >= data.length())
    {
        return false;
    }

    QString id = data[m_idKeyIndex];
    for (auto& item : m_collectDatas)
    {
        if (m_idKeyIndex < item.length() && item[m_idKeyIndex] == id)
        {
            return true;
        }
    }

    return false;
}
