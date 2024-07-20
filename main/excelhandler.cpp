#include "excelhandler.h"

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

ExcelHandler::ExcelHandler(QObject *parent)
    : QObject{parent}
{

}

void ExcelHandler::merge(QString excel1FilePath, QString excel2FilePath)
{
    bool ok = doMerge(excel1FilePath, excel2FilePath);
    emit mergeFinish(ok);
}

QString ExcelHandler::removeQuote(const QString& text)
{
    int begin = 0;
    for (; begin < text.length(); begin++)
    {
        if (text[begin] != '"')
        {
            break;
        }
    }

    if (begin >= text.length())
    {
        return "";
    }

    int end = text.length() - 1;
    for (; end >= begin; end--)
    {
        if (text[end] != '"')
        {
            break;
        }
    }

    return text.mid(begin, end-begin+1);
}

bool ExcelHandler::doMerge(QString excel1FilePath, QString excel2FilePath)
{
    QVector<QVector<QString>> results;

    Document excel1(excel1FilePath);
    if (!excel1.load())
    {
        qCritical("failed to load excel 1");
        return false;
    }

    CellRange excel1Range = excel1.dimension();
    for (int row=2; row <= excel1Range.lastRow(); row++)
    {
        QVector<QString> rowContents;
        for (int column=1; column <=7; column++)
        {
            Cell* cell = excel1.cellAt(row, column);
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

        results.push_back(rowContents);
    }

    Document excel2(excel2FilePath);
    if (!excel2.load())
    {
        qCritical("failed to load excel 2");
        return false;
    }

    QVector<QVector<QString>> excel2Datas;
    CellRange excel2Range = excel2.dimension();
    for (int row=1; row <= excel2Range.lastRow(); row++)
    {
        QVector<QString> rowContents;
        for (int column=1; column <=excel2Range.lastColumn(); column++)
        {
            Cell* cell = excel2.cellAt(row, column);
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

        excel2Datas.push_back(rowContents);
    }

    if (excel2Datas.size() == 0)
    {
        qCritical("there is not data in load excel 2");
        return false;
    }

    int orderIdIndex = getColumnIndex(excel2Datas[0], QString::fromWCharArray(L"线上子订单编号"));
    if (orderIdIndex < 0)
    {
        qCritical("failed to find the column index in load excel 2");
        return false;
    }

    int fahuodateIndex = getColumnIndex(excel2Datas[0], QString::fromWCharArray(L"发货日期"));
    int fahuochangIndex = getColumnIndex(excel2Datas[0], QString::fromWCharArray(L"发货仓"));
    int otherAtt1 = getColumnIndex(excel2Datas[0], QString::fromWCharArray(L"其它属性1"));
    int darenIndex = getColumnIndex(excel2Datas[0], QString::fromWCharArray(L"达人名称"));
    int indexArray[] = {fahuochangIndex, fahuodateIndex, otherAtt1, darenIndex};
    for (int i=0; i<ARRAYSIZE(indexArray); i++)
    {
        if (indexArray[i] < 0)
        {
            qCritical("failed to find the column index in load excel 2");
            return false;
        }
    }

    const int orderIdIndex2 = 1;
    for (auto& result : results)
    {
        bool found = false;
        for (const auto& excel2Data : excel2Datas)
        {
            if (excel2Data.length() > orderIdIndex && excel2Data[orderIdIndex] == result[orderIdIndex2])
            {
                for (int i=0; i<ARRAYSIZE(indexArray); i++)
                {
                    if (excel2Data.length() > indexArray[i])
                    {
                        result.append(excel2Data[indexArray[i]]);
                    }
                    else
                    {
                        result.append("");
                    }
                }
                found = true;
                break;
            }            
        }

        if (!found)
        {
            for (int i=0; i<ARRAYSIZE(indexArray); i++)
            {
                result.append("");
            }
        }
    }

    // 输出表格
    QFileInfo fileInfo(excel1FilePath);
    QString saveFilePath = fileInfo.absolutePath() + QString::fromWCharArray(L"\\评价与聚水潭合并.xlsx");
    if (!saveMergeData(saveFilePath, results))
    {
        qCritical(QString::fromWCharArray(L"保存表格失败").toStdString().c_str());
        return false;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
    return true;
}

int ExcelHandler::getColumnIndex(const QVector<QString>& headers, QString columnName)
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

bool ExcelHandler::saveMergeData(QString destExcelFilePath, QVector<QVector<QString>>& datas)
{
    // 拷贝表格模板到保存目录
    QString excelFileName = QString::fromWCharArray(L"合并后表格.xlsx");
    QString srcExcelFilePath = QString::fromStdWString(CImPath::GetConfPath()) + excelFileName;
    ::DeleteFile(destExcelFilePath.toStdWString().c_str());
    if (!::CopyFile(srcExcelFilePath.toStdWString().c_str(), destExcelFilePath.toStdWString().c_str(), TRUE))
    {
        qCritical("failed to copy the result excel file");
        return false;
    }

    Document xlsx(destExcelFilePath);
    if (!xlsx.load())
    {
        qCritical("failed to load the result excel file");
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
        qCritical("failed to save the result excel file");
        return false;
    }

    return true;
}

