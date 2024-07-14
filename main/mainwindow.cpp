#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "shopmanager.h"
#include "shopitemwidget.h"
#include <QDateTime>
#include <QFileDialog>
#include "uiutil.h"

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

    connect(ui->addShopBtn, &QPushButton::clicked, [this]() {
        onAddShopBtn();
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
    // todo by yejinlong, onAddShopBtn
//    PlanDialog planDlg(PlanItem(), this);
//    planDlg.show();
//    if (planDlg.exec() == QDialog::Accepted)
//    {
//        PlanManager::getInstance()->m_plans.append(planDlg.getPlanItem());
//        PlanManager::getInstance()->save();
//        addPlanListItemCtrl(planDlg.getPlanItem());
//    }
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
    // todo by yejinlong onLoginShopBtn
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
