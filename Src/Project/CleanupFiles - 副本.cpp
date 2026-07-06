#include "CleanupFiles.h"
#include <filesystem>
#include <thread>
#include <chrono>
#include <vector>
#include <queue>
#include <functional>
#include <iomanip>
#include <sstream>

CleanupFiles::~CleanupFiles()
{
    StopCleanupThread();
    WaitForThreadExit();
    Log("CLEANUP_THREAD EXIT");
}

void CleanupFiles::SetConfig(const std::string& mainPath, bool isUsedCleanup,
    unsigned int retentionDays, unsigned int cleanupHour)
{
    m_mainPath = mainPath;
    m_filterDays = retentionDays;
    m_filterHours = cleanupHour;
    m_isUsedCleanup = isUsedCleanup;

    SetCleanupTimeoutState(isUsedCleanup);
}

void CleanupFiles::SetFileFilters(const std::vector<std::string>& extensions)
{
    m_fileExtensions.clear();
    for (const auto& ext : extensions) {
        std::string lowerExt = ext;
        std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
            [](unsigned char c) { return std::tolower(c); });

        // 确保扩展名以点开头
        if (!lowerExt.empty() && lowerExt[0] != '.') {
            lowerExt = "." + lowerExt;
        }

        if (!lowerExt.empty()) {
            m_fileExtensions.insert(lowerExt);
        }
    }

    m_filterAllFiles = m_fileExtensions.empty();
    Log(u8"文件过滤器已更新: " + std::to_string(m_fileExtensions.size()) + u8" 种扩展名");
}

void CleanupFiles::SetDiskSpaceEmergency(bool enable, double thresholdGB)
{
    m_enableDiskEmergency = enable;
    m_emergencyThresholdGB = thresholdGB;

    Log(std::string(u8"磁盘空间紧急清理 ") + (enable ? u8"已启用" : u8"已禁用") +
        u8" (阈值: " + std::to_string(thresholdGB) + " GB)");
}

void CleanupFiles::CleanupTimeoutFiles()
{
    if (m_isClearRun)
    {
        return;
    }

    m_isExit = false;
    m_isClearRun = true;

    // 创建清理线程
    m_cleanupThread = std::thread(&CleanupFiles::CleanupTimeoutFilesFunction, this);
}

void CleanupFiles::StopCleanupThread()
{
    m_isExit = true;
}

void CleanupFiles::WaitForThreadExit()
{
    if (m_cleanupThread.joinable()) {
        m_cleanupThread.join();
    }
}

void CleanupFiles::CleanupTimeoutFilesFunction()
{
    Log("CLEANUP_TIMEOUT_FILE_THREAD_START");
    std::filesystem::path mainPath(m_mainPath);

    // 检查主路径有效性
    if (!std::filesystem::exists(mainPath))
    {
        Log(u8"路径不存在: " + m_mainPath);
        m_isClearRun = false;
        return;
    }
    if (!std::filesystem::is_directory(mainPath))
    {
        Log(u8"路径不是目录: " + m_mainPath);
        m_isClearRun = false;
        return;
    }

    // 配置参数
    const int filterHour = m_filterHours;
    const int filterDays = m_filterDays;
    const auto timeoutDuration = std::chrono::hours(filterDays * 24);
    bool isRunCleanup = false;

    // 紧急检查计数器
    int emergencyCheckCounter = 0;

    while (!m_isExit)
    {
        // 1. 检查磁盘空间（紧急清理）- 每分钟检查一次
        emergencyCheckCounter++;
        if (emergencyCheckCounter >= 60) // 每分钟检查一次
        {
            if (m_enableDiskEmergency && !CheckDiskSpace())
            {
                Log(u8"磁盘空间不足，触发紧急清理");
                m_messageCallback("EMERGENCY_CLEANUP", u8"磁盘空间不足，触发紧急清理");
                ProcessEmergencyCleanup();
            }
            emergencyCheckCounter = 0;
        }

        // 2. 检查是否到达计划清理时间
        const auto now = std::chrono::system_clock::now();
        const std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm ttm;
        localtime_s(&ttm, &t);
        const int currentHour = ttm.tm_hour;

        if (currentHour != filterHour)
        {
            if (isRunCleanup)
            {
                isRunCleanup = false;
                Log(u8"计划清理时间段结束");
                
            }
            // 等待1分钟或直到退出信号
            for (int i = 0; i < 60 && !m_isExit; ++i)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            continue;
        }

        if (isRunCleanup)
        {
            // 等待1小时或直到退出信号
            for (int i = 0; i < 60 && !m_isExit; ++i)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            continue;
        }

        Log(u8"RUN_CLEANUP_BEGIN - 开始计划清理");
        m_messageCallback("RUN_CLEANUP_BEGIN", u8"开始计划清理");
        isRunCleanup = true;

        try
        {
            const auto cleanupTime = std::filesystem::file_time_type::clock::now() - timeoutDuration;

            // 执行计划清理
            ProcessScheduledCleanup(mainPath, cleanupTime);

            Log(u8"RUN_CLEANUP_OVER - 计划清理完成");
            m_messageCallback("RUN_CLEANUP_OVER", u8"计划清理完成");
        }
        catch (const std::exception& e)
        {
            Log(u8"计划清理错误: " + std::string(e.what()));
            m_messageCallback("CLEANUP_ERROR", std::string(e.what()));
        }
    }

    m_isClearRun = false;
    Log("CLEANUP_TIMEOUT_FILE_THREAD_STOP");
}

void CleanupFiles::ProcessScheduledCleanup(const std::filesystem::path& mainPath,
    const std::filesystem::file_time_type& cleanupTime)
{
    int processedCount = 0;
    int deletedCount = 0;
    uintmax_t totalFreedBytes = 0;

    // 处理主目录下的文件
    for (const auto& entry : std::filesystem::directory_iterator(mainPath))
    {
        if (m_isExit) break;

        processedCount++;
        // 每处理100个条目短暂休眠
        if (processedCount % 100 == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (m_isExit)
            {
                break;
            }
        }

        try
        {
            if (!entry.exists())
            {
                continue;
            }

            const auto& path = entry.path();

            // // 处理主目录下的超期文件
            if (std::filesystem::is_regular_file(path))
            {
                if (entry.last_write_time() < cleanupTime && ShouldDeleteFile(path))
                {
                    uintmax_t fileSize = 0;
                    try
                    {
                        fileSize = std::filesystem::file_size(path);
                    }
                    catch (...)
                    {
                    }

                    if (std::filesystem::remove(path))
                    {
                        deletedCount++;
                        totalFreedBytes += fileSize;
                    }
                }
            }
            // 处理子目录
            else if (std::filesystem::is_directory(path))
            {
                DeleteFilesInDirectory(path, cleanupTime);
            }
        }
        catch (const std::exception& e)
        {
            Log(u8"处理错误: " + entry.path().string() + " - " + e.what());
        }
    }

    if (deletedCount > 0) {
        double freedMB = totalFreedBytes / (1024.0 * 1024);
        Log(u8"计划清理完成: 删除了 " + std::to_string(deletedCount) +
            u8" 个文件，释放了 " + std::to_string(freedMB) + u8" MB 空间");
        m_messageCallback("DEL_CONUT", u8"" + std::to_string(deletedCount) +
            "," + std::to_string(totalFreedBytes / (1024.0 * 1024)));
    }
}

void CleanupFiles::ProcessEmergencyCleanup()
{
    if (!m_enableDiskEmergency || m_mainPath.empty())
    {
        return;
    }

    Log(u8"EMERGENCY_CLEANUP_START - 磁盘空间不足，开始删除整个子文件夹");

    // 获取初始磁盘空间
    double initialFreeGB = 0;
    try
    {
        QStorageInfo storage(QString::fromStdString(m_mainPath));
        if (storage.isValid())
        {
            initialFreeGB = storage.bytesAvailable() / (1024.0 * 1024 * 1024);
        }
    }
    catch (...)
    {
    }

    // 持续清理直到空间足够
    int foldersDeleted = 0;
    uintmax_t totalFreedBytes = 0;
    bool spaceEnough = false;

    while (!m_isExit && !spaceEnough)
    {
        // 尝试删除最旧的子文件夹
        uintmax_t folderSize = 0;
        bool deleted = DeleteOldestSubfolder(&folderSize);

        if (deleted)
        {
            foldersDeleted++;
            totalFreedBytes += folderSize;

            // 检查磁盘空间
            spaceEnough = CheckDiskSpace();

            if (!spaceEnough)
            {
                Log(u8"已删除 " + std::to_string(foldersDeleted) +
                    u8" 个子文件夹，继续清理...");
            }
        }
        else
        {
            Log(u8"没有更多子文件夹可删除");
            break;
        }

        // 短暂暂停
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 获取最终磁盘空间
    double finalFreeGB = 0;
    try
    {
        QStorageInfo storage(QString::fromStdString(m_mainPath));
        if (storage.isValid())
        {
            finalFreeGB = storage.bytesAvailable() / (1024.0 * 1024 * 1024);
        }
    }
    catch (...)
    {
    }

    if (foldersDeleted > 0)
    {
        double freedGB = totalFreedBytes / (1024.0 * 1024 * 1024);
        double increasedGB = finalFreeGB - initialFreeGB;

        Log(u8"紧急清理完成: 共删除 " + std::to_string(foldersDeleted) +
            u8" 个子文件夹，释放了 " + std::to_string(freedGB) + u8" GB 空间");
        Log(u8"磁盘空间变化: 从 " + std::to_string(initialFreeGB) + u8" GB 增加到 " +
            std::to_string(finalFreeGB) + u8" GB (增加了 " +
            std::to_string(increasedGB) + u8" GB)");
    }
    else
    {
        Log(u8"紧急清理未删除任何文件夹");
    }
}

bool CleanupFiles::CheckDiskSpace() const
{
    if (!m_enableDiskEmergency || m_mainPath.empty())
    {
        return true; // 未启用或路径为空，视为空间足够
    }

    try
    {
        QStorageInfo storage(QString::fromStdString(m_mainPath));

        if (!storage.isValid()) {
            Log(u8"警告: 无法获取磁盘空间信息");
            return true; // 如果无法获取信息，允许继续
        }

        qint64 bytesAvailable = storage.bytesAvailable();
        qint64 bytesTotal = storage.bytesTotal();
        double freeGB = bytesAvailable / (1024.0 * 1024 * 1024);
        double totalGB = bytesTotal / (1024.0 * 1024 * 1024);
        double usedPercent = bytesTotal > 0 ?
            (100.0 * (bytesTotal - bytesAvailable) / bytesTotal) : 0.0;

        bool isEnough = freeGB >= m_emergencyThresholdGB;

        return isEnough;

    }
    catch (const std::exception& e) {
        Log(u8"错误: 磁盘空间检查失败: " + std::string(e.what()));
        return true; // 出错时允许继续
    }
}

bool CleanupFiles::ShouldDeleteFile(const std::filesystem::path& filePath) const
{
    if (!std::filesystem::is_regular_file(filePath)) {
        return false;
    }

    // 如果未设置文件类型过滤器，删除所有文件
    if (m_filterAllFiles) {
        return true;
    }

    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return std::tolower(c); });

    return m_fileExtensions.find(ext) != m_fileExtensions.end();
}

void CleanupFiles::DeleteFilesInDirectory(const std::filesystem::path& dir,
    const std::filesystem::file_time_type& cleanupTime)
{
    std::vector<std::filesystem::path> dirStack;
    dirStack.push_back(dir);
    int fileCount = 0;
    int deletedCount = 0;

    while (!dirStack.empty() && !m_isExit)
    {
        auto currentDir = dirStack.back();
        dirStack.pop_back();

        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(currentDir))
            {
                if (m_isExit) break;

                try
                {
                    // 收集子目录
                    if (entry.is_directory())
                    {
                        dirStack.push_back(entry.path());
                        continue;
                    }

                    // 只处理普通文件
                    if (!entry.is_regular_file()) continue;

                    // 定期检查退出标志
                    if (++fileCount % 50 == 0)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        if (m_isExit) break;
                    }

                    // 检查文件和时间
                    if (ShouldDeleteFile(entry.path()) && entry.last_write_time() < cleanupTime)
                    {
                        if (std::filesystem::remove(entry.path())) {
                            deletedCount++;
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    Log(u8"文件处理错误: " + entry.path().string() + " - " + e.what());
                }
            }
        }
        catch (const std::exception& e)
        {
            Log(u8"目录错误: " + currentDir.string() + " - " + e.what());
        }
    }
}

std::vector<std::filesystem::path> CleanupFiles::GetAllSubfoldersSortedByAge() const
{
    std::vector<std::pair<std::filesystem::path, std::filesystem::file_time_type>> foldersWithTime;

    try {
        std::filesystem::path mainPath(m_mainPath);

        if (!std::filesystem::exists(mainPath) || !std::filesystem::is_directory(mainPath)) {
            return {};
        }

        // 收集所有子文件夹及其修改时间
        for (const auto& entry : std::filesystem::directory_iterator(mainPath)) {
            if (entry.is_directory()) {
                try {
                    auto lastWriteTime = entry.last_write_time();
                    foldersWithTime.emplace_back(entry.path(), lastWriteTime);
                }
                catch (...) {
                    // 忽略无法获取时间的文件夹
                }
            }
        }

        // 按修改时间排序（从旧到新）
        std::sort(foldersWithTime.begin(), foldersWithTime.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second; // 时间早的在前
            });

        // 提取路径
        std::vector<std::filesystem::path> sortedPaths;
        for (const auto& pair : foldersWithTime) {
            sortedPaths.push_back(pair.first);
        }

        return sortedPaths;

    }
    catch (const std::exception& e) {
        Log(u8"获取子文件夹列表错误: " + std::string(e.what()));
        return {};
    }
}

uintmax_t CleanupFiles::GetFolderSize(const std::filesystem::path& folderPath) const
{
    uintmax_t totalSize = 0;

    try {
        if (!std::filesystem::exists(folderPath) || !std::filesystem::is_directory(folderPath)) {
            return 0;
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(folderPath)) {
            if (entry.is_regular_file()) {
                try {
                    totalSize += entry.file_size();
                }
                catch (...) {
                    // 忽略无法获取大小的文件
                }
            }
        }
    }
    catch (...) {
        // 忽略所有错误
    }

    return totalSize;
}

bool CleanupFiles::DeleteOldestSubfolder(uintmax_t* folderSize)
{
    if (folderSize) *folderSize = 0;

    try {
        // 获取所有子文件夹（按时间排序）
        auto sortedFolders = GetAllSubfoldersSortedByAge();

        if (sortedFolders.empty()) {
            return false;
        }

        // 获取最旧的文件夹
        const std::filesystem::path& oldestFolder = sortedFolders.front();
        std::string folderName = oldestFolder.filename().string();

        // 计算文件夹大小
        uintmax_t size = 0;
        try {
            size = GetFolderSize(oldestFolder);
        }
        catch (...) {
            // 如果无法计算大小，继续尝试删除
        }

        if (folderSize) *folderSize = size;

        // 删除整个文件夹
        uintmax_t removedCount = std::filesystem::remove_all(oldestFolder);

        if (removedCount > 0) {
            // 简化日志输出
            double sizeMB = size / (1024.0 * 1024);
            Log(u8"紧急清理删除子文件夹: " + folderName +
                u8" (大小: " + std::to_string(static_cast<int>(sizeMB)) + u8" MB)");
            return true;
        }

    }
    catch (const std::exception& e) {
        Log(u8"删除子文件夹错误: " + std::string(e.what()));
    }

    return false;
}

void CleanupFiles::SetCleanupTimeoutState(bool enabled)
{
    m_isUsedCleanup = enabled;
    if (enabled && !m_isClearRun)
    {
        CleanupTimeoutFiles();
        Log("CLEANUP_SERVICE_ACTIVATED");
    }
    else if (!enabled)
    {
        StopCleanupThread();
        WaitForThreadExit();
        Log("CLEANUP_SERVICE_DEACTIVATED");
    }
}

std::string CleanupFiles::GetDiskSpaceInfo() const
{
    if (m_mainPath.empty()) {
        return u8"路径未设置";
    }

    try {
        QStorageInfo storage(QString::fromStdString(m_mainPath));

        if (!storage.isValid()) {
            return u8"无法获取磁盘空间信息";
        }

        qint64 bytesAvailable = storage.bytesAvailable();
        qint64 bytesTotal = storage.bytesTotal();
        double freeGB = bytesAvailable / (1024.0 * 1024 * 1024);
        double totalGB = bytesTotal / (1024.0 * 1024 * 1024);
        double usedPercent = bytesTotal > 0 ?
            (100.0 * (bytesTotal - bytesAvailable) / bytesTotal) : 0.0;

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << u8"磁盘空间信息:\n";
        ss << u8"  路径: " << storage.rootPath().toStdString() << "\n";
        ss << u8"  总量: " << totalGB << u8" GB\n";
        ss << u8"  可用: " << freeGB << u8" GB\n";
        ss << u8"  使用率: " << usedPercent << u8"%\n";

        if (m_enableDiskEmergency) {
            ss << u8"  紧急阈值: " << m_emergencyThresholdGB << u8" GB\n";
            ss << u8"  状态: " << (freeGB < m_emergencyThresholdGB ? u8"空间不足" : u8"正常");
        }

        return ss.str();

    }
    catch (const std::exception& e) {
        return u8"错误: " + std::string(e.what());
    }
}

void CleanupFiles::TriggerEmergencyCleanup()
{
    if (!m_isClearRun) {
        Log(u8"手动触发紧急清理失败：清理服务未运行");
        return;
    }

    Log(u8"手动触发紧急清理");
    ProcessEmergencyCleanup();
}

// 修正：添加const修饰符
void CleanupFiles::Log(const std::string& message) const
{
    static std::mutex logMutex;
    std::lock_guard<std::mutex> lock(logMutex);

    // 获取当前时间
    SYSTEMTIME st;
    GetLocalTime(&st);
    char timestamp[32];
    sprintf_s(timestamp, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    // 控制台输出
    std::cout << '[' << timestamp << "][CLN] " << message << std::endl;

    // 文件日志
    std::ofstream logFile("CleanupFiles.log", std::ios::app);
    if (logFile) {
        logFile << '[' << timestamp << "][CLN] " << message << std::endl;
    }
}

void CleanupFiles::AdaptiveThrottle(int& processedCount, int& sleepTime)
{
    if (++processedCount % BASE_INTERVAL == 0)
    {
        // 动态调整休眠时间
        sleepTime = std::clamp(sleepTime, MIN_SLEEP_MS, MAX_SLEEP_MS);

        // 短暂休眠释放CPU
        auto start = std::chrono::steady_clock::now();
        while (!m_isExit &&
            std::chrono::steady_clock::now() - start <
            std::chrono::milliseconds(sleepTime))
        {
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }

        // 根据系统负载动态调整
        if (GetSystemLoad() > 70.0) {  // 如果系统负载高
            sleepTime = std::min(sleepTime + 1, MAX_SLEEP_MS);
        }
        else {
            sleepTime = std::max(sleepTime - 1, MIN_SLEEP_MS);
        }
    }
}

double CleanupFiles::GetSystemLoad()
{
    // 简化的系统负载检测（Windows）
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
    {
        ULARGE_INTEGER idle, kernel, user;
        idle.LowPart = idleTime.dwLowDateTime;
        idle.HighPart = idleTime.dwHighDateTime;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;

        uint64_t total = kernel.QuadPart + user.QuadPart;
        if (total > 0) {
            return 100.0 - (100.0 * idle.QuadPart / total);
        }
    }
    return 50.0; // 默认值
}

void CleanupFiles::SetMessageCallback(MessageCallback callback)
{
    m_messageCallback = callback;
}