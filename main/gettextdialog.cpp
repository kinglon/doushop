#include "gettextdialog.h"
#include "ui_gettextdialog.h"
#include "uiutil.h"

GetTextDialog::GetTextDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GetTextDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowModality(Qt::WindowModal);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);

    initCtrls();
}

GetTextDialog::~GetTextDialog()
{
    delete ui;
}

void GetTextDialog::setTitle(QString title)
{
    setWindowTitle(title);
}

void GetTextDialog::setKeyName(QString keyName)
{
    ui->textKeyLabel->setText(keyName);
}

void GetTextDialog::initCtrls()
{
    connect(ui->cancelButton, &QPushButton::clicked, [this] () {
        close();
    });

    connect(ui->okButton, &QPushButton::clicked, [this] () {
        m_textContent = ui->textValueEdit->text();
        if (m_textContent.isEmpty())
        {
            UiUtil::showTip(QString::fromWCharArray(L"输入内容不能为空"));
            return;
        }
        done(Accepted);
    });
}
