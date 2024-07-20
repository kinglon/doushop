#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QMap>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QPainter>
#include "datamodel.h"
#include "shopitemwidget.h"
#include "loginutil.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MyItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    MyItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

public:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        // Disable the item focus frame
        QStyleOptionViewItem modifiedOption = option;
        modifiedOption.state &= ~QStyle::State_HasFocus;
        QStyledItemDelegate::paint(painter, modifiedOption, index);
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void initCtrls();

    void updateButtonStatus();

    void addListItemCtrl(const Shop& shop);

    void updateListItemCtrl(QString shopId);

    QListWidgetItem* getListItem(QString shopId);

private slots:
    void onAddShopBtn();

    void onSelectShopBtn();

    void onDeleteShopBtn(QString shopId);

    void onLoginShopBtn(QString shopId);

    void onBeginCollectBtn();

    void onContinueCollectBtn();

    void onStopCollectBtn();

    void onExcelBtn();

    void addLog(QString log);

    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;

    LoginUtil* m_loginUtil = nullptr;

    // 标志采集器是否正在运行
    bool m_isCollecting = false;
};
#endif // MAINWINDOW_H
