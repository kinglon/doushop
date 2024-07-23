#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "datamodel.h"
#include <QVector>

class OrderComment
{
public:
    Comment m_comment;

    Order m_order;
};

class DataManager
{
protected:
    DataManager();

public:
    static DataManager* getInstance();

public:
    void addComments(const QVector<Comment>& comments);

    void addOrders(const QVector<Order>& orders);

    const QVector<OrderComment>& getOrderComments() { return m_orderComments; }

private:
    void load();

    void save();

    void removeOldData();

private:
    QVector<OrderComment> m_orderComments;
};

#endif // DATAMANAGER_H
