#include "exceldialog.h"
#include "ui_exceldialog.h"
#include <QFileDialog>
#include <QTimer>
#include "uiutil.h"

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

    m_workerThread.start();
}

ExcelDialog::~ExcelDialog()
{
    delete ui;
}

void ExcelDialog::initCtrls()
{
    connect(ui->selectCommentButton, &QPushButton::clicked, [this]() {
        QString filePath = selectLocalFile();
        if (!filePath.isEmpty())
        {
            ui->excel1LineEdit->setText(filePath);
        }
    });

    connect(ui->selectJushuitanButton, &QPushButton::clicked, [this]() {
        QString filePath = selectLocalFile();
        if (!filePath.isEmpty())
        {
            ui->excel2LineEdit->setText(filePath);
        }
    });

    // 合并
    connect(ui->mergeButton, &QPushButton::clicked, [this]() {
        merge();
    });
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

void ExcelDialog::merge()
{
    QString excel1FilePath = ui->excel1LineEdit->text();
    QString excel2FilePath = ui->excel2LineEdit->text();
    if (excel1FilePath.isEmpty())
    {
        UiUtil::showTip(QString::fromWCharArray(L"请先选择表格1"));
        return;
    }

    if (excel2FilePath.isEmpty())
    {
        UiUtil::showTip(QString::fromWCharArray(L"请先选择表格2"));
        return;
    }

    // 显示进度条
    m_progressDlg = new MyProgressDialog(QString(), QString(), 0, 30, this);
    m_progressDlg->setAttribute(Qt::WA_DeleteOnClose);
    m_progressDlg->setWindowTitle(QString::fromWCharArray(L"合并表格"));
    m_progressDlg->setLabelText(QString::fromWCharArray(L"正在合并表格"));
    m_progressDlg->setValue(0);
    m_progressDlg->show();

    // 更新进度条
    m_progressTimer = new QTimer(m_progressDlg);
    m_progressTimer->setInterval(1000);
    connect(m_progressTimer, &QTimer::timeout, [this]() {
        int value = m_progressDlg->value();
        if (value >= m_progressDlg->maximum() - 2)
        {
            m_progressTimer->stop();
        }
        else
        {
            m_progressDlg->setValue(value+1);
        }
    });
    m_progressTimer->start();

    // 开始合并
    m_excelHandler = new ExcelHandler();
    connect(this, &ExcelDialog::mergeSignal, m_excelHandler, &ExcelHandler::merge);
    connect(m_excelHandler, &ExcelHandler::mergeFinish, this, &ExcelDialog::onMergeFinish);
    m_excelHandler->moveToThread(&m_workerThread);
    emit mergeSignal(excel1FilePath, excel2FilePath);
}

void ExcelDialog::onMergeFinish(bool ok)
{
    if (m_progressTimer)
    {
        m_progressTimer->stop();
        m_progressTimer = nullptr;
    }

    if (m_progressDlg)
    {
        if (!ok)
        {
            UiUtil::showTip(QString::fromWCharArray(L"合并失败"));
        }
        m_progressDlg->setCanClose();
        m_progressDlg->close();
        m_progressDlg = nullptr;
    }

    if (m_excelHandler)
    {
        m_excelHandler->deleteLater();
        m_excelHandler = nullptr;
    }
}

void ExcelDialog::closeEvent(QCloseEvent *e)
{
    m_workerThread.quit();
    e->accept();
}
