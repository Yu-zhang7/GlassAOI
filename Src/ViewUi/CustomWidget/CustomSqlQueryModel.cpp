#include "CustomSqlQueryModel.h"
#include <QDateTime>
#include <QRegularExpression>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include "Global.h"

//CustomSqlQueryModel::CustomSqlQueryModel(QObject* parent, QSqlDatabase db)
//    : QSqlQueryModel(parent)
//    , m_db(db)
//    , m_columnNamesCached(false)
//{
//    // 如果没有传入数据库连接，使用默认连接
//    if (!m_db.isValid()) {
//        m_db = QSqlDatabase::database();
//    }
//}

CustomSqlQueryModel::CustomSqlQueryModel(QObject* parent)
    : QSqlQueryModel(parent)
    , m_columnNamesCached(false)
{
}
void CustomSqlQueryModel::setQuery(const QString& query, const QSqlDatabase& db)
{
    if (db.isValid() && db.isOpen())
    {
        clear();
        QSqlQueryModel::setQuery(query, db);
        m_columnNamesCached = false;
        cacheColumnNames();
    }
    else {
        qWarning() << "Database is not valid or not open for query:" << query;
    }
}

void CustomSqlQueryModel::cacheColumnNames() const
{
    if (m_columnNamesCached) {
        return;
    }

    m_columnNames.clear();
    m_columnIndexes.clear();

    // 获取查询结果的元数据
    for (int i = 0; i < columnCount(); i++) {
        QString columnName = QSqlQueryModel::headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
        m_columnNames[i] = columnName;
        m_columnIndexes[columnName] = i;
    }

    m_columnNamesCached = true;
}

QString CustomSqlQueryModel::getColumnName(int column) const
{
    if (!m_columnNamesCached) {
        cacheColumnNames();
    }

    return m_columnNames.value(column);
}

QVariant CustomSqlQueryModel::getValue(int row, int column) const
{
    QModelIndex index = createIndex(row, column);
    return data(index, Qt::DisplayRole);
}

QVariant CustomSqlQueryModel::getValue(int row, const QString& columnName) const
{
    if (!m_columnNamesCached) {
        cacheColumnNames();
    }

    int column = m_columnIndexes.value(columnName, -1);
    if (column == -1) {
        return QVariant();
    }

    return getValue(row, column);
}

void CustomSqlQueryModel::refresh(const QSqlDatabase& db)
{
    QSqlQuery query = this->query();
    if (query.lastQuery().isEmpty()) {
        return;
    }

    setQuery(query.lastQuery(), db);
}

QVariant CustomSqlQueryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // 获取列名
    QString columnName = getColumnName(index.column());
    //QString columnName = this->record().fieldName(index.column());

    if (role == Qt::DisplayRole)
    {
        if (columnName == "time_produce" || columnName == "date_Time" || columnName == "dateTime")  // 替换为实际的时间列名
        {
            QString originalData = QSqlQueryModel::data(index, role).toString(); 
            QString time = "";
            if (originalData.length() == 17)
            {
                if (Language == u8"中文")
                {
                    time = QDateTime::fromString(originalData, "yyyyMMddhhmmsszzz").toString("yyyy-MM-dd hh:mm:ss_zzz");
                }
                else
                {
                    time = QDateTime::fromString(originalData, "yyyyMMddhhmmsszzz").toString("dd-MM-yyyy hh:mm:ss_zzz");
                }
                
            }
            else if (originalData.length() == 8)
            {
                if (Language == u8"中文")
                {
                    time = QDateTime::fromString(originalData, "yyyyMMdd").toString("yyyy-MM-dd");
                }
                else
                {
                    time = QDateTime::fromString(originalData, "yyyyMMdd").toString("dd-MM-yyyy");
                }
            }
            else// yyyyMMddhhmmss 格式
            {
                if (Language == u8"中文")
                {
                    time = QDateTime::fromString(originalData, "yyyyMMddhhmmss").toString("yyyy-MM-dd hh:mm:ss");
                }
                else
                {
                    time = QDateTime::fromString(originalData, "yyyyMMddhhmmss").toString("dd-MM-yyyy hh:mm:ss");
                }
            }
            return time;
        }

        if (columnName == "errrank_produce" || columnName =="NGStatus")  // 替换为实际的NG列名
        {
            bool ng = QSqlQueryModel::data(index, role).toBool();
            return ng ? tr("OK") : tr("NG");
        }

        if (columnName == "length_produce" || columnName == "length"
            || columnName == "width_produce" || columnName == "width")  // 替换为实际的浮点数列名
        {
            float value = QSqlQueryModel::data(index, role).toFloat();
            return QString::number(value, 'f', 2);
        }
    }

    return QSqlQueryModel::data(index, role);
}

QVariant CustomSqlQueryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        // 获取列名
        QString columnName = getColumnName(section);
        //QString columnName = this->record().fieldName(section);

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

    return QSqlQueryModel::headerData(section, orientation, role);
}