#include "shopmanager.h"
#include <QFile>
#include "Utility/ImPath.h"
#include "Utility/ImCharset.h"
#include "Utility/LogMacro.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ShopManager::ShopManager()
{
    load();
}


ShopManager* ShopManager::getInstance()
{
    static ShopManager* instance = new ShopManager();
    return instance;
}

void ShopManager::save()
{
    QJsonObject root;
    QJsonArray shopsJson;
    for (const auto& shop : m_shops)
    {
        QJsonObject shopJson;
        shopJson["id"] = shop.m_id;
        shopJson["name"] = shop.m_name;
        shopJson["aid"] = shop.m_aid;
        shopJson["bid"] = shop.m_bid;
        shopJson["cookies"] = shop.m_cookies;
        shopJson["token"] = shop.m_token;
        shopJson["platform_source"] = shop.m_platformSource;
        shopsJson.append(shopJson);
    }
    root["shop"] = shopsJson;

    QJsonDocument jsonDocument(root);
    QByteArray jsonData = jsonDocument.toJson(QJsonDocument::Indented);
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"shop.json";
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCritical("failed to save the shop setting");
        return;
    }
    file.write(jsonData);
    file.close();
}

void ShopManager::load()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"shop.json";
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.open(QIODevice::ReadOnly))
    {
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    QJsonObject root = jsonDocument.object();

    QJsonArray shopsJson = root["shop"].toArray();
    for (auto shopJsonItem : shopsJson)
    {
        QJsonObject shopJson = shopJsonItem.toObject();
        Shop shopItem;
        shopItem.m_id = shopJson["id"].toString();
        shopItem.m_name = shopJson["name"].toString();
        shopItem.m_aid = shopJson["aid"].toString();
        shopItem.m_bid = shopJson["bid"].toString();
        shopItem.m_cookies = shopJson["cookies"].toString();
        shopItem.m_token = shopJson["token"].toString();
        shopItem.m_platformSource = shopJson["platform_source"].toString();
        m_shops.append(shopItem);
    }
}

Shop* ShopManager::getShopById(QString id)
{
    for (auto& shop : ShopManager::getInstance()->m_shops)
    {
        if (shop.m_id == id)
        {
            return &shop;
        }
    }

    return nullptr;
}

void ShopManager::deleteShop(QString id)
{
    for (auto it = m_shops.begin(); it != m_shops.end(); it++)
    {
        if (it->m_id == id)
        {
            m_shops.erase(it);
            break;
        }
    }

    save();
}

void ShopManager::updateLoginInfo(const Shop& shop)
{
    for (auto it = m_shops.begin(); it != m_shops.end(); it++)
    {
        if (it->m_id == shop.m_id)
        {
            it->m_aid = shop.m_aid;
            it->m_bid = shop.m_bid;
            it->m_token = shop.m_token;
            it->m_cookies = shop.m_cookies;
            it->m_platformSource = shop.m_platformSource;
            break;
        }
    }

    save();
}


void ShopManager::resetLoginInfo(QString id)
{
    for (auto it = m_shops.begin(); it != m_shops.end(); it++)
    {
        if (it->m_id == id)
        {
            it->resetLoginInfo();
            break;
        }
    }

    save();
}
