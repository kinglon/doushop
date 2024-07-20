#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "shopmanager.h"
#include "shopitemwidget.h"
#include <QDateTime>
#include <QFileDialog>
#include "gettextdialog.h"
#include "uiutil.h"
#include <QUuid>
#include "collectstatusmanager.h"
#include "collectcontroller.h"
#include "exceldialog.h"
#include "browserwindow.h"

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

void MainWindow::updateButtonStatus()
{
    ui->beginCollectBtn->setEnabled(!m_isCollecting);
    ui->continueCollectBtn->setEnabled(!m_isCollecting && CollectStatusManager::getInstance()->hasTaskCollecting());
    ui->stopCollectBtn->setEnabled(ui->continueCollectBtn->isEnabled());
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
        QListWidgetItem* item = (QListWidgetItem*)ui->listWidget->item(i);
        ShopItemWidget* itemWidget = (ShopItemWidget*)ui->listWidget->itemWidget(item);
        if (!itemWidget->isSelected())
        {
            itemWidget->setSelected(true);
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

    Shop* shop = ShopManager::getInstance()->getShopById(shopId);
    if (shop == nullptr)
    {
        qCritical("failed to find the shop");
        return;
    }
    addLog(QString::fromWCharArray(L"开始登录店铺(%1)").arg(shop->m_name));

    m_loginUtil = new LoginUtil();
    m_loginUtil->setShopId(shopId);
    connect(m_loginUtil, &LoginUtil::printLog, this, &MainWindow::addLog);
    connect(m_loginUtil, &LoginUtil::runFinish, [this](bool success) {
        if (!success)
        {
            addLog(QString::fromWCharArray(L"登录失败"));
        }
        else
        {
            ShopManager::getInstance()->updateLoginInfo(m_loginUtil->getShop());
            updateListItemCtrl(m_loginUtil->getShop().m_id);
            addLog(QString::fromWCharArray(L"登录成功"));
        }

        BrowserWindow::getInstance()->hide();

        m_loginUtil->deleteLater();
        m_loginUtil = nullptr;
    });
    m_loginUtil->run();
}

void MainWindow::onBeginCollectBtn()
{
    CollectTaskItem task;
    task.m_goodsInfo = ui->goodsInfoEdit->text();
    task.m_orderId = ui->orderIdEdit->text();
    task.m_beginTime = ui->beginDateTimeEdit->dateTime().toSecsSinceEpoch();
    task.m_endTime = ui->endDateTimeEdit->dateTime().toSecsSinceEpoch();
    if (task.m_beginTime > task.m_endTime)
    {
        UiUtil::showTip(QString::fromWCharArray(L"开始时间不能比结束时间晚"));
        return;
    }

    QVector<CollectTaskItem> tasks;
    for (int i=0; i < ui->listWidget->count(); i++)
    {
        QListWidgetItem* item = (QListWidgetItem*)ui->listWidget->item(i);
        ShopItemWidget* itemWidget = (ShopItemWidget*)ui->listWidget->itemWidget(item);
        if (itemWidget->isSelected())
        {
            task.m_shopId = itemWidget->getShopId();
            Shop* shop = ShopManager::getInstance()->getShopById(task.m_shopId);
            if (shop == nullptr)
            {
                continue;
            }

            if (shop->isLogin())
            {
                tasks.append(task);
            }
            else
            {
                UiUtil::showTip(QString::fromWCharArray(L"店铺(%1)未登录").arg(shop->m_name.toStdString().c_str()));
                return;
            }
        }
    }
    if (tasks.size() == 0)
    {
        UiUtil::showTip(QString::fromWCharArray(L"没有选择店铺"));
        return;
    }

    CollectStatusManager::getInstance()->startNewTasks(tasks);
    updateButtonStatus();
    onContinueCollectBtn();
}

void MainWindow::onContinueCollectBtn()
{
    // 如果采集已经完成，就结束采集
    if (CollectStatusManager::getInstance()->isFinish())
    {
        onStopCollectBtn();
        return;
    }

    m_isCollecting = true;
    updateButtonStatus();
    ui->logEdit->setText("");

    CollectController* collectController = new CollectController(this);
    connect(collectController, &CollectController::printLog, this, &MainWindow::addLog);
    connect(collectController, &CollectController::runFinish, [this, collectController](bool success) {
        m_isCollecting = false;
        updateButtonStatus();

        if (success)
        {
            addLog(QString::fromWCharArray(L"采集完成"));
            onStopCollectBtn();
        }
        collectController->deleteLater();
    });
    collectController->run();
}

void MainWindow::onStopCollectBtn()
{
    m_isCollecting = false;
    updateButtonStatus();

    // 保存采集结果并打开保存目录
    QString savedFilePath = CollectController::saveCollectResult();
    if (savedFilePath.isEmpty())
    {
        UiUtil::showTip(QString::fromWCharArray(L"保存采集结果到表格失败"));
        return;
    }

    addLog(QString::fromWCharArray(L"保存采集结果到%1").arg(savedFilePath));
    CollectStatusManager::getInstance()->reset();
    updateButtonStatus();
}

void MainWindow::onExcelBtn()
{
    ExcelDialog excelDialog(this);
    excelDialog.show();
    excelDialog.exec();
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    BrowserWindow::getInstance()->setCanClose();
    BrowserWindow::getInstance()->close();
    event->accept();
    QApplication::quit();
}
