#ifndef SHOPITEMWIDGET_H
#define SHOPITEMWIDGET_H

#include <QWidget>
#include "datamodel.h"

namespace Ui {
class ShopItemWidget;
}

class ShopItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ShopItemWidget(const QString& shopId, QWidget *parent = nullptr);
    ~ShopItemWidget();

public:
    void updateCtrls();

    bool isSelected();

    QString getShopId() { return m_shopId; }

private:
    void initCtrls();

signals:
    void deleteShop(QString shopId);

    void loginShop(QString shopId);

private:
    Ui::ShopItemWidget *ui;

    QString m_shopId;
};

#endif // SHOPITEMWIDGET_H
