#ifndef EXCELDIALOG_H
#define EXCELDIALOG_H

#include <QDialog>
#include <QThread>
#include "myprogressdialog.h"
#include "excelhandler.h"
#include "datamanager.h"

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

    int getColumnIndex(const QVector<QString>& header, QString columnName);

    bool selectOrderComment(QVector<OrderComment>& orderComments);

private:
    virtual void closeEvent(QCloseEvent *e) override;

public slots:
    void onImportJushuitanData();

    void onExportCommentContentSummary();

    void onExportCommentLevelSummary();

private:
    Ui::ExcelDialog *ui;
};

#endif // EXCELDIALOG_H
