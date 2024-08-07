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
    Document excel1(excel1FilePath);
    if (!excel1.load())
    {
        qCritical("failed to load excel 1");
        return false;
    }

    // 商品评论表
    QVector<QVector<QString>> excel1Datas;
    CellRange excel1Range = excel1.dimension();
    for (int row=1; row <= excel1Range.lastRow(); row++)
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

        excel1Datas.push_back(rowContents);
    }

    if (excel1Datas.size() < 2)
    {
        qCritical("there is not data in excel 1");
        return false;
    }

    Document excel2(excel2FilePath);
    if (!excel2.load())
    {
        qCritical("failed to load excel 2");
        return false;
    }

    // 聚水潭表格
    QVector<QVector<QString>> results;
    CellRange excel2Range = excel2.dimension();
    for (int row=2; row <= excel2Range.lastRow(); row++)
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

        results.push_back(rowContents);
    }

    if (results.size() < 1)
    {
        qCritical("there is not data in excel 2");
        return false;
    }

    // 商品评论表各列的索引
    int orderIdIndex = getColumnIndex(excel1Datas[0], QString::fromWCharArray(L"订单编号"));
    int commentTimeIndex = getColumnIndex(excel1Datas[0], QString::fromWCharArray(L"评价时间"));
    int commentLevelIndex = getColumnIndex(excel1Datas[0], QString::fromWCharArray(L"评价等级"));
    int commentContentIndex = getColumnIndex(excel1Datas[0], QString::fromWCharArray(L"评价内容"));
    int indexArray[] = {orderIdIndex, commentTimeIndex, commentLevelIndex, commentContentIndex};
    for (int i=0; i<ARRAYSIZE(indexArray); i++)
    {
        if (indexArray[i] < 0)
        {
            qCritical("failed to find the column index in excel 1");
            return false;
        }
    }    
    const int orderIdIndex2 = 2; // 聚水潭表格订单号索引

    // 聚水潭数据+商品评论数据
    for (int i=0; i < results.size(); i++)
    {
        for (int j=1; j < excel1Datas.size(); j++)
        {
            if (orderIdIndex2 < results[i].length() && orderIdIndex < excel1Datas[j].length()
                    && results[i][orderIdIndex2] == excel1Datas[j][orderIdIndex])
            {
                for (int k=1; k<ARRAYSIZE(indexArray); k++)
                {
                    if (indexArray[k] < excel1Datas[j].length())
                    {
                        results[i].append(excel1Datas[j][indexArray[k]]);
                    }
                }

                break;
            }
        }
    }

    // 输出表格
    QFileInfo fileInfo(excel2FilePath);
    QString saveFilePath = fileInfo.absolutePath() + QString::fromWCharArray(L"\\聚水潭商品评论.xlsx");
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

