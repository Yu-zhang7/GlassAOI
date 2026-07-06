#include "MainWindow.h"
#include <QtWidgets/QApplication>
#include <dbghelp.h>
//#include "ConfigManager.h"	//读写配置文件
#include "Global.h"
#include "LOG.hpp"
#include "LicenseManager.h"     //授权管理
#include <windows.h>
#include <tchar.h>
#pragma comment(lib, "dbghelp.lib")

QString languageStr;

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ex) {
    HANDLE hFile = CreateFile(L"crash.dmp", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    MINIDUMP_EXCEPTION_INFORMATION info = { GetCurrentThreadId(), ex, FALSE };
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &info, nullptr, nullptr);
    CloseHandle(hFile);
    return EXCEPTION_EXECUTE_HANDLER;
}


// 回调函数：必须返回 LONG
LONG WINAPI CreateFullDump(EXCEPTION_POINTERS* ex) {
    // 生成 dump 文件
    HANDLE hFile = CreateFile(
        _T("full_crash.dmp"),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei = { 0 };
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = ex;
        mei.ClientPointers = TRUE;

        MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
            MiniDumpNormal |
            MiniDumpWithDataSegs |
            MiniDumpWithFullMemoryInfo |
            MiniDumpWithHandleData |
            MiniDumpWithUnloadedModules
            );

        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            dumpType,
            ex ? &mei : nullptr,
            nullptr,
            nullptr
        );
        CloseHandle(hFile);
    }

    // 可选：弹出提示（调试时用，发布版建议去掉）
    // MessageBox(nullptr, _T("程序崩溃，已生成 dump 文件。"), _T("Error"), MB_OK | MB_ICONERROR);

    // 返回 EXCEPTION_EXECUTE_HANDLER 表示处理了异常
    // 但实际上进程仍会终止（因为这是未处理异常）
    return EXCEPTION_EXECUTE_HANDLER;
}

QString ReadLanguageConfig()
{

    QString outPath;
    QSettings settings(
        QString::fromStdString(ConfigPath),         //配置文件路径
        QSettings::IniFormat);

    settings.setIniCodec("UTF-8");

    settings.beginGroup("System");

    languageStr = settings.value("Language").toString();
    settings.endGroup();
    if (languageStr == QString("English"))
    {
        outPath = "./Translation/GlassInspection_en_US.qm";
    }
    else if (languageStr == QString(u8"Русский язык"))
    {
        outPath = "./Translation/GlassInspection_ru_RU.qm";
    }
    else if (languageStr == QString(u8"中文"))
    {
        outPath = "./Translation/GlassInspection_zh_CN.qm";
    }
    else if (languageStr == QString(u8"Español"))  // 西班牙语支持
    {
        outPath = "./Translation/GlassInspection_es_ES.qm";
    }
    else if (languageStr == QString(u8"Português"))  // 葡萄牙语支持
    {
        outPath = "./Translation/GlassInspection_pt-BR.qm";
	}
    else
    {
        outPath = "./Translation/GlassInspection_zh_CN.qm";
    }

    return outPath;

}

void ReadLogLevelConfig()
{

    QString outPath;
    QSettings settings(
        QString::fromStdString(ConfigPath),         //配置文件路径
        QSettings::IniFormat);

    settings.setIniCodec("UTF-8");

    settings.beginGroup("System");
        //读取日志配置
        LogFileLevel = settings.value("LogFileLevel", 2).toInt();
        LogConsoleLevel = settings.value("LogConsoleLevel", 2).toInt();
    settings.endGroup();
}


int main(int argc, char *argv[])
{

    SetUnhandledExceptionFilter(CreateFullDump);  //用于在程序崩溃时，生成dump文件

    SetConsoleOutputCP(CP_UTF8);


    //RegisterConfig();
    QApplication app(argc, argv);

    app.setWindowIcon(QIcon(":/Logo/Logo/Logo_icon.png"));   //注意VS+QT，图标资源不能用bmp图像来生成icon


    // --- 最佳位置：在创建任何窗口/控件之前加载翻译器 ---
    QTranslator translator;
    // 1. 加载默认语言包
    QString path = ReadLanguageConfig();
    if (translator.load(path))
    {
        app.installTranslator(&translator);
    }
    // 根据系统语言加载
    // QString locale = QLocale::system().name(); // e.g., "zh_CN"
    // if (translator.load(QString("myapp_%1").arg(locale), ":/translations/")) {
    //     a.installTranslator(&translator);
    // }

    // 2. 加载 Qt 自带的翻译，用于翻译 Qt 内部字符串 (如标准对话框按钮)
    QTranslator qtTranslator;
    if (qtTranslator.load("qt_" + QLocale::system().name(),
        QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
    {
        app.installTranslator(&qtTranslator);
    }
    if (languageStr == QString(u8"Русский язык"))
    {
        QFont ruFont("Segoe UI", 9);
        // 如果担心字号太小，可以微调：ruFont.setPointSize(9);

        QApplication::setFont(ruFont);
    }
    // --- 翻译器加载结束 ---

    // --- 日志模块初始化 ---
    ReadLogLevelConfig();
            //  LogSystem::Instance().Init();   //老版日志模块，用于追踪崩溃点。有细节改进空间，平时不启用。
        // 转换为 spdlog 枚举类型
        auto file_level = static_cast<spdlog::level::level_enum>(LogFileLevel);
        auto console_level = static_cast<spdlog::level::level_enum>(LogConsoleLevel);

        // 初始化日志模块
        LoggerManager::instance().init("GlassApp", file_level, console_level);

        // 测试日志输出
        FILE_LOG_INFO("Logger initialized with file level: %d, console level: %d",
            LogFileLevel, LogConsoleLevel);
    // --- 日志模块初始化结束 ---


    // --- 验证授权系统 ---
    LicenseManager* licenseManager = LicenseManager::getInstance();

    if (!licenseManager->validateLicenseStatus())
    {
        LicenseManager::releaseInstance();
        return -1;
    }

    //LicenseManager::getInstance()->exportLicenseInfo();

    // --- 验证授权系统结束 ---


    MainWindow window;
    //window.show();
    window.showMaximized();

    int result = app.exec();

    FILE_LOG_INFO(u8"程序正常关闭");
    // 在返回前关闭日志系统
    LoggerManager::instance().shutdown();

    return result;
}


//int main() {
//
//    std::cout << "Hello World!" << std::endl;
//
//    //ImageBeautify imageBeautify;
//    //imageBeautify.mainTest();
//
//      //HDF5Utils hdf5Utils;
//   //   hdf5Utils.mainTest();
//
//    //Demo demo;
//    //demo.mainTest();
//
//    //detect detect;
//    //detect.mainTest();
//
//    //segImage segImg;
//    //segImg.mainTest();
//
//    //detectGlass detectGlass;
//    //detectGlass.mainTest();
//
//
//
//    ImageBeautify imageBeautify;
//    // 测试参数
//    int width = 4096 * 1;      // 2个相机，每个4096像素
//    int height = 300;          // 高度
//    int cameraCol = 4096;      // 每个相机的列数
//    int numCameras = 1;        // 相机数量
//
//    // 生成裁剪范围（每个相机裁剪中间3000像素）
//    std::vector<std::vector<float>> start_end = {
//        {0, 0},                // 索引0不使用
//        {0, 4095},           // 相机1：从548到3548
//        {548, 3548}            // 相机2：从548到3548
//    };
//
//   /* std::cout << "生成三光源测试图像..." << std::endl;
//    cv::Mat testImage3 = generateTestImage(width, height, cameraCol, numCameras, 0);
//    cv::imwrite("test_image_3ch.png", testImage3);
//
//    std::cout << "生成双光源测试图像..." << std::endl;
//    cv::Mat testImage2 = generateTestImage(width, height, cameraCol, numCameras, 2);
//    cv::imwrite("test_image_2ch.png", testImage2);*/
//
//    // 测试三通道拆分函数
//    //std::cout << "\n测试三通道拆分函数..." << std::endl;
//    //// 这里需要调用你的 divImgChannel3 函数
//    //std::vector<cv::Mat> result3 = imageBeautify.divImgChannel3(testImage3, start_end, cameraCol);
//
//    cv::Mat testImage2 = cv::imread("E:\\claude\\拆分demo\\Image_20260329152230135.bmp");
//    // 测试双通道拆分函数
//    std::cout << "\n测试双通道拆分函数..." << std::endl;
//    // 这里需要调用你的 divImgChannel2 函数
//    std::vector<cv::Mat> result2 = imageBeautify.divImgChannel2(testImage2, start_end, cameraCol);
//
//
//    return 0;
//
//
//}
