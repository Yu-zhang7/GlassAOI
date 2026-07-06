#pragma once

#include <QWidget>
#include <QTableView>
#include "ui_HistoryRecordWidget.h"
#include "CustomSqlQueryModel.h"
#include "CustomTableModel.h"
#include "Global.h"

QT_BEGIN_NAMESPACE
namespace Ui { class HistoryRecordWidgetClass; };
QT_END_NAMESPACE

class HistoryRecordWidget : public QWidget
{
	Q_OBJECT

public:
	HistoryRecordWidget(QWidget *parent = nullptr);
	~HistoryRecordWidget();

	void HistoryRecord_InitTable(CustomTableModel* modelHistory);

	void HistoryRecord_InitTableQry(CustomSqlQueryModel* modelHistoryQry);

	void HistoryRecord_GetTimeFileter();

public slots:
    /* 指定缺陷类型是否显示 槽函数*/
    void Slot_SetDefectTypeVisibility(DefectType type, bool visible);

    /* 指定缺陷级别是否显示 槽函数*/
    void Slot_SetDefectLevelVisibility(DefectLevel level, bool visible);


	void Slot_HistoryRecord_SetColumnsDisplayStatus();

signals:
	void Signal_FilterRequested(const QString& filter);

	void Signal_ShowHistoryImage(int typeIndex, QString originalTime, QString glassId, float glassPhysicalWidth, float glassPhysicalHeight, QString filePath);

private slots:

	void Slot_DataTimeChanged(const QDateTime& dateTime);

	void Slot_OnlyNGChanged(bool checked);

	void Slot_RealTimeChange(bool checked);

	/* 打开瑕疵整图 */
	void Slot_HistoryRecord_DoubleClick(const QModelIndex& index);

private:
	/*连接信号和槽函数*/
	void InitConnect();

	/* 计算到下一个0点的毫秒数 */
	void scheduleNextUpdate();

	/* 更新日期时间控件 */
	void UpdateDateTime();

    /* 按照配方设置的显示状态，设置表格中相应列的显示状态 */
    void HistoryRecord_SetDefectTypeDisplay();

    /* 设置指定列是否显示 */
    void SetColumnVisibility(QTableView* tableView, int column, bool display);

	/* 设置指定列隐藏 */
	void HistoryRecord_SetColumnsDisplayStatus();

private:
	Ui::HistoryRecordWidgetClass *ui;

	CustomTableModel* m_modelHistory;				//历史记录表格模型

	int m_currentIndex = 0;

    // 存储瑕疵详情表的行索引
    QMap<DefectType, int> m_defectTableRowIndexMap;
};

