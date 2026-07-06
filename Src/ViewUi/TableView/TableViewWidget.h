#pragma once

#include <QWidget>
#include "ui_TableViewWidget.h"
#include <QtCharts>  // 使用统一头文件代替单独包含
#include <QSplitter>
#include "ChartManager.h"           //图表管理类
#include "CustomSqlQueryModel.h"
#include "CustomTableModel.h"
#include "Global.h"
#include "DefectImageShow.h"
#include "DefectFilterWidget.h"	//储存瑕疵类型过滤器

/* 导出Excel */
#include "xlsxdocument.h"
#include "xlsxformat.h"
#include "xlsxworksheet.h"  

QT_BEGIN_NAMESPACE
namespace Ui { class TableViewWidgetClass; };
QT_END_NAMESPACE

class TableViewWidget : public QWidget
{
	Q_OBJECT

public:
	TableViewWidget(QWidget *parent = nullptr);
	~TableViewWidget();

	/* 显示当次记录的数据 */
	void CurrentRecord_ShowDefectData(std::string timeProduce, std::vector<drawInformation>& data);

	void HistoryRecord_InitTable(CustomTableModel* modelHistory);

	void HistoryRecord_InitTableQry(CustomSqlQueryModel* modelHistoryQry);

	void HistoryRecord_SetColumnsDisplayStatus();

    /* 按照配方设置的显示状态，设置表格中相应列的显示状态 */
    void HistoryRecord_SetDefectTypeDisplay();

	void HistoryRecord_GetTimeFileter();

	/* 弹窗显示瑕疵图像 */
	void HistoryRecord_ShowImageDialog(QString originalTime, const QString& filesPath, const QDateTime& dateTime,
		const float& glassPhysicalWidth, const float& glassPhysicalHeight);

	/* 关闭图像弹窗 */
	void HistoryRecord_CloseImageDialog();

	/* 刷新历史记录的表格 */
	void HistoryRecordDefect_RefreshTable(const QString& filesPath);

	/* 设置默认显示tab页 */
	void SetDefaultTabIndex(int index);

	/* 设置导出数据按钮的可用状态 */
	void SetExportDataButtonEnabled(bool isEnabled);

signals:
	void Signal_FilterRequested(const QString& filter);

	void Signal_ShowHistoryImage(int typeIndex, const QString originalTime, const QString glassId, float glassPhysicalWidth, float glassPhysicalHeight, QString filePath);

	void Signal_ShowThumbnailImage(QString originalTime, QString glassId);

	void Signal_NotifyExportDataStatus(bool isExporting);

public slots:
	void Slot_RealTimeChange(bool checked);

	void Slot_HistoryRecord_SetColumnsDisplayStatus();

	void Slot_HistoryRecord_DataTimeChanged(const QDateTime& dateTime);

	void Slot_HistoryRecord_OnlyNGChanged(bool checked);

	void Slot_HistoryRecord_handleSingleClick();

	void Slot_ShowHistory_Chart(const QMap<DefectType, QMap<QDate, int>>& data);

	/* 打开瑕疵整图 */
	void Slot_HistoryRecord_DoubleClick(const QModelIndex& index);

	/* 打开瑕疵整图 */
	void Slot_ClickHistoryRecord(const QModelIndex& index);

	/* 刷新当前记录表格的瑕疵类型名称 */
	void Slot_RefreshDefectTypeNameToTable();

	/* 清空current图像和数据的槽函数 */
	void Slot_HandleClearDisplay();

	/* 瑕疵类型过滤槽函数 */
	void Slot_HistoryRecord_onDefectFilterChanged(int index, bool checked);

    /* 设置瑕疵类型是否显示 */
    void Slot_SetDefectTypeVisibility(DefectType type, bool visible);

    /* 设置瑕疵j级别是否显示 */
    void Slot_SetDefectLevelVisibility(DefectLevel type, bool visible);

	/* tab切换 */
	void Slot_TableWidget_TabChanged(int index);

	/* 导出数据 */
	void Slot_ExportData();

private:
	Ui::TableViewWidgetClass *ui;

	QTableView*							m_tableHistoryView;					//表格视图
	QTableView*							m_tableNGView;						//表格视图  
	QSqlDatabase						m_db;								//数据库对象
	CustomTableModel*					m_modelHistory;						//历史记录表格模型
	CustomTableModel*					m_modelNG;							//NG记录表格模型

	/* 区分单击双击事件使用 */
	QTimer m_clickTimer;
	QModelIndex m_pendingIndex;

	std::unique_ptr<DefectImageShow>	m_defectImageShow;					//瑕疵图片显示

	PieChartWidget*						m_CurrentRecord_PieChart = nullptr;
	BarChartWidget*						m_CurrentRecord_BarChart = nullptr;

	QChartView*							m_ChartView_LineChart	 = nullptr;
	QChart*								m_Chart_LineChart		 = nullptr;	

	DefectFilterWidget*					m_defectFilterWidget;				//瑕疵类型过滤器

	QString								m_defectFilterStr;

	// 表格管理结构
	struct TableManager 
	{
		QTableWidget* tableWidget;
		QVector<QVector<QTableWidgetItem*>> tableItems;
		int maxRowCount;
		int fixedColumnCount;
		int usedRowCount;
	};

	TableManager m_currentDefectTable;
	TableManager m_historyDefectTable; // 假设另一个表格叫otherDefectTable

	QMutex m_initMutex;                    // 保护初始化过程

    QMutex m_mutexDefectDisplay;                    // 保护瑕疵类型显示状态
	bool m_tablesInitialized = false;

    // 存储系列指针，方便后续控制
    QMap<DefectType, QLineSeries*> m_lineSeriesMap;
    QMap<DefectType, QScatterSeries*> m_scatterSeriesMap;
    QMap<DefectType, QString> m_defectTypeOriginalLabels; 

    // 存储瑕疵详情表的行索引
    QMap<DefectType, int> m_defectTableRowIndexMap;

    // 存储当前记录的瑕疵数量
    QMap<DefectType, int> m_currentCountMap;

private:
    // 通用函数
    void InitializeTableManager(TableManager& manager, QTableWidget* table, int maxRows, int columns);
    void ClearTableItems(TableManager& manager);
    void PopulateDefectTable(TableManager& manager, const std::vector<drawInformation>& showInfo);
    void SetDefectRowDisplayStatus(TableManager& manager, DefectType type, bool isDisplayed);
    void SafeUpdateTableItem(TableManager& manager, int row, int col, const QString& text,
        const QVariant& userData = QVariant(), const QBrush& background = QBrush());

    void InitTableWidget(TableManager& manager);

    void EnsureTablesInitialized(bool refreshFlag = false);


	/* 连接信号和槽函数 */
	void InitConnect();

	/* 计算到下一个0点的毫秒数 */
	void scheduleNextUpdate();

	/* 更新日期时间控件 */
	void UpdateDateTime();

	/* 刷新显示指定数据集(时间段等)的折线图 */
	void HistoryRecordChart_ShowData(const QMap<DefectType, QMap<QDate, int>>& data);

    /* 设置缺陷类型的显示状态 */
    void HistoryRecordChart_SetDefectTypeVisible(DefectType type, bool visible);

	/* 显示历史记录的瑕疵详情 */
	void HistoryRecordDefect_PopulateTable(const std::vector<drawInformation>& showInfo);

    /* 更新图例的间距 */
    void HistoryRecordChart_UpdateLegendSpacing();

    /* 比较函数，按严重程度（Errorinfo的第一个元素）排序 */
	static bool CurrentRecord_CompareBySeverity(const drawInformation& a, const drawInformation& b);

	/* 显示当前记录的瑕疵详情到表格 */
	void CurrentRecord_PopulateDefectTable(const std::vector<drawInformation>& showInfo);

	/* 初始化当前记录 */ 
	void CurrentReocrd_Init();

	/* 初始化当前记录的表格 */
	void CurrentRecord_InitTableWidget();

	/* 显示当前记录的图表 */
	void CurrentReocrd_ShowChart(const std::vector<drawInformation>& drawInfo);

    /* 设置指定列是否显示 */
    void SetColumnVisibility(QTableView* tableView, int column, bool display);

	/* 导出表格数据到Excel */
	bool HistoryRecord_exportTableViewToExcel(QTableView* tableView, const QString& filePath);
};

