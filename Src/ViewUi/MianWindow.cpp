#include "MainWindow.h"
#include <QtConcurrent/QtConcurrentRun>
#include "Log.hpp"
#include "DisplayMessage.h" //界面显示运行信息
#include "LicenseManager.h"     //授权管理

#define CAMERA_NUM 9    //相机的个数

#define SHOWSIZE 256 //弹窗展示图大小

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindowClass())
    , m_noGlassTimer(new QTimer(this))
{
    ui->setupUi(this);


    // 注册指针类型
    qRegisterMetaType<QueueDefectItem>("QueueDefectItem");
    qRegisterMetaType<QueueDefectItemPtr>("QueueDefectItemPtr"); // 使用别名也可以
    qRegisterMetaType<GlassStatus>("GlassStatus");

    InitProjectObject();

    InitChildObject();

    InitConnect();

    InitSettings();


    m_isReady = true;
    if (m_isReady)
    {
        ui->toolButton_StartWorking->setEnabled(true);
        ui->toolButton_StopWorking->setEnabled(false);
    }
    else
    {
        ui->toolButton_StartWorking->setEnabled(false);
        ui->toolButton_StopWorking->setEnabled(false);
    }


#ifdef SQL_QUERY
    m_HisRecordWidget->HistoryRecord_InitTableQry(MainProcess::GetInstance().modelHistoryQry());
    m_TableViewWidget->HistoryRecord_InitTableQry(MainProcess::GetInstance().modelReportQry());
#else
    m_HisRecordWidget->HistoryRecord_InitTable(MainProcess::GetInstance().modelHistory());
    m_TableViewWidget->HistoryRecord_InitTable(MainProcess::GetInstance().modelReport());
#endif

	ui->Widget_TestBtnGroup->setVisible(IS_READ_MODE == 0 ? false : true);
    //ui->toolBtn_TestBegin->setVisible(IS_READ_MODE == 0 ? false : true);
    //ui->toolBtn_TestEnd->setVisible(IS_READ_MODE == 0 ? false : true);

    /****** 窗口位置防护 ******/
    QRect available = QApplication::primaryScreen()->availableGeometry();
    int maxHeight = available.height() - 100; // 扣除任务栏和边距
    setMinimumSize(800, qMin(800, maxHeight)); // 最小高不超过屏幕

    // 在创建 dock 后设置最大高度（不超过可用屏幕高度的合理比例）
    int maxDockHeight = available.height() * 0.84; // 例如最多占 80%
    ui->dockWidget_Left->setMaximumHeight(maxDockHeight);

    // 在 Tab 切换时避免 adjustSize
    connect(ui->tabWidget_main, &QTabWidget::currentChanged, this, [this]() {
        updateLeftDockMaxHeight();
        });

    ui->tabWidget_main->setCurrentIndex(1);
    //ui->tabWidget_main->setCurrentIndex(0);
    m_ImageViewWidget->SetDefaultTabIndex(1);   //1=实时结果
    m_TableViewWidget->SetDefaultTabIndex(0);   //0=实时结果
    /************************/

	emit Signal_RefreshDefectTypeNameToTable(); //刷新表格模式下的瑕疵类型名称
    // 初始日志消息
    DisplayMessage::getInstance().logMessage(tr("Application started."));  //应用程序已启动


    /**************** 瑕疵类型过滤器设置 *******************/
   //初始化瑕疵类型过滤器
    m_defectFilterWidget = new DefectFilterWidget(this);

    // 可以将其放置在任何布局中
    ui->vLayout_defectDisplayFilter->addWidget(m_defectFilterWidget);
	ui->vLayout_defectDisplayFilter->setStretch(1, 0); // 上侧占满剩余空间

    SetDefectFilterTypeDisplay();
	m_defectFilterWidget->setAllCheckBoxesState(true); //默认全部选中显示
    // 居中显示设置窗口
    QTimer::singleShot(0, this, [this]() {
        if (!m_defectFilterWidget) return;

        m_defectFilterWidget->setDisplayStatus(DefectType::TYPE_POORCOATING, !m_defectTypeVisibility[DefectType::TYPE_POORCOATING]);
        m_defectFilterWidget->setDisplayStatus(DefectType::TYPE_POORCOATING, m_defectTypeVisibility[DefectType::TYPE_POORCOATING]);
        // 连接复选框状态变化信号到父对象的槽函数
        m_defectFilterWidget->connectCheckBoxChangesTo(this, SLOT(Slot_onDefectFilterChanged(int, bool)));
        });
    /*****************************************************/
	// 初始化无板定时器
    m_noGlassTimer->setSingleShot(true);
    connect(m_noGlassTimer, &QTimer::timeout, this, [this]() {
        QString noGlass_Time = "____-__-__ __:__:__";
        Slot_CardStatusChanged(GlassStatus::NoGlass, noGlass_Time);
        });

    MainProcess::GetInstance().SetStackLight(0); //黄灯亮
}

void MainWindow::updateLeftDockMaxHeight()
{
    if (!ui->dockWidget_Left || !ui->dockWidget_Left->isVisible())
        return;

    int availableContentHeight = 0;

    if (isMaximized()) {
        // 最大化：使用屏幕可用高度（已扣除 Windows 任务栏）
        QRect screenAvailable = QApplication::primaryScreen()->availableGeometry();
        availableContentHeight = screenAvailable.height();
    }
    else {
        // 非最大化：使用当前窗口高度
        availableContentHeight = height();
    }

    // 扣除程序内部固定区域
    int reservedHeight = 0;

    // 扣除状态栏
    if (statusBar() && statusBar()->isVisible()) {
        reservedHeight += statusBar()->height();
    }

    // 【可选】扣除顶部区域（如 menuBar、toolBar、自定义 banner）
    if (menuBar() && menuBar()->isVisible()) {
        reservedHeight += menuBar()->height();
    }
    // 假设你有一个顶部 banner widget，名为 ui->bannerWidget
    // if (ui->bannerWidget && ui->bannerWidget->isVisible()) {
    //     reservedHeight += ui->bannerWidget->height();
    // }

    int maxDockHeight = availableContentHeight - reservedHeight;
    // 至少保留一点高度，避免为负
    maxDockHeight = qMax(100, maxDockHeight);

    ui->dockWidget_Left->setMaximumHeight(maxDockHeight);
}

MainWindow::~MainWindow()
{
    MainProcess::GetInstance().SetStackLight(3); //关闭所有灯
    //std::cout << "MainWindow::~MainWindow() begin" << std::endl;
    if (m_timer)
    {
        m_timer->stop();
    }
    m_isWorking = false;
    //if (m_configWidget && m_configWidget->isVisible())
    //{
    //    // 断开所有信号连接，避免在关闭过程中触发槽函数
    //    disconnect(m_configWidget.get(), nullptr, this, nullptr);

    //    // 设置关闭标志，避免递归调用
    //    m_configWidget->setProperty("closing", true);

    //    // 关闭窗口但不立即删除
    //    m_configWidget->close();

    //    // 处理事件队列确保关闭完成
    //    QCoreApplication::processEvents();

    //    // 重置指针
        m_configWidget.reset();
    //}
    //m_configWidget->close();
    
    delete ui;
    //std::cout << "MainWindow::~MainWindow() over" << std::endl;
}

void MainWindow::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        // 当安装新的翻译器后，会触发 LanguageChange 事件
        ui->retranslateUi(this); // 重新翻译 UI
        //retranslateUi(); // 刷新非 UI 文件自动生成的文本
    }
}
void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_configWidget/* && m_configWidget->isVisible()*/)
    {
        // 断开所有信号连接，避免在关闭过程中触发槽函数
        disconnect(m_configWidget.get(), nullptr, this, nullptr);

        // 设置关闭标志，避免递归调用
        m_configWidget->setProperty("closing", true);

        // 关闭窗口但不立即删除
        m_configWidget->close();

        // 处理事件队列确保关闭完成
        QCoreApplication::processEvents();

        // 重置指针
        m_configWidget.reset();
    }
    event->accept();
}

void MainWindow::InitChildObject()
{
    /************* BannerDocket子窗口 ******************/
    // 隐藏标题栏
    ui->dockWidget_Banner->setFeatures(QDockWidget::NoDockWidgetFeatures); // 禁用所有交互功能（可选）
    QWidget* emptyTitleBar_Banner = new QWidget(); // 创建一个空的 QWidget
    ui->dockWidget_Banner->setTitleBarWidget(emptyTitleBar_Banner);
    ui->dockWidget_Banner->setFixedHeight(100);
    /***************************************************/

    /************* LeftDockWidget子窗口 ****************/

    // 隐藏标题栏
    ui->dockWidget_Left->setFeatures(QDockWidget::NoDockWidgetFeatures); // 禁用所有交互功能（可选）
    QWidget* emptyTitleBar_left = new QWidget(); // 创建一个空的 QWidget
    ui->dockWidget_Left->setTitleBarWidget(emptyTitleBar_left);
    ui->dockWidget_Left->setMinimumWidth(50);

    //历史记录子窗口
    m_HisRecordWidget = new HistoryRecordWidget(this);
    ui->vLayout_LeftFrame->addWidget(m_HisRecordWidget);

    //缩略图子窗口
    m_ThumbnailWidget = new ThumbnailWidget(this);
    ui->vLayout_LeftFrame->addWidget(m_ThumbnailWidget);

    m_ThumbnailWidget->setVisible(ui->tabWidget_main->currentIndex() == 0 ? false : true);
    m_HisRecordWidget->setVisible(ui->tabWidget_main->currentIndex() == 0 ? true : false);
    //ui->dockWidget_Left->style()->unpolish(ui->dockWidget_Left);
    //ui->dockWidget_Left->style()->polish(ui->dockWidget_Left);
    //ui->dockWidget_Left->update();
    /***************************************************/

    /************* Central区域的子窗口 *****************/
        // 设置为QTabWidget的右上角部件
    ui->tabWidget_main->setCornerWidget(ui->Widget_TestBtnGroup, Qt::TopRightCorner);

    m_ImageViewWidget = new ImageViewWidget(this);
    ui->gLayout_ImageView->addWidget(m_ImageViewWidget);

    m_TableViewWidget = new TableViewWidget(this);
    ui->gLayout_TableView->addWidget(m_TableViewWidget);
    /***************************************************/

    /************* 定时器 ******************************/
    //创建定时器。用于定时获取相机连接状态
    m_timer = new QTimer(this);
    m_timer->start(1000);    //设置定时器1s更新一次/

    m_Timer_Uptime = new QTimer(this);
    m_Timer_Uptime->start(2000);    //设置定时器1s更新一次/
    /***************************************************/

    /************* 状态栏 ******************************/
    //设置运行信息显示
    m_label_messageTitle = new QLabel(this);
    m_label_message = new QLabel(this);
    m_label_messageTitle->setText(tr("Message: "));
    m_label_message->setFixedWidth(300);
    ui->statusBar->addWidget(m_label_messageTitle);
    ui->statusBar->addWidget(m_label_message);
   
    DisplayMessage::getInstance().setDisplayLabel(m_label_message);    // 绑定运行信息显示类

    QFrame* vLine0 = new QFrame();
    vLine0->setFrameShape(QFrame::VLine);
    vLine0->setFrameShadow(QFrame::Sunken);
    ui->statusBar->addWidget(vLine0);

    //设置相机状态显示
    m_ledGreen = QPixmap(":/State/State/Led_G.png");
    m_ledRed = QPixmap(":/State/State/Led_R.png");
    m_cameraLables.resize(CAMERA_NUM);
    for (int i = 0; i < CAMERA_NUM; ++i)
    {
        QLabel* text = new QLabel(this);
        m_cameraLables[i] = new QLabel(this);

        QString string = tr("Camera %1:").arg(i + 1);
        text->setText(string);
        text->setAlignment(Qt::AlignCenter);

        m_cameraLables[i]->setPixmap(m_ledRed);
        m_cameraLables[i]->setAlignment(Qt::AlignCenter);

        ui->statusBar->addWidget(text);
        ui->statusBar->addWidget(m_cameraLables[i]);
    }
    QFrame* vLine1 = new QFrame();
    vLine1->setFrameShape(QFrame::VLine);
    vLine1->setFrameShadow(QFrame::Plain);
    ui->statusBar->addWidget(vLine1);

    //玻璃状态显示
    m_Label_GlassStatus = new QLabel(this);
    m_Label_GlassStatus->setFixedWidth(120);
    m_Label_GlassStatus->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // 文本右对齐

    m_Label_GlassInTime = new QLabel(this);
    m_Label_GlassInTime->setText("");

    ui->statusBar->addWidget(m_Label_GlassStatus);
    ui->statusBar->addWidget(m_Label_GlassInTime);
    QString glassIn_Time = "____-__-__ __:__:__";
    Slot_CardStatusChanged(GlassStatus::NoGlass, glassIn_Time);
    QFrame* vLine2 = new QFrame();
    vLine2->setFrameShape(QFrame::VLine);
    vLine2->setFrameShadow(QFrame::Raised);
    ui->statusBar->addWidget(vLine2);

    //运行时间显示
    m_Label_Uptime = new QLabel(this);
    m_Label_Uptime->setText(tr("Uptime:      h :     m :     s"));
    m_Label_Uptime->setAlignment(Qt::AlignRight | Qt::AlignVCenter); // 文本右对齐

    // 使用 addPermanentWidget，并设置伸缩因子（stretch）
    // stretch = 1 表示该控件会占据所有剩余空间，从而将前面的控件“挤”到左边
    ui->statusBar->addPermanentWidget(m_Label_Uptime, 1);

    // 设置 size policy，确保它能伸缩
    m_Label_Uptime->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    //m_Label_Uptime->setStyleSheet("QLabel { padding-right: 10px; }");
    ui->statusBar->addWidget(m_Label_Uptime);
    m_ElapsedTimer.start();
    /***************************************************/

    /************* 连接检查授权状态信号，定时检查有LicenseManager内部自行处理 ******************************/
    connect(LicenseManager::getInstance(), &LicenseManager::signal_licenseExpired, this, &MainWindow::slot_licenseExpired);

    ui->checkBox_minorLevel->setChecked(DISPLAY_MINOR);
    ui->checkBox_mediumLevel->setChecked(DISPLAY_MEDIUM);
    ui->checkBox_seriousLevel->setChecked(DISPLAY_SERIOUS);

}

void MainWindow::InitProjectObject()
{
    //MainProcess::GetInstance();
    if (MainProcess::GetInstance().InitMainProcess())
    {
    }
    else
    {
        m_isWorking = false;
    }

}

void MainWindow::InitConnect()
{

    /*********************** 测试按钮 信号槽 ************************/
    connect(ui->toolBtn_TestBegin, &QToolButton::clicked, this, &MainWindow::Slot_StartProess);
    connect(ui->toolBtn_TestEnd, &QToolButton::clicked, this, &MainWindow::Slot_StopProcess);
    /****************************************************************/

    /*********************** 配置按钮 信号槽 ************************/
    //配置按钮
    connect(ui->toolButton_Config, &QToolButton::clicked, this, &MainWindow::Slot_OpenConfigWidget);
    //启动检测按钮
    connect(ui->toolButton_StartWorking, &QToolButton::clicked, this, &MainWindow::Slot_StartWorking);
    //停止检测按钮
    connect(ui->toolButton_StopWorking, &QToolButton::clicked, this, &MainWindow::Slot_StopWorking);
    //定时器，更新运行时长
    connect(m_Timer_Uptime, &QTimer::timeout, this, &MainWindow::Slot_Display_Uptime);
    //定时器，更新相机状态
    connect(m_timer, &QTimer::timeout, this, &MainWindow::Slot_UpdateTime);
    //切换Tab标签
    connect(ui->tabWidget_main, &QTabWidget::currentChanged, this, &MainWindow::Slot_TabCurrentChanged);

    //保存缺陷等级显示状态到系统配置文件
    connect(this, &MainWindow::Signal_WriteSystemConfig, &MainProcess::GetInstance(), &MainProcess::Slot_System_WriteConfig);

    connect(ui->checkBox_minorLevel, &QCheckBox::clicked, this, &MainWindow::Slot_Recipe_SetMinorLevelDisplayState);
    connect(ui->checkBox_mediumLevel, &QCheckBox::clicked, this, &MainWindow::Slot_Recipe_SetMediumLevelDisplayStat);
    connect(ui->checkBox_seriousLevel, &QCheckBox::clicked, this, &MainWindow::Slot_Recipe_SetSeriousLevelDisplayStat);
    /****************************************************************/

    /********************* mainProcess 信号槽 ***********************/
    connect(&MainProcess::GetInstance(), &MainProcess::Signal_ShowMessage, this, &MainWindow::Slot_ShowMessage);
    connect(&MainProcess::GetInstance(), &MainProcess::Signal_Enqueue, this, &MainWindow::Slot_Enqueue);
    connect(&MainProcess::GetInstance(), &MainProcess::Signal_Dequeue, this, &MainWindow::Slot_Dequeue);
    connect(&MainProcess::GetInstance(), &MainProcess::Signal_GlassStatusChanged, this, &MainWindow::Slot_CardStatusChanged);
    connect(this, &MainWindow::Signal_RefreshDefectTypeNameToTable, m_TableViewWidget, &TableViewWidget::Slot_RefreshDefectTypeNameToTable);

    /****************************************************************/

    /***************** ImageView图像视图模式 信号槽 *****************/
    connect(this, &MainWindow::Signal_ClearDisplaySignal, m_ImageViewWidget, &ImageViewWidget::Slot_HandleClearDisplay);
    connect(this, &MainWindow::Signal_SetDefectTypeVisibility, m_ImageViewWidget, &ImageViewWidget::Slot_SetDefectTypeVisibility);
    connect(this, &MainWindow::Signal_SetDefectLevelVisibility, m_ImageViewWidget, &ImageViewWidget::Slot_SetDefectLevelVisibility);

    //左侧dock窗口历史记录 信号槽
    connect(m_HisRecordWidget, &HistoryRecordWidget::Signal_FilterRequested, &MainProcess::GetInstance(), &MainProcess::Slot_FilterHistoryRecord);
    connect(&MainProcess::GetInstance(), &MainProcess::Signal_FilterHistoryAfter, m_HisRecordWidget, &HistoryRecordWidget::Slot_HistoryRecord_SetColumnsDisplayStatus);
    connect(m_HisRecordWidget, &HistoryRecordWidget::Signal_ShowHistoryImage, this, &MainWindow::Slot_ShowHistoryImage);
    connect(this, &MainWindow::Signal_SetDefectTypeVisibility,  m_HisRecordWidget, &HistoryRecordWidget::Slot_SetDefectTypeVisibility);
    connect(this, &MainWindow::Signal_SetDefectLevelVisibility, m_HisRecordWidget, &HistoryRecordWidget::Slot_SetDefectLevelVisibility);
    /****************************************************************/

    /****************** TableView表格视图模式下 信号槽 **************/
    connect(this, &MainWindow::Signal_ClearDisplaySignal, m_TableViewWidget, &TableViewWidget::Slot_HandleClearDisplay);
    connect(m_TableViewWidget, &TableViewWidget::Signal_FilterRequested, &MainProcess::GetInstance(), &MainProcess::Slot_FilterReportRecord);
    connect(&MainProcess::GetInstance(), &MainProcess::Signal_FilterReportAfter, m_TableViewWidget, &TableViewWidget::Slot_HistoryRecord_SetColumnsDisplayStatus);
    connect(m_TableViewWidget, &TableViewWidget::Signal_ShowHistoryImage, this, &MainWindow::Slot_ShowHistoryImage);    //显示三通道缩略图到左侧界面
    connect(&MainProcess::GetInstance(), &MainProcess::Signal_DefectTypeNamesReady, m_TableViewWidget, &TableViewWidget::Slot_RefreshDefectTypeNameToTable);
    connect(&MainProcess::GetInstance(), &MainProcess::Signal_ShowHistoryChart, m_TableViewWidget, &TableViewWidget::Slot_ShowHistory_Chart);
    connect(this, &MainWindow::Signal_SetDefectTypeVisibility, m_TableViewWidget, &TableViewWidget::Slot_SetDefectTypeVisibility);
    connect(this, &MainWindow::Signal_SetDefectLevelVisibility, m_TableViewWidget, &TableViewWidget::Slot_SetDefectLevelVisibility);
	connect(m_TableViewWidget, &TableViewWidget::Signal_NotifyExportDataStatus, this, &MainWindow::Slot_GetExportDataStatus);
    /****************************************************************/

    connect(this, &MainWindow::Signal_Dequeue, this, &MainWindow::Slot_Dequeue);
}

// 授权过期
void MainWindow::slot_licenseExpired()
{
    close();
}

void MainWindow::Slot_ShowMessage(QString message)
{
    DisplayMessage::getInstance().logMessage(message);  //调整设置
}

void MainWindow::Slot_GetExportDataStatus(bool isExporting)
{
	m_ExportingData = isExporting;
}

// 在 MainWindow 的某个槽函数中，比如切换语言菜单的响应函数
void MainWindow::Slot_System_LanguageChanged(int langCode)
{
    // 1. 移除旧的翻译器
    qApp->removeTranslator(&m_translator);
    qApp->removeTranslator(&m_qtTranslator);


    // 2. 加载新的翻译器
    QString translatorPath ;
    switch (langCode)
    {
    case 0:
        translatorPath = lang_English;
        translatorPath = "./Translation/GlassInspection_en_US.qm";
        break;
    case 1:
        translatorPath = lang_Russian;
        translatorPath = "./Translation/GlassInspection_ru_RU.qm";
        break;
    case 2:
        translatorPath = lang_Chinese;
        translatorPath = "./Translation/GlassInspection_zh_CN.qm";
		break;
    case 3:
		translatorPath = lang_Spanish;
        translatorPath = "./Translation/GlassInspection_es_ES.qm";
        break;
    default:
        translatorPath = lang_Portuguese;
        translatorPath = "./Translation/GlassInspection_pt_BR.qm";
        break;
    }
    if(m_translator.load(translatorPath))
    {
        qApp->installTranslator(&m_translator);
    }

    // 3. 重新翻译 Qt 内部字符串
    if (m_qtTranslator.load("qt_" + langCode, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
    {
        qApp->installTranslator(&m_qtTranslator);
    }


    // 4. 关键步骤：触发所有子控件重新翻译
    // 方法一：发送 LanguageChange 事件 (推荐)
    qApp->postEvent(this, new QEvent(QEvent::LanguageChange));
    // 这会触发 MainWindow 及其所有子控件的 changeEvent(QEvent *event) 函数

    // 方法二：手动调用 retranslateUi() (如果你用的是 Qt Designer 生成的 ui 文件)
    // ui->retranslateUi(this);
    // 但这种方法只更新 .ui 文件中的文本，不更新代码中动态设置的或 Qt 内部的文本。
}

void MainWindow::Slot_OpenConfigWidget()
{
    // 如果窗口不存在或已被关闭，则创建新窗口
    m_configWidget.reset();
        // 创建新窗口（使用智能指针管理）
        m_configWidget = std::make_unique<ConfigWidget>(this);
    
        // 设置为独立窗口
        m_configWidget->setWindowFlags(Qt::Window |
            Qt::WindowTitleHint |
            Qt::WindowCloseButtonHint/* |
            Qt::WindowMinMaxButtonsHint*/);

        // 连接窗口关闭信号到重置指针的槽函数
        connect(m_configWidget.get(), &QWidget::destroyed, this, [this]() {
            m_configWidget.reset(); // 窗口销毁时重置智能指针
            });
        connect(m_configWidget.get(), &ConfigWidget::Signal_SetDefectTypeVisibility, this, &MainWindow::Slot_SetDefectTypeVisibility);
        connect(m_configWidget.get(), &ConfigWidget::Signal_SetDefectLevelVisibility, this, &MainWindow::Slot_SetDefectLevelVisibility);

        connect(m_configWidget.get(), &ConfigWidget::Signal_WriteSystemConfig, &MainProcess::GetInstance(), &MainProcess::Slot_System_WriteConfig);
        connect(m_configWidget.get(), &ConfigWidget::Signal_LanguageChanged, this, &MainWindow::Slot_System_LanguageChanged);
        connect(m_configWidget.get(), &ConfigWidget::Signal_DefaultRecipeChanged, &MainProcess::GetInstance(), &MainProcess::Slot_DefaultRecipeChanged);
        m_configWidget->SetCurrentRecipe();

    //// 居中显示设置窗口
    //QTimer::singleShot(0, this, [this]() {
    //    if (!m_configWidget) return;

    //    QRect parentRect = this->frameGeometry();
    //    QRect childRect = m_configWidget->frameGeometry();
    //    m_configWidget->move(parentRect.center() - childRect.center());
    //    });

    // 显示窗口
    m_configWidget->show();
    m_configWidget->raise();
    m_configWidget->activateWindow();
    DisplayMessage::getInstance().logMessage(tr("Adjust Settings"));  //调整设置
}


void MainWindow::Slot_StartWorking()
{
    if (m_ExportingData)
    {
		QString message = tr("Exporting historical data. Detection startup is prohibited.");
        QMessageBox::warning(this, tr("Warning"), message);
        FILE_LOG_WARN("Exporting historical data. Detection startup is prohibited.");
        DisplayMessage::getInstance().logMessage(message);  //无效缺陷数据
        return;
    }

    bool isWorking = MainProcess::GetInstance().Vision_StartWorking();

    //m_isWorking = true;
    ui->toolButton_Config->setEnabled(!isWorking);
    ui->toolButton_StartWorking->setEnabled(!isWorking);
    ui->toolButton_StopWorking->setEnabled(isWorking);

    //工作状态，禁止导出数据
	m_TableViewWidget->SetExportDataButtonEnabled(!isWorking);

	//MainProcess::GetInstance().CloseLight(); //关闭光源
    //isGlassStopGrabbing = true;
    if (isWorking)
    {
        DisplayMessage::getInstance().logMessage(tr("Start working"));  //启动工作
    }
	//MainProcess::GetInstance().SetStackLight(1); //绿灯亮
}

void MainWindow::Slot_StopWorking()
{

    bool isWorking = MainProcess::GetInstance().Vision_StopWorking();

    //m_isWorking = false;
    ui->toolButton_Config->setEnabled(true);
    ui->toolButton_StartWorking->setEnabled(true);
    ui->toolButton_StopWorking->setEnabled(false);

    //工作状态，禁止导出数据
    m_TableViewWidget->SetExportDataButtonEnabled(true);

    //MainProcess::GetInstance().CloseLight(); //关闭光源
    //isGlassStopGrabbing = false;
    DisplayMessage::getInstance().logMessage(tr("Stop working"));  //停止工作
    //MainProcess::GetInstance().SetStackLight(0); //黄灯亮
}

void MainWindow::Slot_UpdateTime()
{
    //QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    //m_statusLable->setText(time);

    // 异步获取
    QtConcurrent::run([this]() {
        auto status = MainProcess::GetInstance().GetCameraStatus();

        // 回到主线程更新 UI
        QMetaObject::invokeMethod(this, [this, status]() {
            for (int i = 0; i < status.size(); ++i)
            {
                m_cameraLables[i]->setPixmap(status[i] ? m_ledGreen : m_ledRed);
            }
            }, Qt::QueuedConnection);
        });

}

void MainWindow::Slot_Display_Uptime()
{
   // QElapsedTimer timer;
    //timer.start();

    // 获取已运行的总毫秒数
    qint64 elapsedMs = m_ElapsedTimer.elapsed();

    // 将毫秒转换为秒
    qint64 totalSeconds = elapsedMs / 1000;

    // 计算小时、分钟、秒
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    // 格式化字符串
    QString uptimeText = QString(tr("uptime: %1h : %2m : %3s  "))
        .arg(hours, 4)        // %1: 小时，至少3个字符宽（右对齐，不足补空格）
        .arg(minutes, 3)      // %2: 分钟
        .arg(seconds, 3);     // %3: 秒

    //QString uptimeText = QString("uptime: %1h : %2m  ")
    //    .arg(hours, 4)        // %1: 小时，至少3个字符宽（右对齐，不足补空格）
    //    .arg(minutes, 3);     // %2: 分钟

    // 更新标签文本
    m_Label_Uptime->setText(uptimeText);
    // 打印执行时间
    //qDebug() << "Slot_Display_Uptime took:" << timer.nsecsElapsed() << "nanoseconds";
}

void MainWindow::Slot_CardStatusChanged(GlassStatus state, const QString& dateTimeStr)
{
    // 无论什么状态，先停止可能正在等待的 NoGlass 定时器
    if (m_noGlassTimer->isActive()) {
        m_noGlassTimer->stop();
    }

    switch (state)
    {
    case GlassStatus::GlassIn:
        m_Label_GlassStatus->setText(tr("Glass In"));
        m_Label_GlassStatus->setStyleSheet("background-color: #67c23a;color: white; border - radius: 15px;  padding: 5px 16px;");
        break;
    case GlassStatus::GlassOut:
        m_Label_GlassStatus->setText(tr("Glass Out"));
        m_Label_GlassStatus->setStyleSheet("background-color: #f04b5c;color: white; border - radius: 15px;  padding: 5px 16px;");
        // 启动1秒后自动变为 NoGlass 的定时器
        m_noGlassTimer->start(2000);
        break;
    case GlassStatus::NoGlass:
        m_Label_GlassStatus->setText(tr("No Glass"));
        m_Label_GlassStatus->setStyleSheet("background-color: #79bbff;color: white; border - radius: 15px;  padding: 5px 16px;");
        break;
    case GlassStatus::Error:
        m_Label_GlassStatus->setText(tr("Error"));
        m_Label_GlassStatus->setStyleSheet("background-color: #67c23a;color: white; border - radius: 15px;  padding: 5px 16px;");
        break;
    }
    m_Label_GlassInTime->setText(dateTimeStr);
}

void MainWindow::Slot_StartProess()
{
    //emit MainProcess::GetInstance().Signal_SetVsionGrabbingStatus(true);
    READ_MODE_PORCESS_STATUS = 0;
}

void MainWindow::Slot_StopProcess()
{
    //emit MainProcess::GetInstance().Signal_SetVsionGrabbingStatus(false);
	READ_MODE_PORCESS_STATUS = 1;
}

void MainWindow::Slot_ShowHistoryImage(int typeIndex, QString originalTime, QString glassId,
    float glassPhysicalWidth, float glassPhysicalHeight, QString path)
{
    FILE_LOG_DEBUG("Slot_ShowHistoryImage: Begin.");
    ///* 先进行数据的保存和清除 */
    //SaveAndClearItem(ui->graphicsView);

    //根据双击的行数，获取对应的数据
    QString idRecords = originalTime + glassId;

    //根据时间和玻璃编号查询数据库，获取对应的图片
    //QString path;
    //QString times;
    //bool rst = MainProcess::GetInstance().GetGlassImagePath(idRecords, path, times);
    //只读取第一条数据
    //if (rst)
    {
        QString filePath = path + "/" + originalTime + "_" + glassId;
        QDateTime dateTime = QDateTime::fromString(originalTime.left(14), "yyyyMMddhhmmss");
        if (typeIndex == 0)
        {
            m_ImageViewWidget->ShowHistoryImage(originalTime, filePath, dateTime, glassPhysicalWidth, glassPhysicalHeight);
        }
        else if (typeIndex == 1)
        {
            m_TableViewWidget->HistoryRecord_ShowImageDialog(originalTime, filePath, dateTime, glassPhysicalWidth, glassPhysicalHeight);
            //m_TableViewWidget->HistoryRecordDefect_RefreshTable(filePath);
        }
        else if (typeIndex == 2)
        {
            m_ThumbnailWidget->ShowHistoryThumbnailImage(originalTime, filePath, dateTime, glassPhysicalWidth, glassPhysicalHeight);
            m_TableViewWidget->HistoryRecordDefect_RefreshTable(filePath);
        }
        else
        {
            FILE_LOG_WARN("Slot_ShowHistoryImage Error: typeIndex error.");
            return;
        }
    }
    //else
    //{
    //    FileLogPrintf("Slot_ShowHistoryImage Error: GetGlassImagePath failed.");
    //}
    FILE_LOG_DEBUG("Slot_ShowHistoryImage: Completed.");
}

void MainWindow::Slot_SetDefectTypeVisibility(DefectType type, bool visible)
{
    //更新历史主表过滤器的瑕疵类型显示状态
    if (m_defectFilterWidget)
    {
        // 显示该 defect type 的数据
        m_defectFilterWidget->setDisplayStatus(type, visible);
    }
	
    emit Signal_SetDefectTypeVisibility(type, visible);
}

void MainWindow::Slot_SetDefectLevelVisibility(DefectLevel level, bool visible)
{
    emit Signal_SetDefectLevelVisibility(level, visible);
}

void MainWindow::Slot_TabCurrentChanged(int tabIndex)
{
    m_ThumbnailWidget->setVisible(false);
    m_HisRecordWidget->setVisible(false);

    if (tabIndex ==0)
    {
        m_HisRecordWidget->setVisible(true);
        m_TableViewWidget->HistoryRecord_CloseImageDialog();
        ui->dockWidget_Left->setMinimumWidth(413);
        ui->dockWidget_Left->setMaximumWidth(524287);
    }
    else
    {
        m_ThumbnailWidget->setVisible(true);
        ui->dockWidget_Left->setMinimumWidth(413);
        ui->dockWidget_Left->setMaximumWidth(413);
    }
    FILE_LOG_DEBUG("Slot_TabCurrentChanged: Current tab index changed completed. tabIndex=%d", tabIndex);
}


// 入队槽函数
void MainWindow::Slot_Enqueue(const QueueDefectItemPtr& resultItem)
{
    Enqueue(resultItem);
}

// 入队操作
void MainWindow::Enqueue(const QueueDefectItemPtr& resultItem)
{
    FILE_LOG_INFO("Enqueue: Begin.");
    int queueSize = 0;
    int autoShowValue = 0;

#if 1
    //通用版本
    {
        QMutexLocker locker(&m_queueMutex);

        // 防护动作。限制队列大小
        if (queueSize >= 50)
        {
            if (!m_queueDefect.isEmpty())
            {
                m_queueDefect.dequeue(); // 移除最旧的项目
            }
        }

        // 入队
        m_queueDefect.enqueue(*resultItem);

        // 获取队列大小
        queueSize = m_queueDefect.size();
       // autoShowValue = m_autoShowImage;
    }
#else
    //流式版本使用
    auto&& defectImages_0 = std::move(resultItem->defectImages_ch0);
    auto&& defectImages_1 = std::move(resultItem->defectImages_ch1);
    auto&& defectImages_2 = std::move(resultItem->defectImages_ch2);
    auto&& showInfo = std::move(resultItem->drawInfo);
    auto&& backGround = std::move(resultItem->backgroundImage);
    auto&& pseudoColorImage = std::move(resultItem->pseudoColorImage);
    auto&& timeProduce = std::move(resultItem->timeProduce);

    /* 表格视图显示实时数据和图表 */
    m_TableViewWidget->CurrentRecord_ShowDefectData(timeProduce, showInfo);

    /* 图像视图显示实时图像 */
    m_ImageViewWidget->ShowCurrentImage(
        defectImages_0,
        defectImages_1,
        defectImages_2,
        backGround,
        pseudoColorImage,
        showInfo,
        timeProduce,
        resultItem->scale,
        resultItem->glassPhysicalWidth,
        resultItem->glassPhysicalHeight,
        resultItem->glassPixelWidth,
        resultItem->glassPixelHeight
    );
#endif
    FILE_LOG_INFO("Enqueue: Completed.");

    //emit Signal_UpdateImageCount(queueSize, autoShowValue);

}

// 出队槽函数
void MainWindow::Slot_Dequeue()
{
    Dequeue();
}


//出队操作
void MainWindow::Dequeue()
{
    FILE_LOG_INFO("Dequeue: Begin.");
    int queueSize = 0;
    int autoShowValue = 0;
    std::optional<QueueDefectItem> item;
    {
        QMutexLocker locker(&m_queueMutex);
        // 出队
        if (!m_queueDefect.empty())
        {
            item = std::move(m_queueDefect.dequeue());
        }
        else
        {

        }
        queueSize = m_queueDefect.size();
        autoShowValue = 1;
    }

    if (item)
    {
        auto&& defectImages_0 = std::move(item->defectImages_ch0);
        auto&& defectImages_1 = std::move(item->defectImages_ch1);
        auto&& defectImages_2 = std::move(item->defectImages_ch2);
        auto&& showInfo = std::move(item->drawInfo);
        auto&& backGround = std::move(item->backgroundImage);
        auto&& pseudoColorImage = std::move(item->pseudoColorImage);
        auto&& timeProduce = std::move(item->timeProduce);

        /* 表格视图显示实时数据和图表 */
        m_TableViewWidget->CurrentRecord_ShowDefectData(timeProduce, showInfo);

        /* 图像视图显示实时图像 */
        m_ImageViewWidget->ShowCurrentImage(
            defectImages_0,
            defectImages_1,
            defectImages_2,
            backGround,
            pseudoColorImage,
            showInfo,
            timeProduce,
            item->scale,
            item->glassPhysicalWidth,
            item->glassPhysicalHeight,
            item->glassPixelWidth,
            item->glassPixelHeight
        );

    }
    else
    {
        emit Signal_ClearDisplaySignal();
    }

    //emit Signal_UpdateImageCount(queueSize, autoShowValue);
    FILE_LOG_INFO("Dequeue: Completed.");
}

void MainWindow::InitSettings()
{
    MainProcess::GetInstance().InitSettings();


    m_ImageViewWidget->SetDefectLevelVisibilityFunction(DefectLevel::NORMAL,                DISPLAY_NORMAL          );          //0 正常
    m_ImageViewWidget->SetDefectLevelVisibilityFunction(DefectLevel::MINOR,                 DISPLAY_MINOR           );          //1 轻微缺陷
    m_ImageViewWidget->SetDefectLevelVisibilityFunction(DefectLevel::MEDIUM,                DISPLAY_MEDIUM          );          //2 中等缺陷
    m_ImageViewWidget->SetDefectLevelVisibilityFunction(DefectLevel::SERIOUS,               DISPLAY_SERIOUS         );          //3 严重缺陷
    m_ImageViewWidget->SetDefectLevelVisibilityFunction(DefectLevel::ABNORMAL,              DISPLAY_ABNORMAL        );          //4 异常
    m_ImageViewWidget->SetDefectLevelVisibilityFunction(DefectLevel::AREAERROR,             DISPLAY_AREAERROR       );          //5 区域错误，非玻璃区域判断
    
    for (size_t i = 0; i < DefectType::DEFECT_TYPE_COUNT; i++)
    {
        DefectType type = static_cast<DefectType>(i);
        m_ImageViewWidget->SetDefectTypeVisibilityFunction(type, m_defectTypeVisibility[type]);
    }
}

void MainWindow::Slot_Recipe_SetMinorLevelDisplayState(bool state)
{
    DefectLevel defectLevel = DefectLevel::MINOR;
    DISPLAY_MINOR = state;

    emit Signal_WriteSystemConfig();
    emit Signal_SetDefectLevelVisibility(defectLevel, state);
}

void MainWindow::Slot_Recipe_SetMediumLevelDisplayStat(bool state)
{
    DefectLevel defectLevel = DefectLevel::MEDIUM;
    DISPLAY_MEDIUM = state;

    emit Signal_WriteSystemConfig();
    emit Signal_SetDefectLevelVisibility(defectLevel, state);
}

void MainWindow::Slot_Recipe_SetSeriousLevelDisplayStat(bool state)
{
    DefectLevel defectLevel = DefectLevel::SERIOUS;
    DISPLAY_SERIOUS = state;

    emit Signal_WriteSystemConfig();
    emit Signal_SetDefectLevelVisibility(defectLevel, state);
}

// 处理复选框状态变化的槽函数
void MainWindow::Slot_onDefectFilterChanged(int index, bool checked)
{
    std::cout << "index = " << index << std::endl;
    // 获取缺陷类型
    DefectType type = static_cast<DefectType>(index);

    //Slot_SetDefectTypeVisibility(type, checked);;
    emit Signal_SetDefectTypeVisibility(type, checked);

}


void MainWindow::SetDefectFilterTypeDisplay()
{
    for (size_t i = 0; i < DefectType::DEFECT_TYPE_COUNT; i++)
    {
        DefectType type = static_cast<DefectType>(i);
        if (m_defectFilterWidget)
        {
            m_defectFilterWidget->setDisplayStatus(type, m_defectTypeVisibility[type]);
        }
    }

}
