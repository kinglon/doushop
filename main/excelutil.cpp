#include "excelutil.h"
#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"
#include "datamodel.h"
#include <QVector>
#include <QFileInfo>
#include <QDesktopServices>
#include "Utility/ImPath.h"

using namespace QXlsx;

bool ExcelUtil::loadExcel(const QString& excelFilePath, QVector<QVector<QString>>& excelDatas)
{
    Document excel(excelFilePath);
    if (!excel.load())
    {
        return false;
    }

    CellRange excelRange = excel.dimension();
    for (int row=1; row <= excelRange.lastRow(); row++)
    {
        QVector<QString> rowContents;
        for (int column=1; column <=excelRange.lastColumn(); column++)
        {
            Cell* cell = excel.cellAt(row, column);
            if (cell)
            {
                QVariant rawValue = cell->value();
                QString value = rawValue.toString();
                if (rawValue.type() != QVariant::String)
                {
                    value = cell->readValue().toString();
                }
                rowContents.push_back(value);
            }
            else
            {
                rowContents.push_back("");
            }
        }

        excelDatas.push_back(rowContents);
    }

    return true;
}


bool ExcelUtil::saveExcel(const QString& excelFilePath, QVector<QVector<QString>>& datas)
{
    Document xlsx(excelFilePath);
    if (!xlsx.load())
    {
        return false;
    }

    // 从第2行开始写
    int row = 2;
    for (const auto& data : datas)
    {
        for (int i=0; i<data.size(); i++)
        {
            xlsx.write(row, i+1, data[i]);
        }
        row++;
    }

    if (!xlsx.save())
    {
        return false;
    }

    return true;
}
