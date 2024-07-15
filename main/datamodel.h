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
};

class Comment
{
public:
    // id
    QString m_id;

    // 店铺名称
    QString m_shopName;

    // 订单编号
    QString m_orderId;

    // 商品信息
    QString m_goodsInfo;

    // 商品ID
    QString m_goodsId;

    // 评价时间
    QString m_commentTime;

    // 评价等级
    QString m_commentLevel;

    // 评价内容
    QString m_commentContent;
};

#endif // DATAMODEL_H
