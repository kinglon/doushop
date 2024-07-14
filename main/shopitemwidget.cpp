#include "shopitemwidget.h"
#include "ui_shopitemwidget.h"
#include "settingmanager.h"
#include "shopmanager.h"

ShopItemWidget::ShopItemWidget(const QString& shopId, QWidget *parent) :
    QWidget(parent),    
    ui(new Ui::ShopItemWidget),
    m_shopId(shopId)
{
    ui->setupUi(this);

    initCtrls();
    updateCtrls();
}

ShopItemWidget::~ShopItemWidget()
{
    delete ui;
}

void ShopItemWidget::initCtrls()
{
    connect(ui->loginBtn, &QPushButton::clicked, [this]() {
        emit loginShop(m_shopId);
    });

    connect(ui->deleteBtn, &QPushButton::clicked, [this]() {
        emit deleteShop(m_shopId);
    });
}

void ShopItemWidget::updateCtrls()
{
    Shop* shop = ShopManager::getInstance()->getShopById(m_shopId);
    if (shop == nullptr)
    {
        return;
    }

    ui->shopNameLabel->setText(shop->m_name);
    if (shop->m_cookies.isEmpty())
    {
        ui->loginStatusLabel->setText(QString::fromWCharArray(L"未登录"));
    }
    else
    {
        ui->loginStatusLabel->setText(QString::fromWCharArray(L"已登录"));
    }
}


bool ShopItemWidget::isSelected()
{
    return ui->checkBox->isChecked();
}
