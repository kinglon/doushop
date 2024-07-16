#ifndef EXCELDIALOG_H
#define EXCELDIALOG_H

#include <QDialog>
#include <QThread>
#include "myprogressdialog.h"
#include "excelhandler.h"

namespace Ui {
class ExcelDialog;
}

class ExcelDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExcelDialog(QWidget *parent = nullptr);
    ~ExcelDialog();

private:
    void initCtrls();

    QString selectLocalFile();

    void merge();

private:
    virtual void closeEvent(QCloseEvent *e) override;

public slots:
    void onMergeFinish(bool ok);

signals:
    void mergeSignal(QString gudongFilePath, QString nianbaoFilePath);

private:
    QThread m_workerThread;

    MyProgressDialog* m_progressDlg = nullptr;

    QTimer* m_progressTimer = nullptr;

    ExcelHandler* m_excelHandler = nullptr;

private:
    Ui::ExcelDialog *ui;
};

#endif // EXCELDIALOG_H
