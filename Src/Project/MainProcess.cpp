#include "MainProcess.h"
#include "Log.hpp"
#include "CameraManager.h"  //相机管理类
#include "DataSave.h"       //数据保存类(保存图像文件、保存H5文件、瑕疵数据json文件、数据库记录等)
#include "ZController.h"    //Commander控制类
#include "PlatForm.h"
#include "ConfigManager.h"	//读写System配置文件
#include "RecipeConfig.h"   //读写Recipe配方参数文件
#include "ResultBuffer.h"   //用于保存拼接后的批量图像缓存
#include "DisplayMessage.h" //界面显示运行信息

MainProcess& MainProcess::GetInstance()
{
    static MainProcess instance;  
    return instance;
}

void MainProcess::destroyInstance()
{
    std::cout<<"MainProcess::destroyInstance"<<std::endl;
    Platform::Instance().CloseControlCard();

    //if (MainProcess::GetInstance().m_db.isValid()) {
    //    QString connectionName = MainProcess::GetInstance().m_db.connectionName();

    //    if (MainProcess::GetInstance().m_db.isOpen()) {
    //        MainProcess::GetInstance().m_db.close();
    //    }

    //    QSqlDatabase::removeDatabase(connectionName);
    //}
}

bool MainProcess::InitMainProcess()
{

    //volatile int a = 1;
    //volatile int b = 0;
    //volatile int c = a / b; // 整数除零 → 崩溃（Windows 下会触发异常）
    //(void)c;
    //FILE_LOG_INFO("=== 正常结束（不应执行到此处） ===");

    FILE_LOG_INFO("InitCameras begin.");

    /* 初始化系统配置信息 */
    if (!ReadSystemConfig())
    {
        FILE_LOG_ERROR("[MainProcess]InitMainProcess Error: ReadSystemConfig failed.");
        return false;
    }
    //WriteSystemConfig();

    /* 初始化算法类 */
	AlgorithmProcess::GetInstance().InitAlgorithmProcess(); //主要用于登记算法线程名称使用。

    /* 加载当前默认配方参数 */
    if (!Recipe_ReadConfig())
    {
        FILE_LOG_ERROR("[MainProcess]InitMainProcess Error: ReadRecipeConfig failed.");
        return false;
    }
    Recipe_SetParameter();  //传递配方参数给算法和界面

    m_isInitRecipeOver = true;

    /* 初始化清理文件类 */
    InitCleanupFiles();

    /* 初始化数据库 */
    if (!InitDataBaseAndHistoryRecord())
    {
        FILE_LOG_ERROR("[MainProcess]InitMainProcess Error: InitDataBaseAndHistoryRecord failed.");
        return false;
    }

    InitConnect();

    /* 初始化相机管理类 */
	CameraBrand camBrand = static_cast<CameraBrand>(CameraBrandIndex);  //CameraBrandIndex值： 0=海康,  1=华睿
    FILE_LOG_ERROR("[MainProcess]InitMainProcess: Used Camera Brand is %s",(CameraBrandIndex==0 ? "Hikvision" : "Huaray"));
    CameraManager::GetInstance()->SetCameraBrand(camBrand);  //设置相机品牌
    if (!CameraManager::GetInstance()->InitializeCameras())
    {
        FILE_LOG_ERROR("[MainProcess]InitMainProcess Error: InitializeCameras failed.");
        return false;
    }
    if (IS_READ_MODE == 0)
    {
        if (!CameraManager::GetInstance()->Open())
        {
            FILE_LOG_ERROR("[MainProcess]InitMainProcess Error: OpenCamera failed.");
            return false;
        }

        if (!CameraManager::GetInstance()->ImageHeight(ImageHeight))//初始化各个相机采集行高
        {
            FILE_LOG_ERROR("[MainProcess]InitMainProcess Error: ImageHeight failed.");
            return false;
        }
        else
        {
            FILE_LOG_INFO("[MainProcess]InitMainProcess: Set ImageHeight =%d.", ImageHeight);
        }

        if (!CameraManager::GetInstance()->SetExposureTime(ExposureTime))//初始化各个相机采集曝光
        {
            FILE_LOG_ERROR("[MainProcess]InitMainProcess Error: SetExposureTime failed.");
            return false;
        }
        else
        {
            FILE_LOG_INFO("[MainProcess]InitMainProcess: Set SetExposureTime =%d.", ExposureTime);
        }


        //if (!CameraManager::GetInstance()->StartGrabbing())
        //{
        //    FILE_LOG_ERROR("[MainProcess]InitMainProcess Error: StartGrabbing failed.");
        //    return false;
        //}
    }
    ResultBuffer::GetInstance(1000);    //设置批图像(多个相机单帧横向拼接后的图像)缓存

    //AlgorithmProcess::GetInstance();

    /* 初始化平台控制卡 */
    if (!Platform::Instance().InitControlCard())
    {
        FILE_LOG_ERROR("[MainProcess]InitMainProcess Error: InitControlCard failed.");
        return false;
    }

    /* 和流程控制进行绑定 */
    ZController::GetInstance()->SetWidget(this);    //绑定控制类，使控制类可以按通信信号发出控制指令。

    m_isReady = true;
    m_isInitRecipeOver = true;
    return true;
}

bool MainProcess::InitCleanupFiles()
{
    //std::string path = RecipeInfo.global.dataSavePath.toStdString() + "/SaveData/";
    std::string path = RecipeInfo.global.dataSavePath.toStdString() + "/";
    if (m_CleanupPath == path
        && m_IsAutoCleanup == IsAutoCleanup
        && m_ImagesRetentionDays == ImagesRetentionDays
        && m_AutoCleanupTime == AutoCleanupTime)
    {
        return true;
    }
    FILE_LOG_INFO("Begin.");
    m_CleanupPath = path;
    m_IsAutoCleanup = IsAutoCleanup;
    m_ImagesRetentionDays = ImagesRetentionDays;
    m_AutoCleanupTime = AutoCleanupTime;

    FILE_LOG_INFO("IsAutoCleanup = %s", m_IsAutoCleanup ? "True" : "False");
    FILE_LOG_INFO("CleanupPath = %s", m_CleanupPath.c_str());
    FILE_LOG_INFO("ImagesRetentionDays = %d Days", m_ImagesRetentionDays);
    FILE_LOG_INFO("AutoCleanupTime = %d Hour", m_AutoCleanupTime);
    // 获取单例实例
    CleanupFiles& cleaner = CleanupFiles::GetInstance();

    // 设置消息回调函数
    cleaner.SetMessageCallback([this](const std::string& type, const std::string& message) {
        // 在主线程中处理消息（使用Qt的信号槽确保线程安全）
        QMetaObject::invokeMethod(this, [this, type, message]() {
            this->OnCleanupMessage(QString::fromStdString(type),
                QString::fromStdString(message));
            }, Qt::QueuedConnection);
        });
    // 1. 先停止自动清理一次
    cleaner.SetCleanupTimeoutState(false);  //停止自动清理

    // 2. 设置自动清理参数
    cleaner.SetConfig(
        m_CleanupPath,   // 主目录 - 结果保存位置。从配方读取
        m_IsAutoCleanup,                                  // 启用自动清理
        m_ImagesRetentionDays,                            // 保留30天
        m_AutoCleanupTime                                 // 每天凌晨3点清理
    );

    // 需求3：设置清理的文件类型
    //std::vector<std::string> fileTypes = {
    //    ".png", ".jpg", ".jpeg", ".bmp", ".gif",  // 图片文件
    //    ".log", ".txt", ".tmp", ".bak", ".old",   // 日志和临时文件
    //    ".avi", ".mp4", ".mov", ".wmv",           // 视频文件
    //    ".dat", ".dmp", ".err"                    // 数据文件
    //};

    // 3. 设置文件过滤器
    std::vector<std::string> fileTypes = { ".png" };
    cleaner.SetFileFilters(fileTypes);

    std::ostringstream oss;
    std::string delimiter = ", ";
    for (size_t i = 0; i < fileTypes.size(); ++i) {
        if (i != 0) oss << delimiter;
        oss << fileTypes[i];
    }
    std::string fileTypeStr = oss.str();
    FILE_LOG_INFO("AutoCleanupFileType = {%s}", fileTypeStr);


    // 4. 设置磁盘空间紧急清理（自动检测SetConfigs设置的目录所在分区的剩余空间并清理磁盘）
    cleaner.SetDiskSpaceEmergency(true, EmergencyThreshol, EmergencyTarget); //启用磁盘剩余空间紧急清理，检查阈值5GB，删除到的目标阈值10GB
    FILE_LOG_INFO("EmergencyThreshol = %d GB", EmergencyThreshol);
    FILE_LOG_INFO("EmergencyTarget = %d GB", EmergencyTarget);

    // 获取当前磁盘空间信息并记录
    std::string diskInfo = cleaner.GetDiskSpaceInfo();
    FILE_LOG_INFO("Disk space info: %s", diskInfo.c_str());

    FILE_LOG_INFO("Completed.");
    return true;
}

void MainProcess::InitSettings()
{
    emit Signal_DefectTypeNamesReady(); //传递瑕疵类型名称给表格模式下的表格
}

// 在主线程中处理消息
void MainProcess::OnCleanupMessage(const QString& type, const QString& message)
{
    // 根据消息类型进行处理
    QString displayMessage;
    if (type == "CLEANUP_ERROR")
    {
        FILE_LOG_ERROR("[Cleanup] %s", message.toStdString().c_str());
        displayMessage = tr("Automatic cleanup failed");
    }
    else if (type == "RUN_CLEANUP_BEGIN")
    {
        FILE_LOG_INFO("[Cleanup] %s", message.toStdString().c_str());
        displayMessage = tr("Automatic cleanup started");
    }
    else if (type == "RUN_CLEANUP_OVER")
    {
        FILE_LOG_INFO("[Cleanup] %s", message.toStdString().c_str());
        displayMessage = tr("Automatic cleanup finished");
    }
    else if (type == "DEL_CONUT") {

        QString msg = message;
        QStringList parts = message.split(',');
        int count_num = 0;
        double count_mb = 0;
        std::string strMsg = "0 files, 0 MB";
        if (parts.size() >= 2) {
            // 转换为整数和浮点数
            count_num = parts[0].toInt();        // 第一个部分是整数
            count_mb = parts[1].toDouble();   // 第二个部分是浮点数

            strMsg = parts[0].toStdString() + " files, " + parts[1].toStdString() + " MB";
        }


        FILE_LOG_INFO("[Cleanup] Delect count: %s", strMsg.c_str());

        displayMessage = QString(tr("Cleaned up %1 files, %2 MB")).arg(count_num).arg(count_mb, 0, 'f', 2);
    }
    else if (type == "EMERGENCY_CLEANUP")
    {
        FILE_LOG_INFO("[Cleanup] %s", message.toStdString().c_str());
        displayMessage = tr("Insufficient disk space, emergency cleanup triggered");
    }
    else if (type == "EMERGENCY_CLEANUP_OVER")
    {
        FILE_LOG_INFO("[Cleanup] %s", message.toStdString().c_str());
        //displayMessage = tr("Disk Emergency Cleanup End");
    }
    else
    {
        FILE_LOG_INFO("[Cleanup] %s", message.toStdString().c_str());
    }
    emit Signal_ShowMessage(displayMessage);
}

bool MainProcess::ReadSystemConfig()
{
    FILE_LOG_INFO("ReadSystemConfig: Begin.");
    QSettings settings(
        QString::fromStdString(ConfigPath),         //配置文件路径
        QSettings::IniFormat);

    settings.setIniCodec("UTF-8");

    bool isOK;

    settings.beginGroup("System");
	    CameraBrandIndex = settings.value("CameraBrand").toInt();            //相机品牌。 0=海康，1=华睿
        if (CameraBrandIndex > 1 || CameraBrandIndex < 0)
        {
            CameraBrandIndex = 0;
        }
        CameraCount = settings.value("CameraCount").toInt(&isOK);     //从配置文件读取使用的相机数量。配合config.ini文件camera分组情况使用。
        LightCount = settings.value("LightCount").toInt();
        ModelPath_cu = settings.value("ModelPath_cu").toString().toStdString();
        ModelPath_jing = settings.value("ModelPath_jing").toString().toStdString();

        DefaultRecipeName = settings.value("CurrentRecipe").toString().toStdString();      //当前配方名称(文件名)

        Language = settings.value("Language").toString().toStdString();

        IsDoubleImage = settings.value("IsDoubleImage").toBool();

        IsPseudoColorImage = settings.value("IsPseudoColorImage").toBool();

        QVariant variant = settings.value("IS_READ_MODE");
        if (variant.canConvert<bool>())
        {
            IS_READ_MODE = variant.toInt();
            // 使用布尔值
        }
        else
        {
            // 转换失败处理
            IS_READ_MODE = 0;
        }

        //是否发送检测结果
        SendResults = settings.value("SendResults").toBool();

        //读取日志文件的日志级别
        LogFileLevel = settings.value("LogFileLevel", 2).toInt();

        //读取控制台日志级别
        LogConsoleLevel = settings.value("LogConsoleLevel", 2).toInt();

        IsSaveLargeImage = settings.value("IsSaveLargeImage", false).toInt();    //是否保存合并后未分割的原始大图
    settings.endGroup();

    //配方使用类型设置
    settings.beginGroup("Recipe");
        m_defectTypeUsability[DefectType::TYPE_SCREENPRINTING] = settings.value("IsScreenPrintingUsed", false).toBool();
        m_defectTypeUsability[DefectType::TYPE_CHIPPED_EDGE] = settings.value("IsChippedEdgeUsed", false).toBool();
        m_defectTypeUsability[DefectType::TYPE_PITTING] = settings.value("IsPittingUsed", false).toBool();
        m_defectTypeUsability[DefectType::TYPE_GLASS_CULLET] = settings.value("IsGlassCulletUsed", false).toBool();
        m_defectTypeUsability[DefectType::TYPE_WAVINESS] = settings.value("IsWavinessUsed", false).toBool();
        m_defectTypeUsability[DefectType::TYPE_OTHER] = settings.value("IsOtherUsed", false).toBool();
    settings.endGroup();

    //相机名称
    settings.beginGroup("Camera");
        Camera1 = settings.value("Camera1").toString().toStdString();
        Camera2 = settings.value("Camera2").toString().toStdString();
        Camera3 = settings.value("Camera3").toString().toStdString();
        Camera4 = settings.value("Camera4").toString().toStdString();
        Camera5 = settings.value("Camera5").toString().toStdString();
        Camera6 = settings.value("Camera6").toString().toStdString();
        Camera7 = settings.value("Camera7").toString().toStdString();
        Camera8 = settings.value("Camera8").toString().toStdString();
        Camera9 = settings.value("Camera9").toString().toStdString();
    settings.endGroup();

    settings.beginGroup("Customer");
        CustomerName = settings.value("CustomerName").toString().toStdString();
    settings.endGroup();

    settings.beginGroup("Compute");
        widthScale = settings.value("WidthScale").toDouble();
        heightScale = settings.value("HeightScale").toDouble();
        Pixle2MM_X = settings.value("Pixle2MM_X").toFloat();
        Pixle2MM_Y = settings.value("Pixle2MM_Y").toFloat();

        PedalTimeInterval = settings.value("PedalTimeInterval").toInt();    //踏板间隔时间，国外项目无踏板，暂时无意义。
        if (PedalTimeInterval <= 0)
        {
            PedalTimeInterval = 2;
        }
        GrayReal = settings.value("GrayReal").toInt();  //范围0-255
        if (GrayReal < 0)
        {
            GrayReal = 0;
        }
        if (GrayReal > 255)
        {
            GrayReal = 255;
        }
        heightSingle = settings.value("heightSingle").toInt();
        glassThreshold_val = settings.value("glassThreshold_val").toInt();
        range_val = settings.value("range_val").toInt();

        IsAutoShowImage = settings.value("IsAutoShowImage").toBool();

        glassThreshold_display = settings.value("glassThreshold_display", 20).toInt();
    settings.endGroup();

    //图像自动清理设置
    settings.beginGroup("AutoCleanup");
        IsAutoCleanup = settings.value("IsAutoCleanup").toBool();                   //是否自动清理历史图像
        ImagesRetentionDays = settings.value("ImagesRetentionDays").toInt();        //历史图像保留天数
        AutoCleanupTime = settings.value("AutoCleanupTime").toInt();                //每日自动清理的时段
        EmergencyThreshol = settings.value("EmergencyThreshol").toInt();            //紧急清理启动的检测阈值 GB
        EmergencyTarget = settings.value("EmergencyTarget").toInt();                //紧急清理启动的清理目标(剩余磁盘空间) GB
    settings.endGroup();

    //图像参数设置
    settings.beginGroup("ImageParameter");
        ExposureTime =settings.value("ExposureTime").toInt();                       //曝光时间
        ImageHeight = settings.value("ImageHeight").toInt();                        //图像高度
        DelayStopTime = settings.value("DelayStopTime").toDouble();                 //延时停止采集的时间
        DelayStartTime = settings.value("DelayStartTime", 0).toDouble();            //延时开始采集的时间
		RotationDirectionFlag = settings.value("RotationDirectionFlag").toInt();    //图像旋转方向标志。 0-顺时针旋转90度，1-逆时针旋转90度
		UsedIrregularImage = settings.value("UsedIrregularImage").toBool();         //隐藏功能：虚拟图生成方法是否使用不规则玻璃图像生成方法
    settings.endGroup();

    settings.beginGroup("ScreenPrinting");
        Light0GlassGrayValue = settings.value("Light0GlassGrayValue").toInt();
        Light0SilkGrayValue = settings.value("Light0SilkGrayValue").toInt();
        SilkAreaThreshold = settings.value("SilkAreaThreshold").toFloat();
    settings.endGroup();

    //瑕疵类型和等级显示设置
    settings.beginGroup("LevelDisplay");
        DISPLAY_NORMAL          = settings.value("DISPLAY_NORMAL").toBool();
        DISPLAY_MINOR           = settings.value("DISPLAY_MINOR").toBool();
        DISPLAY_MEDIUM          = settings.value("DISPLAY_MEDIUM").toBool();
        DISPLAY_SERIOUS         = settings.value("DISPLAY_SERIOUS").toBool();
        DISPLAY_ABNORMAL        = settings.value("DISPLAY_ABNORMAL").toBool();
        DISPLAY_AREAERROR        = settings.value("DISPLAY_AREAERROR").toBool();
    settings.endGroup();

    settings.beginGroup("TypeDisplay");
        m_defectTypeVisibility[DefectType::TYPE_POORCOATING] = settings.value("DISPLAY_POORCOATING").toBool();
        m_defectTypeVisibility[DefectType::TYPE_SCRATCH] = settings.value("DISPLAY_SCRATCH").toBool();
        m_defectTypeVisibility[DefectType::TYPE_CALCULUS] = settings.value("DISPLAY_CALCULUS").toBool();
        m_defectTypeVisibility[DefectType::TYPE_BUBBLE] = settings.value("DISPLAY_BUBBLE").toBool();
        m_defectTypeVisibility[DefectType::TYPE_TRADEMARK] = settings.value("DISPLAY_TRADEMARK").toBool();
        m_defectTypeVisibility[DefectType::TYPE_WATERSTAIN] = settings.value("DISPLAY_WATERSTAIN").toBool();
        m_defectTypeVisibility[DefectType::TYPE_SMUDGE] = settings.value("DISPLAY_SMUDGE").toBool();

        if (!m_defectTypeUsability[DefectType::TYPE_SCREENPRINTING])
        {
            m_defectTypeVisibility[DefectType::TYPE_SCREENPRINTING] = false;
        }
        else
        {
            m_defectTypeVisibility[DefectType::TYPE_SCREENPRINTING] = settings.value("DISPLAY_SCREENPRINTING").toBool();
        }
        if (!m_defectTypeUsability[DefectType::TYPE_CHIPPED_EDGE])
        {
            m_defectTypeVisibility[DefectType::TYPE_CHIPPED_EDGE] = false;
        }
        else
        {
            m_defectTypeVisibility[DefectType::TYPE_CHIPPED_EDGE] = settings.value("DISPLAY_CHIPPED_EDGE").toBool();
        }
        if (!m_defectTypeUsability[DefectType::TYPE_PITTING])
        {
            m_defectTypeVisibility[DefectType::TYPE_PITTING] = false;
        }
        else
        {
            m_defectTypeVisibility[DefectType::TYPE_PITTING] = settings.value("DISPLAY_PITTING").toBool();
        }
        if (!m_defectTypeUsability[DefectType::TYPE_GLASS_CULLET])
        {
            m_defectTypeVisibility[DefectType::TYPE_GLASS_CULLET] = false;
        }
        else
        {
            m_defectTypeVisibility[DefectType::TYPE_GLASS_CULLET] = settings.value("DISPLAY_GLASS_CULLET").toBool();
        }
        if (!m_defectTypeUsability[DefectType::TYPE_WAVINESS])
        {
            m_defectTypeVisibility[DefectType::TYPE_WAVINESS] = false;
        }
        else
        {
            m_defectTypeVisibility[DefectType::TYPE_WAVINESS] = settings.value("DISPLAY_WAVINESS").toBool();
        }
        if (!m_defectTypeUsability[DefectType::TYPE_OTHER])
        {
            m_defectTypeVisibility[DefectType::TYPE_OTHER] = false;
        }
        else
        {
            m_defectTypeVisibility[DefectType::TYPE_OTHER] = settings.value("DISPLAY_OTHER").toBool();
        }
    settings.endGroup();

    std::vector<float> temp;
    temp.push_back(CameraCount);
    temp.push_back(Pixle2MM_X);
    temp.push_back(Pixle2MM_Y);

    camerainfo.push_back(temp);


    //防护动作
    if (!isOK || CameraCount == 0)
    {
        CameraCount = 1;
    }
    for (int i = 1; i < CameraCount + 1; i++)
    {
        std::vector<float> temp;
        std::string name = "Camera" + std::to_string(i);
        settings.beginGroup(QString(name.c_str()));
        temp.push_back(settings.value("pixelstart").toFloat());
        temp.push_back(settings.value("pixelend").toFloat());
        temp.push_back(settings.value("ditancemm").toFloat());
        temp.push_back(settings.value("yoffsetpxile").toFloat());
        settings.endGroup();
        camerainfo.push_back(temp);
    }

    //if (QFile::exists(QString::fromStdString(model1Name)))
    //{
    //    FileLogPrintf("model 1 exist");
    //    //std::cout << "model1 exist" << std::endl;
    //}

    //if (QFile::exists(QString::fromStdString(model2Name)))
    //{
    //    FileLogPrintf("model 2 exist");
    //    //std::cout << "model 2 exist" << std::endl;
    //}
    FILE_LOG_INFO("ReadSystemConfig: Completed.");
    return true;

}

bool MainProcess::Recipe_ReadConfig()
{
    FILE_LOG_INFO("ReadRecipeConfig: Begin.");
    //**********必选瑕疵类型均启用**********
    m_defectTypeUsability[DefectType::TYPE_POORCOATING] = true;
    m_defectTypeUsability[DefectType::TYPE_SCRATCH] = true;
    m_defectTypeUsability[DefectType::TYPE_CALCULUS] = true;
    m_defectTypeUsability[DefectType::TYPE_BUBBLE] = true;
    m_defectTypeUsability[DefectType::TYPE_TRADEMARK] = true;
    m_defectTypeUsability[DefectType::TYPE_WATERSTAIN] = true;
    m_defectTypeUsability[DefectType::TYPE_SMUDGE] = true;
    //*****************************************

    if (DefaultRecipeName.empty()) {
        qWarning() << "DefaultRecipeName 为空";
        FILE_LOG_ERROR("ReadSystemConfig error: Default recipe name error. DefaultRecipeName.size() = %s", DefaultRecipeName.c_str());
        return false;
    }
    QString configPath = "./Recipes/" + QString::fromStdString(DefaultRecipeName) + ".json"; // 替换为你的实际路径
    QFileInfo fileInfo(configPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        qWarning() << "配置文件不存在:" << configPath;
        FILE_LOG_ERROR("ReadSystemConfig error: Default recipe not found. %s", configPath.toStdString().c_str());

        return false;
    }
    
    RecipeInfo = RecipeConfig::Config::load(configPath);
    FILE_LOG_INFO("ReadRecipeConfig: Load Completed.");

    if (RecipeInfo.defects.isEmpty())
    {
        FILE_LOG_ERROR("Load Recipe Parameter Failed.");
        return false;
    }
    FILE_LOG_INFO("=====TYPE=========DisplayName==========confidence===unit====minor====moderate===major=====");

    //检查并处理没有使用的瑕疵类型
    QSet<int> typesToRemove;
    for (const auto& defect : RecipeInfo.defects)
    {
        //设置可选瑕疵类型是否启用
        DefectType defectType = static_cast<DefectType>(defect.id);

        if (!m_defectTypeUsability[defectType])
        {
            typesToRemove.insert(defect.id);
        }
    }
    for (const int& type : typesToRemove)
    {
        RecipeInfo.removeDefect(type);
    }

    int defectCount = RecipeInfo.defects.size();
    std::cout<<"defectCount="<<defectCount<<std::endl;
    m_DefectNamesMutex.lock();
    //m_DefectTypeNames.clear();
    m_defectConfidence.clear();
    //m_DefectTypeNames.resize(defectCount);
    m_defectConfidence.resize(defectCount);

    //
    int id = 0;
    for (auto& defect : RecipeInfo.defects)
    {
        //--同步是否显示标志到默认配方中，用于config界面使用
        DefectType defectType = static_cast<DefectType>(defect.id);
        defect.isDisplayed = m_defectTypeVisibility[defectType];

        //----

        /*int id = defect.id;*/

        auto type = PadOrTruncate(defect.type, 15);   // type 固定宽度
        auto name = PadOrTruncate(defect.name, 20);   // name 固定宽度
        auto unit = PadOrTruncate(defect.thresholdUnit, 3);
        FILE_LOG_INFO("%s: %s %s      %s %s %s %s",
            type.c_str(),
            name.c_str(),
            FormatFixedWidthFloat(defect.confidence, 4,4).c_str(),
            unit.c_str(),
            FormatFixedWidthFloat(defect.minorThreshold,4,4).c_str(),
            FormatFixedWidthFloat(defect.moderateThreshold, 4,4).c_str(),
            FormatFixedWidthFloat(defect.majorThreshold, 4,4).c_str()
        );

        //添加瑕疵名称到数组，用于动态显示
        //m_DefectTypeNames[id]=defect.name;
        m_defectConfidence[id] = defect.confidence;
        id++;
    }
    m_DefectNamesMutex.unlock();
    //FILE_LOG_INFO("----------------------------------------------------------------------------------------------");
    FILE_LOG_INFO("-----TYPE------------DisplayName--------------value----");
    FILE_LOG_INFO("MERGE_DISTANCE     : %s    %s"
        , PadOrTruncate(RecipeInfo.global.mergeDistanceName, 20).c_str()   // type 固定宽度
        , QString::number(RecipeInfo.global.mergeDistance, 'f', 4).toStdString().c_str()
    );

    //20251118: 取消映射比例的使用
    //FileLogPrintf("MAPPING_SCALE      : %s    %s"
    //    , PadOrTruncate(RecipeInfo.global.mappingScaleName, 20).c_str()
    //    , QString::number(RecipeInfo.global.mappingScale, 'f', 4).toStdString().c_str()
    //);
    FILE_LOG_INFO("DATA_SAVE_PATH     : %s    %s"
        //, RecipeInfo.global.dataSavePathName.toUtf8().toStdString().c_str()
        , PadOrTruncate(RecipeInfo.global.dataSavePathName, 20).c_str()
        , RecipeInfo.global.dataSavePath.toStdString().c_str()
    );
    FILE_LOG_INFO("================================================================================================");
    //emit Signal_DefectTypeNamesReady();   //改为在单独的Recipe_SetParameter函数中进行传递
    FILE_LOG_INFO("ReadRecipeConfig: Completed.");
}

//更新配置信息到ini文件
void MainProcess::WriteSystemConfig()
{
    FILE_LOG_INFO("WriteSystemConfig: Begin.");
    if (!QFile::exists(QString::fromStdString(ConfigPath)))
    {
        QFile file(QString::fromStdString(ConfigPath));
        QDir dir;
        QString dirPath = QFileInfo(QString::fromStdString(ConfigPath)).absolutePath();

        // 创建目录
        if (!dir.exists(dirPath))
        {
            if (!dir.mkpath(dirPath))
            {
                FILE_LOG_ERROR("Failed to create directory : %s", dirPath.toStdString().c_str());
                //qDebug() << "Failed to create directory:" << dirPath;
                return;
            }
        }
    }
    QSettings settings(QString::fromStdString(ConfigPath), QSettings::IniFormat);
    settings.setIniCodec("UTF-8");  // 必须在任何读写操作前设置！直接用字符串 "UTF-8"
    //settings.setIniCodec(QTextCodec::codecForName("utf-8"));

    settings.beginGroup("System");
        settings.setValue("CameraBrand", CameraBrandIndex);                                                             //隐藏功能：相机品牌。 0=海康，1=华睿
        settings.setValue("CameraCount", CameraCount);                                                                  //界面功能：配合config.ini文件camera分组情况使用。
        settings.setValue("LightCount", LightCount);                                                                    //界面功能：是否三通道光源相机
		settings.setValue("ModelPath_cu", QString::fromStdString(ModelPath_cu));                                        //隐藏功能：算法模型路径。
		settings.setValue("ModelPath_jing", QString::fromStdString(ModelPath_jing));                                    //隐藏功能：算法模型路径。

        settings.setValue("CurrentRecipe", QString::fromStdString(DefaultRecipeName));                                  //界面功能：当前配方名称
		settings.setValue("Language", QString::fromStdString(Language));                                                //界面功能：界面语言
        settings.setValue("IsDoubleImage", IsDoubleImage);                                                              //隐藏功能：实时模式是否为双视图模式
        settings.setValue("IsPseudoColorImage", IsPseudoColorImage);                                                    //隐藏功能：实时图像为双视图模式下，左图是否显示伪彩图

		settings.setValue("IS_READ_MODE", IS_READ_MODE);                                                                //隐藏功能：是否为读图模式，
                                                                                                                        //              0=实时模式；
                                                                                                                        //              1=读多个相机的多个帧图，并使用算法计算；
                                                                                                                        //              2=读图：读取结果跳过算法。

        settings.setValue("SendResults", SendResults);                                                                  //界面功能：是否发送检测结果给控制卡

        settings.setValue("LogFileLevel", LogFileLevel);                                                                //隐藏功能：日志文件的日志级别
        settings.setValue("LogConsoleLevel", LogConsoleLevel);                                                          //隐藏功能：控制台模式下，控制台显示日志的级别

        settings.setValue("IsSaveLargeImage", IsSaveLargeImage);                                                        //隐藏功能：是否保存合并后未分割的原始大图
    settings.endGroup();

    //配方使用类型设置
    settings.beginGroup("Recipe");
	    settings.setValue("IsScreenPrintingUsed", m_defectTypeUsability[DefectType::TYPE_SCREENPRINTING]);              //隐藏功能：7  是否启用丝印瑕疵类型
        settings.setValue("IsChippedEdgeUsed", m_defectTypeUsability[DefectType::TYPE_CHIPPED_EDGE]);                   //隐藏功能：8  是否启用崩边瑕疵类型
        settings.setValue("IsPittingUsed", m_defectTypeUsability[DefectType::TYPE_PITTING]);                            //隐藏功能：9  是否启用麻点瑕疵类型
        settings.setValue("IsGlassCulletUsed", m_defectTypeUsability[DefectType::TYPE_GLASS_CULLET]);                   //隐藏功能：10 是否启用玻渣瑕疵类型
        settings.setValue("IsWavinessUsed", m_defectTypeUsability[DefectType::TYPE_WAVINESS]);                          //隐藏功能：11 是否启用云朵瑕疵类型
		settings.setValue("IsOtherUsed", m_defectTypeUsability[DefectType::TYPE_OTHER]);                                //隐藏功能：12 是否启用其他瑕疵类型
    settings.endGroup();


    settings.beginGroup("Camera");
        settings.setValue("Camera1", QString::fromStdString(Camera1));
        settings.setValue("Camera2", QString::fromStdString(Camera2));
        settings.setValue("Camera3", QString::fromStdString(Camera3));
        settings.setValue("Camera4", QString::fromStdString(Camera4));
        settings.setValue("Camera5", QString::fromStdString(Camera5));
        settings.setValue("Camera6", QString::fromStdString(Camera6));
        settings.setValue("Camera7", QString::fromStdString(Camera7));
        settings.setValue("Camera8", QString::fromStdString(Camera8));
        settings.setValue("Camera9", QString::fromStdString(Camera9));
    settings.endGroup();


    settings.beginGroup("Customer");
        settings.setValue("CustomerName", QString::fromStdString(CustomerName));
    settings.endGroup();


    settings.beginGroup("Compute");
        settings.setValue("WidthScale", widthScale);
        settings.setValue("HeightScale", heightScale);
        settings.setValue("Pixle2MM_X", QString::number(Pixle2MM_X));
        settings.setValue("Pixle2MM_Y", QString::number(Pixle2MM_Y));
        settings.setValue("PedalTimeInterval", PedalTimeInterval);
        settings.setValue("GrayReal", GrayReal);    //范围0-255
        settings.setValue("heightSingle", heightSingle);
        settings.setValue("glassThreshold_val", glassThreshold_val);    //算法计算用的阈值
        settings.setValue("range_val", range_val);

		settings.setValue("IsAutoShowImage", IsAutoShowImage);                                                          //隐藏功能：算法计算完成后，是否自动显示图像结果(false时，需配合脚踏板活其他硬件来控制图像显示)
        settings.setValue("glassThreshold_display", glassThreshold_display);                                            //隐藏功能：显示玻璃用于微调部分玻璃区域显示不完整。
        
    settings.endGroup();


    //图像自动清理设置
    settings.beginGroup("AutoCleanup");
        settings.setValue("IsAutoCleanup", IsAutoCleanup);                                                              //是否自动清理历史图像
        settings.setValue("ImagesRetentionDays", ImagesRetentionDays);                                                  //历史图像保留天数
        settings.setValue("AutoCleanupTime", AutoCleanupTime);                                                          //每日自动清理的时段
        settings.setValue("EmergencyThreshol", EmergencyThreshol);                                                      //隐藏功能：紧急清理启动的检测阈值 GB
        settings.setValue("EmergencyTarget", EmergencyTarget);                                                          //隐藏功能：紧急清理启动的清理目标(剩余磁盘空间) GB
    settings.endGroup();

    //图像参数设置
    settings.beginGroup("ImageParameter");
        settings.setValue("ExposureTime", ExposureTime);                                                                //曝光时间
        settings.setValue("ImageHeight", ImageHeight);                                                                  //图像高度
        settings.setValue("DelayStopTime", double(DelayStopTime));                                                      //延时停止采集的时间
		settings.setValue("DelayStartTime",double(DelayStartTime));                                                     //隐藏功能：延时开始采集的时间
		settings.setValue("RotationDirectionFlag", RotationDirectionFlag);                                              //图像旋转方向标志。 0-顺时针旋转90度，1-逆时针旋转90度
		settings.setValue("UsedIrregularImage", UsedIrregularImage);                                                    //隐藏功能：虚拟图生成方法是否使用不规则玻璃图像生成方法
    settings.endGroup();

    settings.beginGroup("ScreenPrinting");
        settings.setValue("Light0GlassGrayValue", Light0GlassGrayValue);
        settings.setValue("Light0SilkGrayValue", Light0SilkGrayValue);
        settings.setValue("SilkAreaThreshold", double(SilkAreaThreshold));
    settings.endGroup();


    //瑕疵类型和等级显示设置
    settings.beginGroup("LevelDisplay");
        settings.setValue("DISPLAY_NORMAL",             DISPLAY_NORMAL           );
        settings.setValue("DISPLAY_MINOR",              DISPLAY_MINOR            );
        settings.setValue("DISPLAY_MEDIUM",             DISPLAY_MEDIUM           );
        settings.setValue("DISPLAY_SERIOUS",            DISPLAY_SERIOUS          );
        settings.setValue("DISPLAY_ABNORMAL",           DISPLAY_ABNORMAL         );
        settings.setValue("DISPLAY_AREAERROR",          DISPLAY_AREAERROR        );
    settings.endGroup();

    settings.beginGroup("TypeDisplay");
        settings.setValue("DISPLAY_POORCOATING",        m_defectTypeVisibility[DefectType::TYPE_POORCOATING]     );
        settings.setValue("DISPLAY_SCRATCH",            m_defectTypeVisibility[DefectType::TYPE_SCRATCH]         );
        settings.setValue("DISPLAY_CALCULUS",           m_defectTypeVisibility[DefectType::TYPE_CALCULUS]        );
        settings.setValue("DISPLAY_BUBBLE",             m_defectTypeVisibility[DefectType::TYPE_BUBBLE]          );
        settings.setValue("DISPLAY_TRADEMARK",          m_defectTypeVisibility[DefectType::TYPE_TRADEMARK]       );
        settings.setValue("DISPLAY_WATERSTAIN",         m_defectTypeVisibility[DefectType::TYPE_WATERSTAIN]      );
        settings.setValue("DISPLAY_SMUDGE",             m_defectTypeVisibility[DefectType::TYPE_SMUDGE]          );
        settings.setValue("DISPLAY_SCREENPRINTING",     m_defectTypeVisibility[DefectType::TYPE_SCREENPRINTING]  );
        settings.setValue("DISPLAY_CHIPPED_EDGE",       m_defectTypeVisibility[DefectType::TYPE_CHIPPED_EDGE]    );
        settings.setValue("DISPLAY_PITTING",            m_defectTypeVisibility[DefectType::TYPE_PITTING]         );
        settings.setValue("DISPLAY_GLASS_CULLET",       m_defectTypeVisibility[DefectType::TYPE_GLASS_CULLET]    );
        settings.setValue("DISPLAY_WAVINESS",           m_defectTypeVisibility[DefectType::TYPE_WAVINESS]        );
        settings.setValue("DISPLAY_OTHER",              m_defectTypeVisibility[DefectType::TYPE_OTHER]           );
    settings.endGroup();


    for (int i = 1; i < camerainfo.size(); i++)
    {
        //std::cout << "i = " << i << std::endl;
        std::vector<float> temp = camerainfo[i];
        std::string name = "Camera" + std::to_string(i);
        settings.beginGroup(QString(name.c_str()));
            settings.setValue("pixelstart",QString::number(temp[0]));
            settings.setValue("pixelend", QString::number(temp[1]));
            settings.setValue("ditancemm", QString::number(temp[2]));
            settings.setValue("yoffsetpxile", QString::number(temp[3]));
        settings.endGroup();
    }
    FILE_LOG_INFO("WriteSystemConfig: Completed.");
}

void MainProcess::Recipe_UpdateAtComputeAfter()
{
    //if (m_isComputeRunning)  //计算线程计算时，不执行修改。
    //{
    //    FILE_LOG_INFO("computeThread is running. RecipeChanged waitting to run.");
    //    return;
    //}
    FILE_LOG_INFO("Recipe Update Begin.");
    Recipe_SetParameter();
    //m_isRecipeUpdate = false;   //完成更新配方参数
    FILE_LOG_INFO("Recipe Update Completed.");
}

void MainProcess::Recipe_SetParameter()
{
    FILE_LOG_INFO("Recipe: Update DefaultRecipe to compute begin.");

    AlgorithmProcess::GetInstance().SetRecipeParameterToCompute();  //配方参数传递给计算线程算法
    FILE_LOG_INFO("Recipe: Update DefaultRecipe to compute Completed.");
    if (m_isInitRecipeOver)
    {
        emit Signal_DefectTypeNamesReady(); //传递瑕疵类型名称给表格模式下的表格
    }

}

//bool MainProcess::InitDataBaseAndHistoryRecord()
//{
//    FILE_LOG_INFO("InitHistoryRecord Begin");
//
//
//    // 打印可用的数据库驱动
//    qDebug() << "Available drivers:";
//    QStringList drivers = QSqlDatabase::drivers();
//    for (const QString& driver : drivers) {
//        qDebug() << driver;
//    }
//
//    // 检查SQLite驱动是否可用
//    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
//        FILE_LOG_ERROR("SQLite driver not available!");
//        return false;
//    }
//
//    if (m_db.isValid()) {
//        QString connectionName = m_db.connectionName();
//
//        if (m_db.isOpen()) {
//            m_db.close();
//        }
//
//        QSqlDatabase::removeDatabase(connectionName);
//    }
//    m_db = QSqlDatabase::addDatabase("QSQLITE");
//
//    QString dbName = QString::fromStdString(DataBasePath);
//
//    m_db.setDatabaseName(dbName);
//    if (!m_db.open())
//    {
//        FILE_LOG_ERROR("Open database failed!");
//        //qDebug() << "Open database failed!" << m_db.lastError();
//        return false;
//    }
//
//    /* ImageVIew模式下，设置历史记录表格 */
//    m_modelHistory = new CustomTableModel(this, m_db);
//    m_modelHistory->setTable("HistoryView");
//    m_modelHistory->setSort(0, Qt::DescendingOrder);    //增加的排序
//
//    /* TableVIew模式下，设置历史记录表格 */
//    m_modelReport = new CustomTableModel(this, m_db);
//    m_modelReport->setTable("HistoryView");
//    m_modelReport->setSort(0, Qt::DescendingOrder);    //增加的排序
//
//    QString beginTimeStr = QDateTime(QDate::currentDate(), QTime(0, 0, 0)).toString("yyyyMMddhhmmss");
//    //QString beginTimeStr = QDateTime(QDate(2025,07,01), QTime(0, 0, 0)).toString("yyyyMMddhhmmss");
//    QString endTimeStr = QDateTime(QDate::currentDate(), QTime(23, 59, 59)).toString("yyyyMMddhhmmss");
//    QString filter = QString("dateTime >= '%1' and dateTime <= '%2'").arg(beginTimeStr).arg(endTimeStr);
//
//    m_modelHistory->setFilter(filter);
//    m_modelHistory->select();
//
//    m_modelReport->setFilter(filter);
//    m_modelReport->select();
//
//    FILE_LOG_INFO("InitHistoryRecord End");
//    return true;
//}

bool MainProcess::InitDataBaseAndHistoryRecord()
{
    FILE_LOG_INFO("InitHistoryRecord Begin");


    // 打印可用的数据库驱动
    qDebug() << "Available drivers:";
    QStringList drivers = QSqlDatabase::drivers();
    for (const QString& driver : drivers) {
        qDebug() << driver;
    }

    // 检查SQLite驱动是否可用
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        FILE_LOG_ERROR("SQLite driver not available!");
        return false;
    }
    m_dbName = QString::fromStdString(DataBasePath);

    sql_refreshWriteConnection(m_dbName);

    sql_refreshReadConnection(m_dbName);
    /********************************************************/
#ifdef SQL_QUERY

    m_modelChartQry = new CustomSqlQueryModel(this);
    m_modelReportQry = new CustomSqlQueryModel(this);
    m_modelHistoryQry = new CustomSqlQueryModel(this);

#else
    m_modelHistory = new CustomTableModel(this, m_db_read);
    m_modelHistory->setTable("HistoryView");

    /* TableVIew模式下，设置历史记录表格 */
    m_modelReport = new CustomTableModel(this, m_db_read);
    m_modelReport->setTable("HistoryView");
#endif

    QString beginTimeStr = QDateTime(QDate::currentDate(), QTime(0, 0, 0)).toString("yyyyMMddhhmmss");
    //QString beginTimeStr = QDateTime(QDate(2025,07,01), QTime(0, 0, 0)).toString("yyyyMMddhhmmss");
    QString endTimeStr = QDateTime(QDate::currentDate(), QTime(23, 59, 59)).toString("yyyyMMddhhmmss");
    QString filter = QString("dateTime >= '%1' and dateTime <= '%2'").arg(beginTimeStr).arg(endTimeStr);

    sql_refreshHistoryRecord(filter);
    sql_refreshReportRecord(filter);

    FILE_LOG_INFO("InitHistoryRecord End");
    return true;
}

// 静态工具函数，任何线程均可安全调用
QSqlDatabase MainProcess::createWriteConnectionForCurrentThread()
{
    QString connName = QString("write_%1").arg(quintptr(QThread::currentThreadId()));
    if (QSqlDatabase::contains(connName))
        QSqlDatabase::removeDatabase(connName);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(QString::fromStdString(DataBasePath));
    if (!db.open())
        return QSqlDatabase();

    QSqlQuery query(db);
    query.exec("PRAGMA journal_mode = WAL;"
        "PRAGMA synchronous = NORMAL;"
        "PRAGMA mmap_size = 268435456;"
        "PRAGMA cache_size = -10000;");
    return db;
}

bool MainProcess::sql_refreshWriteConnection(const QString& dbName)
{
    // 关闭并重新打开只写连接
    QString connectionName;

    // 1. 先获取连接信息并关闭旧连接
    if (m_db.isValid()) {
        connectionName = m_db.connectionName();

        if (m_db.isOpen()) {
            m_db.close();
        }

        // 2. 移除旧的数据库连接
        QSqlDatabase::removeDatabase(connectionName);
    }
    else {
        // 如果连接无效，创建新的连接名称
        connectionName = "write_connection";
    }

    // 3. 创建新的数据库连接
    m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_db.setDatabaseName(dbName);

    // 4. 打开数据库
    if (!m_db.open()) {
        FILE_LOG_ERROR("Open database failed!");
        return false;
    }

    // 5. 设置PRAGMA参数
    QSqlQuery writeQuery(m_db);

    // 写入连接特有的设置
    // writeQuery.exec("PRAGMA query_only = 0;"); // 默认就是可写
    writeQuery.exec("PRAGMA journal_mode = WAL;");
    writeQuery.exec("PRAGMA synchronous = NORMAL;");
    // writeQuery.exec("PRAGMA temp_store = MEMORY;");
    writeQuery.exec("PRAGMA mmap_size = 268435456;");
    writeQuery.exec("PRAGMA cache_size = -10000;");
    writeQuery.exec("PRAGMA optimize");

    return true;
}

bool MainProcess::sql_refreshReadConnection(const QString& dbName)
{
    // 关闭并重新打开只读连接
    QString connectionName;

    // 1. 先获取连接信息并关闭旧连接
    if (m_db_read.isValid()) {
        connectionName = m_db_read.connectionName();

        if (m_db_read.isOpen()) {
            m_db_read.close();
        }

        // 2. 移除旧的数据库连接
        QSqlDatabase::removeDatabase(connectionName);
    }
    else {
        // 如果连接无效，创建新的连接名称
        connectionName = "read_connection";
    }

    // 3. 创建新的数据库连接
    m_db_read = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_db_read.setDatabaseName(dbName);

    // 4. 打开数据库
    if (!m_db_read.open()) {
        FILE_LOG_ERROR("Open database failed!");
        return false;
    }

    // 5. 设置PRAGMA参数
    QSqlQuery readQuery(m_db_read);

    // 只读连接特有的设置
    readQuery.exec("PRAGMA query_only = 1;");
    readQuery.exec("PRAGMA journal_mode = WAL;");
    readQuery.exec("PRAGMA synchronous = NORMAL;");
    // readQuery.exec("PRAGMA temp_store = MEMORY;");
    readQuery.exec("PRAGMA mmap_size = 268435456;");
    readQuery.exec("PRAGMA cache_size = -10000;");

    readQuery.exec("PRAGMA read_uncommitted = 0;");   // 保证读一致性
    readQuery.exec("PRAGMA busy_timeout = 5000;");    // 避免锁等待超时
    readQuery.exec("PRAGMA wal_autocheckpoint = 0;"); // 读连接绝不自动检查点

    readQuery.exec("PRAGMA optimize");

    return true;
}

void MainProcess::sql_refreshHistoryRecord(const QString& filter)
{
#ifdef SQL_QUERY
    if (m_modelHistoryQry == nullptr)
    {
        return;
    }
    QString sql = QString(
        "SELECT * "
        "FROM HistoryView "
        "WHERE " + filter + " "
        "ORDER BY dateTime DESC "
        //"LIMIT 100"
    );
    if (m_db_read.isOpen()) {
        m_db_read.commit();   // 如果当前有事务，会提交；如果没有事务，无操作
    }
    m_modelHistoryQry->setQuery(sql, m_db_read);
    // ---------- 3. 加载全部结果行 ----------
    while (m_modelHistoryQry->canFetchMore()) {
        m_modelHistoryQry->fetchMore();
    }
    // 检查查询是否成功
    if (m_modelHistoryQry->lastError().isValid())
    {
        FILE_LOG_ERROR("Sql Query error: %s\nLast query: %s"
            , m_modelHistoryQry->lastError().text().toStdString()
            , sql.toStdString()
        );
    }
    else {
        qDebug() << "Query executed successfully";
        qDebug() << "Row count:" << m_modelHistoryQry->rowCount();
        qDebug() << "Column count:" << m_modelHistoryQry->columnCount();
    }
   // std::cout << "m_modelHistoryQry->rowCount() = " << m_modelHistoryQry->rowCount() << ", colCount() = " << m_modelHistoryQry->columnCount() << std::endl;
    std::cout << "SQL:: " << sql.toStdString() << std::endl;


#else
    if (m_modelHistory == nullptr)
    {
        return;
    }

    m_modelHistory->setFilter(filter);
    m_modelHistory->setSort(0, Qt::DescendingOrder);
    m_modelHistory->select();  // 执行查询
#endif

    emit Signal_FilterHistoryAfter();
}

void MainProcess::sql_refreshReportRecord(const QString& filter)
{
#ifdef SQL_QUERY
    if (m_modelReportQry == nullptr)
    {
        return;
    }
    QString sql = QString(
        "SELECT * "
        "FROM HistoryView "
        "WHERE " + filter + " "
        "ORDER BY dateTime DESC "
        //"LIMIT 100"
    );
    if (m_db_read.isOpen()) {
        m_db_read.commit();   // 如果当前有事务，会提交；如果没有事务，无操作
    }

    m_modelReportQry->setQuery(sql, m_db_read);
    while (m_modelReportQry->canFetchMore()) {
        m_modelReportQry->fetchMore();
    }
    // 检查查询是否成功
    if (m_modelReportQry->lastError().isValid())
    {
        FILE_LOG_ERROR("Sql Query error: %s\nLast query: %s"
            , m_modelReportQry->lastError().text().toStdString()
            , sql.toStdString()
        );
    }
    else {
        qDebug() << "Query executed successfully";
        qDebug() << "Row count:" << m_modelReportQry->rowCount();
        qDebug() << "Column count:" << m_modelReportQry->columnCount();
    }
    //std::cout << "m_modelHistoryQry->rowCount() = " << m_modelHistoryQry->rowCount() << ", colCount() = " << m_modelHistoryQry->columnCount() << std::endl;
    //std::cout << "SQL:: " << sql.toStdString() << std::endl;


#else
    if (m_modelReport == nullptr)
    {
        return;
    }

    m_modelReport->setFilter(filter);
    m_modelReport->setSort(0, Qt::DescendingOrder);
    m_modelReport->select();  // 执行查询
#endif

    emit Signal_FilterReportAfter();
}

CustomTableModel* MainProcess::modelHistory() const
{
    return m_modelHistory;
}

CustomTableModel* MainProcess::modelReport() const
{
    return m_modelReport;
}

CustomSqlQueryModel* MainProcess::modelHistoryQry() const
{
    return m_modelHistoryQry;
}


CustomSqlQueryModel* MainProcess::modelReportQry() const
{
    return m_modelReportQry;
}

void MainProcess::InitConnect()
{
    connect(this, &MainProcess::Signal_Pedal, this, &MainProcess::Slot_Pedal);  //接受踏板状态信号
    connect(this, &MainProcess::Signal_SetVsionGrabbingStatus, this, &MainProcess::Slot_SetVsionGrabbingStatus);
    connect(this, &MainProcess::Slignal_FilterHistoryData, this, &MainProcess::Slot_FilterHistoryData);    //刷新历史数据到图像视图右侧和表格视图
    connect(this, &MainProcess::Signal_ShowGlassIn, this, &MainProcess::Slot_ShowGlassIn);
    connect(this, &MainProcess::Signal_ShowGlassOut, this, &MainProcess::Slot_ShowGlassOut);
    connect(this, &MainProcess::Signal_SaveData2DB, this, &MainProcess::Slot_SaveData2DB);
}
//开始视觉流程(启动拍照)
void MainProcess::Sensor_StartVisionGrabbing()
{
    //Vision_StartProcess();
    emit Signal_SetVsionGrabbingStatus(true);
}

//停止视觉流程(停止拍照)
void MainProcess::Sensor_StopVisionGrabbing()
{
    //Vision_StopProcess();
    emit Signal_SetVsionGrabbingStatus(false);
}

void MainProcess::Sensor_ShowGlassIn()
{
    emit Signal_ShowGlassIn();
}

void MainProcess::Sensor_ShowGlassOut()
{
    emit Signal_ShowGlassOut();
}

//踩下脚踏板
void MainProcess::Sensor_StepOnPedal()
{
    emit Signal_Pedal(true);
}

void MainProcess::Slot_SetVsionGrabbingStatus(bool state)
{
    if (state)
    {
        Vision_StartProcess();
        if (LOOPTEST)
        {
            QTimer::singleShot(7000, []() {
                qDebug() << "5秒后执行！";
                emit MainProcess::GetInstance().Signal_SetVsionGrabbingStatus(false);
                });
        }
    }
    else
    {
        Vision_StopProcess();
    }
}

//松开脚踏板
void MainProcess::Sensor_ReleasePedal()
{
    emit Signal_Pedal(false);
}


void MainProcess::Slot_Pedal(bool signal)
{
    if (m_isShowNextImage != signal)
    {
        if (signal)
        {
            //判断上一次松开踏板的时间m_LastOverTime，到这一次按下踏板的时间currentDateTime间隔是否少于防护时长TimeInterval。在config中设置TimeInterval.
            if (m_LastOverTime.secsTo(QDateTime::currentDateTime()) <= m_TimeInterval)
            {
                //少于间隔时长，直接丢弃
                FILE_LOG_WARN("Interval Too Brief.");
                return;
            }
        }
        else    //完成一次踩下并松开的动作
        {
            FILE_LOG_INFO("Show image from the queue.");
            m_isShowNextImage = signal;
            m_LastOverTime = QDateTime::currentDateTime();  //更新Last-Time
            emit Signal_Dequeue();
        }

        m_isShowNextImage = signal;
    }
}

void MainProcess::Slot_System_WriteConfig()
{
    WriteSystemConfig();
    InitCleanupFiles();
}

void MainProcess::Slot_DefaultRecipeChanged()
{
    Recipe_ReadConfig();
    Recipe_SetParameter();
    InitCleanupFiles();
}

void MainProcess::Slot_FilterHistoryRecord(const QString& filter)
{
    sql_refreshHistoryRecord(filter);
}

void MainProcess::Slot_FilterReportRecord(const QString& filter)
{
    sql_refreshReportRecord(filter);
    GetChartData(filter);   // 获取折线图表用的数据
}

void MainProcess::Slot_FilterHistoryData()
{
    //刷新列表
    QDateTime beginTime = m_FilterDateTim_Start;
    QDateTime endTime = QDateTime::currentDateTime();
    if (beginTime > endTime)
    {
        //弹窗提示时间不合法
        FILE_LOG_ERROR("HistoryRecord_GetTimeFileter error: Start time cannot be later than end time!");
        return;
    }

    //根据时间查询数据库
    QString beginTimeStr = beginTime.toString("yyyyMMddhhmmss");
    QString endTimeStr = endTime.toString("yyyyMMddhhmmss");
    QString filter = QString("dateTime >= '%1' and dateTime <= '%2'").arg(beginTimeStr).arg(endTimeStr);

    //m_modelHistory->setSort(m_modelHistory->fieldIndex(tr("dateTime")), Qt::DescendingOrder);
    //m_modelHistory->setFilter(filter);
    //m_modelHistory->setSort(0, Qt::DescendingOrder);
    //m_modelHistory->select();

    ////更新NG列表
    //m_modelReport->setFilter(filter);
    //m_modelReport->setSort(0, Qt::DescendingOrder);
    //m_modelReport->select();
    if (IsRealTimeChange_Main)
    {
        sql_refreshHistoryRecord(filter);
    }  
    if (IsRealTimeChange_Tab)
    {
        sql_refreshReportRecord(filter);
    }
}

bool MainProcess::GetGlassImagePath(const QString& idRecords, QString& outPath, QString& outDateTime)
{
    try
    {
        QSqlQuery query(m_db);
        QString sql = QString("select * from glasserrorinfo_records where id_records = '%1'").arg(idRecords);
        bool rst = query.exec(sql);
        if (!rst)
        {
            return false;
        }
        //只读取第一条数据
        if (query.next())
        {
            outPath = query.value("detail_records").toString();
            outDateTime = query.value("id_records").toString();
        }
        else
        {
            return false;
        }

    }
    catch (const std::exception&)
    {
        return false;
    }
    return true;
}


QMap<DefectType, QMap<QDate, int>> MainProcess::GetChartData(const QString& filter)
{

    QMap<DefectType, QMap<QDate, int>> result;
    result.remove(DefectType::DEFECT_TYPE_COUNT);   //删除计数行
    // 遍历模型中的所有行
#ifdef SQL_QUERY
    if (m_db_read.isOpen()) {
        m_db_read.commit();   // 如果当前有事务，会提交；如果没有事务，无操作
    }
    QString sql = QString(
        "SELECT SUBSTR(CAST(dateTime AS TEXT), 1, 8) AS dateTime, "
        "SUM(errnum0 ) AS errnum0 , "
        "SUM(errnum1 ) AS errnum1 , "
        "SUM(errnum2 ) AS errnum2 , "
        "SUM(errnum3 ) AS errnum3 , "
        "SUM(errnum4 ) AS errnum4 , "
        "SUM(errnum5 ) AS errnum5 , "
        "SUM(errnum6 ) AS errnum6 , "
        "SUM(errnum7 ) AS errnum7 , "
        "SUM(errnum8 ) AS errnum8 , "
        "SUM(errnum9 ) AS errnum9 , "
        "SUM(errnum10) AS errnum10, "
        "SUM(errnum11) AS errnum11, "
        "SUM(errnum12) AS errnum12, "
        "SUM(errnum13) AS errnum13, "
        "SUM(errnum14) AS errnum14, "
        "SUM(errnum15) AS errnum15  "
        "FROM HistoryView "
        "WHERE " + filter + " "
        "GROUP BY SUBSTR(CAST(dateTime AS TEXT), 1, 8) "
        //"ORDER BY gip.time_produce DESC "
        //"LIMIT 100"
    );
    //m_db_read.rollback();
    m_modelChartQry->setQuery(sql,m_db_read);
    // 检查查询是否成功
    if (m_modelChartQry->lastError().isValid()) {
        FILE_LOG_ERROR("Sql Query error: %s\nLast query: %s"
            , m_modelChartQry->lastError().text().toStdString()
            , sql.toStdString()
        );
    }
    else {
        qDebug() << "Query executed successfully";
        qDebug() << "Row count:" << m_modelChartQry->rowCount();
        qDebug() << "Column count:" << m_modelChartQry->columnCount();
        qWarning() << "Last query:" << sql;
    }
    // 获取date_Time列的索引
    int dateTimeColumnIndex = m_modelChartQry->record().indexOf("dateTime");

    for (int row = 0; row < m_modelChartQry->rowCount(); row++) {

        // 获取时间数据
        QModelIndex timeIndex = m_modelChartQry->index(row, dateTimeColumnIndex);
        // QString times = m_modelChartQry->data(timeIndex).toString().left(10);
        QString times = m_modelChartQry->getValue(row, "dateTime").toString();
        //QString times = m_modelChartQry->data(timeIndex).toString();
        times.replace("-", "");
        qDebug() << "times: " << times;;

        QDate dateTime;
        if (Language == u8"中文")
        {
            dateTime = QDate::fromString(times, "yyyyMMdd");
        }
        else
        {
            dateTime = QDate::fromString(times, "ddMMyyyy");
        }

        // 使用你的CustomSqlQueryModel的getValue方法，传入列名
        int err0 = m_modelChartQry->getValue(row, "errnum0").toInt();
        int err1 = m_modelChartQry->getValue(row, "errnum1").toInt();
        int err2 = m_modelChartQry->getValue(row, "errnum2").toInt();
        int err3 = m_modelChartQry->getValue(row, "errnum3").toInt();
        int err4 = m_modelChartQry->getValue(row, "errnum4").toInt();
        int err5 = m_modelChartQry->getValue(row, "errnum5").toInt();
        int err6 = m_modelChartQry->getValue(row, "errnum6").toInt();
        int err7 = m_modelChartQry->getValue(row, "errnum7").toInt();
        int err8 = m_modelChartQry->getValue(row, "errnum8").toInt();
        int err9 = m_modelChartQry->getValue(row, "errnum9").toInt();
        int err10 = m_modelChartQry->getValue(row, "errnum10").toInt();
        int err11 = m_modelChartQry->getValue(row, "errnum11").toInt();
        int err12 = m_modelChartQry->getValue(row, "errnum12").toInt();
        int err13 = m_modelChartQry->getValue(row, "errnum13").toInt();
        int err14 = m_modelChartQry->getValue(row, "errnum14").toInt();
        int err15 = m_modelChartQry->getValue(row, "errnum15").toInt();

        // 累加缺陷数量
        result[DefectType::TYPE_POORCOATING][dateTime] = err0;
        result[DefectType::TYPE_SCRATCH][dateTime] = err1;
        result[DefectType::TYPE_CALCULUS][dateTime] = err2;
        result[DefectType::TYPE_BUBBLE][dateTime] = err3;
        result[DefectType::TYPE_TRADEMARK][dateTime] = err4;
        result[DefectType::TYPE_WATERSTAIN][dateTime] = err5;
        result[DefectType::TYPE_SMUDGE][dateTime] = err6;

        if (m_defectTypeUsability[DefectType::TYPE_SCREENPRINTING])
        {
            result[DefectType::TYPE_SCREENPRINTING][dateTime] = err7;   //7 丝印列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_CHIPPED_EDGE])
        {
            result[DefectType::TYPE_CHIPPED_EDGE][dateTime] = err8;   //8 崩边列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_PITTING])
        {
            result[DefectType::TYPE_PITTING][dateTime] = err9;   //9 麻点列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_GLASS_CULLET])
        {
            result[DefectType::TYPE_GLASS_CULLET][dateTime] = err10;   //10 玻渣列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_WAVINESS])
        {
            result[DefectType::TYPE_WAVINESS][dateTime] = err11;   //11 云朵列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_OTHER])
        {
            result[DefectType::TYPE_OTHER][dateTime] = err12;   //12 其他列是否显示
        }
    }
#else
    if (m_modelReport == nullptr)
    {
        return result;
    }
    /* 过滤条件，在视图未到位前，剔除表中不存在的过滤条件 */
    QString newFilter = filter;
    newFilter.remove("and NG Status = 0");
    
    m_modelReport->setFilter("");
    // 设置新的过滤器
    m_modelReport->setFilter(newFilter);

    // 确保数据是最新的
    m_modelReport->select();

    // 遍历模型中的所有行
    for (int row = 0; row < m_modelReport->rowCount(); ++row) {
        // 获取时间数据
        QModelIndex timeIndex = m_modelReport->index(row, m_modelReport->record().indexOf("dateTime"));
        QString times = m_modelReport->data(timeIndex).toString().left(10);
        times.replace("-", "");
        QDate dateTime = QDate::fromString(times, "yyyyMMdd");

        // 获取各种缺陷数量
        QModelIndex err0Index  = m_modelReport->index(row, m_modelReport->record().indexOf("errnum0"));
        QModelIndex err1Index  = m_modelReport->index(row, m_modelReport->record().indexOf("errnum1"));
        QModelIndex err2Index  = m_modelReport->index(row, m_modelReport->record().indexOf("errnum2"));
        QModelIndex err3Index  = m_modelReport->index(row, m_modelReport->record().indexOf("errnum3"));
        QModelIndex err4Index  = m_modelReport->index(row, m_modelReport->record().indexOf("errnum4"));
        QModelIndex err5Index  = m_modelReport->index(row, m_modelReport->record().indexOf("errnum5"));
        QModelIndex err6Index  = m_modelReport->index(row, m_modelReport->record().indexOf("errnum6"));
        QModelIndex err7Index  = m_modelReport->index(row, m_modelReport->record().indexOf("errnum7"));
        QModelIndex err8Index  = m_modelReport->index(row, m_modelReport->record().indexOf("errnum8"));
        QModelIndex err9Index  = m_modelReport->index(row, m_modelReport->record().indexOf("errnum9"));
        QModelIndex err10Index = m_modelReport->index(row, m_modelReport->record().indexOf("errnum10"));
        QModelIndex err11Index = m_modelReport->index(row, m_modelReport->record().indexOf("errnum11"));
        QModelIndex err12Index = m_modelReport->index(row, m_modelReport->record().indexOf("errnum12"));
        
        
        // QModelIndex err7Index = m_modelReport->index(row, m_modelReport->record().indexOf("errnum7"));
        if (m_defectTypeUsability[DefectType::TYPE_SCREENPRINTING])
        {
            err7Index = m_modelReport->index(row, m_modelReport->record().indexOf("errnum7"));   //7 丝印列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_CHIPPED_EDGE])
        {
            err8Index = m_modelReport->index(row, m_modelReport->record().indexOf("errnum8"));   //8 崩边列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_PITTING])
        {
            err9Index = m_modelReport->index(row, m_modelReport->record().indexOf("errnum9"));   //9 麻点列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_GLASS_CULLET])
        {
            err10Index = m_modelReport->index(row, m_modelReport->record().indexOf("errnum10"));   //10 玻渣列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_WAVINESS])
        {
            err11Index = m_modelReport->index(row, m_modelReport->record().indexOf("errnum11"));   //11 云朵列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_OTHER])
        {
            err12Index = m_modelReport->index(row, m_modelReport->record().indexOf("errnum12"));   //12 其他列是否显示
        }


        // 累加缺陷数量
        result[DefectType::TYPE_POORCOATING][dateTime] += m_modelReport->data(err0Index).toInt();
        result[DefectType::TYPE_SCRATCH][dateTime] += m_modelReport->data(err1Index).toInt();
        result[DefectType::TYPE_CALCULUS][dateTime] += m_modelReport->data(err2Index).toInt();
        result[DefectType::TYPE_BUBBLE][dateTime] += m_modelReport->data(err3Index).toInt();
        result[DefectType::TYPE_TRADEMARK][dateTime] += m_modelReport->data(err4Index).toInt();
        result[DefectType::TYPE_WATERSTAIN][dateTime] += m_modelReport->data(err5Index).toInt();
        result[DefectType::TYPE_SMUDGE][dateTime] += m_modelReport->data(err6Index).toInt();
        // result[DefectType::TYPE_ScreenPrintingDefects][dateTime] += model.data(err7Index).toInt();
        if (m_defectTypeUsability[DefectType::TYPE_SCREENPRINTING])
        {
            result[DefectType::TYPE_SCREENPRINTING][dateTime] += m_modelReport->data(err7Index).toInt();   //7 丝印列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_CHIPPED_EDGE])
        {
            result[DefectType::TYPE_CHIPPED_EDGE][dateTime] += m_modelReport->data(err8Index).toInt();   //8 崩边列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_PITTING])
        {
            result[DefectType::TYPE_PITTING][dateTime] += m_modelReport->data(err9Index).toInt();   //9 麻点列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_GLASS_CULLET])
        {
            result[DefectType::TYPE_GLASS_CULLET][dateTime] += m_modelReport->data(err10Index).toInt();   //10 玻渣列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_WAVINESS])
        {
            result[DefectType::TYPE_WAVINESS][dateTime] += m_modelReport->data(err11Index).toInt();   //11 云朵列是否显示
        }
        if (m_defectTypeUsability[DefectType::TYPE_OTHER])
        {
            result[DefectType::TYPE_OTHER][dateTime] += m_modelReport->data(err12Index).toInt();   //12 其他列是否显示
        }
    }
#endif
    emit Signal_ShowHistoryChart(result);
    return result;
}


std::vector<bool> MainProcess::GetCameraStatus()
{
    std::vector<bool> status;
    int num = CameraManager::GetInstance()->GetCameraNum();
    if (num <= 0)
    {
        return status;
    }
    for (int i = 0; i < num; ++i)
    {
        auto camera = CameraManager::GetInstance()->GetCamera(i);
        if (camera) // 检查指针是否有效
        {
            status.push_back(camera->IsDeviceConnect()); // 使用 -> 操作符访问成员函数
        }
        else
        {
            status.push_back(false); // 如果相机不存在，返回 false
        }
    }
    return status;
}

MainProcess::MainProcess(QObject* parent)
{

}


bool MainProcess::Vision_StartWorking()
{
	//step1: 先关闭光源，确保在相机开始采集前，光源处于关闭状态，避免误触发拍照。
    CloseLight();

	//step2: 开始采集(等待打开光源(硬触发采集信号))
    if (!CameraManager::GetInstance()->StartGrabbing() && IS_READ_MODE == 0)
    {
        FILE_LOG_ERROR("StartGrabbing failed.");
        return false;
    }

	//setp3: 设置设备三色灯状态：0 = 待机（黄灯亮）1 = 运行中/检测正常（绿灯亮）2 = 检测到NG（红灯亮）
    SetStackLight(1); //绿灯亮
    m_isWorking = true;
    isGlassStopGrabbing = true;
    FILE_LOG_INFO("START_WORKING");

    return true;
}


bool MainProcess::Vision_StopWorking()
{
    //step1: 先结束当前执行中的流程。
    Vision_StopProcess();

	//step2: 关闭光源，停止硬触发
    CloseLight(); //关闭光源

	//step3: 停止采集(等待玻璃完全板出，避免误触发拍照) 
    /****************************相机停止采集********************************************/
    if (!CameraManager::GetInstance()->StopGrabbing() && IS_READ_MODE == 0)
    {
        FILE_LOG_ERROR("StopGrabbing failed.");
        return false;
    }
    
    isGlassStopGrabbing = false;
    m_isWorking = false;
    //setp4: 设置设备三色灯状态：0 = 待机（黄灯亮）1 = 运行中/检测正常（绿灯亮）2 = 检测到NG（红灯亮）
    SetStackLight(0); //黄灯亮
    return true;
}


void MainProcess::Vision_StartProcess()
{
    if (!m_isWorking)
    {
        FILE_LOG_WARN("[Process]StartProcess Error: Machine stopped. Detection not running.");
        DisplayMessage::getInstance().logMessage(tr("Machine stopped. Detection not running."));  //设备未工作，不执行检测。
        return;
    }
    //if (m_isSignalRunning)
    //{
    //    FILE_LOG_WARN("[Process]StartProcess Error:Detection active. New detection blocked..");
    //    DisplayMessage::getInstance().logMessage(tr("Detection active. New detection blocked."));  //设备未工作检测进行中，不执行检测。
    //    return;
    //}
    ////防护当前检测进行中，收到新的检测命令
    //if (m_isRunning_Compute)
    //{
    //    FILE_LOG_WARN("[Process]StartProcess Error:Detection in progress, discard new command.");
    //    DisplayMessage::getInstance().logMessage(tr("Detection in progress, discard new command."));  //设备检测进行中，不执行检测。
    //    return;
    //}

    /****************************延迟开始采集的等待时间***********************************/
    FILE_LOG_INFO(u8"准备开始采集。Sleep=%f", (DelayStartTime * 1000));
    std::this_thread::sleep_for(std::chrono::milliseconds(int(1000 * DelayStartTime)));//20260129：按现场意见添加。延时采集开始时间
    //Sleep(1000 * DelayStartTime);//20260129：按现场意见添加。延时采集开始时间
    FILE_LOG_INFO(u8"开始采集");
	/////////****************************相机开始采集********************************************/
    ////////   if (!CameraManager::GetInstance()->StartGrabbing() && IS_READ_MODE == 0)
    ////////   {
    ////////       FILE_LOG_ERROR("[MainProcess]StartProcess Error: StartGrabbing failed.");
    ////////       return;
    ////////   }
    ////////   FILE_LOG_INFO("Cameras Start Grabbing Completed");
    /*****************打开光源开关(后续亮灯由硬触发自动处理)***************************/
    unsigned short Bit_Output_Number = 0;
    bool bRst = Platform::Instance().SetIOOutState(Bit_Output_Number, Bit_Output_Close);
    if (!bRst)
    {
        FILE_LOG_ERROR(u8"控制卡打开光源指令发送失败");
    }
    else
    {
        FILE_LOG_INFO(u8"控制卡打开光源指令发送成功");
    }
    /***********************************************************************************/
    isGlassStopGrabbing = false;

    m_isRunning_Compute = true;
    FILE_LOG_INFO("[Process]StartProcess Begin.");
    m_isSignalRunning = true;
    FILE_LOG_INFO("[Process]StartProcess: Card in! Start Grabbing Images.");

    //2026.02.09 按要求，俄罗斯版本修改板进板出信号到监听光电信号处。
    /*
    m_SignalDateTime = QDateTime::currentDateTime();
    m_time_produce_Real = m_SignalDateTime.toString("yyyy-MM-dd HH:mm:ss");
    emit Signal_GlassStatusChanged(GlassStatus::GlassIn, m_time_produce_Real);   //在界面显示玻璃来料状态和处理状态信号。0=板进
    */

    CameraManager::GetInstance()->StartProcessImages();
 
    std::thread thread_Vision(&MainProcess::Vision_ComputerThread, this);  //启动线程
    thread_Vision.detach();

    FILE_LOG_INFO("[Process]StartProcess Completed.");
    DisplayMessage::getInstance().logMessage(tr("Start Detecting"));  //启动检测
}


void MainProcess::Vision_StopProcess()
{
    //if (!m_isRunning_Compute)
    //{
    //    FILE_LOG_ERROR("Process is not running. Discard stop command");
    //    DisplayMessage::getInstance().logMessage(tr("Process is not running. Discard stop command."));  //未执行检测。不执行停止检测动作。
    //    return;
    //}
    //if (!m_isSignalRunning)
    //{
    //    FILE_LOG_ERROR("Porcess is not nunning. New Stop command not exec");
    //    DisplayMessage::getInstance().logMessage(tr("Detection inactive. Stop blocked."));  //未执行检测。不执行停止检测动作。
    //    return;
    //}

    /******************************延迟结束采集*******************************************/
    FILE_LOG_INFO(u8"准备结束采集。Sleep=%f", (DelayStopTime * 1000));
    std::this_thread::sleep_for(std::chrono::milliseconds(int(1000 * DelayStopTime)));//延时采集结束时间，等待玻璃完全板出
    //Sleep(1000 * DelayStopTime);//延时采集结束时间，等待玻璃完全板出
    FILE_LOG_INFO(u8"结束采集");
    /*****************关闭光源开关(后续亮灯由硬触发自动处理)***************************/
    bool bRst_close = Platform::Instance().SetIOOutState(0, Bit_Output_Open);
    if (!bRst_close)
    {
        FILE_LOG_ERROR(u8"控制卡关闭光源指令发送失败");
    }
    else
    {
        FILE_LOG_INFO(u8"控制卡关闭光源指令发送成功");
    }
    /***********************************************************************************/
    ///////////****************************相机停止采集********************************************/
    //////////if (!CameraManager::GetInstance()->StopGrabbing() && IS_READ_MODE == 0)
    //////////{
    //////////    FILE_LOG_ERROR("[MainProcess]StopProcess Error: StopGrabbing failed.");
    //////////    return;
    //////////}
    FILE_LOG_INFO("Cameras Stop Grabbing Completed");
    isGlassStopGrabbing = true;

    //m_isProessImage = false;
    FILE_LOG_INFO("Card out! Stop Grabbing Images");

    //2026.02.09 按要求，俄罗斯版本修改板进板出信号到监听光电信号处。
    //emit Signal_GlassStatusChanged(GlassStatus::GlassOut, m_time_produce_Real);   //在界面显示板进信号。1=板出

    CameraManager::GetInstance()->StopProcessImages();
    FILE_LOG_INFO("StopGrabbing");
    m_isSignalRunning = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //    
    ////启动视觉模块状态检测线程
    //std::thread thread_Vision(&MainProcess::ComputerThread, this);	//启动线程
    //thread_Vision.detach();

    DisplayMessage::getInstance().logMessage(tr("Stop Detecting"));  //结束检测
}

bool MainProcess::Vision_ProcessImage(QueueDefectItem& resultItem)
{
    FILE_LOG_INFO("ProcessResultImages: begin.");

    if (resultItem.image_0.empty() || resultItem.image_1.empty() || resultItem.image_2.empty())
    {
        FILE_LOG_ERROR("ProcessResultImages error: CurrentImage is empty!");
        return false;
    }

    //int col1 = resultItem.image_1.cols / 2 - 1;  // 第一列
    //int col2 = resultItem.image_2.cols / 2;      // 第二列
    //double avg1 = m_GeneralMethod.getAverageOfTwoColumns(resultItem.image_2, col1, col2);
    //double avg2 = m_GeneralMethod.getAverageOfTwoColumns(resultItem.image_1, col1, col2);
    cv::Mat currentImage_Display = resultItem.image_1;
    //if (abs(avg1 - GrayReal) < abs(avg2 - GrayReal)) //avg1的玻璃更合适
    //{
    //    currentImage_Display = resultItem.image_2;

    //}
    //else {

    //    currentImage_Display = resultItem.image_1;
    //}
    //int grayVal = cv::mean(currentImage_Display)[0];
    //setp1. 根据图像尺寸，创建背景图像
    // 获取示例图相对于原图的缩放比
    resultItem.scale = m_GeneralMethod.getScale(resultItem.image_1.cols, currentImage_Display.rows);
    double mergeWidthScale = resultItem.scale * widthScale;
    double mergeHeightScale = resultItem.scale * heightScale;


    if (IsPseudoColorImage)
    {
        //Step2. 创建伪彩图像
        //resultItem.pseudoColorImage = m_showVirtualInfo.generateGlassPseudoColor(resultItem.image_0, 5, 10);
        resultItem.pseudoColorImage = m_showVirtualInfo.generateGlassPseudoColorChannel2(resultItem.image_0, resultItem.image_1, 5, 10);
        if (resultItem.pseudoColorImage.empty()) {
            FILE_LOG_ERROR("ProcessResultImages error: generateGlassPseudoColor error");
            resultItem.pseudoColorImage = cv::Mat::zeros(static_cast<int>(resultItem.image_0.rows / 10), static_cast<int>(resultItem.image_0.cols / 10), CV_8UC3);
        }

        //qxz0428修改: 旋转伪彩图(小图)，保持与原来显示方向一致
        {
            cv::Mat tmpPseudo;
            cv::transpose(resultItem.pseudoColorImage, tmpPseudo);
            if (RotationDirectionFlag == 0) {
                cv::flip(tmpPseudo, resultItem.pseudoColorImage, 1);
            }
            else {
                cv::flip(tmpPseudo, resultItem.pseudoColorImage, 0);
            }
            tmpPseudo.release();
        }
    }
    ////setp3. 显示玻璃尺寸到界面顶部指定区域（数据库登记玻璃尺寸需要）
    // //qxz0428修改: 旋转伪彩图(小图)，保持与原来显示方向一致
    //resultItem.glassPhysicalWidth = currentImage_Display.cols * Pixle2MM_X;
    resultItem.glassPhysicalWidth = currentImage_Display.rows * Pixle2MM_X;
    if (int(resultItem.glassPhysicalHeight) == 0)   //防护动作
    {
        resultItem.glassPhysicalHeight = currentImage_Display.rows * Pixle2MM_Y;
    }

    resultItem.glassPixelWidth = currentImage_Display.cols;
    resultItem.glassPixelHeight = currentImage_Display.rows;

    // 在创建背景前，增加尺寸校验并使用 grayVal 填充
    // // //qxz0428修改: 交换cols/rows计算背景图尺寸，等价于原代码旋转后
    //int widthBackGround = static_cast<int>(currentImage_Display.cols * mergeWidthScale);
    //int heightBackGround = static_cast<int>(currentImage_Display.rows * mergeHeightScale);
    int widthBackGround = static_cast<int>(currentImage_Display.rows * mergeWidthScale);
    int heightBackGround = static_cast<int>(currentImage_Display.cols * mergeHeightScale);

    const int minSize = 10; // 可调整的最小尺寸阈值
    if (widthBackGround < minSize || heightBackGround < minSize) {
        FILE_LOG_INFO("Invalid background size (%d x %d), adjust to minimum %d", widthBackGround, heightBackGround, minSize);
        widthBackGround = std::max(widthBackGround, minSize);
        heightBackGround = std::max(heightBackGround, minSize);
    }
    //setp4. 创建背景图片
    if (resultItem.backgroundImage.empty()) //背景图不存在时，按照老的逻辑创建背景图(即虚拟图)
    {
        //QColor fillColor(grayVal, grayVal, grayVal); // 你可以根据需要更改颜色
        if (!UsedIrregularImage)
        {
            resultItem.backgroundImage = cv::Mat::ones(         //cv::Mat::ones() 的参数是 (height, width)，即 (rows, cols)
                widthBackGround, //qxz0428 调换宽高
                heightBackGround,//qxz0428 调换宽高
                CV_8UC1) * 195;
        }
        else
        {

            int showGrayValue = 58;//背景区域显示灰度值    #3A3A3A 或58
            int BackgroundColor = 195;//玻璃区域显示灰度值 //195与背景颜色一致
            cv::Mat glassMaskImageVirtual;
            //qxz0428 调换宽高
            //cv::resize(resultItem.image_1, glassMaskImageVirtual, cv::Size(widthBackGround, heightBackGround), 0, 0, cv::INTER_NEAREST);//亮场（通道1为亮场图）获取异形玻璃
            cv::resize(resultItem.image_1, glassMaskImageVirtual, cv::Size(heightBackGround, widthBackGround ), 0, 0, cv::INTER_NEAREST);//亮场（通道1为亮场图）获取异形玻璃
            std::vector<cv::Mat> virtuals;
            virtuals.push_back(glassMaskImageVirtual);
            //resultItem.backgroundImage = m_showVirtualInfo.createVirtualMap(virtuals, showGrayValue, BackgroundColor,20);glassThreshold_display
            resultItem.backgroundImage = m_showVirtualInfo.createVirtualMap(virtuals, showGrayValue, BackgroundColor, glassThreshold_display);
        }
        //qxz0428修改: 旋转虚拟图(小图)，保持与原来显示方向一致
        {
            cv::Mat tmpBg;
            cv::transpose(resultItem.backgroundImage, tmpBg);
            if (RotationDirectionFlag == 0) {
                cv::flip(tmpBg, resultItem.backgroundImage, 1);
            }
            else {
                cv::flip(tmpBg, resultItem.backgroundImage, 0);
            }
            tmpBg.release();
        }
        //多块玻璃、异形玻璃背景图
        //int showGrayValue = 100;//玻璃区域显示灰度值
        //int BackgroundColor = 58;//背景区域显示灰度值    #3A3A3A 或58
        //cv::Size virtualSize = cv::Size(widthBackGround, heightBackGround);
        //cv::Mat glassMaskImageVirtual = m_showVirtualInfo.generateVirtualImage(resultItem.image_1, virtualSize, showGrayValue, BackgroundColor, glassThreshold_val);
        // //cv::Mat glassMaskImageVirtual = m_showVirtualInfo.generateVirtualImage(resultItem.image_1, virtualSize, showGrayValue, BackgroundColor, glassThreshold_display);   //20260307修改，保持和createVirtualMap一致。保持原有注释状态
        //resultItem.backgroundImage = glassMaskImageVirtual.clone();   //传递背景图(虚拟图)

        //// 2. 设置内边框2像素为133（即图像四周2像素宽的区域）
        //int borderSize = 2;
        //// 上边框
        //resultItem.backgroundImage(cv::Rect(0, 0, widthBackGround, borderSize)) = 133;
        //// 下边框
        //resultItem.backgroundImage(cv::Rect(0, heightBackGround - borderSize, widthBackGround, borderSize)) = 133;
        //// 左边框
        //resultItem.backgroundImage(cv::Rect(0, borderSize, borderSize, heightBackGround - 2 * borderSize)) = 133;
        //// 右边框
        //resultItem.backgroundImage(cv::Rect(widthBackGround - borderSize, borderSize, borderSize, heightBackGround - 2 * borderSize)) = 133;
    }
    else
    {
        heightBackGround=resultItem.backgroundImage.rows;
        widthBackGround=resultItem.backgroundImage.cols;
    }
    //setp5. 缩放图像

    int showSize = widthBackGround; //图像缩放后的最大宽度(长宽哪个边长大，就选哪个)
    float scaleShow = 0.05;//图例相对于整张图的缩放
    if (widthBackGround < heightBackGround)
    {
        showSize = static_cast<int>(widthBackGround * scaleShow);
    }
    else {
        showSize = static_cast<int>(heightBackGround * scaleShow);
    }

    //drawInfos转绘制图尺寸信息
    if (IS_READ_MODE != 2)
    {
        /**************** COMPUTE结果处理1. 合并指定范围内的同类型瑕疵 ***********************/
        //原始尺寸下，按配方参数合并指定距离mm之内额的瑕疵
     
        int Dis = (int)(RecipeInfo.global.mergeDistance / Pixle2MM_X);
        std::vector<drawInformation> mergeInfos;
        if (Dis != 0) {//合并指定距离mm之内缺陷
            mergeInfos = m_GeneralMethod.mergeDrawInformationsWithDistance(resultItem.drawInfo, Dis);
        }
        else {//不合并
            mergeInfos = resultItem.drawInfo;
        }
        /*************************************************************************************/

        /**************** COMPUTE结果处理2. 过滤排序 *****************************************/
        //原始尺寸下，只保留轻微、中等、严重等三种瑕疵。并且实现界面显示的瑕疵图例，按照从顶部图层到底部图层的顺序为 红 黄 绿
        /*2025050602 对缺陷等级进行排序，解决图例显示红色图例在下的问题*/
        std::vector<drawInformation> filterInfos = m_GeneralMethod.filterAndSortDefects(mergeInfos);
        /*************************************************************************************/

        // 遍历 results 并调整 rect
        //int infoIndex = 0;
        //for (auto& info : filterInfos) 
        //{

        //    drawInformation tempdraw = info;
        //    if (tempdraw.rect.x <0)
        //    {
        //        FileLogPrintf("CHECK_SOUREC_RECT: drawInformation[%d]rect.x <0", infoIndex);
        //    }
        //    if (tempdraw.rect.y < 0)
        //    {
        //        FileLogPrintf("CHECK_SOUREC_RECT: drawInformation[%d]rect.y <0", infoIndex);
        //    }
        //    if (tempdraw.rect.width >= resultItem.image_2.cols)  //图像是存在第0列，所以要减1
        //    {
        //        FileLogPrintf("CHECK_SOUREC_RECT: drawInformation[%d]rect.width[%d] > image.widht[%d]", infoIndex, tempdraw.rect.width, resultItem.image_2.cols);
        //    }
        //    if (tempdraw.rect.x + tempdraw.rect.width >= resultItem.image_2.cols)  //图像是存在第0列，所以要减1
        //    {
        //        FileLogPrintf("CHECK_SOUREC_RECT: drawInformation[%d]rect.center_x[%d] > image.width[%d]", infoIndex, tempdraw.rect.x + tempdraw.rect.width / 2, resultItem.image_2.cols);
        //    
        //    }
        //    if (tempdraw.rect.height >= resultItem.image_2.rows)  //图像是存在第0列，所以要减1
        //    {
        //        FileLogPrintf("CHECK_SOUREC_RECT: drawInformation[%d]rect.height[%d] > image.height[%d]", infoIndex, tempdraw.rect.height, resultItem.image_2.rows);
        //    }
        //    if (tempdraw.rect.y + tempdraw.rect.height >= resultItem.image_2.rows)  //图像是存在第0列，所以要减1
        //    {
        //        FileLogPrintf("CHECK_SOUREC_RECT: drawInformation[%d]rect.center_y[%d] > image.height[%d]", infoIndex, tempdraw.rect.y + tempdraw.rect.height / 2, resultItem.image_2.rows);
        //    }
        //    infoIndex++;
        //}

        int infoIndex = 0;
        for (auto& info : filterInfos)
        {
            // 创建图像边界矩形
            cv::Rect imageBounds(0, 0, resultItem.image_2.cols, resultItem.image_2.rows);

            // 使用交集操作确保矩形在图像边界内
            info.rect = info.rect & imageBounds;

            // 检查修正后的矩形是否有效
            if (info.rect.width <= 0 || info.rect.height <= 0) {
			 
                FILE_LOG_INFO("CHECK_SOUREC_RECT: drawInformation[%d] invalid rect after correction: [%d, %d, %d, %d]",
                    infoIndex, info.rect.x, info.rect.y, info.rect.width, info.rect.height);
            }
            else if (info.rect.x < 0 || info.rect.y < 0 ||
                info.rect.x + info.rect.width > resultItem.image_2.cols ||
                info.rect.y + info.rect.height > resultItem.image_2.rows) {
                FILE_LOG_INFO("CHECK_SOUREC_RECT: drawInformation[%d] rect out of bounds after correction: [%d, %d, %d, %d], image size: [%d, %d]",
                    infoIndex, info.rect.x, info.rect.y, info.rect.width, info.rect.height,
                    resultItem.image_2.cols, resultItem.image_2.rows);
            }

            infoIndex++;
        }
        /**************** COMPUTE结果处理3. 瑕疵Rect位置转换到虚拟图 **************************/
        //按照虚拟图尺寸，进行瑕疵Rect的转换。
        resultItem.drawInfo.clear();

        //qxz0428 调换宽高
        /*resultItem.drawInfo = m_GeneralMethod.real2VirtualResize(widthBackGround, heightBackGround, 
            filterInfos, mergeWidthScale, mergeHeightScale, showSize);*/
        resultItem.drawInfo = m_GeneralMethod.real2VirtualResize(heightBackGround, widthBackGround ,
            filterInfos, mergeHeightScale , mergeWidthScale, showSize);

        //qxz0428修改: real2VirtualResize后旋转drawInfo的rect坐标，匹配旋转后的虚拟图
        {
            int virtW = widthBackGround;
            int virtH = heightBackGround;
            for (auto& info : resultItem.drawInfo) {
                cv::Rect oldRect = info.rect;
                if (RotationDirectionFlag == 0) {
                    //qxz0414 CW90: (x,y,w,h) -> (H'-y-h, x, h, w)
                    int preRotH = virtW;
                    info.rect.x = preRotH - oldRect.y - oldRect.height;
                    info.rect.y = oldRect.x;
                    info.rect.width = oldRect.height;
                    info.rect.height = oldRect.width;
                }
                else {
                    //qxz0414 CCW90: (x,y,w,h) -> (y, W'-x-w, h, w)
                    int preRotW = virtH;
                    info.rect.x = oldRect.y;
                    info.rect.y = preRotW - oldRect.x - oldRect.width;
                    info.rect.width = oldRect.height;
                    info.rect.height = oldRect.width;
                }
            }
        }
        /*************************************************************************************/
    }

    std::vector<cv::Mat> detectImages_ch0, detectImages_ch1, detectImages_ch2;
    if (!resultItem.drawInfo.empty())
    {
        
        bool isLastItem = false;
        for (int i = 0; i < resultItem.drawInfo.size(); i++)
        {
            cv::Rect colorRect;
            // 计算 tempRect 后，限定在源图像范围内再裁剪
            //cv::Rect tempRect = m_GeneralMethod.computeCropRect2Color(resultItem.drawInfo[i].realRect, resultItem.glassPixelWidth, resultItem.glassPixelHeight, 256, &colorRect);
            //qxz0428修改: 用实际图像尺寸(image_2.cols/rows)而非交换后的glassPixelWidth/Height
            cv::Rect tempRect = m_GeneralMethod.computeCropRect2Color(resultItem.drawInfo[i].realRect, resultItem.image_2.cols, resultItem.image_2.rows, 256, &colorRect);
            // 限制在 image_0 范围内（假设三通道图尺寸相同）
            cv::Rect imgBounds(0, 0, resultItem.image_0.cols, resultItem.image_0.rows);
            tempRect &= imgBounds; // 取交集
            if (tempRect.width <= 0 || tempRect.height <= 0) {
                FILE_LOG_INFO("computeCropRect2Color returned out-of-bounds rect, skip defect %d", i);
                continue;
            }

            cv::Mat rectMat0 = resultItem.image_0(tempRect);
            cv::Mat rectMat1 = resultItem.image_1(tempRect);
            cv::Mat rectMat2 = resultItem.image_2(tempRect);


            //////////////////cv::Mat connected = m_GeneralMethod.segUtils.GetDrawImage(rectMat2, 20, 0);
            //////////////////// 定义颜色映射
            //////////////////static const std::vector<cv::Scalar> color_map = {
            //////////////////    cv::Scalar(0, 255, 0),    // 绿色 - level 0
            //////////////////    cv::Scalar(0, 255, 255),  // 黄色 - level 1
            //////////////////    cv::Scalar(0, 0, 255)     // 红色 - level 2
            //////////////////};
            //////////////////int level = 0;
            //////////////////// 确保level在有效范围内
            //////////////////int color_index = std::clamp(level, 0, static_cast<int>(color_map.size() - 1));
            //////////////////cv::Scalar color = color_map[color_index];
            //////////////////
            ////////////////////MINOR,//轻微缺陷  1
            ////////////////////MEDIUM,//中等缺陷 2
            ////////////////////SERIOUS,//严重缺陷 3
            //////////////////if (resultItem.drawInfo[i].ErrorType == MEDIUM) {
            //////////////////    level = 1;
            //////////////////
            //////////////////}
            //////////////////else if (resultItem.drawInfo[i].ErrorType == SERIOUS) {
            //////////////////    level = 2;
            //////////////////}
            //////////////////// 在原图上绘制边缘
            //////////////////cv::Mat rectImage0 = m_GeneralMethod.segUtils.drawEdgesOnImage(rectMat0, connected, colorRect, color, level);
            //////////////////cv::Mat rectImage1 = m_GeneralMethod.segUtils.drawEdgesOnImage(rectMat1, connected, colorRect, color, level);
            //////////////////cv::Mat rectImage2 = m_GeneralMethod.segUtils.drawEdgesOnImage(rectMat2, connected, colorRect, color, level);

            //cv::Mat rectImage0 = m_GeneralMethod.drawSemiTransparentRedOverlayWithCircle(rectMat0, colorRect, 0.1);
            //cv::Mat rectImage1 = m_GeneralMethod.drawSemiTransparentRedOverlayWithCircle(rectMat1, colorRect, 0.1);
            //cv::Mat rectImage2 = m_GeneralMethod.drawSemiTransparentRedOverlayWithCircle(rectMat2, colorRect, 0.1);


            //std::filesystem::create_directories("./WriteIn/");
            //ImageSaver* m_imgSaver = new(ImageSaver);
            //if (!rectMat0.empty())
            //{
            //    std::string saveImage = "./WriteIn/" + std::to_string(i) + "_0.png";
            //    m_imgSaver->saveImageAsync(saveImage, rectMat0);       // 异步保存图像
            //}
            //if (!rectMat1.empty())
            //{
            //    std::string saveImage = "./WriteIn/" + std::to_string(i) + "_1.png";
            //    m_imgSaver->saveImageAsync(saveImage, rectMat1);       // 异步保存图像
            //}
            //////if (!rectMat2.empty())
            //////{
            //////    std::string saveImage = "./WriteIn/" + std::to_string(i) + "_2.png";
            //////    m_imgSaver->saveImageAsync(saveImage, rectMat2);       // 异步保存图像
            //////}

            if (!rectMat0.empty() && !rectMat1.empty() && !rectMat2.empty())
            {
                //resultItem.defectImages_ch0.push_back(std::move(rectImage0));
                //resultItem.defectImages_ch1.push_back(std::move(rectImage1));
                //resultItem.defectImages_ch2.push_back(std::move(rectImage2));

                resultItem.defectImages_ch0.push_back(std::move(rectMat0));
                resultItem.defectImages_ch1.push_back(std::move(rectMat1));
                resultItem.defectImages_ch2.push_back(std::move(rectMat2));

                resultItem.drawInfo[i].DefectId = i;
            }
        }
    }

    FILE_LOG_INFO("ProcessResultImages: end.");
    return true;
}

//bool MainProcess::Vision_ProcessImage(const cv::Mat& currentImage_0, const cv::Mat& currentImage_1,
//    const cv::Mat& currentImage_2, std::vector<drawInformation>& showInfo, std::string& timeProduce,
//    std::vector<std::vector<cv::Mat>>& detectImages, cv::Mat& backGroundImage, double& scale,
//    float& glassWidth, float& glassHeight, float& glassPixelWidth, float& glassPixelHeight)
//{
//    FileLogPrintf("ProcessResultImages: begin.");
//
//    if (currentImage_0.empty() || currentImage_1.empty() || currentImage_2.empty())
//    {
//        FileLogPrintf("ProcessResultImages error: CurrentImage is empty!");																	   
//        return false;
//    }
//
//    int col1 = currentImage_1.cols / 2 - 1;  // 第一列
//    int col2 = currentImage_2.cols / 2;      // 第二列
//    double avg1 = m_GeneralMethod.getAverageOfTwoColumns(currentImage_2, col1, col2);
//    double avg2 = m_GeneralMethod.getAverageOfTwoColumns(currentImage_1, col1, col2);
//    cv::Mat currentImage_Display;
//    if (abs(avg1 - GrayReal) < abs(avg2 - GrayReal)) //avg1的玻璃更合适
//    {
//        currentImage_Display = currentImage_2;
//
//    }
//    else {
//
//        currentImage_Display = currentImage_1;
//    }
//    int grayVal = cv::mean(currentImage_Display)[0];
//    //setp1. 根据图像尺寸，创建背景图像
//    // 获取示例图相对于原图的缩放比
//    scale = m_GeneralMethod.getScale(currentImage_Display.cols, currentImage_Display.rows);
//    double mergeWidthScale = scale * widthScale;
//    double mergeHeightScale = scale * heightScale;
//
//    ////setp3. 显示玻璃尺寸到界面顶部指定区域（数据库登记玻璃尺寸需要）
//    glassWidth = currentImage_Display.cols * GlassX;
//    //glassHeight = currentImage_Display.rows * GlassY + 30;
//
//    glassPixelWidth == currentImage_Display.cols;
//    glassPixelHeight == currentImage_Display.rows;
//
//    //setp4. 创建背景图片并传入右侧场景
//    QColor fillColor(grayVal, grayVal, grayVal); // 你可以根据需要更改颜色
//    backGroundImage = cv::Mat::ones(glassPixelHeight * mergeHeightScale, glassPixelWidth * mergeWidthScale, CV_8UC1) * grayVal; //cv::Mat::ones() 的参数是 (height, width)，即 (rows, cols)
//
//
//    //setp5. 缩放图像
//    int widthBackGround = static_cast<int>(currentImage_Display.cols * mergeWidthScale);
//    int heightBackGround = static_cast<int>(currentImage_Display.rows * mergeHeightScale);
//    int showSize = widthBackGround;
//    float scaleShow = 0.05;//图例相对于整张图的缩放
//    if (widthBackGround < heightBackGround)
//    {
//        showSize = static_cast<int>(widthBackGround * scaleShow);
//    }
//    else {
//        showSize = static_cast<int>(heightBackGround * scaleShow);
//    }
//
//    //drawInfos转绘制图尺寸信息
//    int Dis = (int)(RecipeInfo.global.mergeDistance / PIXEL_X_MM);
//    if (IS_READ_MODE != 2)
//    {
//        std::vector<drawInformation> mergeInfos = m_GeneralMethod.mergeDrawInformationsWithDistance(showInfo, Dis);
//        /*2025050602 对缺陷等级进行排序，解决图例显示红色图例在下的问题*/
//
//
//        std::vector<drawInformation> filterInfos = m_GeneralMethod.filterAndSortDefects(mergeInfos);
//        showInfo.clear();
//        showInfo = m_GeneralMethod.real2VirtualResize(widthBackGround, heightBackGround, filterInfos, mergeWidthScale, mergeHeightScale, showSize);
//    }
//
//    std::vector<cv::Mat> detectImages_ch0, detectImages_ch1, detectImages_ch2;
//    if (!showInfo.empty())
//    {
//        //ImageSaver* m_imgSaver = new(ImageSaver);
//        bool isLastItem = false;
//        for (int i = 0; i < showInfo.size(); i++)
//        {
//            cv::Rect colorRect;
//            cv::Rect tempRect = m_GeneralMethod.computeCropRect2Color(showInfo[i].realRect, glassPixelWidth, glassPixelHeight, 256, &colorRect);
//
//            cv::Mat rectMat0 = currentImage_0(tempRect);
//            cv::Mat rectImage0 = m_GeneralMethod.drawSemiTransparentRedOverlayWithCircle(rectMat0, colorRect, 0.1);
//
//            cv::Mat rectMat1 = currentImage_1(tempRect);
//            cv::Mat rectImage1 = m_GeneralMethod.drawSemiTransparentRedOverlayWithCircle(rectMat1, colorRect, 0.1);
//
//            cv::Mat rectMat2 = currentImage_2(tempRect);
//            cv::Mat rectImage2 = m_GeneralMethod.drawSemiTransparentRedOverlayWithCircle(rectMat2, colorRect, 0.1);
//
//
//            std::filesystem::create_directories("./WriteIn/");
//            //if (!rectMat0.empty())
//            //{
//            //    std::string saveImage = "./WriteIn/" + std::to_string(i) + "_0.png";
//            //    m_imgSaver->saveImageAsync(saveImage, rectMat0);       // 异步保存图像
//            //}
//            //if (!rectMat1.empty())
//            //{
//            //    std::string saveImage = "./WriteIn/" + std::to_string(i) + "_1.png";
//            //    m_imgSaver->saveImageAsync(saveImage, rectMat1);       // 异步保存图像
//            //}
//            //////if (!rectMat2.empty())
//            //////{
//            //////    std::string saveImage = "./WriteIn/" + std::to_string(i) + "_2.png";
//            //////    m_imgSaver->saveImageAsync(saveImage, rectMat2);       // 异步保存图像
//            //////}
//
//            if (!rectImage0.empty() && !rectImage1.empty() && !rectImage2.empty())
//            {
//                detectImages_ch0.push_back(std::move(rectMat0));
//                detectImages_ch1.push_back(std::move(rectMat1));
//                detectImages_ch2.push_back(std::move(rectMat2));
//
//                showInfo[i].DefectId = i;
//            }
//        }
//        detectImages.clear();
//        detectImages.push_back(std::move(detectImages_ch0));
//        detectImages.push_back(std::move(detectImages_ch1));
//        detectImages.push_back(std::move(detectImages_ch2));
//
//    }
//    else
//    {   //无瑕疵时，保持数据格式一致。
//        detectImages.clear();
//        detectImages.resize(3);
//    }
//
//    FileLogPrintf("ProcessResultImages: end.");
//    return true;
//}


/**
 * @brief 判断严重瑕疵在图像纵向方向上的分布情况
 *
 * @param drawInfo 所有瑕疵信息
 * @param seriousIndexItem 严重瑕疵的索引数组
 * @param height 图像高度
 * @return int 返回码：
 *            0 - 没有严重瑕疵
 *            1 - 所有严重瑕疵都在上半部分(0-Height/2)
 *            2 - 所有严重瑕疵都在下半部分(Height/2-Height)
 *            3 - 严重瑕疵同时存在于上下两部分
 */
int checkSeriousDefectsVerticalDistribution(
    const std::vector<drawInformation>& drawInfo,
    const std::vector<int>& seriousIndexItem,
    int height) {

    // 如果没有严重瑕疵，返回0
    if (seriousIndexItem.empty()) {
        return 0;
    }

    bool hasTopHalf = false;    // 是否存在上半部分的瑕疵
    bool hasBottomHalf = false; // 是否存在下半部分的瑕疵
    int halfHeight = height / 2;

    // 遍历所有严重瑕疵索引
    for (int index : seriousIndexItem) {
        // 确保索引有效
        if (index < 0 || index >= static_cast<int>(drawInfo.size())) {
            continue; // 跳过无效索引
        }

        const drawInformation& defect = drawInfo[index];

        // 计算瑕疵的垂直中心点y坐标
        // 使用realRect的y坐标（矩形左上角）和高度来计算中心点
        int defectCenterY = defect.realRect.y + defect.realRect.height / 2;

        // 判断瑕疵中心点是否在图像上半部分
        if (defectCenterY < halfHeight) {
            hasTopHalf = true;
        }
        else {
            hasBottomHalf = true;
        }

        // 如果已经同时发现上下两部分都有瑕疵，可以提前退出
        if (hasTopHalf && hasBottomHalf) {
            return 3;
        }
    }

    // 根据统计结果返回相应的值
    if (hasTopHalf && !hasBottomHalf) {
        return 2; // 只在上半部分
    }
    else if (!hasTopHalf && hasBottomHalf) {
        return 1; // 只在下半部分
    }
    else if (hasTopHalf && hasBottomHalf) {
        return 3; // 上下都有（虽然理论上已经在循环中返回，但这里作为兜底）
    }
    else {
        return 0; // 没有有效的严重瑕疵
    }
}

void MainProcess::Vision_ComputerThread()
{
    // 登记主线程
    //REGISTER_THREAD_NAME("MainProcess_ComputerThread");

    FILE_LOG_INFO("[MainProcess] ComputerThread Begin.-----------------------------------------");
    //m_isRunning_Compute = true;
    std::vector<drawInformation> currentDrawInfo;
    DataSave dataSave;
    auto resultItem = std::make_shared<QueueDefectItem>();
    //resultItem->timeProduce = dataSave.GetTimeProduce();"%Y%m%d%H%M%S"
    resultItem->timeProduce = QDateTime::currentDateTime().toString("yyyyMMddHHmmss").toStdString();
    FILE_LOG_INFO("ComputerThread Begin: %s", resultItem->timeProduce.c_str());
    //std::string resultPath;
    dataSave.CreatResultDirectory(resultItem->timeProduce, resultItem->resultPath, resultItem->glassIndex);
    bool isSuccess = false;
    /*************计算识别结果***********************/
    //isSuccess = AlgorithmProcess::GetInstance().DetectImages(*resultItem, resultPath);
    isSuccess = AlgorithmProcess::GetInstance().DetectImages_new(*resultItem, resultItem->resultPath);
    if (!isSuccess)
    {
        FILE_LOG_INFO("[MainProcess] ComputerThread Error: AlgorithmDetectImages failed!");

        //SaveErrorImages(*resultItem, resultPath, "AlgorithmDetectImages failed!");  //保存错误数据

        //QString glassStatus = "____-__-__ __:__:__";
        //emit MainProcess::GetInstance().Signal_GlassStatusChanged(GlassStatus::NoGlass, glassStatus);   //在界面显示板进信号。2=无板

        MainProcess::GetInstance().m_isRunning_Compute = false;
        m_isSignalRunning = false;
        FILE_LOG_INFO("[MainProcess] ComputerThread END-----------------------------");

        return;
    }

    /*std::string saveJsonPath = resultPath + "/tmp.json";
    dataSave.SaveDefectInfoToJson(resultItem->drawInfo, resultItem->NgFlag, resultItem->glassPixelWidth, resultItem->glassPixelHeight, saveJsonPath);*/
    FILE_LOG_INFO("[MainProcess] ComputerThread: Compute Completed!");
    /***********************************************/

    /*************处理识别结果***********************/
    isSuccess = MainProcess::GetInstance().Vision_ProcessImage(*resultItem);
    if (!isSuccess)
    {
        FILE_LOG_INFO("[MainProcess] ComputerThread Error: Vision_ProcessImage failed!");

        SaveErrorImages(*resultItem, resultItem->resultPath, "Vision_ProcessImage failed!");  //保存错误数据

        //QString glassStatus = "____-__-__ __:__:__";
        //emit MainProcess::GetInstance().Signal_GlassStatusChanged(GlassStatus::NoGlass, glassStatus);   //在界面显示板进信号。2=无板

        MainProcess::GetInstance().m_isRunning_Compute = false;
        m_isSignalRunning = false;
        FILE_LOG_INFO("[MainProcess] ComputerThread END-----------------------------");

        return;
    }
    FILE_LOG_INFO("[MainProcess] ComputerThread: ProcessResultImages Completed!");
    /***********************************************/

    /*************** 入队/出队 *********************/
    emit Signal_Enqueue(resultItem);
    FILE_LOG_INFO("[MainProcess] ComputerThread: Enqueue Completed!");
    //程序运行第一次得到结果图像时，或勾选自动显示图像时，直接出队显示
    if (MainProcess::GetInstance().m_isFristImage || MainProcess::GetInstance().IsAutoShowImage)
    {
        if (MainProcess::GetInstance().m_isFristImage)
        {
            MainProcess::GetInstance().m_isFristImage = false;
        }
        emit Signal_Dequeue();  //显示图像和数据
        FILE_LOG_INFO("[MainProcess] ComputerThread: Dequeue Completed!");
    }
    /***********************************************/

    //QString glassStatus = "____-__-__ __:__:__";
    //emit MainProcess::GetInstance().Signal_GlassStatusChanged(GlassStatus::NoGlass, glassStatus);   //在界面显示板进信号。1=板出
        /********向控制板卡发送是否Ng信号*************/
    
    int NgFlag = 0;     //算法判断输出结果
	int NgFlag_0 = 1; //左侧玻璃是否NG
	int NgFlag_1 = 1; //右侧玻璃是否NG


    //resultItem->drawInfo[3].ErrorType = DefectLevel::SERIOUS;//模拟左侧
    //resultItem->drawInfo[2].ErrorType = DefectLevel::SERIOUS;//模拟右侧


	//统计严重瑕疵
    std::vector<int> seriousIndexItem;  //传入算法的严重瑕疵的index数组
    if (resultItem->drawInfo.size() > 0)
    {
        for (int i = 0; i < resultItem->drawInfo.size(); i++)
        {
            //20250705: 按照新规则，有严重缺陷的才视为NG
            if (resultItem->drawInfo[i].ErrorType == DefectLevel::SERIOUS)
            {
                seriousIndexItem.push_back(i);
            }
        }
    }
	//添加判断左右玻璃是否NG NgFlag:0-左右无NG，1-左OK 右侧NG，2-左NG,右侧OK，3-左右NG
    NgFlag = checkSeriousDefectsVerticalDistribution(resultItem->drawInfo, seriousIndexItem, resultItem->glassPixelHeight);
    FILE_LOG_INFO("[MainProcess] NgFlag(0-3):%d.", NgFlag);
    //整理判断结果
    if (NgFlag !=0)
    {
        resultItem->NgFlag = 0; //NG, 用于写入数据库
        MainProcess::GetInstance().SetStackLight(2);    //红灯亮
    }
    else
    {
        resultItem->NgFlag = 1; //OK, 用于写入数据库
        MainProcess::GetInstance().SetStackLight(1);    //绿灯亮
    }
    if (SendResults)
    {
        int sendFlag = Platform::Instance().ReturnNgOrOK_0(resultItem->NgFlag); //ngFlag 1：正常，0：NG
        if (sendFlag)
        {
            FILE_LOG_ERROR("Send PlatForm Info failed.");
        }
        else
        {
            FILE_LOG_INFO("Send Platform Completed!value: %d", resultItem->NgFlag);
        }
        
    }
    /***********************************************/

    /************** 保存数据 ***********************/
#if 0
    QSqlDatabase db = MainProcess::createWriteConnectionForCurrentThread();
    if (db.isValid())
    {
        bool isSaveSuccess = dataSave(db, *resultItem, MainProcess::GetInstance().m_customer);
        if (!isSaveSuccess)
        {
            FILE_LOG_ERROR("[MainProcess] ComputerThread Error: Data Save failed!");
            //SaveErrorImages(*resultItem, resultItem->resultPath, "Data_Save failed!");  //保存错误数据
        }
        else
        {
            FILE_LOG_INFO("[MainProcess] ComputerThread: DataSave Completed!");
        }
        db.close();
        QSqlDatabase::removeDatabase(db.connectionName());
    }
	
    emit MainProcess::GetInstance().Slignal_FilterHistoryData();    //刷新界面历史数据
#else 
    FILE_LOG_INFO("Data Save BEGIN_emit: %s", resultItem->timeProduce.c_str());
    if (dataSave.saveData2Files(*resultItem))
    {
        emit Signal_SaveData2DB(resultItem);
    }
#endif  
    //m_isRunning_Compute = false;
    //FileLogPrintf("[MainProcess] ComputerThread Completed.-----------------------------");
    //emit SignalCheckRecipeChanged();//结束运行时，检查配方是否更换，以及检查配方是否更新

    /***********************************************/

    MainProcess::GetInstance().m_isRunning_Compute = false;
    m_isSignalRunning = false;

    if (LOOPTEST)
    {
        Sleep(2000);
        emit Signal_SetVsionGrabbingStatus(true);   //2025.09.06: 用於測試循環讀圖跑

    }

    FILE_LOG_INFO("[MainProcess] ComputerThread Completed.-----------------------------");
}


void MainProcess::SaveErrorImages(QueueDefectItem& resultItem, std::string savePath, std::string errMsg)
{
    FILE_LOG_INFO("[MainProcess] SaveErrorImages: Begin.");

    /****** STEP1: 创建记录数据对应的文件夹**********/
    std::string nameSave = resultItem.timeProduce + "_" + std::to_string(glassIndex);
    std::string recordPath = savePath;


    /****** STEP4: 保存图像PNG/H5、瑕疵json等数据**********/

    ImageSaver* m_imgSaver = nullptr;
    if (m_imgSaver == nullptr)
    {
        m_imgSaver = new ImageSaver();
    }

    if (!resultItem.image_0.empty())
    {
        std::string saveImage = recordPath + "/" + nameSave + "_0.png";
        //cv::imwrite(saveImage, largeImgs[0]);
        m_imgSaver->saveImageAsync(saveImage, resultItem.image_0);       // 异步保存图像
    }
    if (!resultItem.image_1.empty())
    {
        std::string saveImage = recordPath + "/" + nameSave + "_1.png";
        //cv::imwrite(saveImage, largeImgs[1]);
        m_imgSaver->saveImageAsync(saveImage, resultItem.image_1);       // 异步保存图像
    }
    if (!resultItem.image_2.empty())
    {
        std::string saveImage = recordPath + "/" + nameSave + "_2.png";
        //cv::imwrite(saveImage, largeImgs[2]);
        m_imgSaver->saveImageAsync(saveImage, resultItem.image_2);       // 异步保存图像
    }

    if (!resultItem.defectImages_ch0.empty())
    {
        std::string saveDetectImage = recordPath + "/" + nameSave + "_detectImages_0.h5";
        m_imgSaver->saveHDF5(saveDetectImage, resultItem.defectImages_ch0);
    }
    if (!resultItem.defectImages_ch1.empty())
    {
        std::string saveDetectImage = recordPath + "/" + nameSave + "_detectImages_1.h5";
        m_imgSaver->saveHDF5(saveDetectImage, resultItem.defectImages_ch1);
    }
    if (!resultItem.defectImages_ch2.empty())
    {
        std::string saveDetectImage = recordPath + "/" + nameSave + "_detectImages_2.h5";
        m_imgSaver->saveHDF5(saveDetectImage, resultItem.defectImages_ch2);
    }
    if (!resultItem.backgroundImage.empty())
    {
        std::string saveImage = recordPath + "/" + nameSave + "backgroundImage.jpg";
        m_imgSaver->saveImageAsync(saveImage, resultItem.backgroundImage);
    }

    //向文件夹写入错误信息
    // 构造文件路径：folderPath/error.txt
    fs::path filePath = recordPath + "/error.txt";  // 使用 / 操作符拼接路径（跨平台）

    std::ofstream file(filePath);
    if (!file.is_open()) {
        //std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        FILE_LOG_INFO("[MainProcess] SaveErrorImages error: Failed to open file for writing: %s", filePath.c_str());
        return;
    }
    //写入错误信息
    file << errMsg << std::endl;
    file.close();
}


bool MainProcess::CloseLight()
{
    /*****************关闭光源开关(后续亮灯由硬触发自动处理)***************************/
    bool bRst_close = Platform::Instance().SetIOOutState(0, Bit_Output_Open);
    if (!bRst_close)
    {
        FILE_LOG_ERROR(u8"控制卡关闭光源指令发送失败");
    }
    else
    {
        FILE_LOG_INFO(u8"控制卡关闭光源指令发送成功");
    }
    /***********************************************************************************/
	return bRst_close;
}

void MainProcess::Slot_ShowGlassIn()
{
    m_SignalDateTime = QDateTime::currentDateTime();
    m_time_produce_Real = m_SignalDateTime.toString("yyyy-MM-dd HH:mm:ss");
    emit Signal_GlassStatusChanged(GlassStatus::GlassIn, m_time_produce_Real);   //在界面显示玻璃来料状态和处理状态信号。0=板进
}

void MainProcess::Slot_ShowGlassOut()
{
    m_SignalDateTime = QDateTime::currentDateTime();
    m_time_produce_Real = m_SignalDateTime.toString("yyyy-MM-dd HH:mm:ss");
    emit Signal_GlassStatusChanged(GlassStatus::GlassOut, m_time_produce_Real);   //在界面显示玻璃来料状态和处理状态信号。1=板出
}

void MainProcess::Slot_SaveData2DB(const QueueDefectItemPtr& resultItem)
{
    //QueueDefectItem resultItem_save = std::move(*resultItem);
    sql_saveData2DB(*resultItem);
}

void MainProcess::sql_saveData2DB(QueueDefectItem& resultItem)
{
    if (m_db.isValid())
    {
        DataSave dataSave;
        FILE_LOG_INFO("Data Save BEGIN: %s", resultItem.timeProduce.c_str());
        bool isSaveSuccess = dataSave.saveData2Database(m_db, resultItem, MainProcess::GetInstance().m_customer);
        if (!isSaveSuccess)
        {
            FILE_LOG_ERROR("Data Save failed!%s", resultItem.timeProduce.c_str());
            //SaveErrorImages(*resultItem, resultItem->resultPath, "Data_Save failed!");  //保存错误数据
        }
        else
        {
            FILE_LOG_INFO("DataSave Completed!%s", resultItem.timeProduce.c_str());
        }
        //db.close();
        //QSqlDatabase::removeDatabase(db.connectionName());
    }

    emit MainProcess::GetInstance().Slignal_FilterHistoryData();    //刷新界面历史数据
}

/// <summary>
/// 返回软件系统状态给控制卡。
/// 0 = 待机（黄灯亮）
/// 1 = 运行中/检测正常（绿灯亮）
/// 2 = 检测到NG（红灯亮）
/// </summary>
void MainProcess::SetStackLight(int status)
{
	Platform::Instance().SetStackLight(status);
}
