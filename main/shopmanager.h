#ifndef SHOPMANAGER_H
#define SHOPMANAGER_H

#include "datamodel.h"
#include <QVector>

class ShopManager
{    
protected:
    ShopManager();

public:
    static ShopManager* getInstance();

public:
    void save();

    Shop* getShopById(QString id);

    void deleteShop(QString id);

private:
    void load();

public:
    // 计划列表
    QVector<Shop> m_shops;
};

#endif // SHOPMANAGER_H
