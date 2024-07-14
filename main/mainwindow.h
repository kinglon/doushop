﻿#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QMap>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QPainter>
#include "datamodel.h"
#include "shopitemwidget.h"

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

    void addListItemCtrl(const Shop& shop);

    void updateListItemCtrl(QString shopId);

    QListWidgetItem* getListItem(QString shopId);

private slots:
    void onAddShopBtn();

    void onDeleteShopBtn(QString shopId);

    void onLoginShopBtn(QString shopId);

    void addLog(QString log);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
