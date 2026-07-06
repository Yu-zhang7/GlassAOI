#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/details/log_msg.h>

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <filesystem>
#include <thread>
#include <sstream>
#include <cstdio>
#include <cstdarg>
#include <fstream>
#include <atomic>
#include <cstdlib>
#include <exception>

// 辅助：将 std::thread::id 转为字符串（用于 %s）
inline std::string to_string(std::thread::id id) {
    std::ostringstream ss;
    ss << id;
    return ss.str();
}

// ========== 宏定义：{} 风格（推荐）==========

/* LEVEL_0: 跟踪信息 */
#define FILE_TRACE(...)    LoggerManager::instance().trace(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)

/* LEVEL_1: 调试信息 */
#define FILE_DEBUG(...)    LoggerManager::instance().debug(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)

/* LEVEL_2: 常规信息 */
#define FILE_INFO(...)     LoggerManager::instance().info(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)

/* LEVEL_3: 警告信息 */
#define FILE_WARN(...)     LoggerManager::instance().warn(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)

/* LEVEL_4: 错误信息 */
#define FILE_ERROR(...)    LoggerManager::instance().error(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)

/* LEVEL_5: 严重错误 */
#define FILE_CRITICAL(...) LoggerManager::instance().critical(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)

// ========== 宏定义：% 风格（printf-style）==========
/* LEVEL_0: 跟踪信息 */
#define FILE_LOG_TRACE(fmt, ...)    LoggerManager::instance().trace_fmt(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, fmt, ##__VA_ARGS__)

/* LEVEL_1: 调试信息 */
#define FILE_LOG_DEBUG(fmt, ...)    LoggerManager::instance().debug_fmt(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, fmt, ##__VA_ARGS__)

/* LEVEL_2: 常规信息 */
#define FILE_LOG_INFO(fmt, ...)     LoggerManager::instance().info_fmt(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, fmt, ##__VA_ARGS__)

/* LEVEL_3: 警告信息 */
#define FILE_LOG_WARN(fmt, ...)     LoggerManager::instance().warn_fmt(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, fmt, ##__VA_ARGS__)

/* LEVEL_4: 错误信息 */
#define FILE_LOG_ERROR(fmt, ...)    LoggerManager::instance().error_fmt(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, fmt, ##__VA_ARGS__)

/* LEVEL_5: 严重错误 */
#define FILE_LOG_CRITICAL(fmt, ...) LoggerManager::instance().critical_fmt(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, fmt, ##__VA_ARGS__)

class LoggerManager {
public:
    static LoggerManager& instance() {
        static LoggerManager inst;
        return inst;
    }

    void init(const std::string& logger_name = "app",
        spdlog::level::level_enum file_level = spdlog::level::info,
        spdlog::level::level_enum console_level = spdlog::level::info) {
        std::lock_guard<std::mutex> lock(init_mutex_);

        if (initialized_.load(std::memory_order_acquire)) {
            fprintf(stderr, "LoggerManager already initialized\n");
            return;
        }

        try {
            // 清理现有资源
            cleanupExistingLogger();

            // 确保日志目录存在
            createLogDirectory();

            // 日志文件：log/yyyyMMdd.log
            std::string filename_pattern = "log/Glass.log";

            // ====== spdlog日志的信息级别(在set_level进行设置) ====== 
            /*
                trace           = 0,        // 最低级别，最详细
                debug           = 1,        // 调试信息
                info            = 2,        // 常规信息
                warn            = 3,        // 警告信息
                err             = 4,        // 错误信息
                critical        = 5,        // 严重错误
                off             = 6,        // 关闭所有日志
                n_levels                    // 级别数量
            */

            // Daily file sink（每天 00:00 轮转）
            auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                filename_pattern, 0, 0);
            file_sink->set_level(file_level);

            // 彩色控制台 sink
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(console_level);

            sinks_.clear();
            sinks_.push_back(file_sink);
            sinks_.push_back(console_sink);

            logger_ = std::make_shared<spdlog::logger>(logger_name, sinks_.begin(), sinks_.end());
            logger_->set_level(spdlog::level::trace); // 实际由 sink 控制
            logger_->flush_on(spdlog::level::warn);

            // 重要：不设置默认logger，避免全局注册表问题
            // spdlog::set_default_logger(logger_);

            // 设置日志格式：包含文件名、行号和函数名
            // %s = 源文件名, %# = 行号, 
            // %! = 函数名(通过SPDLOG_FUNCTION函数获得。显示格式为"ClassName::FunctionName")
            // %v = 日志消息, %l = 日志级别
            logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%!] %v");

            file_level_ = file_level;
            console_level_ = console_level;

            // 标记为已初始化
            initialized_.store(true, std::memory_order_release);

        }
        catch (const std::exception& ex) {
            // 初始化失败，重置标志
            initialized_.store(false, std::memory_order_release);
            fprintf(stderr, "LoggerManager::init failed: %s\n", ex.what());
            throw;
        }
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(init_mutex_);
        performShutdown();
    }

    // 安全的关闭方法，不抛出异常
    void safeShutdown() noexcept {
        try {
            std::lock_guard<std::mutex> lock(init_mutex_);
            performShutdown();
        }
        catch (const std::exception& e) {
            // 在退出时忽略所有异常，但记录错误信息
            fprintf(stderr, "LoggerManager::safeShutdown failed: %s\n", e.what());
        }
        catch (...) {
            // 捕获所有其他异常
            fprintf(stderr, "LoggerManager::safeShutdown failed with unknown exception\n");
        }
    }

    void setLogLevel(spdlog::level::level_enum level) {
        setFileLevel(level);
        setConsoleLevel(level);
    }

    void setFileLevel(spdlog::level::level_enum level) {
        std::lock_guard<std::mutex> lock(level_mutex_);
        if (!initialized_.load(std::memory_order_acquire) || !logger_) return;
        if (!sinks_.empty()) sinks_[0]->set_level(level);
        file_level_ = level;
    }

    void setConsoleLevel(spdlog::level::level_enum level) {
        std::lock_guard<std::mutex> lock(level_mutex_);
        if (!initialized_.load(std::memory_order_acquire) || !logger_) return;
        if (sinks_.size() > 1) sinks_[1]->set_level(level);
        console_level_ = level;
    }

    spdlog::level::level_enum getFileLevel() const { return file_level_; }
    spdlog::level::level_enum getConsoleLevel() const { return console_level_; }

    bool isInitialized() const { return initialized_.load(std::memory_order_acquire); }

    // 添加一个安全的日志检查方法
    bool isSafeToLog() const {
        return initialized_.load(std::memory_order_acquire) && logger_ != nullptr;
    }

    // ====== {} 风格日志（接受外部传入的源位置）======
    template<typename... Args>
    void trace(const spdlog::source_loc& loc, const char* fmt, const Args&... args) {
        if (!isSafeToLog()) return;

        try {
            logger_->log(loc, spdlog::level::trace, fmt, args...);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Log trace failed: %s\n", e.what());
        }
    }

    template<typename... Args>
    void debug(const spdlog::source_loc& loc, const char* fmt, const Args&... args) {
        if (!isSafeToLog()) return;

        try {
            logger_->log(loc, spdlog::level::debug, fmt, args...);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Log debug failed: %s\n", e.what());
        }
    }

    template<typename... Args>
    void info(const spdlog::source_loc& loc, const char* fmt, const Args&... args) {
        if (!isSafeToLog()) return;

        try {
            logger_->log(loc, spdlog::level::info, fmt, args...);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Log info failed: %s\n", e.what());
        }
    }

    template<typename... Args>
    void warn(const spdlog::source_loc& loc, const char* fmt, const Args&... args) {
        if (!isSafeToLog()) return;

        try {
            logger_->log(loc, spdlog::level::warn, fmt, args...);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Log warn failed: %s\n", e.what());
        }
    }

    template<typename... Args>
    void error(const spdlog::source_loc& loc, const char* fmt, const Args&... args) {
        if (!isSafeToLog()) return;

        try {
            logger_->log(loc, spdlog::level::err, fmt, args...);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Log error failed: %s\n", e.what());
        }
    }

    template<typename... Args>
    void critical(const spdlog::source_loc& loc, const char* fmt, const Args&... args) {
        if (!isSafeToLog()) return;

        try {
            logger_->log(loc, spdlog::level::critical, fmt, args...);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Log critical failed: %s\n", e.what());
        }
    }

    // ====== % 风格日志（接受外部传入的源位置）======
    void trace_fmt(const spdlog::source_loc& loc, const char* fmt, ...) {
        if (!isSafeToLog()) return;

        try {
            va_list args;
            va_start(args, fmt);
            logFormatted(loc, spdlog::level::trace, fmt, args);
            va_end(args);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Log trace_fmt failed: %s\n", e.what());
        }
    }

    void debug_fmt(const spdlog::source_loc& loc, const char* fmt, ...) {
        if (!isSafeToLog()) return;

        try {
            va_list args;
            va_start(args, fmt);
            logFormatted(loc, spdlog::level::debug, fmt, args);
            va_end(args);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Log debug_fmt failed: %s\n", e.what());
        }
    }

    void info_fmt(const spdlog::source_loc& loc, const char* fmt, ...) {
        if (!isSafeToLog()) return;

        try {
            va_list args;
            va_start(args, fmt);
            logFormatted(loc, spdlog::level::info, fmt, args);
            va_end(args);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Log info_fmt failed: %s\n", e.what());
        }
    }

    void warn_fmt(const spdlog::source_loc& loc, const char* fmt, ...) {
        if (!isSafeToLog()) return;

        try {
            va_list args;
            va_start(args, fmt);
            logFormatted(loc, spdlog::level::warn, fmt, args);
            va_end(args);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Log warn_fmt failed: %s\n", e.what());
        }
    }

    void error_fmt(const spdlog::source_loc& loc, const char* fmt, ...) {
        if (!isSafeToLog()) return;

        try {
            va_list args;
            va_start(args, fmt);
            logFormatted(loc, spdlog::level::err, fmt, args);
            va_end(args);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Log error_fmt failed: %s\n", e.what());
        }
    }

    void critical_fmt(const spdlog::source_loc& loc, const char* fmt, ...) {
        if (!isSafeToLog()) return;

        try {
            va_list args;
            va_start(args, fmt);
            logFormatted(loc, spdlog::level::critical, fmt, args);
            va_end(args);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Log critical_fmt failed: %s\n", e.what());
        }
    }

private:
    LoggerManager() = default;
    LoggerManager(const LoggerManager&) = delete;
    LoggerManager& operator=(const LoggerManager&) = delete;

    ~LoggerManager() {
        // 不在析构函数中自动关闭，由用户显式调用 shutdown
        // 这样可以避免静态对象销毁顺序问题
    }

    void createLogDirectory() {
        try {
            auto log_dir = std::filesystem::path("log");
            if (!std::filesystem::exists(log_dir)) {
                if (!std::filesystem::create_directories(log_dir)) {
                    throw std::runtime_error("Cannot create log directory");
                }
            }

            // 检查目录是否可写
            std::filesystem::path test_file = log_dir / "test_write.tmp";
            std::ofstream test(test_file);
            if (!test) {
                throw std::runtime_error("Log directory is not writable");
            }
            test.close();
            std::filesystem::remove(test_file);

        }
        catch (const std::filesystem::filesystem_error& e) {
            throw std::runtime_error(std::string("Filesystem error: ") + e.what());
        }
    }

    void cleanupExistingLogger() {
        if (logger_) {
            try {
                logger_->flush();
                // 重要：不调用 spdlog::drop，避免访问已销毁的全局注册表
                // 直接重置 logger 和 sinks
            }
            catch (const std::exception& e) {
                // 在关闭阶段，忽略清理异常，但输出错误信息
                fprintf(stderr, "Logger cleanup failed: %s\n", e.what());
            }
            logger_.reset();
        }
        sinks_.clear();
    }

    void performShutdown() {
        // 检查是否已经关闭
        if (!initialized_.load(std::memory_order_acquire)) {
            return;
        }

        // 先标记为未初始化，防止新的日志写入
        initialized_.store(false, std::memory_order_release);

        // 然后清理资源
        cleanupExistingLogger();
    }

    void logFormatted(const spdlog::source_loc& loc, spdlog::level::level_enum level,
        const char* fmt, va_list args) {
        if (!isSafeToLog() || !fmt) return;

        // 使用安全的格式化
        std::string formatted_msg;

        // 第一次调用获取所需长度
        va_list args_copy;
        va_copy(args_copy, args);
        int needed = vsnprintf(nullptr, 0, fmt, args_copy);
        va_end(args_copy);

        if (needed < 0) {
            fprintf(stderr, "Format string error in logFormatted\n");
            return;
        }

        // 分配足够空间（包括null终止符）
        formatted_msg.resize(static_cast<size_t>(needed) + 1);

        // 第二次调用实际格式化
        int result = vsnprintf(&formatted_msg[0], formatted_msg.size(), fmt, args);
        if (result < 0 || result >= static_cast<int>(formatted_msg.size())) {
            fprintf(stderr, "String formatting failed in logFormatted\n");
            return;
        }

        // 移除null终止符
        formatted_msg.resize(static_cast<size_t>(result));

        // 双重检查，防止在格式化过程中被关闭
        if (isSafeToLog()) {
            try {
                logger_->log(loc, level, "{}", formatted_msg);
            }
            catch (const std::exception& e) {
                fprintf(stderr, "Log formatted message failed: %s\n", e.what());
            }
        }
    }

private:
    std::atomic<bool> initialized_{ false };
    mutable std::mutex init_mutex_;
    mutable std::mutex level_mutex_;

    std::shared_ptr<spdlog::logger> logger_;
    std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks_;

    spdlog::level::level_enum file_level_ = spdlog::level::info;
    spdlog::level::level_enum console_level_ = spdlog::level::info;
};