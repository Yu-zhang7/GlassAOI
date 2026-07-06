#ifndef CUSTOMTABLEMODEL_H
#define CUSTOMTABLEMODEL_H

#include <qsqltablemodel.h>

class CustomTableModel :public QSqlTableModel
{
    Q_OBJECT

public:
    CustomTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase()):
        QSqlTableModel(parent, db)
    {

    }
    ~CustomTableModel() = default;

    /* 重写data函数，让界面显示的日期为标准时间 */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /* 重写headerData, 让表头显示为中文名称 */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:

};

#endif
