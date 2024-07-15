#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "shopmanager.h"
#include "shopitemwidget.h"
#include <QDateTime>
#include <QFileDialog>
#include "gettextdialog.h"
#include "uiutil.h"
#include <QUuid>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);

    initCtrls();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initCtrls()
{
    ui->listWidget->setItemDelegate(new MyItemDelegate(ui->listWidget));
    for (const auto& shop : ShopManager::getInstance()->m_shops)
    {
        addListItemCtrl(shop);
    }

    QDateTime currentDateTime = QDateTime::currentDateTime();
    currentDateTime.setTime(QTime(0, 0, 0));
    ui->beginDateTimeEdit->setDateTime(currentDateTime);
    QDateTime nextDateTime = currentDateTime.addDays(1);
    ui->endDateTimeEdit->setDateTime(nextDateTime);

    connect(ui->addShopBtn, &QPushButton::clicked, [this]() {
        onAddShopBtn();
    });

    connect(ui->selectShopBtn, &QPushButton::clicked, [this]() {
        onSelectShopBtn();
    });

    connect(ui->beginCollectBtn, &QPushButton::clicked, [this]() {
        onBeginCollectBtn();
    });

    connect(ui->continueCollectBtn, &QPushButton::clicked, [this]() {
        onContinueCollectBtn();
    });

    connect(ui->stopCollectBtn, &QPushButton::clicked, [this]() {
        onStopCollectBtn();
    });

    connect(ui->excelBtn, &QPushButton::clicked, [this]() {
        onExcelBtn();
    });
}

void MainWindow::addListItemCtrl(const Shop& shop)
{
    QListWidgetItem *item = new QListWidgetItem(ui->listWidget);
    item->setData(Qt::UserRole, shop.m_id);

    ShopItemWidget* itemWidget = new ShopItemWidget(shop.m_id, ui->listWidget);
    connect(itemWidget, &ShopItemWidget::deleteShop, this, &MainWindow::onDeleteShopBtn, Qt::QueuedConnection);
    connect(itemWidget, &ShopItemWidget::loginShop, this, &MainWindow::onLoginShopBtn, Qt::QueuedConnection);

    ui->listWidget->addItem(item);
    ui->listWidget->setItemWidget(item, itemWidget);
}

void MainWindow::updateListItemCtrl(QString shopId)
{
    QListWidgetItem* item = getListItem(shopId);
    if (item)
    {
        ShopItemWidget* itemWidget = (ShopItemWidget*)(ui->listWidget->itemWidget(item));
        if (itemWidget)
        {
            itemWidget->updateCtrls();
        }
    }
}

QListWidgetItem* MainWindow::getListItem(QString shopId)
{
    for (int i=0; i < ui->listWidget->count(); i++)
    {
        QListWidgetItem* item = ui->listWidget->item(i);
        QVariant data = item->data(Qt::UserRole);
        if (data.toString() == shopId)
        {
            return item;
        }
    }

    return nullptr;
}

void MainWindow::onAddShopBtn()
{    
    GetTextDialog getTextDialog(this);
    getTextDialog.setTitle(QString::fromWCharArray(L"添加店铺"));
    getTextDialog.setKeyName(QString::fromWCharArray(L"店铺名字"));
    getTextDialog.show();
    if (getTextDialog.exec() == QDialog::Accepted)
    {
        Shop shop;
        shop.m_id = QUuid::createUuid().toString();
        shop.m_name = getTextDialog.getText();
        ShopManager::getInstance()->m_shops.append(shop);
        ShopManager::getInstance()->save();
        addListItemCtrl(shop);
    }
}

void MainWindow::onSelectShopBtn()
{
    for (int i=0; i < ui->listWidget->count(); i++)
    {
        ShopItemWidget* item = (ShopItemWidget*)ui->listWidget->item(i);
        if (!item->isSelected())
        {
            item->setSelected(true);
        }
    }
}

void MainWindow::onDeleteShopBtn(QString shopId)
{
    ShopManager::getInstance()->deleteShop(shopId);

    QListWidgetItem* item = getListItem(shopId);
    if (item)
    {
        ui->listWidget->removeItemWidget(item);
        delete item;
    }
}

void MainWindow::onLoginShopBtn(QString shopId)
{
    if (m_loginUtil)
    {
        UiUtil::showTip(QString::fromWCharArray(L"正在登录中"));
        return;
    }

    m_loginUtil = new LoginUtil();
    m_loginUtil->setShopId(shopId);
    connect(m_loginUtil, &LoginUtil::printLog, this, &MainWindow::addLog);
    connect(m_loginUtil, &LoginUtil::runFinish, [this](bool success) {
        if (!success)
        {
            addLog(QString::fromWCharArray(L"登录失败"));
            return;
        }

        ShopManager::getInstance()->updateLoginInfo(m_loginUtil->getShop());
        updateListItemCtrl(m_loginUtil->getShop().m_id);
        addLog(QString::fromWCharArray(L"登录成功"));

        m_loginUtil->deleteLater();
        m_loginUtil = nullptr;
    });
    m_loginUtil->run();
}

void MainWindow::onBeginCollectBtn()
{
    // todo by yejinlong onBeginCollectBtn
}

void MainWindow::onContinueCollectBtn()
{
    // todo by yejinlong onContinueCollectBtn
}

void MainWindow::onStopCollectBtn()
{
    // todo by yejinlong onStopCollectBtn
}

void MainWindow::onExcelBtn()
{
    // todo by yejinlong onExcelBtn
}

void MainWindow::addLog(QString log)
{
    static int lineCount = 0;
    if (lineCount >= 1000)
    {
        ui->logEdit->clear();
        lineCount = 0;
    }
    lineCount++;

    qInfo(log.toStdString().c_str());
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString currentTimeString = currentDateTime.toString("[MM-dd hh:mm:ss] ");
    QString line = currentTimeString + log;
    ui->logEdit->append(line);
}
