#include "myprogressdialog.h"
#include <QMessageBox>

MyProgressDialog::MyProgressDialog(const QString &labelText, const QString &cancelButtonText,
                                   int minimum, int maximum, QWidget *parent,
                                   Qt::WindowFlags flags)
    :QProgressDialog(labelText, cancelButtonText, minimum, maximum, parent, flags)
{    
    setWindowModality(Qt::WindowModal);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(470, 80);
}

void MyProgressDialog::showTip(QString tip)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(QString::fromWCharArray(L"提示"));
    msgBox.setText(tip);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void MyProgressDialog::closeEvent(QCloseEvent *e)
{    
    if (m_canClose)
    {
        e->accept();
    }
    else
    {
        //showTip(QString::fromWCharArray(L"正在处理中"));
        e->ignore();
    }
}
