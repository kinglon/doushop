#include "exceldialog.h"
#include "ui_exceldialog.h"
#include <QFileDialog>
#include <QTimer>
#include <QDesktopServices>
#include "uiutil.h"
#include "excelutil.h"
#include "datamanager.h"
#include "Utility/ImPath.h"

using namespace QXlsx;

ExcelDialog::ExcelDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExcelDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowModality(Qt::WindowModal);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);

    initCtrls();
}

ExcelDialog::~ExcelDialog()
{
    delete ui;
}

void ExcelDialog::initCtrls()
{
    ui->commentLevelComboBox->addItem(QString::fromWCharArray(L"不限"));
    ui->commentLevelComboBox->addItem(QString::fromWCharArray(L"差评"));
    ui->commentLevelComboBox->addItem(QString::fromWCharArray(L"好评"));
    ui->commentLevelComboBox->addItem(QString::fromWCharArray(L"中评"));
    ui->commentLevelComboBox->setCurrentIndex(0);

    QDateTime currentDateTime = QDateTime::currentDateTime();
    ui->shippingEndDateTimeEdit->setDateTime(currentDateTime);
    ui->commentEndDateTimeEdit->setDateTime(currentDateTime);
    currentDateTime.setTime(QTime(0, 0, 0));
    ui->shippingBeginDateTimeEdit->setDateTime(currentDateTime);
    ui->commentBeginDateTimeEdit->setDateTime(currentDateTime);

    connect(ui->selectJushuitanButton, &QPushButton::clicked, [this]() {
        QString filePath = selectLocalFile();
        if (!filePath.isEmpty())
        {
            ui->jushuitanLineEdit->setText(filePath);
        }
    });

    connect(ui->importJushuitanButton, &QPushButton::clicked, this, &ExcelDialog::onImportJushuitanData);
    connect(ui->exportContentSummaryBtn, &QPushButton::clicked, this, &ExcelDialog::onExportCommentContentSummary);
    connect(ui->exportCommentLevelSummaryBtn, &QPushButton::clicked, this, &ExcelDialog::onExportCommentLevelSummary);
}

QString ExcelDialog::selectLocalFile()
{
    // Create a QFileDialog object
    QFileDialog fileDialog;

    // Set the dialog's title
    fileDialog.setWindowTitle(QString::fromWCharArray(L"选择表格文件"));

    // Set the dialog's filters
    fileDialog.setNameFilters(QStringList() << "Excel files (*.xlsx *.xls)");

    // Open the dialog and get the selected files
    if (fileDialog.exec() == QDialog::Accepted)
    {
        QStringList selectedFiles = fileDialog.selectedFiles();
        if (selectedFiles.size() > 0)
        {
            return selectedFiles[0];
        }
    }

    return "";
}

void ExcelDialog::closeEvent(QCloseEvent *e)
{    
    e->accept();
}

void ExcelDialog::onImportJushuitanData()
{
    QString excelFilePath = ui->jushuitanLineEdit->text();
    if (excelFilePath.isEmpty())
    {
        UiUtil::showTip(QString::fromWCharArray(L"请先选择表格"));
        return;
    }

    QVector<QVector<QString>> excelDatas;
    bool success = ExcelUtil::loadExcel(excelFilePath, excelDatas);
    if (!success)
    {
        UiUtil::showTip(QString::fromWCharArray(L"读取表格失败"));
        return;
    }

    if (excelDatas.length() < 2)
    {
        UiUtil::showTip(QString::fromWCharArray(L"表格没有数据"));
        return;
    }

    int orderIdIndex = getColumnIndex(excelDatas[0], QString::fromWCharArray(L"线上子订单编号"));
    int fahuodateIndex = getColumnIndex(excelDatas[0], QString::fromWCharArray(L"发货日期"));
    int fahuochangIndex = getColumnIndex(excelDatas[0], QString::fromWCharArray(L"发货仓"));
    int otherAtt1 = getColumnIndex(excelDatas[0], QString::fromWCharArray(L"其它属性1"));
    int darenIndex = getColumnIndex(excelDatas[0], QString::fromWCharArray(L"达人名称"));
    int indexArray[] = {orderIdIndex, fahuochangIndex, fahuodateIndex, otherAtt1, darenIndex};
    for (int i=0; i<sizeof(indexArray)/sizeof(int); i++)
    {
        if (indexArray[i] < 0)
        {
            UiUtil::showTip(QString::fromWCharArray(L"表格缺失数据列"));
            return;
        }
    }

    QVector<Order> orders;
    for (int row=1; row < excelDatas.length(); row++)
    {
        Order order;
        order.m_darenName = excelDatas[row][darenIndex];
        order.m_goodsName = excelDatas[row][otherAtt1];
        order.m_orderId = excelDatas[row][orderIdIndex];
        order.m_shippingDate = excelDatas[row][fahuodateIndex];
        order.m_shippingWareHouse = excelDatas[row][fahuochangIndex];
        orders.append(order);
    }
    DataManager::getInstance()->addOrders(orders);

    UiUtil::showTip(QString::fromWCharArray(L"导入成功"));
}

int ExcelDialog::getColumnIndex(const QVector<QString>& headers, QString columnName)
{
    for (int i=0; i<headers.size(); i++)
    {
        if (headers[i] == columnName)
        {
            return i;
        }
    }

    return -1;
}

bool ExcelDialog::selectOrderComment(QVector<OrderComment>& orderComments)
{
    QString warehouse = ui->warehouseEdit->text();
    QString commentLevel = ui->commentLevelComboBox->currentText();
    qint64 shippingBeginTime = ui->shippingBeginDateTimeEdit->dateTime().toSecsSinceEpoch();
    qint64 shippingEndTime = ui->shippingEndDateTimeEdit->dateTime().toSecsSinceEpoch();
    qint64 commentBeginTime = ui->commentBeginDateTimeEdit->dateTime().toSecsSinceEpoch();
    qint64 commentEndTime = ui->commentEndDateTimeEdit->dateTime().toSecsSinceEpoch();
    QString goodName = ui->goodNameEdit->text();
    QString darenName = ui->darenEdit->text();
    QString commentContent = ui->commentContentEdit->text();

    if (shippingBeginTime > shippingEndTime)
    {
        UiUtil::showTip(QString::fromWCharArray(L"发货开始时间不能比结束时间晚"));
        return false;
    }

    if (commentBeginTime > commentEndTime)
    {
        UiUtil::showTip(QString::fromWCharArray(L"评价开始时间不能比结束时间晚"));
        return false;
    }

    const QString all = QString::fromWCharArray(L"不限");
    const QVector<OrderComment>& rawOrderComments = DataManager::getInstance()->getOrderComments();
    for (auto& orderComment : rawOrderComments)
    {
        if (!warehouse.isEmpty() && orderComment.m_order.m_shippingWareHouse.indexOf(warehouse) < 0)
        {
            continue;
        }

        if (commentLevel != all && orderComment.m_comment.m_commentLevel.indexOf(commentLevel) < 0)
        {
            continue;
        }

        qint64 shippingTimeUtc = orderComment.m_order.getShippingTimeUtc();
        if (shippingTimeUtc < shippingBeginTime || shippingTimeUtc > shippingEndTime)
        {
            continue;
        }

        qint64 commentTimeUtc = orderComment.m_comment.getCommentTimeUtc();
        if (commentTimeUtc < commentBeginTime || commentTimeUtc > commentEndTime)
        {
            continue;
        }

        if (!goodName.isEmpty() && orderComment.m_order.m_goodsName.indexOf(goodName) < 0)
        {
            continue;
        }

        if (!darenName.isEmpty() && orderComment.m_order.m_darenName.indexOf(darenName) < 0)
        {
            continue;
        }

        if (!commentContent.isEmpty() && orderComment.m_comment.m_commentContent.indexOf(commentContent) < 0)
        {
            continue;
        }

        orderComments.append(orderComment);
    }

    return true;
}

void ExcelDialog::onExportCommentContentSummary()
{
    QVector<OrderComment> orderComments;
    if (!selectOrderComment(orderComments))
    {
        return;
    }

    QVector<QVector<QString>> datas;
    for (auto& orderComment : orderComments)
    {
        QVector<QString> data;
        data.append(orderComment.m_comment.m_shopName);
        data.append(orderComment.m_comment.m_orderId);
        data.append(orderComment.m_comment.m_goodsInfo);
        data.append(orderComment.m_comment.m_goodsId);
        data.append(orderComment.m_comment.m_commentTime);
        data.append(orderComment.m_comment.m_commentLevel);
        data.append(orderComment.m_comment.m_commentContent);
        data.append(orderComment.m_order.m_shippingWareHouse);
        data.append(orderComment.m_order.m_shippingDate);
        data.append(orderComment.m_order.m_goodsName);
        data.append(orderComment.m_order.m_darenName);
        datas.append(data);
    }

    QString srcExcelFileName = QString::fromWCharArray(L"评价内容统计表.xlsx");
    QString srcExcelFilePath = QString::fromStdWString(CImPath::GetConfPath()) + srcExcelFileName;
    QString destExcelFileName = QString::fromWCharArray(L"评价内容统计表_") + QDateTime::currentDateTime().toString("yyyyMMdd_hhmm") + ".xlsx";
    QString destExcelFilePath = QString::fromStdWString(CImPath::GetDataPath()) + destExcelFileName;
    ::DeleteFile(destExcelFilePath.toStdWString().c_str());
    if (!::CopyFile(srcExcelFilePath.toStdWString().c_str(), destExcelFilePath.toStdWString().c_str(), TRUE))
    {
        UiUtil::showTip(QString::fromWCharArray(L"导出失败，无法拷贝模板表格"));
        return;
    }

    if (!ExcelUtil::saveExcel(destExcelFilePath, datas))
    {
        UiUtil::showTip(QString::fromWCharArray(L"导出失败，无法保存数据"));
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdWString(CImPath::GetDataPath())));
}

void ExcelDialog::onExportCommentLevelSummary()
{
    QVector<OrderComment> orderComments;
    if (!selectOrderComment(orderComments))
    {
        return;
    }

    // 评价等级个数统计
    QVector<CommentLevelSummary> levelSummaries;
    QVector<CommentLevelSummary> totals; // 不分等级统计
    for (auto& orderComment : orderComments)
    {
        bool found = false;
        for (auto& levelSummary : levelSummaries)
        {
            if (levelSummary.m_goodsName == orderComment.m_order.m_goodsName
                    && levelSummary.m_wareHouse == orderComment.m_order.m_shippingWareHouse
                    && levelSummary.m_commentLevel == orderComment.m_comment.m_commentLevel)
            {
                levelSummary.m_commentCount++;
                found = true;
                break;
            }
        }

        if (!found)
        {
            CommentLevelSummary levelSummaryItem;
            levelSummaryItem.m_commentCount = 1;
            levelSummaryItem.m_commentLevel = orderComment.m_comment.m_commentLevel;
            levelSummaryItem.m_goodsName = orderComment.m_order.m_goodsName;
            levelSummaryItem.m_wareHouse = orderComment.m_order.m_shippingWareHouse;
            levelSummaries.append(levelSummaryItem);
        }

        found = false;
        for (auto& total : totals)
        {
            if (total.m_goodsName == orderComment.m_order.m_goodsName
                    && total.m_wareHouse == orderComment.m_order.m_shippingWareHouse)
            {
                total.m_commentCount++;
                found = true;
                break;
            }
        }

        if (!found)
        {
            CommentLevelSummary summaryItem;
            summaryItem.m_commentCount = 1;
            summaryItem.m_goodsName = orderComment.m_order.m_goodsName;
            summaryItem.m_wareHouse = orderComment.m_order.m_shippingWareHouse;
            totals.append(summaryItem);
        }
    }

    // 计算比例
    for (auto& levelSummary : levelSummaries)
    {
        for (auto& total : totals)
        {
            if (levelSummary.m_goodsName == total.m_goodsName
                    && levelSummary.m_wareHouse == total.m_wareHouse)
            {
                levelSummary.m_commentRatio = levelSummary.m_commentCount * 1.0f / total.m_commentCount * 100;
                break;
            }
        }
    }

    // 按商品简名和仓库、评价等级排序
    std::sort(levelSummaries.begin(), levelSummaries.end(), [](const CommentLevelSummary& a, const CommentLevelSummary b) {
            if (a.m_goodsName != b.m_goodsName)
            {
                return a.m_goodsName < b.m_goodsName;
            }

            if (a.m_wareHouse != b.m_wareHouse)
            {
                return a.m_wareHouse < b.m_wareHouse;
            }

            if (a.m_commentLevel != b.m_commentLevel)
            {
                return a.m_commentLevel < b.m_commentLevel;
            }

            return true;
        });

    // 整理输出表格
    QVector<QString> empty;
    for (int i=0; i<5; i++)
    {
        empty.push_back("");
    }

    QString currentGoodsName;
    QString currentWareHouseName;
    QVector<QVector<QString>> datas;
    for (auto& levelSummary : levelSummaries)
    {
        if (levelSummary.m_goodsName != currentGoodsName)
        {
            QVector<QString> data = empty;
            data[0] = levelSummary.m_goodsName;
            datas.append(data);
            currentGoodsName = levelSummary.m_goodsName;
            currentWareHouseName = "";
        }

        if (levelSummary.m_wareHouse != currentWareHouseName)
        {
            QVector<QString> data = empty;
            data[1] = levelSummary.m_wareHouse;
            datas.append(data);
            currentWareHouseName = levelSummary.m_wareHouse;
        }

        QVector<QString> data = empty;
        data[2] = levelSummary.m_commentLevel;
        data[3] = QString::number(levelSummary.m_commentCount);
        data[4] = QString::number(levelSummary.m_commentRatio, 'f', 2);
        datas.append(data);
    }

    QString srcExcelFileName = QString::fromWCharArray(L"评价等级统计表.xlsx");
    QString srcExcelFilePath = QString::fromStdWString(CImPath::GetConfPath()) + srcExcelFileName;
    QString destExcelFileName = QString::fromWCharArray(L"评价等级统计表_") + QDateTime::currentDateTime().toString("yyyyMMdd_hhmm") + ".xlsx";
    QString destExcelFilePath = QString::fromStdWString(CImPath::GetDataPath()) + destExcelFileName;
    ::DeleteFile(destExcelFilePath.toStdWString().c_str());
    if (!::CopyFile(srcExcelFilePath.toStdWString().c_str(), destExcelFilePath.toStdWString().c_str(), TRUE))
    {
        UiUtil::showTip(QString::fromWCharArray(L"导出失败，无法拷贝模板表格"));
        return;
    }

    if (!ExcelUtil::saveExcel(destExcelFilePath, datas))
    {
        UiUtil::showTip(QString::fromWCharArray(L"导出失败，无法保存数据"));
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdWString(CImPath::GetDataPath())));
}
