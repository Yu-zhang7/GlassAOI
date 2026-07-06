#pragma once

#include <QtWidgets/QMainWindow>
#include <QCloseEvent>
#include <QStyle>
#include <QTimer>
#include <QElapsedTimer>

#include <memory>
#include "ui_MainWindow.h"
#include <optional>                 //用于队列的出队操作
#include "ConfigWidget.h"           //配置文件设置窗口

#include "ImageViewWidget.h"        //图像视图模式主窗口
#include "HistoryRecordWidget.h"    //图像视图模式左侧子窗口-历史记录展示类

#include "TableViewWidget.h"        //表格视图模式主窗口
#include "ThumbnailWidget.h"        //表格视图模式左侧子窗口-缩略图

#include "DefectFilterWidget.h"	//储存瑕疵类型过滤器

#include "Global.h"
#include "MainProcess.h"        //主进程类

// 声明为 Qt 元类型
//Q_DECLARE_METATYPE(QueueDefectItem)

// 定义共享指针别名（便于使用）
using QueueDefectItemPtr = std::shared_ptr<QueueDefectItem>;

// 声明 shared_ptr 版本的元类型
Q_DECLARE_METATYPE(QueueDefectItemPtr)

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindowClass; };
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

private:
    Ui::MainWindowClass *ui;

protected:

    void changeEvent(QEvent* e) override;           // 重写 changeEvent 以响应语言改变事件
    
    void closeEvent(QCloseEvent* event) override;   // 重写关闭事件

public:

signals:
    /* 开始流程信号 */
    void Sianal_StartProcess();

    /* 结束流程信号 */
    void Sinal_StopProcess(bool isTimeout);

    /* 更新板状态 */
    void Signal_ChangeCardStatus(int Status);

    /* 更新标题时间 */
    void Sinal_ChangeTitleTime(int recondType, QDateTime dateTime);

    /* 设置踏板信号*/
    void Signal_Pedal(bool signal);

    /* 更新瑕疵类型显示状态信号 */
    void Signal_SetDefectTypeVisibility(DefectType type, bool visible);

    /* 更新瑕疵等级显示状态信号 */
    void Signal_SetDefectLevelVisibility(DefectLevel type, bool visible);

    /* 清空current的图像和数据 信号 */
    void Signal_ClearDisplaySignal();

    /* 刷新表格视图中，单张玻璃的瑕疵统计表格式 */
    void Signal_RefreshDefectTypeNameToTable();

    void Signal_Dequeue();

    /* 传递保存系统配置的信号 */
    void Signal_WriteSystemConfig();
	
private slots:
    /* 打开设置对话框 */
    void Slot_OpenConfigWidget();

    /* 启动瑕疵检测(来料时自动检测) */
    void Slot_StartWorking();

    /* 停止瑕疵检测(来料时不检测) */
    void Slot_StopWorking();

    /* 刷新相机连接状态 */
    void Slot_UpdateTime();

    /* 刷新程序运行时长 */
    void Slot_Display_Uptime();

    /* 刷新玻璃状态 */
    void Slot_CardStatusChanged(GlassStatus state, const QString& dateTimeStr);

    /* 启动流程 */
    void Slot_StartProess();

    /* 停止流程 */
    void Slot_StopProcess();

    /* 显示历史记录图片 */
    void Slot_ShowHistoryImage(int typeIndex, QString originalTime, QString glassId, 
        float glassPhysicalWidth, float glassPhysicalHeight, QString path);

    /* 接受配置窗口传递来的瑕疵类型显示状态 槽信号*/
    void Slot_SetDefectTypeVisibility(DefectType type, bool visible);

    /* 刷新瑕疵等级显示状态信号 */
    void Slot_SetDefectLevelVisibility(DefectLevel level, bool visible);

    /* 传递视图模式切换 槽函数*/
    void Slot_TabCurrentChanged(int tabIndex);

    /* 入队槽函数 */
    void Slot_Enqueue(const QueueDefectItemPtr& resultItem);


    /* 出队槽函数*/
    void Slot_Dequeue();

    /* 接受授权失效信号 */
    void slot_licenseExpired();

    /* 传递MainProcess消息*/
    void Slot_ShowMessage(QString message);

	/* 接受导出数据状态信号 */
	void Slot_GetExportDataStatus(bool isExporting);

    /* 设置轻微缺陷是否显示 */
    void Slot_Recipe_SetMinorLevelDisplayState(bool state);

    /* 设置中等缺陷是否显示 */
    void Slot_Recipe_SetMediumLevelDisplayStat(bool state);

    /* 设置严重缺陷是否显示 */
    void Slot_Recipe_SetSeriousLevelDisplayStat(bool state);

    /* 瑕疵显示过滤变化 */
    void Slot_onDefectFilterChanged(int index, bool checked);

private:
    /* 刷新左侧dock窗口高度，避免高度超出主窗口范围*/
    void updateLeftDockMaxHeight();

    void InitConnect();

    /* 设置语言 */
    void Slot_System_LanguageChanged(int langCode);
    
    void InitChildObject();

    void InitProjectObject();


    /* 入队操作 */
    void Enqueue(const QueueDefectItemPtr& resultItem);

    /* 出队操作 */
    void Dequeue();

    /* 执行初始化设置 */
    void InitSettings();

    /* 设置瑕疵过滤器显示的类型 */
    void SetDefectFilterTypeDisplay();

private:
    std::unique_ptr<ConfigWidget>   m_configWidget;                     // 配置界面

    ImageViewWidget*                m_ImageViewWidget;                  // 图像视图模式主界面
    HistoryRecordWidget*            m_HisRecordWidget;		            // 图像视图模式左侧子窗口-历史记录展示子窗口。左侧Dock窗口。

    TableViewWidget*                m_TableViewWidget;                  // 表格视图模式主界面
    ThumbnailWidget*                m_ThumbnailWidget;                  // 表格视图模式左侧子窗口-缩略图子窗口。左侧Dock窗口。

    /* 状态栏显示内容*/
    QLabel*                         m_label_messageTitle;               //运行信息(只显示最后一条消息)
    QLabel*                         m_label_message;                    //运行信息(只显示最后一条消息)
    
    QTimer*                         m_timer;                            //定时器，用于定时获取相机状态等待信息
    QVector<QLabel*>                m_cameraLables;                     //相机状态数组，用于标记每个相机的状态
    QPixmap                         m_ledGreen;                         //相机状态：绿灯
    QPixmap                         m_ledRed;                           //相机状态：红灯

    QTimer*                         m_Timer_Uptime;                     //定时器，用于定时获取相机状态等待信息
    QLabel*                         m_Label_Uptime;                     //显示程序运行时长
    QElapsedTimer                   m_ElapsedTimer;                     //程序运行时长计时器

    QLabel*                         m_Label_GlassStatus;                //显示玻璃状态(进板、出版、无板)                        m_Label_GlassStatus;                //显示程序运行时长
    QLabel*                         m_Label_GlassInTime;                //显示玻璃进入时间(记录条目的时间)

    /* 图像队列操作*/
    QMutex                          m_queueMutex;						//互斥锁。用于队列操作
    QQueue<QueueDefectItem>         m_queueDefect;						//图像数据队列

    QTranslator                     m_translator;                       //翻译器
    QTranslator                     m_qtTranslator;                     //qt对话框翻译器。用于按钮文本等自动翻译

    QTimer*                         m_noGlassTimer;                     // 用于延迟切换到 NoGlass

	bool							m_ExportingData = false;            // 导出数据状态标志

    DefectFilterWidget*             m_defectFilterWidget;				//瑕疵类型过滤器
};

