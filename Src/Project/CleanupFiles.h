/************************************************************************
Copyright (C), Your Company
File name: 文件定时清理器
Author: Sail(Yang Fan)
Version: V2.1.0
Date: 2025.12.28
Description: 定时清理历史文件，支持磁盘空间监控和多文件类型
Others:
Function List:
    1. SetConfig: 设置清理器参数，及启动清理线程
    2. SetFileFilters: 设置要清理的文件类型
    3. SetDiskSpaceEmergency: 设置磁盘空间紧急清理
    4. SetCleanupTimeoutState: 启动或停止清理线程
    5. GetDiskSpaceInfo: 获取磁盘空间信息
    6. TriggerEmergencyCleanup: 手动触发紧急清理（用于测试）
    7. AdaptiveThrottle: 预留动态资源占用调整
    8. GetSystemLoad: 获取系统负载
    9. ProcessScheduledCleanup: 处理定时清理模式
    10. DeleteOldestSubfolder: 删除最旧的子文件夹
    11. GetFolderSize: 获取文件夹大小
    12. GetAllSubfoldersSortedByAge: 获取所有子文件夹，按创建时间排序
    13. DeleteFilesInDirectory: 删除指定目录下的文件
    14. GetAllFilesInDirectory: 获取指定目录下的所有文件
Example:
    // 获取单例实例
    CleanupFiles& cleaner = CleanupFiles::GetInstance();

    // 需求1 & 3：设置主目录、每日清理时间、保留天数
    cleaner.SetConfig(
        "D:/result",      // 主目录 - 结果保存位置
        true,             // 启用自动清理
        30,               // 保留30天
        3                 // 每天凌晨3点清理
    );

    // 需求3：设置清理的文件类型
    std::vector<std::string> fileTypes = {
        ".png", ".jpg", ".jpeg", ".bmp", ".gif",  // 图片文件
        ".log", ".txt", ".tmp", ".bak", ".old",   // 日志和临时文件
        ".avi", ".mp4", ".mov", ".wmv",           // 视频文件
        ".dat", ".dmp", ".err"                    // 数据文件
    };
    cleaner.SetFileFilters(fileTypes);

    // 需求2：设置磁盘空间紧急清理（5GB阈值）
    cleaner.SetDiskSpaceEmergency(true, 5.0);

    // 需求4：程序初始化后，自动开始清理
    // SetConfig中已经调用了SetCleanupTimeoutState(true)，所以清理线程会自动启动

    // 显示磁盘空间信息
    std::cout << cleaner.GetDiskSpaceInfo() << std::endl;

    // 保持程序运行（在实际应用中，这里会是你的主程序逻辑）
    std::cout << "清理系统已启动，按任意键停止..." << std::endl;
    std::cin.get();

    // 停止清理系统
    cleaner.SetCleanupTimeoutState(false);

************************************************************************/
#pragma once
#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>
#include <windows.h>
#include <mutex>
#include <algorithm>
#include <cstdlib>
#include <atomic>
#include <thread>
#include <vector>
#include <set>
#include <queue>
#include <QStorageInfo>
#include <QString>
#include <QDebug>
#include <chrono>

class CleanupFiles
{
public:
    CleanupFiles() = default;
    ~CleanupFiles();

    // Meyer's Singleton 单例模式
    static CleanupFiles& GetInstance() {
        static CleanupFiles instance;
        return instance;
    }

    // 删除拷贝构造函数和赋值操作符
    CleanupFiles(const CleanupFiles&) = delete;
    CleanupFiles& operator=(const CleanupFiles&) = delete;

    /// <summary>
    /// 自动清理设置
    /// </summary>
    /// <param name="mainPath">清理的主目录</param>
    /// <param name="isUsedCleanup">是否使用自动清理功能</param>
    /// <param name="retentionDays">自动清理的保留天数</param>
    /// <param name="cleanupHour">每日自动清理的时段</param>
    void SetConfig(const std::string& mainPath, bool isUsedCleanup = true,
        unsigned int retentionDays = 30, unsigned int cleanupHour = 12);

    /// <summary>
    /// 设置文件过滤器（支持多后缀）
    /// </summary>
    /// <param name="extensions">文件后缀列表，如 {".png", ".jpg", ".tmp"}</param>
    void SetFileFilters(const std::vector<std::string>& extensions);



    /// <summary>
    /// 设置磁盘空间紧急清理
    /// </summary>
    /// <param name="enable">是否启用</param>
    /// <param name="thresholdGB">触发清理的阈值(GB)</param>
	/// <param name="deleteThresholdGB">删除到的目标阈值(GB)</param>
    void SetDiskSpaceEmergency(bool enable, double checkThresholdGB = 5.0, double deleteThresholdGB = 10.0);

    /// <summary>
    /// 设置自动清理功能状态
    /// </summary>
    /// <param name="enabled">清理状态</param>
    void SetCleanupTimeoutState(bool enabled);

    /// <summary>
    /// 获取磁盘空间信息
    /// </summary>
    std::string GetDiskSpaceInfo() const;

    /// <summary>
    /// 手动触发紧急清理（用于测试）
    /// </summary>
    void TriggerEmergencyCleanup();

    // 消息回调函数类型
    using MessageCallback = std::function<void(const std::string&, const std::string&)>;

    /// <summary>
    /// 设置消息回调函数
    /// </summary>
    /// <param name="callback">消息回调函数</param>
    void SetMessageCallback(MessageCallback callback);

private:
    void CleanupTimeoutFiles();
    void CleanupTimeoutFilesFunction();

    // 处理不同清理模式
    void ProcessScheduledCleanup(const std::filesystem::path& mainPath,
        const std::filesystem::file_time_type& cleanupTime);
    void ProcessEmergencyCleanup_old();
    void ProcessEmergencyCleanup();

    // 辅助函数
    bool CheckDiskSpace(bool bValue = false) const;
    bool ShouldDeleteFile(const std::filesystem::path& filePath) const;
    void DeleteFilesInDirectory(const std::filesystem::path& dir,
        const std::filesystem::file_time_type& cleanupTime);

    // 紧急清理专用函数
    bool DeleteOldestSubfolder(uintmax_t* folderSize = nullptr);
    std::vector<std::filesystem::path> GetAllSubfoldersSortedByAge() const;
    uintmax_t GetFolderSize(const std::filesystem::path& folderPath) const;

    // 修正：Log函数添加const修饰符
    void Log(const std::string& message) const;

    // 线程控制
    void StopCleanupThread();
    void WaitForThreadExit();

    //********预留动态资源占用调整
    void AdaptiveThrottle(int& processedCount, int& sleepTime);
    double GetSystemLoad();
    //*****************

private:
    std::string m_mainPath = "";
    bool m_isUsedCleanup = false;
    int m_filterDays = 30;
    int m_filterHours = 12;
    std::atomic<bool> m_isExit{ false };
    std::atomic<bool> m_isClearRun{ false };

    // 文件过滤器
    std::set<std::string> m_fileExtensions;
    bool m_filterAllFiles = false;  // 如果扩展名列表为空，则清理所有文件

    // 回调函数
    MessageCallback m_messageCallback = nullptr;

    // 磁盘空间紧急清理
    bool m_enableDiskEmergency = false;
    double m_emergencyThresholdGB = 5.0;
	double m_deleteTargetThresholdGB = 10.0;

    // 自适应节流控制
    static constexpr int BASE_INTERVAL = 50;  // 每50个文件检查一次
    static constexpr int MAX_SLEEP_MS = 100;   // 最大休眠时间
    static constexpr int MIN_SLEEP_MS = 1;     // 最小休眠时间

    // 线程句柄
    std::thread m_cleanupThread;

};