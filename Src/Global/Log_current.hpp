#pragma once
#ifndef LOG_HPP
#define LOG_HPP

#include <string>
#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <sstream>
#include <mutex>
#include <fstream>
#include <iomanip>
#include <atomic>
#include <vector>
#include <thread>
#include <condition_variable>
#include <queue>
#include <memory>
#include <filesystem>

// 添加必要的头文件
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <tchar.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <csignal>
#include <cstdlib>
#endif

// 日志目录设置
constexpr const char* LOG_DIR = "LOG";

// 日志系统管理类（单例模式）
class LogSystem {
public:
    static LogSystem& Instance() {
        static LogSystem instance;
        return instance;
    }

    // 日志条目结构
    struct LogEntry {
        std::chrono::system_clock::time_point timestamp;
        std::string message;
        bool isFileLog;
    };

    // 初始化日志系统
    void Init() {
        if (initialized_.exchange(true)) return;

        // 第一步：设置异常处理程序（在创建线程之前）
        SetupCrashHandlers();

        // 第二步：创建日志目录和初始化其他资源
        EnsureLogDirectory();
        running_ = true;

        // 第三步：启动工作线程
        worker_ = std::thread([this] {
            WorkerThread();
            });



        // 第四步：设置控制台关闭处理程序
        SetupConsoleCloseHandler();

        // 记录初始化完成
        PushLog({
            std::chrono::system_clock::now(),
            u8"[SYSTEM] 日志系统初始化完成，异常处理程序已启用设置",
            true
            });
    }

    // 清理日志系统
    void Cleanup() {
        if (!running_) return;

        // 记录程序正常关闭日志
        LogShutdownMessage(u8"程序正常关闭");

        running_ = false;
        {
            std::lock_guard<std::mutex> lock(workerMutex_);
            workerCond_.notify_one();
        }

        if (worker_.joinable()) {
            worker_.join();
        }

        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

    // 添加日志条目
    bool PushLog(LogEntry&& entry) {
        if (!running_) return false;

        for (int i = 0; i < 5; ++i) {
            if (TryPushEntry(std::move(entry))) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }

        return TryPushEntry(std::move(entry));
    }

    // 强制刷新所有缓冲区中的日志到文件
    void ForceFlush() {
        if (!running_) return;

        // 通知工作线程立即处理
        {
            std::lock_guard<std::mutex> lock(workerMutex_);
            workerCond_.notify_one();
        }

        // 等待工作线程处理完当前缓冲区
        std::unique_lock<std::mutex> lock(flushMutex_);
        flushCond_.wait(lock, [this] {
            return buffer_.size() == 0 || !running_;
            });
    }

    // 记录程序关闭消息
    void LogShutdownMessage(const std::string& reason) {
        if (!running_) return;

        std::string message = u8"[SHUTDOWN] 程序关闭 - 原因: " + reason;

        PushLog({
            std::chrono::system_clock::now(),
            message,
            true  // 总是写入文件
            });

        // 同时输出到控制台
        //std::lock_guard<std::mutex> consoleLock(consoleMutex_);
        //std::cout << message << std::endl;
    }

private:
    // 高性能环形缓冲区
    class LogRingBuffer {
    public:
        LogRingBuffer(size_t size) : buffer_(size), head_(0), tail_(0), count_(0) {}

        bool try_push(LogEntry&& entry) {
            size_t current_head = head_.load(std::memory_order_relaxed);
            size_t current_tail = tail_.load(std::memory_order_acquire);
            size_t next_tail = (current_tail + 1) % buffer_.size();

            if (next_tail == current_head) return false;

            buffer_[current_tail] = std::move(entry);
            tail_.store(next_tail, std::memory_order_release);
            count_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }

        bool try_pop_batch(std::vector<LogEntry>& batch, size_t max_size) {
            size_t current_head = head_.load(std::memory_order_relaxed);
            size_t current_tail = tail_.load(std::memory_order_acquire);

            if (current_head == current_tail) return false;

            size_t available = (current_tail >= current_head) ?
                (current_tail - current_head) :
                (buffer_.size() - current_head + current_tail);

            size_t to_pop = std::min(available, max_size);
            batch.reserve(to_pop);

            for (size_t i = 0; i < to_pop; ++i) {
                batch.push_back(std::move(buffer_[current_head]));
                current_head = (current_head + 1) % buffer_.size();
            }

            head_.store(current_head, std::memory_order_release);
            count_.fetch_sub(to_pop, std::memory_order_relaxed);
            return true;
        }

        size_t size() const { return count_.load(std::memory_order_relaxed); }

    private:
        std::vector<LogEntry> buffer_;
        std::atomic<size_t> head_ = 0;
        std::atomic<size_t> tail_ = 0;
        std::atomic<size_t> count_ = 0;
    };

    LogSystem() {
        // 确保日志目录在构造时创建
        static std::once_flag dirFlag;
        std::call_once(dirFlag, [this] {
            try {
                if (!std::filesystem::exists(LOG_DIR)) {
                    std::filesystem::create_directory(LOG_DIR);
                }
#ifndef _WIN32
                std::filesystem::permissions(LOG_DIR,
                    std::filesystem::perms::owner_all |
                    std::filesystem::perms::group_read |
                    std::filesystem::perms::group_exec |
                    std::filesystem::perms::others_read |
                    std::filesystem::perms::others_exec);
#endif
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::lock_guard<std::mutex> consoleLock(consoleMutex_);
                std::cerr << "[ERROR] Failed to create log directory: "
                    << e.what() << std::endl;
            }
            });
    }

    ~LogSystem() {
        // 在析构时记录关闭日志
        if (running_.load()) {
            LogShutdownMessage(u8"日志系统析构关闭");
        }
        Cleanup();
    }

    LogSystem(const LogSystem&) = delete;
    LogSystem& operator=(const LogSystem&) = delete;
    LogSystem(LogSystem&&) = delete;
    LogSystem& operator=(LogSystem&&) = delete;

    // 设置控制台关闭处理程序
    void SetupConsoleCloseHandler() {
#ifdef _WIN32
        // 设置控制台控制处理程序
        SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
#endif
    }

    // 控制台控制处理程序（静态成员函数）
#ifdef _WIN32
    static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
        // 确保我们获取的是最新的实例
        LogSystem& instance = LogSystem::Instance();

        if (!instance.IsRunning()) {
            return FALSE; // 如果日志系统没运行，让默认处理程序处理
        }
        switch (dwCtrlType) {
        case CTRL_C_EVENT:
            Instance().LogShutdownMessage(u8"控制台Ctrl+C关闭");
            Instance().ForceFlush();
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 给日志写入一点时间
            return FALSE; // 让默认处理程序也执行

        case CTRL_CLOSE_EVENT:
            Instance().LogShutdownMessage(u8"控制台直接点击关闭");
            Instance().ForceFlush();
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 给日志写入更多时间
            return TRUE; // 阻止默认关闭，让日志有时间写入

        case CTRL_BREAK_EVENT:
            Instance().LogShutdownMessage(u8"控制台Ctrl+Break关闭");
            Instance().ForceFlush();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return FALSE;

        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            Instance().LogShutdownMessage(u8"系统注销或关机");
            Instance().ForceFlush();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return FALSE;

        default:
            return FALSE;
        }
    }
#endif

    // 设置异常/信号处理程序
    void SetupCrashHandlers() {
#ifdef _WIN32
        // 使用 VEH（向量化异常处理），优先级高于 UnhandledExceptionFilter
        AddVectoredExceptionHandler(1, [](PEXCEPTION_POINTERS ExceptionInfo) -> LONG {
            if (!LogSystem::Instance().IsRunning()) {
                //TerminateProcess(GetCurrentProcess(), 1);
                return EXCEPTION_EXECUTE_HANDLER;
            }
            // 记录崩溃信息
            LogSystem::Instance().LogCrashMessage(ExceptionInfo);

            // 强制刷新日志
            LogSystem::Instance().ForceFlush();

            // 给日志写入一些时间
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            // 立即退出，不再继续搜索或执行任何代码
            //_exit(1);  // 或 
            //TerminateProcess(GetCurrentProcess(), 1);
            // 注意：_exit 不会返回，所以后面的 return 不会执行
            // 
            // 
            //return EXCEPTION_CONTINUE_SEARCH; // 或 EXCEPTION_EXECUTE_HANDLER（终止）
            return EXCEPTION_EXECUTE_HANDLER;
            });
        //TerminateProcess(GetCurrentProcess(), 1);
        //previousExceptionFilter_ = SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* ExceptionInfo) -> LONG {
        //    // 确保日志系统实例存在且运行
        //    if (!LogSystem::Instance().IsRunning()) {
        //        // 如果日志系统没运行，直接输出到控制台
        //        std::cerr << u8"[紧急] 程序崩溃，但日志系统未运行" << std::endl;
        //        return EXCEPTION_CONTINUE_SEARCH;
        //    }
        //    // 记录崩溃信息
        //    LogSystem::Instance().LogCrashMessage(ExceptionInfo);

        //    // 强制刷新日志
        //    LogSystem::Instance().ForceFlush();

        //    // 给日志写入一些时间
        //    std::this_thread::sleep_for(std::chrono::milliseconds(300));

        //    // 调用之前的异常过滤器或返回继续搜索
        //    if (LogSystem::Instance().previousExceptionFilter_) {
        //        return LogSystem::Instance().previousExceptionFilter_(ExceptionInfo);
        //    }
        //    // 立即退出，不再继续搜索或执行任何代码
        //    _exit(1);  // 或 TerminateProcess(GetCurrentProcess(), 1);
        //    注意：_exit 不会返回，所以后面的 return 不会执行
        //    return EXCEPTION_CONTINUE_SEARCH;
        //    });
#else
        // 设置信号处理程序
        auto signal_handler = [](int sig) {
            LogSystem::Instance().LogCrashMessage(sig);
            LogSystem::Instance().ForceFlush();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // 重新注册默认处理程序并重新引发信号
            signal(sig, SIG_DFL);
            raise(sig);
            };

        std::signal(SIGSEGV, signal_handler); // 段错误
        std::signal(SIGABRT, signal_handler); // 中止信号
        std::signal(SIGFPE, signal_handler);  // 浮点异常
        std::signal(SIGILL, signal_handler);  // 非法指令
        std::signal(SIGTERM, [](int) {        // 终止信号
            LogSystem::Instance().LogShutdownMessage(u8"接收到终止信号");
            LogSystem::Instance().ForceFlush();
            std::_Exit(EXIT_SUCCESS);
            });
#endif
    }

    // 添加一个公共方法来检查运行状态
    bool IsRunning() const {
        return running_.load();
    }

    // 记录崩溃消息 (Windows)
#ifdef _WIN32
    void LogCrashMessage(EXCEPTION_POINTERS* ExceptionInfo) {
        if (!running_) return;

        // 添加进程和线程信息
        DWORD processId = GetCurrentProcessId();
        DWORD threadId = GetCurrentThreadId();

        std::string message0 = u8"[CRASH] 程序崩溃检测 - 进程ID: " +
            std::to_string(processId) + u8", 线程ID: " + std::to_string(threadId) + "\n";

        // 使用更可靠的日志写入方式
        DirectEmergencyLog(message0);

        std::string exceptionType;
        std::string chineseDescription;
        DWORD exceptionCode = ExceptionInfo->ExceptionRecord->ExceptionCode;

        // 详细的异常类型和中文描述映射
        switch (exceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:
            exceptionType = "ACCESS_VIOLATION";
            chineseDescription = u8"内存访问违规 - 尝试读取或写入不可访问的内存地址";
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            exceptionType = "ARRAY_BOUNDS_EXCEEDED";
            chineseDescription = u8"数组越界 - 尝试访问超出数组边界的元素";
            break;
        case EXCEPTION_BREAKPOINT:
            exceptionType = "BREAKPOINT";
            chineseDescription = u8"断点异常 - 遇到调试断点";
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            exceptionType = "DATATYPE_MISALIGNMENT";
            chineseDescription = u8"数据对齐错误 - 在未对齐的内存地址上访问数据";
            break;
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            exceptionType = "FLT_DENORMAL_OPERAND";
            chineseDescription = "浮点异常 - 操作数是非正规数";
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            exceptionType = "FLT_DIVIDE_BY_ZERO";
            chineseDescription = u8"浮点除零错误 - 浮点数除以零";
            break;
        case EXCEPTION_FLT_INEXACT_RESULT:
            exceptionType = "FLT_INEXACT_RESULT";
            chineseDescription = u8"浮点不精确结果 - 浮点运算结果无法精确表示";
            break;
        case EXCEPTION_FLT_INVALID_OPERATION:
            exceptionType = "FLT_INVALID_OPERATION";
            chineseDescription = u8"无效浮点操作 - 未定义的浮点运算";
            break;
        case EXCEPTION_FLT_OVERFLOW:
            exceptionType = "FLT_OVERFLOW";
            chineseDescription = u8"浮点上溢 - 浮点运算结果超出表示范围";
            break;
        case EXCEPTION_FLT_STACK_CHECK:
            exceptionType = "FLT_STACK_CHECK";
            chineseDescription = u8"浮点栈检查失败 - 浮点运算导致栈溢出或下溢";
            break;
        case EXCEPTION_FLT_UNDERFLOW:
            exceptionType = "FLT_UNDERFLOW";
            chineseDescription = u8"浮点下溢 - 浮点运算结果太小无法表示";
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            exceptionType = "ILLEGAL_INSTRUCTION";
            chineseDescription = u8"非法指令 - CPU无法识别的指令";
            break;
        case EXCEPTION_IN_PAGE_ERROR:
            exceptionType = "IN_PAGE_ERROR";
            chineseDescription = u8"页面错误 - 无法从虚拟内存加载所需页面";
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            exceptionType = "INT_DIVIDE_BY_ZERO";
            chineseDescription = u8"整数除零错误 - 整数除以零";
            break;
        case EXCEPTION_INT_OVERFLOW:
            exceptionType = "INT_OVERFLOW";
            chineseDescription = u8"整数溢出 - 整数运算结果超出表示范围";
            break;
        case EXCEPTION_INVALID_DISPOSITION:
            exceptionType = "INVALID_DISPOSITION";
            chineseDescription = u8"无效处置 - 异常处理程序返回无效处置";
            break;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            exceptionType = "NONCONTINUABLE_EXCEPTION";
            chineseDescription = u8"不可继续异常 - 在不可继续异常后尝试继续执行";
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            exceptionType = "PRIV_INSTRUCTION";
            chineseDescription = u8"特权指令 - 在用户模式执行特权指令";
            break;
        case EXCEPTION_SINGLE_STEP:
            exceptionType = "SINGLE_STEP";
            chineseDescription = u8"单步执行 - 遇到单步调试陷阱";
            break;
        case EXCEPTION_STACK_OVERFLOW:
            exceptionType = "STACK_OVERFLOW";
            chineseDescription = u8"栈溢出 - 函数调用层次过深或局部变量占用过多栈空间";
            break;
        case EXCEPTION_GUARD_PAGE:
            exceptionType = "GUARD_PAGE";
            chineseDescription = u8"保护页违规 - 访问内存保护页";
            break;
        case EXCEPTION_INVALID_HANDLE:
            exceptionType = "INVALID_HANDLE";
            chineseDescription = u8"无效句柄 - 使用了无效的系统句柄";
            break;
        case STATUS_NO_MEMORY:
            exceptionType = "STATUS_NO_MEMORY";
            chineseDescription = u8"内存不足 - 系统内存资源耗尽";
            break;
        default:
            //exceptionType = "UNKNOWN_EXCEPTION";
            //chineseDescription = u8"未知异常类型";
            if (exceptionCode <= 31) {
                // 处理原始 CPU 异常向量
                switch (exceptionCode) {
                case 0:
                    exceptionType = "DIVIDE_BY_ZERO";
                    chineseDescription = u8"除零错误（原始中断号 #0）";
                    break;
                case 1:
                    exceptionType = "DEBUG_EXCEPTION";
                    chineseDescription = u8"调试异常（原始中断号 #1）";
                    break;
                case 3:
                    exceptionType = "BREAKPOINT";
                    chineseDescription = u8"断点异常（原始中断号 #3）";
                    break;
                case 4:
                    exceptionType = "OVERFLOW";
                    chineseDescription = u8"算术溢出（原始中断号 #4）";
                    break;
                case 5:
                    exceptionType = "BOUND_RANGE_EXCEEDED";
                    chineseDescription = u8"边界检查失败（原始中断号 #5）";
                    break;
                case 6:
                    exceptionType = "INVALID_OPCODE";
                    chineseDescription = u8"无效操作码 - CPU执行了无法识别的指令（原始中断号 #6）";
                    break;
                case 7:
                    exceptionType = "DEVICE_NOT_AVAILABLE";
                    chineseDescription = u8"设备不可用（原始中断号 #7）";
                    break;
                case 8:
                    exceptionType = "DOUBLE_FAULT";
                    chineseDescription = u8"双重错误 - 处理一个异常时发生了另一个异常（原始中断号 #8）";
                    break;
                case 10:
                    exceptionType = "INVALID_TSS";
                    chineseDescription = u8"无效任务状态段（原始中断号 #10）";
                    break;
                case 11:
                    exceptionType = "SEGMENT_NOT_PRESENT";
                    chineseDescription = u8"段不存在（原始中断号 #11）";
                    break;
                case 12:
                    exceptionType = "STACK_SEGMENT_FAULT";
                    chineseDescription = u8"栈段错误（原始中断号 #12）";
                    break;
                case 13:
                    exceptionType = "GENERAL_PROTECTION";
                    chineseDescription = u8"通用保护错误 - 内存访问或权限违规（原始中断号 #13）";
                    break;
                case 14:
                    exceptionType = "PAGE_FAULT";
                    chineseDescription = u8"页错误 - 虚拟内存访问错误（原始中断号 #14）";
                    break;
                case 16:
                    exceptionType = "FLOATING_POINT_ERROR";
                    chineseDescription = u8"浮点运算错误（原始中断号 #16）";
                    break;
                case 17:
                    exceptionType = "ALIGNMENT_CHECK";
                    chineseDescription = u8"对齐检查失败（原始中断号 #17）";
                    break;
                case 18:
                    exceptionType = "MACHINE_CHECK";
                    chineseDescription = u8"机器检查异常 - 硬件错误（原始中断号 #18）";
                    break;
                default:
                    exceptionType = "CPU_EXCEPTION";
                    chineseDescription = u8"原始 CPU 异常向量（中断号 #" + std::to_string(exceptionCode) + "）";
                    break;
                }
            }
            else {
                // 对于大于31的未知代码，提供更详细的信息
                exceptionType = "UNKNOWN_EXCEPTION";
                chineseDescription = u8"未知异常类型 - 代码: 0x" +
                    [](DWORD code) {
                    std::stringstream ss;
                    ss << std::hex << std::uppercase << code;
                    return ss.str();
                    }(exceptionCode)+
                        " (十进制: " + std::to_string(exceptionCode) + ")";
            }
            break;
        }

        // 获取异常地址和详细信息
        PVOID exceptionAddress = ExceptionInfo->ExceptionRecord->ExceptionAddress;
        DWORD exceptionFlags = ExceptionInfo->ExceptionRecord->ExceptionFlags;

        std::string additionalInfo;
        if (exceptionCode == EXCEPTION_ACCESS_VIOLATION) {
            // 对于访问违规，提供更详细的信息
            // 修正后的代码：
            PVOID violationAddress = reinterpret_cast<PVOID>(
                static_cast<uintptr_t>(ExceptionInfo->ExceptionRecord->ExceptionInformation[1]));
            //PVOID violationAddress = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
            DWORD accessType = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];

            std::string accessTypeStr;
            if (accessType == 0) {
                accessTypeStr = u8"读取";
            }
            else if (accessType == 1) {
                accessTypeStr = u8"写入";
            }
            else if (accessType == 8) {
                accessTypeStr = u8"执行";
            }
            else {
                accessTypeStr = u8"未知操作";
            }

            additionalInfo = u8" - 违规地址: 0x" +
                std::to_string(reinterpret_cast<uintptr_t>(violationAddress)) +
                u8" (" + accessTypeStr + u8"访问)";
        }

        std::string message = u8"[CRASH] 程序崩溃检测\n" +
            std::string(u8"    异常类型: ") + exceptionType + "\n" +
            std::string(u8"    中文描述: ") + chineseDescription + "\n" +
            std::string(u8"    异常代码: 0x") + std::to_string(exceptionCode) + "\n" +
            std::string(u8"    异常地址: 0x") +
            std::to_string(reinterpret_cast<uintptr_t>(exceptionAddress)) +
            additionalInfo + "\n" +
            std::string(u8"    异常标志: ") + std::to_string(exceptionFlags);

        // 使用更可靠的日志写入方式
        DirectEmergencyLog(message);
        //PushLog({
        //    std::chrono::system_clock::now(),
        //    message,
        //    true  // 总是写入文件
        //    });

        //// 同时输出到控制台
        //std::lock_guard<std::mutex> consoleLock(consoleMutex_);
        //std::cerr << message << std::endl;

        // 尝试生成堆栈跟踪（如果可能）
        //GenerateStackTrace(ExceptionInfo);

    }

    // 生成堆栈跟踪信息
    void GenerateStackTrace(EXCEPTION_POINTERS* ExceptionInfo) {
        // 初始化符号处理
        SymInitialize(GetCurrentProcess(), NULL, TRUE);
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

        STACKFRAME64 stackFrame;
        ZeroMemory(&stackFrame, sizeof(STACKFRAME64));

        DWORD machineType;
#ifdef _M_IX86
        machineType = IMAGE_FILE_MACHINE_I386;
        stackFrame.AddrPC.Offset = ExceptionInfo->ContextRecord->Eip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = ExceptionInfo->ContextRecord->Ebp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = ExceptionInfo->ContextRecord->Esp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
        machineType = IMAGE_FILE_MACHINE_AMD64;
        stackFrame.AddrPC.Offset = ExceptionInfo->ContextRecord->Rip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = ExceptionInfo->ContextRecord->Rbp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = ExceptionInfo->ContextRecord->Rsp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
#endif

        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        // 收集堆栈帧
        std::vector<std::string> stackTrace;
        for (int i = 0; i < 64; i++) { // 最多64帧
            if (!StackWalk64(machineType, process, thread, &stackFrame,
                ExceptionInfo->ContextRecord, NULL,
                SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
                break;
            }

            if (stackFrame.AddrPC.Offset == 0) {
                break;
            }

            // 获取符号信息
            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 displacement = 0;
            if (SymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, symbol)) {
                std::string frameInfo = "        #" + std::to_string(i) + " 0x" +
                    std::to_string(stackFrame.AddrPC.Offset) + " " + symbol->Name +
                    " + 0x" + std::to_string(displacement);
                stackTrace.push_back(frameInfo);
            }
            else {
                std::string frameInfo = "        #" + std::to_string(i) + " 0x" +
                    std::to_string(stackFrame.AddrPC.Offset) + u8" [未知函数]";
                stackTrace.push_back(frameInfo);
            }
        }

        // 记录堆栈跟踪
        if (!stackTrace.empty()) {
            std::string stackMessage = u8"[CRASH] 堆栈跟踪:";
            PushLog({
                std::chrono::system_clock::now(),
                stackMessage,
                true
                });

            for (const auto& frame : stackTrace) {
                PushLog({
                    std::chrono::system_clock::now(),
                    frame,
                    true
                    });
            }

            //// 控制台输出堆栈跟踪
            //std::lock_guard<std::mutex> consoleLock(consoleMutex_);
            //std::cerr << u8"[CRASH] 堆栈跟踪:" << std::endl;
            //for (const auto& frame : stackTrace) {
            //    std::cerr << frame << std::endl;
            //}
        }

        SymCleanup(process);
    }

    // 添加直接紧急日志写入方法
    void DirectEmergencyLog(const std::string& message) {
        auto timestamp = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()) % 1000;

        std::tm now_tm;
        localtime_s(&now_tm, &now_c);

        std::ostringstream oss;
        oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

        const std::string fullLog = "[" + oss.str() + "] " + message;

        //// 直接输出到控制台（使用原子操作确保线程安全）
        //static std::mutex emergencyConsoleMutex;
        //{
        //    std::lock_guard<std::mutex> lock(emergencyConsoleMutex);
        //    std::cerr << fullLog << std::endl;
        //}

        // 直接写入文件（不依赖工作线程）
        try {
            std::string logDate = oss.str().substr(0, 10);
            std::string logFilePath = GetLogFilePath(logDate);

            std::ofstream emergencyFile(logFilePath, std::ios::app);
            if (emergencyFile.is_open()) {
                emergencyFile << fullLog << "\n";
                emergencyFile.flush();
                emergencyFile.close();
            }
        }
        catch (...) {
            // 忽略文件写入错误，至少控制台输出已经完成
        }
    }
#endif

    // 记录崩溃消息 (Linux/Unix)
#ifndef _WIN32
    void LogCrashMessage(int signal) {
        if (!running_) return;

        std::string signalName;
        std::string chineseDescription;
        switch (signal) {
        case SIGSEGV:
            signalName = "SIGSEGV";
            chineseDescription = u8"段错误 - 无效内存访问";
            break;
        case SIGABRT:
            signalName = "SIGABRT";
            chineseDescription = u8"中止信号 - 程序主动中止";
            break;
        case SIGFPE:
            signalName = "SIGFPE";
            chineseDescription = u8"浮点异常 - 算术运算错误";
            break;
        case SIGILL:
            signalName = "SIGILL";
            chineseDescription = u8"非法指令 - 执行了非法CPU指令";
            break;
        case SIGBUS:
            signalName = "SIGBUS";
            chineseDescription = u8"总线错误 - 内存地址对齐错误";
            break;
        case SIGSYS:
            signalName = "SIGSYS";
            chineseDescription = u8"系统调用错误 - 错误的系统调用参数";
            break;
        default:
            signalName = "未知信号";
            chineseDescription = u8"未知信号类型";
            break;
        }

        std::string message = u8"[CRASH] 程序崩溃检测\n" +
            std::string(u8"    信号类型: ") + signalName + "\n" +
            std::string(u8"    中文描述: ") + chineseDescription + "\n" +
            std::string(u8"    信号代码: ") + std::to_string(signal);

        PushLog({
            std::chrono::system_clock::now(),
            message,
            true  // 总是写入文件
            });

        // 同时输出到控制台
        std::lock_guard<std::mutex> consoleLock(consoleMutex_);
        std::cerr << message << std::endl;
    }
#endif

    // 确保日志目录存在
    void EnsureLogDirectory() {
        // 现在只是一个空操作，目录在构造函数中已创建
    }

    // 获取日志文件路径
    std::string GetLogFilePath(const std::string& dateStr) {
        return std::string(LOG_DIR) + "/" + dateStr + ".log";
    }

    bool TryPushEntry(LogEntry&& entry) {
        if (buffer_.try_push(std::move(entry))) {
            std::lock_guard<std::mutex> lock(workerMutex_);
            workerCond_.notify_one();
            return true;
        }
        return false;
    }

    void WorkerThread() {
        std::vector<LogEntry> batch;
        batch.reserve(BATCH_SIZE);

        while (running_ || buffer_.size() > 0) {
            // 等待新日志或停止信号
            {
                std::unique_lock<std::mutex> lock(workerMutex_);
                if (buffer_.size() == 0 && running_) {
                    workerCond_.wait_for(lock, std::chrono::milliseconds(10)); // 减少等待时间
                }
            }

            // 批量处理日志
            while (buffer_.try_pop_batch(batch, BATCH_SIZE)) {
                ProcessBatch(batch);
                batch.clear();

                // 通知可能等待的刷新操作
                if (buffer_.size() == 0) {
                    std::lock_guard<std::mutex> lock(flushMutex_);
                    flushCond_.notify_all();
                }
            }

            // 即使没有批量处理，也定期刷新文件
            if (logFile_.is_open()) {
                std::lock_guard<std::mutex> fileLock(fileMutex_);
                logFile_.flush();
            }
        }

        // 线程结束前处理剩余日志
        while (buffer_.try_pop_batch(batch, BATCH_SIZE)) {
            ProcessBatch(batch);
            batch.clear();
        }

        // 最终通知所有等待的刷新操作
        std::lock_guard<std::mutex> lock(flushMutex_);
        flushCond_.notify_all();
    }

    void ProcessBatch(std::vector<LogEntry>& batch) {
        std::lock_guard<std::mutex> fileLock(fileMutex_);

        for (auto& entry : batch) {
            // 时间格式化
            auto now_c = std::chrono::system_clock::to_time_t(entry.timestamp);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                entry.timestamp.time_since_epoch()) % 1000;

            std::tm now_tm;
#ifdef _WIN32
            localtime_s(&now_tm, &now_c);
#else
            localtime_r(&now_c, &now_tm);
#endif

            std::ostringstream oss;
            oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
            oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

            const std::string fullLog = "[" + oss.str() + "] " + entry.message;

            // 控制台输出
            {
                std::lock_guard<std::mutex> consoleLock(consoleMutex_);
                if (entry.message.find("[CRASH]") != std::string::npos ||
                    entry.message.find("[SHUTDOWN]") != std::string::npos) {
                    std::cerr << fullLog << "\n";
                }
                else {
                    std::cout << fullLog << "\n";
                }
            }

            // 文件日志处理
            if (entry.isFileLog) {
                std::string logDate = oss.str().substr(0, 10);

                // 检查是否需要切换日志文件
                if (currentLogDate_ != logDate || !logFile_.is_open()) {
                    if (logFile_.is_open()) {
                        logFile_.close();
                    }

                    // 打开新日志文件
                    std::string logFilePath = GetLogFilePath(logDate);
                    logFile_.open(logFilePath, std::ios::app);
                    currentLogDate_ = logDate;

                    if (logFile_.is_open()) {
                        logFile_ << "===== Log started at " << oss.str() << " =====" << std::endl;
                    }
                }

                // 写入文件
                if (logFile_.is_open()) {
                    logFile_ << fullLog << "\n";
                }
            }
        }

        // 刷新文件缓冲区
        if (logFile_.is_open()) {
            logFile_.flush();
        }
    }

    // 常量
    static constexpr size_t LOG_BUFFER_SIZE = 65536; // 64K条目
    static constexpr size_t BATCH_SIZE = 32;         // 减小批量大小以提高及时性

    // 成员变量
    LogRingBuffer buffer_{ LOG_BUFFER_SIZE };
    std::atomic<bool> running_{ false };
    std::atomic<bool> initialized_{ false };
    std::thread worker_;

    // 同步原语
    std::mutex fileMutex_;
    std::mutex consoleMutex_;
    std::mutex workerMutex_;
    std::mutex flushMutex_;
    std::condition_variable workerCond_;
    std::condition_variable flushCond_;

    // 文件资源
    std::ofstream logFile_;
    std::string currentLogDate_;

#ifdef _WIN32
    LPTOP_LEVEL_EXCEPTION_FILTER previousExceptionFilter_ = nullptr;
#endif
};

// 线程安全的控制台日志函数
inline void LogPrintf(const char* format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    LogSystem::Instance().PushLog({
        std::chrono::system_clock::now(),
        buffer,
        false
        });
}

// 高性能文件日志函数
inline void FileLogPrintf(const char* format, ...) {
    static bool initialized = [] {
        LogSystem::Instance().Init();
        return true;
        }();

    char buffer[4096];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    if (len < 0) len = 0;
    if (len >= (int)sizeof(buffer)) len = sizeof(buffer) - 1;
    buffer[len] = '\0';

    va_end(args);

    LogSystem::Instance().PushLog({
        std::chrono::system_clock::now(),
        std::string(buffer),
        true
        });
}

// 强制刷新日志到文件
inline void FlushLogs() {
    LogSystem::Instance().ForceFlush();
}

// 记录程序关闭日志
inline void LogShutdown(const std::string& reason) {
    LogSystem::Instance().LogShutdownMessage(reason);
}

// 辅助宏，自动添加换行
#define LOG_INFO(...) LogPrintf(__VA_ARGS__)
#define FILE_LOG_INFO(...) FileLogPrintf(__VA_ARGS__)
#define FLUSH_LOGS() FlushLogs()
#define LOG_SHUTDOWN(reason) LogShutdown(reason)

#endif // LOG_HPP