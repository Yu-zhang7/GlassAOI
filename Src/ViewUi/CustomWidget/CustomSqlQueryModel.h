#ifndef CUSTOMSQLQUERYMODEL_H
#define CUSTOMSQLQUERYMODEL_H

#include <QSqlQueryModel>
#include <QSqlDatabase>

class CustomSqlQueryModel : public QSqlQueryModel
{
    Q_OBJECT

public:
    // 构造函数
    //CustomSqlQueryModel(QObject* parent = nullptr, QSqlDatabase db = QSqlDatabase());
    CustomSqlQueryModel(QObject* parent = nullptr);
    ~CustomSqlQueryModel() = default;

    /* 重写data函数，让界面显示的日期为标准时间 */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    /* 重写headerData, 让表头显示为中文名称 */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /* 设置查询并刷新模型 */
    //void setQuery(const QString& query);
    void setQuery(const QString& query, const QSqlDatabase& db = QSqlDatabase());
    /* 获取原始列名 */
    QString getColumnName(int column) const;

    /* 获取行数据 */
    QVariant getValue(int row, int column) const;
    QVariant getValue(int row, const QString& columnName) const;


    /* 刷新数据 */
    void refresh(const QSqlDatabase& db);

    /* 获取关联的数据库 */
    QSqlDatabase database() const { return m_db; }

private:
    QSqlDatabase m_db;

    // 缓存列名映射
    mutable QHash<int, QString> m_columnNames;
    mutable QHash<QString, int> m_columnIndexes;
    mutable bool m_columnNamesCached;

    // 缓存列名
    void cacheColumnNames() const;
};

#endif // CUSTOMSQLQUERYMODEL_H