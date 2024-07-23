#ifndef EXCELUTIL_H
#define EXCELUTIL_H

#include <QString>
#include <QVector>

class ExcelUtil
{
public:
    static bool loadExcel(const QString& excelFilePath, QVector<QVector<QString>>& datas);

    static bool saveExcel(const QString& excelFilePath, QVector<QVector<QString>>& datas);
};

#endif // EXCELUTIL_H
