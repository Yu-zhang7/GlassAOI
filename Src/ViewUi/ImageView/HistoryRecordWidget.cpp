#include "HistoryRecordWidget.h"
#include <QTimer>
#include "Log.hpp"
#include "Global.h"
#include "DisplayMessage.h" //界面显示运行信息
#include <QScrollBar>

HistoryRecordWidget::HistoryRecordWidget( QWidget *parent)
	: QWidget(parent)

	, ui(new Ui::HistoryRecordWidgetClass())
{
	ui->setupUi(this);

    InitConnect();

    ui->checkBox_RealTime->setChecked(true);

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

    QTimer::singleShot(500, this, [this]() {
        HistoryRecord_GetTimeFileter(); //刷新初始筛选条件下的历史数据
        });
    /*****************************************************/

}

HistoryRecordWidget::~HistoryRecordWidget()
{
    //std::cout << "HistoryRecordWidget::~HistoryRecordWidget()" << std::endl;
	delete ui;
}

void HistoryRecordWidget::InitConnect()
{
    connect(ui->dateTimeEdit_begin, &QDateTimeEdit::dateTimeChanged, this, &HistoryRecordWidget::Slot_DataTimeChanged);
    connect(ui->dateTimeEdit_end, &QDateTimeEdit::dateTimeChanged, this, &HistoryRecordWidget::Slot_DataTimeChanged);
    connect(ui->checkBox_OnlyNG, &QCheckBox::clicked, this, &HistoryRecordWidget::Slot_OnlyNGChanged);
    connect(ui->checkBox_RealTime, &QCheckBox::clicked, this, &HistoryRecordWidget::Slot_RealTimeChange);
    connect(ui->tableView_History, &QTableView::doubleClicked, this, &HistoryRecordWidget::Slot_HistoryRecord_DoubleClick);
}

void HistoryRecordWidget::scheduleNextUpdate()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime nextMidnight = now.date().addDays(1).startOfDay(); // 明天的0点
    int msecsToMidnight = now.msecsTo(nextMidnight);

    // 设置单次定时器，在0点触发
    QTimer::singleShot(msecsToMidnight, this, [this]() {
        if (IsRealTimeChange_Main)
        {
            UpdateDateTime();
            scheduleNextUpdate(); // 安排下一次更新
        }
        });
}

void HistoryRecordWidget::UpdateDateTime()
{
    //先取消旧的信号槽，避免同时改两个日期时间，造成的重复刷新
    disconnect(ui->dateTimeEdit_begin, &QDateTimeEdit::dateTimeChanged, this, &HistoryRecordWidget::Slot_DataTimeChanged);

    QDateTime now = QDateTime::currentDateTime();
    // 设置开始时间为7天前的0点0分0秒
    QDateTime startDate = now.addDays(-7);
    startDate.setTime(QTime(0, 0, 0));
    ui->dateTimeEdit_begin->setDateTime(startDate);

    //恢复信号槽
    connect(ui->dateTimeEdit_begin, &QDateTimeEdit::dateTimeChanged, this, &HistoryRecordWidget::Slot_DataTimeChanged);

    // 设置结束时间为当天的23点59分59秒
    QDateTime endDate = now;
    endDate.setTime(QTime(23, 59, 59));
    ui->dateTimeEdit_end->setDateTime(endDate);

    m_FilterDateTim_Start = startDate;
    m_FilterDateTim_End = endDate;
}

void HistoryRecordWidget::HistoryRecord_InitTable(CustomTableModel* modelHistory)
{
	ui->tableView_History->setModel(modelHistory);
    // 设置只允许整行选中
    ui->tableView_History->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_History->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->tableView_History->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView_History->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    //ui->tableView_History->setColumnHidden(2, true);
    ui->tableView_History->setColumnHidden(6, true);    //隐藏瑕疵文件路径列

    HistoryRecord_SetDefectTypeDisplay();

    //---------隐藏预留的瑕疵类型列
    ui->tableView_History->setColumnHidden(20, true);
    ui->tableView_History->setColumnHidden(21, true);
    ui->tableView_History->setColumnHidden(22, true);
    //---------

    ui->tableView_History->show();
}


void HistoryRecordWidget::HistoryRecord_InitTableQry(CustomSqlQueryModel* modelHistoryQry)
{
    ui->tableView_History->setModel(modelHistoryQry);
    // 设置只允许整行选中
    ui->tableView_History->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_History->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->tableView_History->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView_History->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    //ui->tableView_History->setColumnHidden(2, true);
    ui->tableView_History->setColumnHidden(6, true);    //隐藏瑕疵文件路径列

    HistoryRecord_SetDefectTypeDisplay();

    //---------隐藏预留的瑕疵类型列
    ui->tableView_History->setColumnHidden(20, true);
    ui->tableView_History->setColumnHidden(21, true);
    ui->tableView_History->setColumnHidden(22, true);
    //---------

    ui->tableView_History->show();
}


void HistoryRecordWidget::Slot_HistoryRecord_SetColumnsDisplayStatus()
{
    HistoryRecord_SetColumnsDisplayStatus();
}

void HistoryRecordWidget::HistoryRecord_SetColumnsDisplayStatus()
{  
    //QSqlQueryModel 刷新玩数据后，需要重新设置列显示状态
    ui->tableView_History->setColumnHidden(6, true);        //隐藏瑕疵文件路径列

    HistoryRecord_SetDefectTypeDisplay();
    //---------隐藏预留的瑕疵类型列
    ui->tableView_History->setColumnHidden(20, true);
    ui->tableView_History->setColumnHidden(21, true);
    ui->tableView_History->setColumnHidden(22, true);
}

void HistoryRecordWidget::Slot_SetDefectTypeVisibility(DefectType type, bool visible)
{
    //更新当前实时瑕疵类型表的显示状态
    int column = static_cast<int>(type) + 7;    //从column=7开始是瑕疵类型的列

    //更新历史主表的瑕疵类型列的显示状态
    SetColumnVisibility(ui->tableView_History, column, visible);
}

void HistoryRecordWidget::Slot_SetDefectLevelVisibility(DefectLevel level, bool visible)
{

    //SetDefectLevelVisibilityFunction(level, visible);
}

void HistoryRecordWidget::Slot_DataTimeChanged(const QDateTime& dateTime)
{
    m_FilterDateTim_Start = ui->dateTimeEdit_begin->dateTime();
    m_FilterDateTim_End = ui->dateTimeEdit_end->dateTime();
    HistoryRecord_GetTimeFileter();
}

void HistoryRecordWidget::Slot_OnlyNGChanged(bool checked)
{
    HistoryRecord_GetTimeFileter();
}

void HistoryRecordWidget::Slot_RealTimeChange(bool checked)
{
    IsRealTimeChange_Main = checked;
}

void HistoryRecordWidget::HistoryRecord_GetTimeFileter()
{
    FILE_LOG_INFO("ImageViewWidget: HistoryRecord_GetTimeFileter begin.");
    //先判断两个DateTimeEdit控件的时间是否合法
    QDateTime beginTime = ui->dateTimeEdit_begin->dateTime();
    QDateTime endTime = ui->dateTimeEdit_end->dateTime();
    if (beginTime > endTime)
    {
        //弹窗提示时间不合法
        //QMessageBox::warning(this, tr("Incorrect Time"), tr("Start time cannot be later than end time!"));
        FILE_LOG_ERROR("ImageViewWidget: HistoryRecord_GetTimeFileter error: Start time cannot be later than end time!");
        DisplayMessage::getInstance().logMessage(tr("Invalid date/time settings"));  //日期/时间设置错误
        return;
    }

    //根据时间查询数据库
    QString beginTimeStr = beginTime.toString("yyyyMMddhhmmss");
    QString endTimeStr = endTime.toString("yyyyMMddhhmmss");
    QString onlyNG = ui->checkBox_OnlyNG->isChecked() ? " and NGStatus = 0" : "";
    QString filter = QString("dateTime >= '%1' and dateTime <= '%2' %3").arg(beginTimeStr).arg(endTimeStr).arg(onlyNG);
    emit Signal_FilterRequested(filter); 
    FILE_LOG_INFO("ImageViewWidget: HistoryRecord_GetTimeFileter Completed.");
    return;
}

void HistoryRecordWidget::Slot_HistoryRecord_DoubleClick(const QModelIndex& index)
{
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

    emit Signal_ShowHistoryImage(0 ,originalTime, glassId, glassPhysicalWidth, glassPhysicalHeight, filePath);
    DisplayMessage::getInstance().logMessage(tr("Display Image History"));  //显示历史图像
    FILE_LOG_INFO("Double_Click_History_Item_after.");
}


void HistoryRecordWidget::HistoryRecord_SetDefectTypeDisplay()
{
    for (size_t i = 0; i < DefectType::DEFECT_TYPE_COUNT; i++)
    {
        DefectType type = static_cast<DefectType>(i);
        SetColumnVisibility(ui->tableView_History, i + 7, m_defectTypeVisibility[type]);
    }

}

void HistoryRecordWidget::SetColumnVisibility(QTableView* tableView, int column, bool display)
{
    tableView->setColumnHidden(column, !display);

}