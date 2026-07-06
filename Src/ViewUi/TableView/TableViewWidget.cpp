#include "TableViewWidget.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMutexLocker>
#include "Log.hpp"
#include "DefectInfoItem.h"
#include "Json/glassData2Json.h"    //Json操作类。用于读取历史数据的json信息使用。
#include "DisplayMessage.h"         //界面显示运行信息
#include "Global.h"

TableViewWidget::TableViewWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::TableViewWidgetClass())
{
	ui->setupUi(this);

    //EnsureTablesInitialized();

    // 初始化单击定时器
    m_clickTimer.setSingleShot(true);
    m_clickTimer.setInterval(250); // 设置双击检测间隔，通常为250ms
    connect(&m_clickTimer, &QTimer::timeout, this, &TableViewWidget::Slot_HistoryRecord_handleSingleClick);

    CurrentReocrd_Init();
    InitConnect();
    QMap<DefectType, QMap<QDate, int>> data;
    HistoryRecordChart_ShowData(data);

    ui->checkBox_RealTime->setChecked(true);

    /**************** 瑕疵类型过滤器设置 *******************/
    //初始化瑕疵类型过滤器
    m_defectFilterWidget = new DefectFilterWidget(this);
    // 连接复选框状态变化信号到父对象的槽函数
    m_defectFilterWidget->connectCheckBoxChangesTo(this, SLOT(Slot_HistoryRecord_onDefectFilterChanged(int, bool)));
    // 可以将其放置在任何布局中
    ui->hLayout_filter->addWidget(m_defectFilterWidget);
    /*****************************************************/

        /**************** 日期时间控件设置 *******************/
    // 初始更新时间
    UpdateDateTime();

    // 计算到下一个0点的毫秒数
    scheduleNextUpdate();

    //设置DataTimeEdit控件时间显示，并设置日历弹出
    ui->dateTimeEdit_begin->setCalendarPopup(true);
    ui->dateTimeEdit_end->setCalendarPopup(true);

    //解决点击日期区域年份会变的问题
    ui->dateTimeEdit_begin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dateTimeEdit_end->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dateTimeEdit_begin_Chart->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->dateTimeEdit_end_Chart->setButtonSymbols(QAbstractSpinBox::NoButtons);

    QTimer::singleShot(500, this, [this]() {
        HistoryRecord_GetTimeFileter(); //刷新初始筛选条件下的历史数据
        });
    /*****************************************************/
}

TableViewWidget::~TableViewWidget()
{
    //std::cout << "TableViewWidget::~TableViewWidget()00000000" << std::endl;
    if (m_defectImageShow)
    {
        m_defectImageShow.reset(); // 自动释放旧对象，无需手动 delete
    }
    //std::cout << "TableViewWidget::~TableViewWidget()111111" << std::endl;
	delete ui;
}


void TableViewWidget::InitConnect()
{
    connect(ui->dateTimeEdit_begin, &QDateTimeEdit::dateTimeChanged, this, &TableViewWidget::Slot_HistoryRecord_DataTimeChanged);
    connect(ui->dateTimeEdit_end, &QDateTimeEdit::dateTimeChanged, this, &TableViewWidget::Slot_HistoryRecord_DataTimeChanged);
    connect(ui->dateTimeEdit_begin_Chart, &QDateTimeEdit::dateTimeChanged, ui->dateTimeEdit_begin, &QDateTimeEdit::setDateTime);
    connect(ui->dateTimeEdit_end_Chart, &QDateTimeEdit::dateTimeChanged, ui->dateTimeEdit_end, &QDateTimeEdit::setDateTime);
    connect(ui->checkBox_OnlyNG, &QCheckBox::clicked, this, &TableViewWidget::Slot_HistoryRecord_OnlyNGChanged);
    connect(ui->tableView_History, &QTableView::doubleClicked, this, &TableViewWidget::Slot_HistoryRecord_DoubleClick);
    connect(ui->tableView_History, &QTableView::clicked, this, &TableViewWidget::Slot_ClickHistoryRecord);

    connect(ui->tabWidget_TableView, &QTabWidget::currentChanged, this, &TableViewWidget::Slot_TableWidget_TabChanged); //用于切换currentIndex时，刷新历史记录页面的过滤器

    //导出历史数据
    connect(ui->pBtn_ExportData, &QPushButton::clicked, this, &TableViewWidget::Slot_ExportData);

    connect(ui->checkBox_RealTime, &QCheckBox::clicked, this, &TableViewWidget::Slot_RealTimeChange);
}

void TableViewWidget::Slot_RealTimeChange(bool checked)
{
    IsRealTimeChange_Tab = checked;
}

void TableViewWidget::SetExportDataButtonEnabled(bool isEnabled)
{
	ui->pBtn_ExportData->setEnabled(isEnabled);
}

void TableViewWidget::scheduleNextUpdate()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime nextMidnight = now.date().addDays(1).startOfDay(); // 明天的0点
    int msecsToMidnight = now.msecsTo(nextMidnight);

    // 设置单次定时器，在0点触发
    QTimer::singleShot(msecsToMidnight, this, [this]() {
        UpdateDateTime();
        scheduleNextUpdate(); // 安排下一次更新
        });
}

void TableViewWidget::UpdateDateTime()
{
    //先取消旧的信号槽，避免同时改两个日期时间，造成的重复刷新
    disconnect(ui->dateTimeEdit_begin, &QDateTimeEdit::dateTimeChanged, this, &TableViewWidget::Slot_HistoryRecord_DataTimeChanged);

    QDateTime now = QDateTime::currentDateTime();
    // 设置开始时间为7天前的0点0分0秒
    QDateTime startDate = now.addDays(-7);
    startDate.setTime(QTime(0, 0, 0));
    ui->dateTimeEdit_begin->setDateTime(startDate);

    //恢复信号槽
    connect(ui->dateTimeEdit_begin, &QDateTimeEdit::dateTimeChanged, this, &TableViewWidget::Slot_HistoryRecord_DataTimeChanged);

    // 设置结束时间为当天的23点59分59秒
    QDateTime endDate = now;
    endDate.setTime(QTime(23, 59, 59));
    ui->dateTimeEdit_end->setDateTime(endDate);
    //ui->dateTimeEdit_end_Chart->setDateTime(endDate);
    
    m_FilterDateTim_Start = startDate;
    m_FilterDateTim_End = endDate;
}

void TableViewWidget::HistoryRecord_InitTable(CustomTableModel* modelHistory)
{
    ui->tableView_History->setModel(modelHistory);
    // 设置只允许整行选中
    ui->tableView_History->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_History->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->tableView_History->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView_History->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    
    ui->tableView_History->setColumnHidden(6, true);        //隐藏瑕疵文件路径列

    HistoryRecord_SetDefectTypeDisplay();
    //---------隐藏预留的瑕疵类型列
    ui->tableView_History->setColumnHidden(20, true);
    ui->tableView_History->setColumnHidden(21, true);
    ui->tableView_History->setColumnHidden(22, true);
    //---------

    ui->tableView_History->show();
}

void TableViewWidget::HistoryRecord_InitTableQry(CustomSqlQueryModel* modelHistoryQry)
{
    ui->tableView_History->setModel(modelHistoryQry);
    // 设置只允许整行选中
    ui->tableView_History->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_History->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->tableView_History->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView_History->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui->tableView_History->setColumnHidden(6, true);        //隐藏瑕疵文件路径列

    HistoryRecord_SetDefectTypeDisplay();
    //---------隐藏预留的瑕疵类型列
    ui->tableView_History->setColumnHidden(20, true);
    ui->tableView_History->setColumnHidden(21, true);
    ui->tableView_History->setColumnHidden(22, true);
    //---------

    ui->tableView_History->show();
}

void TableViewWidget::Slot_HistoryRecord_SetColumnsDisplayStatus()
{ 
    HistoryRecord_SetColumnsDisplayStatus();
}

void TableViewWidget::HistoryRecord_SetColumnsDisplayStatus()
{ 
    //QSqlQueryModel 刷新玩数据后，需要重新设置列显示状态
    ui->tableView_History->setColumnHidden(6, true);        //隐藏瑕疵文件路径列

    HistoryRecord_SetDefectTypeDisplay();
    //---------隐藏预留的瑕疵类型列
    ui->tableView_History->setColumnHidden(20, true);
    ui->tableView_History->setColumnHidden(21, true);
    ui->tableView_History->setColumnHidden(22, true);
}

// 处理复选框状态变化的槽函数
void TableViewWidget::Slot_HistoryRecord_onDefectFilterChanged(int index, bool checked)
{
    // 获取缺陷类型
    DefectType type = static_cast<DefectType>(index);
    QString typeName = DefectTypeToString(type);

    // 根据复选框状态执行相应操作
    if (checked) {
        qDebug() << "Defect type" << typeName << "is now checked";
        // 执行选中时的操作
    }
    else {
        qDebug() << "Defect type" << typeName << "is now unchecked";
        // 执行取消选中时的操作
    }

    // 获取所有选中的缺陷类型
    QList<DefectType> selectedTypes = m_defectFilterWidget->getSelectedDefectTypes();
    qDebug() << "Selected defect types count:" << selectedTypes.size();
    int count = 0;
    m_defectFilterStr = "";
    for (DefectType type : selectedTypes)
    {
        if (count > 0)
        {
            m_defectFilterStr += " or ";
        }
        m_defectFilterStr += "errnum"+ QString::number(type) + " > 0";
        count++;
    }
    if (count > 1)
    {
        m_defectFilterStr = "and ("+ m_defectFilterStr + ")";
    }
    else if (count == 1)
    {
        m_defectFilterStr = "and " + m_defectFilterStr;
    }
    else 
    {
        m_defectFilterStr = "";
    }
    
    HistoryRecord_GetTimeFileter();
}

void TableViewWidget::Slot_HistoryRecord_DataTimeChanged(const QDateTime& dateTime)
{
    m_FilterDateTim_Start = ui->dateTimeEdit_begin->dateTime();
    m_FilterDateTim_End = ui->dateTimeEdit_end->dateTime();
    HistoryRecord_GetTimeFileter();
}

void TableViewWidget::Slot_HistoryRecord_OnlyNGChanged(bool checked)
{
    HistoryRecord_GetTimeFileter();
}

void TableViewWidget::Slot_HistoryRecord_handleSingleClick()
{
    if (!m_pendingIndex.isValid())
        return;

    // 处理单击事件
    FILE_LOG_INFO("Click_History_Item_begin.");
    DisplayMessage::getInstance().logMessage(tr("Select Record"), tr("HistoryTable"));  //选择记录
    int row = m_pendingIndex.row();//根据单击的行数，获取对应的数据

    auto time = ui->tableView_History->model()->data(ui->tableView_History->model()->index(row, 0)).toString();
    auto glassId = ui->tableView_History->model()->data(ui->tableView_History->model()->index(row, 1)).toString();

    float glassPhysicalHeight = ui->tableView_History->model()->data(ui->tableView_History->model()->index(row, 2)).toFloat();
    float glassPhysicalWidth = ui->tableView_History->model()->data(ui->tableView_History->model()->index(row, 3)).toFloat();

    QString originalTime = QDateTime::fromString(time, "yyyy-MM-dd hh:mm:ss").toString("yyyyMMddhhmmss");
    QString idRecords = originalTime + glassId;
    QString filePath = ui->tableView_History->model()->data(ui->tableView_History->model()->index(row, 6)).toString();

    emit Signal_ShowHistoryImage(2, originalTime, glassId, glassPhysicalWidth, glassPhysicalHeight, filePath);
    FILE_LOG_INFO("Click_History_Item_after.");
    
    m_pendingIndex = QModelIndex(); // 处理完成后清空
}

void TableViewWidget::Slot_HistoryRecord_DoubleClick(const QModelIndex& index)
{
    m_clickTimer.stop();
    FILE_LOG_INFO("Double_Click_History_Item_begin.");
    //根据双击的行数，获取对应的数据
    int row = index.row();

    auto time = ui->tableView_History->model()->data(ui->tableView_History->model()->index(row, 0)).toString();
    auto glassId = ui->tableView_History->model()->data(ui->tableView_History->model()->index(row, 1)).toString();

    float glassPhysicalHeight = ui->tableView_History->model()->data(ui->tableView_History->model()->index(row, 2)).toFloat();
    float glassPhysicalWidth = ui->tableView_History->model()->data(ui->tableView_History->model()->index(row, 3)).toFloat();

    QString originalTime;
    if (Language == u8"中文")
    {
        originalTime = QDateTime::fromString(time, "yyyy-MM-dd hh:mm:ss").toString("yyyyMMddhhmmss");
    }
    else
    {
        originalTime = QDateTime::fromString(time, "dd-MM-yyyy hh:mm:ss").toString("yyyyMMddhhmmss");
    }
    QString idRecords = originalTime + glassId;
    QString filePath = ui->tableView_History->model()->data(ui->tableView_History->model()->index(row, 6)).toString();

    emit Signal_ShowHistoryImage(1,originalTime, glassId, glassPhysicalWidth, glassPhysicalHeight, filePath);
    FILE_LOG_INFO("Double_Click_History_Item_after.");
    DisplayMessage::getInstance().logMessage(tr("Display Record Image"), tr("HistoryTable"));  //显示记录图像
}

void TableViewWidget::Slot_ClickHistoryRecord(const QModelIndex& index)
{
    FILE_LOG_INFO("Click_History_Item_begin.");
    // 保存索引并启动定时器
    m_pendingIndex = index;
    m_clickTimer.start();

    FILE_LOG_INFO("Click_History_Item_after.");

}

// 清空显示处理函数
void TableViewWidget::Slot_HandleClearDisplay()
{
    /* 清空图表 */
    m_CurrentRecord_PieChart->clearData();
    m_CurrentRecord_BarChart->clearData();

    /* 清空current表格 */
    ui->tableWidget_CurrentDefect->clearContents();
    CurrentRecord_InitTableWidget();
}

void TableViewWidget::SetColumnVisibility(QTableView* tableView, int column, bool display)
{
    tableView->setColumnHidden(column, !display);
    
}

void TableViewWidget::HistoryRecord_SetDefectTypeDisplay()
{
    for (size_t i = 0; i < DefectType::DEFECT_TYPE_COUNT; i++)
    {
        DefectType type = static_cast<DefectType>(i);
        SetColumnVisibility(ui->tableView_History, i + 7, m_defectTypeVisibility[type]);
        if (m_defectFilterWidget)
        {
            m_defectFilterWidget->setDisplayStatus(type, m_defectTypeVisibility[type]);
        }
    }

}

void TableViewWidget::Slot_SetDefectTypeVisibility(DefectType type, bool visible)
{
    int column = static_cast<int>(type) + 7;    //从column=7开始是瑕疵类型的列

    //更新历史主表的瑕疵类型列的显示状态
    SetColumnVisibility(ui->tableView_History, column, visible);

    //更新历史主表过滤器的瑕疵类型显示状态
    if (m_defectFilterWidget)
    {
        // 显示该 defect type 的数据
        m_defectFilterWidget->setDisplayStatus(type, visible);
    }

    //更新历史折线图的瑕疵类型的显示状态
    HistoryRecordChart_SetDefectTypeVisible(type, visible);

    //更新当前实时瑕疵类型表的显示状态
    SetDefectRowDisplayStatus(m_currentDefectTable, type, visible);
    //更新当前历史瑕疵类型表的显示状态
    SetDefectRowDisplayStatus(m_historyDefectTable, type, visible);
    
    {
        QMutexLocker locker(&m_mutexDefectDisplay);
        //更新当前实时瑕疵饼状图的显示状态
        m_CurrentRecord_PieChart->clearData();  // 清空数据
        m_CurrentRecord_PieChart->updateData(m_currentCountMap);    // 替换旧内容
        //更新当前实时瑕疵柱状图的显示状态
        m_CurrentRecord_BarChart->clearData();
        m_CurrentRecord_BarChart->updateData(m_currentCountMap);
    }
}


void TableViewWidget::Slot_SetDefectLevelVisibility(DefectLevel level, bool visible)
{

    //SetDefectLevelVisibilityFunction(level, visible);
}

void TableViewWidget::Slot_TableWidget_TabChanged(int index)
{
    if (index == 1)
    {
        m_defectFilterWidget->setDisplayStatus(DefectType::TYPE_POORCOATING, !m_defectTypeVisibility[DefectType::TYPE_POORCOATING]);
        m_defectFilterWidget->setDisplayStatus(DefectType::TYPE_POORCOATING, m_defectTypeVisibility[DefectType::TYPE_POORCOATING]);
    }
}

void TableViewWidget::Slot_ExportData()
{
    emit Signal_NotifyExportDataStatus(true);
    QString exportPath = QFileDialog::getExistingDirectory(this, tr("Export Data"), ".");
    exportPath = exportPath + "/" + m_FilterDateTim_Start.toString("yyyyMMdd_HHmmss") + "----" + m_FilterDateTim_End.toString("yyyyMMdd_HHmmss") + ".xlsx";
    bool isExport = false;
    isExport = HistoryRecord_exportTableViewToExcel(ui->tableView_History, exportPath);
    emit Signal_NotifyExportDataStatus(false);
}

void TableViewWidget::CurrentRecord_ShowDefectData(std::string timeProduce, std::vector<drawInformation>& data)
{
    FILE_LOG_INFO("TableView:: Current_ShowDefectData Begin.");

    QString tmpTime = QString::fromStdString(timeProduce);

    QDateTime dt = QDateTime::fromString(tmpTime, "yyyyMMddhhmmss");
    QString dateTime = dt.toString("yyyy-MM-dd hh:mm:ss");
    ui->label_CurrentIndex->setText(dateTime);
    CurrentRecord_PopulateDefectTable(data);
    CurrentReocrd_ShowChart(data);
    FILE_LOG_INFO("TableView:: Current_ShowDefectData Completed.");
}

void TableViewWidget::CurrentReocrd_Init()
{
    /******************** 初始化图表*******************************/
    // 创建图表
    m_CurrentRecord_PieChart = new PieChartWidget(this);
    m_CurrentRecord_BarChart = new BarChartWidget(this);
    // 创建并添加图表
    ui->hLayout_CurrentChart->addWidget(m_CurrentRecord_PieChart);
    ui->hLayout_CurrentChart->addWidget(m_CurrentRecord_BarChart);
    /**************************************************************/

    /******************** 初始化表格 ******************************/
    //CurrentRecord_InitTableWidget();
    /**************************************************************/

    ui->label_CurrentIndex->setText(QString(""));
}

void TableViewWidget::Slot_RefreshDefectTypeNameToTable()
{
    EnsureTablesInitialized(true);
    //CurrentRecord_InitTableWidget();
    // 初始化两个表格
    //InitializeTableManager(m_currentDefectTable, ui->tableWidget_CurrentDefect, 20, 21);    // 实时数据表格
    //InitializeTableManager(m_historyDefectTable, ui->tableWidget_HistoryDefect, 20, 21);    // 历史数据表格

}

void TableViewWidget::CurrentRecord_InitTableWidget()
{
    // 初始化表格样式
    InitTableWidget(m_currentDefectTable);

}

void TableViewWidget::CurrentReocrd_ShowChart(const std::vector<drawInformation>& drawInfo)
{
    QMutexLocker locker(&m_mutexDefectDisplay);
    FILE_LOG_INFO("TableView:: Current_ShowChart Begin.");
    if (drawInfo.empty())
    {
        FILE_LOG_INFO("TableView:: Current_ShowChart: defect is empty.Return.");
        DisplayMessage::getInstance().logMessage(tr("no defect"), tr("CurrentChart"));  //无缺陷
        return;
    }
    m_currentCountMap.clear();
    for (drawInformation info : drawInfo)
    {
        m_currentCountMap[info.DefectType]++;
    }

    // 清空数据
    m_CurrentRecord_PieChart->clearData();
    m_CurrentRecord_BarChart->clearData();

    // 替换旧内容
    m_CurrentRecord_PieChart->updateData(m_currentCountMap);

    //m_CurrentRecord_BarChart->updateAxis();
    m_CurrentRecord_BarChart->updateData(m_currentCountMap);

    FILE_LOG_INFO("TableView:: Current_ShowChart Completed.");
}

void TableViewWidget::HistoryRecordDefect_RefreshTable(const QString& filesPath)
{ 
    FILE_LOG_INFO("begin.");
    std::vector<drawInformation> showInfo;
    QString jsonPath = filesPath + ".json";
    ////验证图片和json文件是否存在
    QFile fileJson(jsonPath);
    if (!fileJson.exists())
    {
        HistoryRecordDefect_PopulateTable(showInfo);
        //QMessageBox::warning(this, tr("Warning"), tr("The json does not exist!"));
        FILE_LOG_ERROR("TableView_ShowHistoryImage error: The json does not exist!");
        DisplayMessage::getInstance().logMessage(tr("Invalid defect data"), tr("HistoryRecord"));  //无效缺陷数据
        return;
    }
    //读取json文件
    glassData2Json json;
    showInfo = json.parseJsonToDrawInfos(jsonPath.toStdString(), m_glassPixelWidth_His, m_glassPixelHeight_His);
    FILE_LOG_INFO("HistoryRecordDefect_RefreshTable: read json Completed!");

    HistoryRecordDefect_PopulateTable(showInfo);
    FILE_LOG_INFO("After.");

}

// 历史表格的填充函数
void TableViewWidget::HistoryRecordDefect_PopulateTable(const std::vector<drawInformation>& showInfo)
{
    EnsureTablesInitialized(); // 确保表格已初始化
    PopulateDefectTable(m_historyDefectTable, showInfo);
}


void TableViewWidget::HistoryRecord_ShowImageDialog(QString originalTime, const QString& filesPath, const QDateTime& dateTime,
    const float& glassPhysicalWidth, const float& glassPhysicalHeight)
{
    try
    {
        if (m_defectImageShow)
        {
            m_defectImageShow.reset(); // 自动释放旧对象，无需手动 delete
        }
        m_defectImageShow = std::make_unique<DefectImageShow>(this);
        // 设置为独立窗口
        m_defectImageShow->setWindowFlags(Qt::Window |
            Qt::WindowTitleHint |
            Qt::WindowCloseButtonHint/* |
            Qt::WindowMinMaxButtonsHint*/);

        connect(m_defectImageShow.get(), &QWidget::destroyed, this, [this]() {
            m_defectImageShow.reset(); // 窗口销毁时重置智能指针
            });
        m_defectImageShow->ShowHistoryImage(originalTime, filesPath, dateTime, glassPhysicalWidth, glassPhysicalHeight);
        m_defectImageShow->setWindowTitle(tr("History Image"));
        //m_defectImageShow->setWindowTitle("RectItem Image");
                // 连接窗口关闭信号到重置指针的槽函数


        m_defectImageShow->show();
        m_defectImageShow->raise();
        m_defectImageShow->activateWindow();


    }
    catch (const std::bad_alloc& e) {
        std::cerr << "内存分配失败: " << e.what() << std::endl;
        qDebug() << "m_defectImageShow show failed";
        DisplayMessage::getInstance().logMessage(tr("Failed to display defect data"), tr("HistoryRecord"));  //显示缺陷数据失败
        return;
    }
    HistoryRecordDefect_RefreshTable(filesPath);
}

void TableViewWidget::HistoryRecord_CloseImageDialog()
{ 
    if (m_defectImageShow)
    {
        m_defectImageShow.reset(); // 自动释放旧对象，无需手动 delete
    }
}

void TableViewWidget::HistoryRecord_GetTimeFileter()
{
    FILE_LOG_INFO("begin.");
    //先判断两个DateTimeEdit控件的时间是否合法
    QDateTime beginTime = ui->dateTimeEdit_begin->dateTime();
    QDateTime endTime = ui->dateTimeEdit_end->dateTime();
    //--用来同步chart界面的时间
    ui->dateTimeEdit_begin_Chart->setDateTime(beginTime);   
    ui->dateTimeEdit_end_Chart->setDateTime(endTime);
    //--
    if (beginTime > endTime)
    {
        //弹窗提示时间不合法
        //QMessageBox::warning(this, tr("Incorrect Time"), tr("Start time cannot be later than end time!"));
        FILE_LOG_ERROR("Start time cannot be later than end time!");
        DisplayMessage::getInstance().logMessage(tr("Invalid date/time settings"),tr("HistoryRecord"));  //日期/时间设置错误
        return;
    }

    //根据时间查询数据库
    QString beginTimeStr = beginTime.toString("yyyyMMddhhmmss");
    QString endTimeStr = endTime.toString("yyyyMMddhhmmss");
    QString onlyNG = ui->checkBox_OnlyNG->isChecked() ? " and NGStatus = 0" : "";
    
    QString filter = QString("dateTime >= '%1' and dateTime <= '%2' %3 %4")
        .arg(beginTimeStr).arg(endTimeStr).arg(onlyNG)
        .arg(m_defectFilterStr);
    emit Signal_FilterRequested(filter);
    FILE_LOG_INFO("Completed.");
    return;
}

void TableViewWidget::Slot_ShowHistory_Chart(const QMap<DefectType, QMap<QDate, int>>& data)
{
    std::cout << "Slot_ShowHistory_Chart begin." << std::endl;
    HistoryRecordChart_ShowData(data);
}

// 生成折线图
void TableViewWidget::HistoryRecordChart_ShowData(const QMap<DefectType, QMap<QDate, int>>& data)
{
    std::cout << "HistoryRecordChart_ShowData begin." << std::endl;
    // 如果是首次调用，则初始化 QChart 和 QChartView
    if (!m_ChartView_LineChart)
    {
        m_ChartView_LineChart = new QChartView(this);
        ui->vLayout_Chart->addWidget(m_ChartView_LineChart, 1);
        m_Chart_LineChart = new QChart();
        m_ChartView_LineChart->setChart(m_Chart_LineChart);
        m_ChartView_LineChart->setRenderHint(QPainter::Antialiasing);

        // 设置图表背景色
        m_Chart_LineChart->setBackgroundBrush(QBrush(QColor("#C3C3C3")));
        m_Chart_LineChart->setPlotAreaBackgroundBrush(QBrush(QColor("#C3C3C3")));
        m_Chart_LineChart->setPlotAreaBackgroundVisible(true);
    }
    else
    {
        // 清除所有旧的数据系列
        m_Chart_LineChart->removeAllSeries();
    }

    // 更新图表标题
    m_Chart_LineChart->setTitle(tr("Defect Trend Statistics"));

    // 重置轴
    // 安全删除旧的轴
    auto axesXList = m_Chart_LineChart->axes(Qt::Horizontal);
    if (!axesXList.isEmpty())
    {
        axesXList.first()->deleteLater();
    }

    auto axesYList = m_Chart_LineChart->axes(Qt::Vertical);
    if (!axesYList.isEmpty())
    {
        axesYList.first()->deleteLater();
    }
    // 创建新的轴
    QValueAxis* axisY = new QValueAxis();
    QCategoryAxis* axisX = new QCategoryAxis();

    // 折线颜色列表（13种）
    QList<QColor> colorList =
    {
        QColor(Qt::red),                // 1. 红
        QColor(255, 165, 0),            // 2. 橙
        QColor(Qt::yellow),             // 3. 黄
        QColor(Qt::green),              // 4. 绿
        QColor(Qt::cyan),               // 5. 青
        QColor(Qt::blue),               // 6. 蓝
        QColor(128, 0, 128),            // 7. 紫（Purple）
        QColor(255, 20, 147),           // 8. 深粉红（DeepPink）
        QColor(0, 191, 255),            // 9. 深天蓝（DeepSkyBlue）
        QColor(50, 205, 50),            // 10. 酸橙绿（LimeGreen）
        QColor(255, 105, 180),          // 11. 热粉红（HotPink）
        QColor(139, 69, 19),            // 12. 棕色（SaddleBrown）
        QColor(0, 128, 128)             // 13. 青绿/水鸭色（Teal）
    };

    int colorIndex = 0;

    axisY->setTitleText(tr("Defect Count"));
    axisY->setLabelFormat("%d");

    // 计算Y轴最大值（最大值 * 1.1）
    int maxY = 0;
    for (const auto& defectData : data)
    {

        for (int count : defectData.values())
        {
            if (count > maxY) maxY = count;
        }
    }

    // 如果没有数据，设置一个默认范围
    if (maxY == 0) {
        maxY = 10; // 默认最大值为10
    }

    axisY->setRange(0, maxY * 1.1);
    m_Chart_LineChart->addAxis(axisY, Qt::AlignLeft);

    // 横轴（X轴）
    axisX->setTitleText(tr("Date"));
    axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    axisX->setLabelsAngle(90);

    // 统一所有日期，并添加额外的空白边界
    QSet<QDate> allDates;
    for (const auto& dateMap : data)
    {
        //allDates.unite(dateMap.keys().toSet());   //Qt5.14.2下提示C4996 已弃用接口。
        for (const QDate& date : dateMap.keys())
        {
            allDates.insert(date);
        }
    }

    QList<QDate> sortedDates = allDates.values();
    std::sort(sortedDates.begin(), sortedDates.end());

    // 在两侧添加空白
    if (!sortedDates.isEmpty())
    {
        sortedDates.prepend(sortedDates.front().addDays(-1));
        sortedDates.append(sortedDates.back().addDays(1));
    }

    QHash<QDate, int> dateIndexMap;
    for (int i = 0; i < sortedDates.size(); ++i)
    {
        dateIndexMap[sortedDates[i]] = i;
		QString dateLabel;
        if (Language == u8"中文")
        {
            dateLabel = sortedDates[i].toString("yyyy-MM-dd");
        }
        else
        {
            dateLabel = sortedDates[i].toString("dd-MM-yyyy");
        }
        axisX->append(dateLabel, i);
    }
    if (sortedDates.isEmpty())
    {
        // 如果没有数据，添加一个虚拟的日期范围
        axisX->append("No Data", 0);
        axisX->setRange(0, 1);
    }
    else
    {
        axisX->setRange(0, sortedDates.size() - 1);
    }
    m_Chart_LineChart->addAxis(axisX, Qt::AlignBottom);

    QLegend* legend = m_Chart_LineChart->legend();
    legend->setAlignment(Qt::AlignRight); // 可按需设为 AlignBottom、AlignTop、AlignLeft
    legend->setFont(QFont("Microsoft YaHei", 10)); // 字体略大些，看起来更有间距
    legend->setContentsMargins(10, 10, 10, 10);    // 图例整体边距，模拟内边距效果

    // 清除现有的系列映射
    m_lineSeriesMap.clear();
    m_scatterSeriesMap.clear();

    for (auto it = data.begin(); it != data.end(); ++it)
    {
        DefectType type = it.key();
        QMap<QDate, int> dateMap = it.value();

        QLineSeries* series = new QLineSeries();
        QScatterSeries* scatterSeries = new QScatterSeries(); // 添加散点系列
        scatterSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle); // 圆形
        scatterSeries->setMarkerSize(8.0); // 设置点的大小
        scatterSeries->setPen(QPen(Qt::black, 1)); // 黑色边框
        scatterSeries->setBrush(QBrush(Qt::white)); // 空心（白色填充）
        //scatterSeries->setBrush(QBrush(QColor("#C3C3C3")));
        scatterSeries->setName(""); // 不在图例中显示

        // 设置系列名称
        QString originalLabel = tr("%1").arg(DefectTypeToString(type));
        series->setName(originalLabel);
        m_defectTypeOriginalLabels[type] = originalLabel;

        series->setPointLabelsVisible(true);
        series->setPointLabelsFormat("@yPoint"); // 只显示 Y 轴值

        QColor color = colorList[colorIndex % colorList.size()];
        series->setPen(QPen(color, 2)); // 折线颜色
        scatterSeries->setBrush(QBrush(color)); // 点的颜色

        ++colorIndex;

        // 添加数据点
        for (auto dateIt = dateMap.begin(); dateIt != dateMap.end(); ++dateIt)
        {
            int x = dateIndexMap[dateIt.key()];
            int y = dateIt.value();
            series->append(x, y);
            scatterSeries->append(x, y); // 散点和折线同步
        }

        // 将系列添加到图表
        m_Chart_LineChart->addSeries(series);
        m_Chart_LineChart->addSeries(scatterSeries); // 添加散点
        series->attachAxis(axisX);
        series->attachAxis(axisY);
        scatterSeries->attachAxis(axisX);
        scatterSeries->attachAxis(axisY);

        // 保存系列指针
        m_lineSeriesMap[type] = series;
        m_scatterSeriesMap[type] = scatterSeries;

        // 隐藏散点系列在图例中的显示 - 这是关键！
        auto scatterMarkers = legend->markers(scatterSeries);
        for (QLegendMarker* marker : scatterMarkers) {
            marker->setVisible(false);
            // 确保散点系列的图例标记完全隐藏
            marker->setLabel("");
        }

        // 设置系列可见性（根据配置）
        bool isVisible = m_defectTypeVisibility[type];
        series->setVisible(isVisible);
        scatterSeries->setVisible(isVisible);

        // 设置折线系列的图例标记可见性
        auto lineMarkers = legend->markers(series);
        for (QLegendMarker* marker : lineMarkers) {
            marker->setVisible(isVisible);
            if (isVisible) {
                // 添加前缀空格
                marker->setLabel("   " + marker->label());
            }
            else {
                // 隐藏时设置为空字符串
                marker->setLabel("");
            }
        }
    }

    // 最后，调整图表显示
    m_ChartView_LineChart->update();
}

void TableViewWidget::HistoryRecordChart_SetDefectTypeVisible(DefectType type, bool visible)
{
    // 如果系列已经创建，立即更新可见性
    if (m_lineSeriesMap.contains(type)) {
        QLineSeries* lineSeries = m_lineSeriesMap[type];
        QScatterSeries* scatterSeries = m_scatterSeriesMap[type];

        if (lineSeries && scatterSeries) {
            lineSeries->setVisible(visible);
            scatterSeries->setVisible(visible);

            // 更新图例可见性
            QLegend* legend = m_Chart_LineChart->legend();

            // 1. 处理折线系列的图例标记
            auto lineMarkers = legend->markers(lineSeries);
            for (QLegendMarker* marker : lineMarkers) {
                marker->setVisible(visible);
                if (visible) {
                    // 使用原始标签并添加前缀空格
                    QString originalLabel = m_defectTypeOriginalLabels.value(type, "");
                    if (originalLabel.isEmpty()) {
                        originalLabel = tr("%1").arg(DefectTypeToString(type));
                        m_defectTypeOriginalLabels[type] = originalLabel;
                    }
                    marker->setLabel("   " + originalLabel);
                }
                else {
                    marker->setLabel(""); // 隐藏时设置为空字符串
                }
            }

            // 2. 处理散点系列的图例标记 - 确保它们保持隐藏
            auto scatterMarkers = legend->markers(scatterSeries);
            for (QLegendMarker* marker : scatterMarkers) {
                marker->setVisible(false); // 永远保持隐藏
                marker->setLabel(""); // 标签设置为空
            }

            // 更新图表
            m_ChartView_LineChart->update();
        }
    }
}

// 更新图例间距（重新添加前缀空格）
void TableViewWidget::HistoryRecordChart_UpdateLegendSpacing() {
    if (!m_Chart_LineChart) return;

    QLegend* legend = m_Chart_LineChart->legend();

    // 清除所有现有的前缀空格
    for (QLegendMarker* marker : legend->markers()) {
        QString label = marker->label();
        // 移除可能已经存在的前导空格
        while (label.startsWith(' ')) {
            label.remove(0, 1);
        }
        marker->setLabel(label);
    }

    // 重新为可见的图例项添加前缀空格
    for (QLegendMarker* marker : legend->markers()) {
        if (marker->isVisible()) {
            marker->setLabel("   " + marker->label());
        }
    }
}

bool TableViewWidget::CurrentRecord_CompareBySeverity(const drawInformation& a, const drawInformation& b) {
    if (a.Errorinfo.empty() && b.Errorinfo.empty()) return false;
    if (a.Errorinfo.empty()) return false;
    if (b.Errorinfo.empty()) return true;
    return a.Errorinfo[0] > b.Errorinfo[0];
}

// 实时表格的填充函数
void TableViewWidget::CurrentRecord_PopulateDefectTable(const std::vector<drawInformation>& showInfo)
{
    EnsureTablesInitialized(); // 确保表格已初始化
    PopulateDefectTable(m_currentDefectTable, showInfo);
}
void TableViewWidget::SetDefaultTabIndex(int index)
{
    ui->tabWidget_TableView->setCurrentIndex(index);
}

void TableViewWidget::EnsureTablesInitialized(bool refreshFlag)
{
    QMutexLocker locker(&m_initMutex);
    if (m_tablesInitialized && !refreshFlag) return;

    // 初始化两个表格
    InitializeTableManager(m_currentDefectTable, ui->tableWidget_CurrentDefect, 20, 21);
    InitializeTableManager(m_historyDefectTable, ui->tableWidget_HistoryDefect, 20, 21);

    m_tablesInitialized = true;
}

void TableViewWidget::InitializeTableManager(TableManager& manager, QTableWidget* table, int maxRows, int columns)
{

    // 添加安全检查
    if (!table) {
        qWarning() << "Table widget is not ready for initialization";
        return;
    }
    //QMutexLocker locker(&m_initMutex); // 添加互斥锁保护初始化过程

    manager.tableWidget = table;
    manager.maxRowCount = maxRows;
    manager.fixedColumnCount = columns;

    if (!manager.tableWidget) {
        qWarning() << "Table widget is null";
        return;
    }

    // 暂停表格更新以提高性能并减少竞争
    manager.tableWidget->setUpdatesEnabled(false);
    try {
        // 设置表格固定尺寸
        manager.tableWidget->setColumnCount(columns);
        manager.tableWidget->setRowCount(maxRows);

        // 预先创建所有item
        manager.tableItems.resize(maxRows);
        for (int row = 0; row < maxRows; ++row) {
            manager.tableItems[row].resize(columns);
            for (int col = 0; col < columns; ++col) {
                QTableWidgetItem* item = new QTableWidgetItem();
                item->setTextAlignment(Qt::AlignCenter);
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);

                manager.tableItems[row][col] = item;
                manager.tableWidget->setItem(row, col, item);
            }
        }

        // 初始化为空状态
        ClearTableItems(manager);

        // 初始化表格样式
        InitTableWidget(manager);

        // 设置表格属性
        manager.tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
        manager.tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        manager.tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

        manager.tableWidget->setUpdatesEnabled(true);

        FILE_LOG_INFO("TableViewWidget: InitializeTableManager Completed.");
    }
    catch (const std::exception& e) {
        qCritical() << "Exception during table initialization:" << e.what();
        manager.tableWidget->setUpdatesEnabled(true);
    }
}

void TableViewWidget::ClearTableItems(TableManager& manager)
{
    if ( manager.usedRowCount > 20)
    {
        manager.usedRowCount = 20;
    }
    for (int row = 0; row < manager.usedRowCount; row++)
    {

        qDebug() << "Outer loop row:" << row; // 这里最多打印到 19
        for (int col = 1; col < manager.fixedColumnCount; col++)
        {
            if (manager.tableItems[row][col])
            {
                manager.tableItems[row][col]->setText("");
                manager.tableItems[row][col]->setBackground(QBrush());
                manager.tableItems[row][col]->setData(Qt::UserRole, QVariant());
            }
        }
    }
    FILE_LOG_INFO("TableViewWidget: ClearTableItems Completed.");
}

void TableViewWidget::SafeUpdateTableItem(TableManager& manager, int row, int col,
    const QString& text, const QVariant& userData, const QBrush& background)
{
    // 安全检查
    if (row < 0 || row >= manager.maxRowCount || col < 0 || col >= manager.fixedColumnCount) {
        qWarning() << "Invalid table coordinates:" << row << col;
        return;
    }

    QTableWidgetItem* item = manager.tableItems[row][col];
    if (!item) {
        qWarning() << "Null item at:" << row << col;
        return;
    }

    item->setText(text);
    if (userData.isValid()) {
        item->setData(Qt::UserRole, userData);
    }
    if (background != QBrush()) {
        item->setBackground(background);
    }
    else {
        item->setBackground(QBrush());
    }
}

void TableViewWidget::InitTableWidget(TableManager& manager)
{
    //设置表格列数
    manager.tableWidget->setColumnCount(21);
    //设置表格列标题
    manager.tableWidget->setHorizontalHeaderLabels(QStringList()
        << tr("DefectType")
        << tr("Defect_1") << tr("Defect_2")
        << tr("Defect_3") << tr("Defect_4")
        << tr("Defect_5") << tr("Defect_6")
        << tr("Defect_7") << tr("Defect_8")
        << tr("Defect_9") << tr("Defect_10")
        << tr("Defect_11") << tr("Defect_12")
        << tr("Defect_13") << tr("Defect_14")
        << tr("Defect_15") << tr("Defect_16")
        << tr("Defect_17") << tr("Defect_18")
        << tr("Defect_19") << tr("Defect_20"));

    // 计算实际需要显示的瑕疵类型,获取行数
    int defectTypeCount = RecipeInfo.defects.size();

    // 安全检查
    if (defectTypeCount > manager.maxRowCount)
    {
        qWarning() << "Defect type count exceeds pre-allocated rows:" << defectTypeCount << ">" << manager.maxRowCount;
        defectTypeCount = manager.maxRowCount;
    }

    // 设置实际行数
    manager.tableWidget->setRowCount(defectTypeCount);
    manager.usedRowCount = defectTypeCount;
    // 为每一行设置首列内容
    m_DefectNamesMutex.lock();
    // 清空映射
    m_defectTableRowIndexMap.clear();
    int rowNumber = 0;
    for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
    {
        if (!RecipeInfo.defects[i].isDisplayed)
        {
            manager.tableWidget->setRowHidden(i, true);
        }
        else
        {
            if (manager.tableWidget->isRowHidden(i))
            {
                manager.tableWidget->setRowHidden(i, false);
            }
            rowNumber++;
        }
        manager.tableWidget->setVerticalHeaderItem(i, new QTableWidgetItem(QString::number(rowNumber)));
        DefectType defectType = static_cast<DefectType>(RecipeInfo.defects[i].id);
        QTableWidgetItem* item = new QTableWidgetItem(DefectTypeToString(defectType));
        manager.tableWidget->setItem(i, 0, item);

        m_defectTableRowIndexMap[defectType] = i;
    }
    m_DefectNamesMutex.unlock();

    FILE_LOG_INFO("TableViewWidget: InitTableWidget Completed.");
}

void TableViewWidget::PopulateDefectTable(TableManager& manager, const std::vector<drawInformation>& showInfo)
{
    if (!manager.tableWidget) {
        qWarning() << "Table widget is null";
        return;
    }


    // 清空现有内容
    ClearTableItems(manager);

    // 初始化表格样式
    InitTableWidget(manager);

    // 排序逻辑
    auto customCompare = [](const drawInformation& a, const drawInformation& b) {
        if (a.ErrorType != b.ErrorType) {
            return a.ErrorType > b.ErrorType;
        }

        double valueA = 0.0, valueB = 0.0;
        if (a.DefectType == DefectType::TYPE_SCRATCH ||
            a.DefectType == DefectType::TYPE_CALCULUS ||
            a.DefectType == DefectType::TYPE_BUBBLE) {
            valueA = (a.Errorinfo.size() >= 3 && a.Errorinfo[2] > 0.0) ? a.Errorinfo[2] :
                (a.Errorinfo.size() >= 2) ? a.Errorinfo[1] : 0.0;
            valueB = (b.Errorinfo.size() >= 3 && b.Errorinfo[2] > 0.0) ? b.Errorinfo[2] :
                (b.Errorinfo.size() >= 2) ? b.Errorinfo[1] : 0.0;
        }
        else {
            valueA = (a.Errorinfo.size() >= 1) ? a.Errorinfo[0] : 0.0;
            valueB = (b.Errorinfo.size() >= 1) ? b.Errorinfo[0] : 0.0;
        }
        return valueA > valueB;
        };

    // 分组和排序
    int defectTypeCount = manager.usedRowCount;
    std::vector<std::vector<drawInformation>> defectsByType(defectTypeCount);
    for (const auto& defect : showInfo)
    {
        int type = static_cast<int>(defect.DefectType);
        //// 不显示的数据丢弃
        //if (!m_defectTypeVisibility[defect.DefectType])
        //{
        //    continue;
        //}

        // 查找对应缺陷在默认配方RecipeInfo.defects 中的索引
        auto it = std::find_if(RecipeInfo.defects.begin(), RecipeInfo.defects.end(),
            [type](const RecipeConfig::DefectConfig& defect) {
                return defect.id == type;
            });
        // 丢弃不存在的瑕疵类型。用于调整应用配方瑕疵类型时，当前使用的配方类型与历史记录中的配方类型不一致时，忽略记录中的该瑕疵
        if (it == RecipeInfo.defects.end()) {
            continue;
        }


        if (type >= 0 && type < defectTypeCount) {
            defectsByType[type].push_back(defect);
        }
    }

    for (auto& defects : defectsByType) {
        std::sort(defects.begin(), defects.end(), customCompare);
    }

    // 更新表格内容
    for (int type = 0; type < defectTypeCount; ++type) {
        const auto& defects = defectsByType[type];

        int col = 1;
        for (const auto& defect : defects) {
            if (col >= manager.fixedColumnCount) break;

            // 准备显示数据
            QString area = "0.0", length = "0.0", diagonal = "0.0";
            if (defect.Errorinfo.size() == 3) {
                area = QString::number(defect.Errorinfo[0], 'f', 1);
                length = QString::number(defect.Errorinfo[1], 'f', 1);
                diagonal = QString::number(defect.Errorinfo[2], 'f', 1);
            }

            QString typeName, LengthOrArea;
            if (defect.DefectType == DefectType::TYPE_SCRATCH ||
                defect.DefectType == DefectType::TYPE_CALCULUS ||
                defect.DefectType == DefectType::TYPE_BUBBLE) {
                typeName = tr("Length");
                LengthOrArea = (diagonal == "0.0") ? length : diagonal;
            }
            else {
                typeName = tr("Area");
                LengthOrArea = area;
            }

            if (LengthOrArea == "0.0") {
                LengthOrArea = "0.1";
            }
            int mm_center_x = Pixle2MM_X * (defect.realRect.x + (defect.realRect.width / 2));
            int mm_center_y = Pixle2MM_Y * (defect.realRect.y + (defect.realRect.height / 2));

            DefectLevel errorType = defect.ErrorType;
            QString cellText = QString(
                tr("Level: %1\n") +
                tr("DefectLocate: %2, %3\n") +
                tr("%4: %5"))
                .arg(DefectLevelToString(errorType))
                .arg(mm_center_x)
                .arg(mm_center_y)
                .arg(typeName)
                .arg(LengthOrArea);

            // 设置背景色
            QBrush background;
            if (!defect.Errorinfo.empty()) {
                double severity = defect.Errorinfo[0];
                if (severity > 5.0) {
                    background = Qt::red;
                }
                else if (severity > 2.0) {
                    background = Qt::yellow;
                }
            }

            // 安全更新item
            SafeUpdateTableItem(manager, type, col, cellText, defect.DefectId, background);
            col++;
        }
    }

    // 调整显示
    manager.tableWidget->resizeColumnsToContents();
    manager.tableWidget->resizeRowsToContents();
}

void TableViewWidget::SetDefectRowDisplayStatus(TableManager& manager, DefectType type, bool isDisplayed)
{ 
      manager.tableWidget->setRowHidden(m_defectTableRowIndexMap[type], !isDisplayed);
      int rowNumber = 0;
      for (size_t i = 0; i < manager.tableWidget->rowCount(); i++)
      {
          if (!manager.tableWidget->isRowHidden(i))
          {
              rowNumber++;
          }
          manager.tableWidget->setVerticalHeaderItem(i, new QTableWidgetItem(QString::number(rowNumber)));
      }
}

bool TableViewWidget::HistoryRecord_exportTableViewToExcel(QTableView* tableView, const QString& filePath)
{
    if (!tableView) return false;

    QAbstractItemModel* model = tableView->model();
    if (!model) return false;

    // 获取数据范围
    int rowCount = model->rowCount();
    int colCount = model->columnCount();
    if (rowCount == 0 || colCount == 0) {
        //QMessageBox::warning(nullptr, tr("警告"), tr("没有数据可导出"));
        return false;
    }

    // 创建Excel文档
    QXlsx::Document xlsx;
    QXlsx::Worksheet* sheet = xlsx.currentWorksheet();
    if (!sheet) return false;

    // --- 设置全局样式 ---
    QXlsx::Format headerFormat;      // 表头样式
    headerFormat.setFontBold(true);
    headerFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    headerFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    headerFormat.setPatternBackgroundColor(QColor(220, 220, 220));  // 浅灰色背景

    QXlsx::Format titleFormat;       // 顶层标题样式
    titleFormat.setFontBold(true);
    titleFormat.setFontSize(14);
    titleFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    titleFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    titleFormat.setPatternBackgroundColor(QColor(180, 200, 240));   // 浅蓝色背景

    QXlsx::Format dataFormat;        // 数据区域样式
    dataFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    dataFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    // --- 第一步：创建复杂表头（以4列为例）---
    // 假设列结构：A列(姓名)、B列(年龄)、C列(语文)、D列(数学)
    // 顶层：A1:B1合并为"个人信息"，C1:D1合并为"成绩"

    //// 合并A1:B1，写入"个人信息"
    //sheet->mergeCells(QXlsx::CellRange(1, 1, 1, 2), titleFormat);
    //sheet->write(1, 1, "个人信息", titleFormat);

    //// 合并C1:D1，写入"成绩"
    //sheet->mergeCells(QXlsx::CellRange(1, 3, 1, 4), titleFormat);
    //sheet->write(1, 3, "成绩", titleFormat);

    int titleRow = 1;
    int firstRow = 2;

    // 第二层：写入各列的具体标题
    QStringList headers;
    int writeCol = 0;
    for (int col = 0; col < colCount; ++col)
    {
        //--跳过隐藏列
        if (col == 6 || col == 20 || col == 21 || col == 22)
        {
            continue;
        }
        if (col >= 7)
        {
            DefectType type = static_cast<DefectType>(col - 7);

            if (!m_defectTypeVisibility[type])
            {
                continue;
            }
        }
        //--

        QString headerText = model->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
        headers << headerText;
        sheet->write(titleRow, writeCol + 1, headerText, headerFormat);
        writeCol++;
    }

    // --- 第二步：写入数据 ---
    for (int row = 0; row < rowCount; ++row)
    {
        int writeCol = 0;
        for (int col = 0; col < colCount; ++col)
        {
            //--跳过隐藏列
            if (col == 6 || col == 20 || col == 21 || col == 22)
            {
                continue;
            }
            if (col >= 7)
            {
                DefectType type = static_cast<DefectType>(col - 7);

                if (!m_defectTypeVisibility[type])
                {
                    continue;
                }
            }
            //--

            QModelIndex index = model->index(row, col);
            QVariant data = model->data(index, Qt::DisplayRole);

            // 根据数据类型选择合适的写入方式
            if (data.type() == QVariant::Int || data.type() == QVariant::Double)
            {
                sheet->write(row + firstRow, writeCol + 1, data.toDouble(), dataFormat);
            }
            else if (data.type() == QVariant::Date || data.type() == QVariant::DateTime)
            {
                sheet->write(row + firstRow, writeCol + 1, data.toDateTime(), dataFormat);
            }
            else
            {
                sheet->write(row + firstRow, writeCol + 1, data.toString(), dataFormat);
            }
            writeCol++;
        }
    }

    // 设置时间列的宽度（加2个字符的缓冲，并限制最大宽度为50）
    int maxWidth = 10;
    QString header1 = headers[0];
    maxWidth = qMax(maxWidth, header1.length());
    QModelIndex idx = model->index(0, 0);
    QString text = model->data(idx, Qt::DisplayRole).toString();
    maxWidth = qMax(maxWidth, text.length());
    sheet->setColumnWidth(1, 1, qMin(maxWidth + 2, 50));

    //// --- 第三步：自动调整列宽 ---
    //for (int col = 0; col < headers.size(); ++col)
    //{

    //    int maxWidth = 10;  // 最小宽度

    //    // 检查表头宽度（包括两层表头）
    //    QString header1 = headers[col];
    //    maxWidth = qMax(maxWidth, header1.length());

    //    // 检查数据行宽度
    //    for (int row = 0; row < rowCount; ++row) {
    //        QModelIndex idx = model->index(row, col);
    //        QString text = model->data(idx, Qt::DisplayRole).toString();
    //        maxWidth = qMax(maxWidth, text.length());
    //    }

    //    // 设置列宽（加2个字符的缓冲，并限制最大宽度为50）
    //    sheet->setColumnWidth(col + 1, col + 1, qMin(maxWidth + 2, 50));
    //}

    // --- 第四步：添加边框（可选）---
    // 为整个数据区域（包括表头）添加边框
    QXlsx::Format borderFormat;
    borderFormat.setBorderStyle(QXlsx::Format::BorderThin);

    // 保存文件
    return xlsx.saveAs(filePath);
}
