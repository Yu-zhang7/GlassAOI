#include "CustomTableModel.h"
#include <QDateTime>
#include <QSqlRecord>
#include <QRegularExpression>
#include "Global.h"

QVariant CustomTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // 获取列名
    QString columnName = this->record().fieldName(index.column());

    if (role == Qt::DisplayRole)
    {
        if (columnName == "time_produce" || columnName == "dateTime")  // 替换为实际的时间列名
        {
            QString originalData = QSqlTableModel::data(index, role).toString();
            QString time = QDateTime::fromString(originalData, "yyyyMMddhhmmss").toString("yyyy-MM-dd hh:mm:ss");
            return time;
        }

        if (columnName == "errrank_produce" || columnName =="NGStatus")  // 替换为实际的NG列名
        {
            bool ng = QSqlTableModel::data(index, role).toBool();
            return ng ? tr("OK") : tr("NG");
        }

        if (columnName == "length_produce" || columnName == "length"
            || columnName == "width_produce" || columnName == "width")  // 替换为实际的浮点数列名
        {
            float value = QSqlTableModel::data(index, role).toFloat();
            return QString::number(value, 'f', 2);
        }
    }

    return QSqlTableModel::data(index, role);
}

QVariant CustomTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        // 获取列名
        QString columnName = this->record().fieldName(section);

        // 基础列名映射
         QHash<QString, QString> baseColumnNames = {
            {"dateTime", tr("Date Time")},
            {"glassId", tr("Glass ID")},
            {"length", tr("Length")},
            {"width", tr("Width")},
            {"errorCount", tr("Defect Count")},
            {"NGStatus", tr("NG Status")},
            {"defectFilePath", tr("Defect File Path")}
        };

        // 检查是否是基础列
        if (baseColumnNames.contains(columnName)) {
            return baseColumnNames[columnName];
        }

        // 检查是否是errnum列
        QRegularExpression re("errnum(\\d+)");
        QRegularExpressionMatch match = re.match(columnName);

        if (match.hasMatch()) {
            int index = match.captured(1).toInt();
            if (index >= 0 && index < defectTypes.size()) {
                return DefectTypeToString(defectTypes[index]);
            }
        }
    }

    return QSqlTableModel::headerData(section, orientation, role);
}