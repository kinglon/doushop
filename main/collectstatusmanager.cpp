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
    m_collectDatas.append(dataModel);
    m_nextPageIndex++;
}

void CollectStatusManager::finishCurrentTask(const QVector<QVector<QString>>& dataModel)
{
    m_collectDatas.append(dataModel);
    m_nextTaskIndex++;
    m_nextPageIndex = 0;
}

void CollectStatusManager::reset()
{
    m_nextTaskIndex = 0;
    m_nextPageIndex = 0;
    m_collectTasks.clear();
    m_collectDatas.clear();
}
