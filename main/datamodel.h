#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QString>
#include <QDateTime>

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

public:
    // 获取评价时间的Utc时间
    qint64 getCommentTimeUtc() const
    {
        QDateTime dateTime = QDateTime::fromString(m_commentTime, "yyyy/MM/dd hh:mm:ss");
        if (!dateTime.isValid())
        {
            return 0;
        }
        dateTime.setTimeSpec(Qt::LocalTime);
        return dateTime.toSecsSinceEpoch();
    }
};

class Order
{
public:
    // 订单编号
    QString m_orderId;

    // 发货日期
    QString m_shippingDate;

    // 发货仓
    QString m_shippingWareHouse;

    // 商品简名
    QString m_goodsName;

    // 达人名称
    QString m_darenName;

public:
    // 获取发货时间的Utc时间
    qint64 getShippingTimeUtc() const
    {
        QDateTime dateTime = QDateTime::fromString(m_shippingDate, "yyyy-MM-dd hh:mm:ss");
        if (!dateTime.isValid())
        {
            return 0;
        }
        dateTime.setTimeSpec(Qt::LocalTime);
        return dateTime.toSecsSinceEpoch();
    }
};

// 评价等级统计
class CommentLevelSummary
{
public:
    // 商品简称
    QString m_goodsName;

    // 仓库
    QString m_wareHouse;

    // 评价等级
    QString m_commentLevel;

    // 评价数量
    int m_commentCount = 0;

    // 评价比例
    float m_commentRatio = 0.0f;
};

#endif // DATAMODEL_H
