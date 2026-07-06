#ifndef MAINPROCESS_H
#define MAINPROCESS_H

#include <QObject>
#include <QtSql>
#include <vector>
#include "Global.h"                 // 全局变量类
#include "GeneralMethod.h"          // 通用方法类
#include "PlatForm.h"               // 控制板通讯类(接受控制板指令，启动和终止拍照。脚踏板控制)
#include "AlgorithmProcess.h"       // 算法类。软件和算法主要交互操作类。
#include "CustomSqlQueryModel.h"
#include "CustomTableModel.h"
#include "CleanupFiles.h"           //文件清理类

#include "../Algorithm/showVirtualInfo.h"

// 定义共享指针类型
using QueueDefectItemPtr = std::shared_ptr<QueueDefectItem>;

class MainProcess : public QObject
{
    Q_OBJECT

public:
    /* 删除拷贝构造函数和赋值运算符 */
    MainProcess(const MainProcess&) = delete;
    MainProcess& operator=(const MainProcess&) = delete;

    /* 获取单例实例的静态方法 */
    static MainProcess& GetInstance();

    /* 清理单例资源 */
    static void destroyInstance();

    /* 初始化MainProcess*/
    bool InitMainProcess();

    /* 初始化自动清理 */
    bool InitCleanupFiles();

    /* 执行初始化设置(完成process初始化和widget初始化后) */
    void InitSettings();

    /* 自动清理模块消息处理 */
    void OnCleanupMessage(const QString& type, const QString& message);

    /* 从配置文件读入配置信息 */
    bool ReadSystemConfig();

    /* 读取配方参数信息 */
    bool Recipe_ReadConfig();

    /* 保存配置信息到配置文件 */
    void WriteSystemConfig();

    /* 初始化数据库和历史记录 */
    bool InitDataBaseAndHistoryRecord();

    void sql_setupCommonPragmas(QSqlDatabase& db);

    bool sql_refreshWriteConnection(const QString& dbName);

    bool sql_refreshReadConnection(const QString& dbName);

    void sql_refreshHistoryRecord(const QString& filter);

    void sql_refreshReportRecord(const QString& filter);

    /* 提供模型供ImageView模式下的历史记录使用 */
    CustomSqlQueryModel* modelHistoryQry() const;
    CustomTableModel* modelHistory() const;

    /* 提供模型供TableView模式下的数据报表使用 */
    CustomSqlQueryModel* modelReportQry() const;
    CustomTableModel* modelReport() const;


    /* 获取数据库连接*/
    //QSqlDatabase* GetDb();  //用于传递给界面展示数据

    /* 启动视觉流程*/
    void Sensor_StartVisionGrabbing();

    /* 停止视觉流程*/
    void Sensor_StopVisionGrabbing();

    /* 显示板进信息 */
    void Sensor_ShowGlassIn();

    /* 显示板出信息 */
    void Sensor_ShowGlassOut();

    /* 踩下脚踏板 */
    void Sensor_StepOnPedal();

    /* 设置视觉流程状态改变 */
    void Slot_SetVsionGrabbingStatus(bool state);

    /* 松开脚踏板 */
    void Sensor_ReleasePedal();

    /* 启动拍照 */
    void Vision_StartProcess();

    /* 停止拍照 */
    void Vision_StopProcess();

	/* 启动工作(进入工作状态，等待) */
    bool Vision_StartWorking();

	/* 停止工作 */
    bool Vision_StopWorking();

    /* 获得所有相机状态 */
    std::vector<bool> GetCameraStatus();

    /* 获得历史记录对应图片文件夹*/
    bool GetGlassImagePath(const QString& idRecords, QString& outPath, QString& outDateTime);

    /* 在当次流程结束后更新配方 */
    void Recipe_UpdateAtComputeAfter();

    /* 设置配方参数(传递配方参数给算法和界面) */
    void Recipe_SetParameter();

    /* 关闭光源 */
    bool CloseLight();

    /* 设置三色灯状态 */
    void SetStackLight(int status);

signals:

    /* 传递入队数据信号*/
    void Signal_Enqueue(const QueueDefectItemPtr& resultItem);

    /* 出队信号 */
    void Signal_Dequeue();

    /* 来料状态更新信号 */
    void Signal_GlassStatusChanged(GlassStatus state, const QString& dateTimeStr);

    /* 拿图完成信号*/
    void SignalGetImageOver();

    /* 视觉采集状态信号*/
    void Signal_SetVsionGrabbingStatus(bool state);

    /* 脚踏板操作信号 */
    void Signal_Pedal(bool state);

    /* 历史记录折线图数据传递 信号*/
    void Signal_ShowHistoryChart(const QMap<DefectType, QMap<QDate, int>>& data);

    /* 刷新历史数据表格 */
    void Slignal_FilterHistoryData();

    /* 传递瑕疵类型名称读入完成信号 */
    void Signal_DefectTypeNamesReady();

    /* 传递清理模块消息 */
    void Signal_ShowMessage(const QString& message);

    /* 显示板进信息*/
    void Signal_ShowGlassIn();

    /* 显示板出信息 */
    void Signal_ShowGlassOut();

    /* 刷新列显示装填*/
    void Signal_FilterReportAfter();

    /* 刷新列显示装填*/
    void Signal_FilterHistoryAfter();

    /* 传递保存数据信号*/
    void Signal_SaveData2DB(const QueueDefectItemPtr& resultItem);


public slots:
    /* ImageView模式下执行记录筛选槽函数 */
    void Slot_FilterHistoryRecord(const QString& filter);

    /* TableView模式下执行记录筛选槽函数 */
    void Slot_FilterReportRecord(const QString& filter);

    /* 刷新历史数据(视图模式左侧)和历史数据(表格模式) */
    void Slot_FilterHistoryData();

    /* 脚踏板槽函数 */
    void Slot_Pedal(bool signal);

    void Slot_System_WriteConfig();

    /* 修改默认配方 */
    void Slot_DefaultRecipeChanged();

    /* 显示板进信息 */
    void Slot_ShowGlassIn();

    /* 显示板出信息 */
    void Slot_ShowGlassOut();

    /* 保存数据 */
    void Slot_SaveData2DB(const QueueDefectItemPtr& resultItem);

private:
    /* 私有构造函数 */
    MainProcess(QObject* parent = nullptr);

    /* 连接信号和槽函数*/
    void InitConnect();

    /* 整理历史数据，用于显示历史统计折线图 */
    QMap<DefectType, QMap<QDate, int>> GetChartData(const QString& filter);

    //bool Vision_ProcessImage(const cv::Mat& currentImage_0, const cv::Mat& currentImage_1,
    //    const cv::Mat& currentImage_2, std::vector<drawInformation>& showInfo, std::string& timeProduce,
    //    std::vector<std::vector<cv::Mat>>& detectImages, cv::Mat& backGroundImage,
    //    double& scale, float& glassWidth, float& glassHeight, float& glassPixelWidth, float& glassPixelHeight);

    bool Vision_ProcessImage(QueueDefectItem& resultItem);

    //// 计算线程
    //void ComputerThread();

    // 计算线程
    void Vision_ComputerThread();

    // 保存错误图片
    void SaveErrorImages(QueueDefectItem& resultItem, std::string savePath, std::string errMsg);

    static QSqlDatabase createWriteConnectionForCurrentThread();

    void sql_saveData2DB(QueueDefectItem& resultItem);

private:
    GeneralMethod       m_GeneralMethod;                //通用方法类
    showVirtualInfo     m_showVirtualInfo;              //显示虚拟图
    /*************** 数据库相关 ***************/
    QString             m_dbName;                       //数据库名称
    QSqlDatabase        m_db;                           //数据库对象--写入
    QSqlDatabase        m_db_read;                      //数据库对象--读取
    CustomTableModel*   m_modelHistory;                 //历史记录表格模型
    CustomTableModel*   m_modelReport;                  //数据报表表格模型

    CustomSqlQueryModel* m_modelHistoryQry;                 //历史记录表格模型
    CustomSqlQueryModel* m_modelReportQry;                  //数据报表表格模型
    CustomSqlQueryModel* m_modelChartQry;                  //数据报表表格模型
    /******************************************/

    /************** 单次流程相关 **************/
    QString             m_time_produce_Real;            //单次流程启动时间

    bool                m_isFristImage=true;            //程序启动的第一次结果图像。用于按照逻辑自动显示第一次结果。

    bool                m_isRunning_Compute = false;    //计算线程运行状态

    bool                m_isRunning_Proess = false;     //视觉流程运行状态

    bool                IsAutoShowImage = false;
    /******************************************/
    std::string          m_customer="";                 //客户名称

    /*********** 脚踏板操作相关相关 ***********/
    bool                m_isShowNextImage = false;

    QDateTime           m_LastOverTime;                 //用于防护两次操作时间间隔过短

    int                 m_TimeInterval = 2;	            //单位：秒。踏板踩踏的防护时间间隔，避免频繁踩踏产生的流程问题
    /******************************************/

    /*************** 配方参数相关 ***************/
    std::vector<double> m_defectConfidence;             //各项瑕疵的置信度

    /************** 语言包相关参数 *************/
    QString             m_currentLanguage;              //当前语言包

    bool m_isInitRecipeOver = false;                    //初始化配方完成标志

    bool                m_IsAutoCleanup = false;        //是否执行自动清理。用于系统设置中，相关的参数是否变化的对比

    int                 m_ImagesRetentionDays = 0;      //保留文件的天数。用于系统设置中，相关的参数是否变化的对比

    int                 m_AutoCleanupTime = 0;          //定时执行的时间。用于系统设置中，相关的参数是否变化的对比

    std::string         m_CleanupPath = "D:/Glass/";    //定时清理的文件夹。用于系统设置中，相关的参数是否变化的对比

};

#endif // MAINPROCESS_H