#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QString>

class Shop
{
public:
    // id
    QString m_id;

    // 名字
    QString m_name;

    // Cookies
    QString m_cookies;

    // token
    QString m_token;

    // bid
    QString m_bid;

    // aid
    QString m_aid;

    // aftersale_platform_source
    QString m_platformSource;

public:
    bool isLogin()
    {
        return !m_cookies.isEmpty() && !m_token.isEmpty();
    }

    void resetLoginInfo()
    {
        m_cookies = "";
        m_token = "";
        m_bid = "";
        m_aid = "";
        m_platformSource = "";
    }
};

#endif // DATAMODEL_H
