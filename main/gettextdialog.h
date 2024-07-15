#ifndef GETTEXTDIALOG_H
#define GETTEXTDIALOG_H

#include <QDialog>

namespace Ui {
class GetTextDialog;
}

class GetTextDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GetTextDialog(QWidget *parent = nullptr);
    ~GetTextDialog();

public:
    // 设置窗口标题
    void setTitle(QString title);

    // 设置字段名字
    void setKeyName(QString keyName);

    // 获取输入的文本
    QString getText() { return m_textContent; }

private:
    void initCtrls();

private:
    QString m_textContent;

private:
    Ui::GetTextDialog *ui;
};

#endif // GETTEXTDIALOG_H
