#pragma once
#ifndef GLOBALLOG_H
#define GLOBALLOG_H

#include "Log.hpp"
#include "LogManager.h"

class CGlobalLog {
public:
    static CGlobalLog* GetInstance()
    static void Init()
    static void Shutdown()
    static void Flush()
    static void LogInfo(const std::string& message)
    static void LogError(const std::string& message)
    static void LogWarning(const std::string& message)
    static void LogDebug(const std::string& message)
    static void LogFatal(const std::string& message)
    static void LogPrintf(const char* format, ...)
    static void LogShutdown(const std::string& reason)
    static void DirectEmergencyLog(const std::string& message)
    static void RegisterThread(const std::string& threadName = "")
}
#endif // GLOBALLOG_H