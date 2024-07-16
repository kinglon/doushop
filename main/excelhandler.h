#ifndef EXCELHANDLER_H
#define EXCELHANDLER_H

#include <QObject>
#include <QMap>
#include "datamodel.h"
#include "xlsxdocument.h"

class ExcelHandler : public QObject
{
    Q_OBJECT
public:
    explicit ExcelHandler(QObject *parent = nullptr);

public slots:
    void merge(QString excel1FilePath, QString excel2FilePath);

signals:
    void mergeFinish(bool ok);

private:
    // 移除前后的双引号
    QString removeQuote(const QString& text);

    bool doMerge(QString excel1FilePath, QString excel2FilePath);

    int getColumnIndex(const QVector<QString>& header, QString columnName);

    bool saveMergeData(QString filePath, QVector<QVector<QString>>& datas);
};

#endif // EXCELHANDLER_H
