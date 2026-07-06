#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "generaldefinition.h"
//#include "qtypeinfo.h"

#define LOG_BUFF_LENGTH 512

#define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1):__FILE__)

#define FILE_INFOLOG(modelname, info, ...)\
		Logger::GetInstance()->WriteLogFile(std::string(modelname),Info,__FILENAME__,__LINE__,info,##__VA_ARGS__); \

#define FILE_DEBUGLOG(modelname, info, ...)\
		Logger::GetInstance()->WriteLogFile(std::string(modelname),Debug,__FILENAME__,__LINE__,info,##__VA_ARGS__); \

#define FILE_WARNLOG(modelname, info, ...)\
		Logger::GetInstance()->WriteLogFile(std::string(modelname),Warning,__FILENAME__,__LINE__,info,##__VA_ARGS__); \

#define FILE_ERRORLOG(modelname, info, ...)\
		Logger::GetInstance()->WriteLogFile(std::string(modelname),Error,__FILENAME__,__LINE__,info,##__VA_ARGS__); \


#define CONSO_INFOLOG(info, ...)\
		Logger::GetInstance()->ConsoleLog(Info,__FILENAME__,__LINE__,info,##__VA_ARGS__); \

#define CONSO_DEBUGLOG(info, ...)\
		Logger::GetInstance()->ConsoleLog(Debug,__FILENAME__,__LINE__,info,##__VA_ARGS__); \

#define CONSO_WARNLOG(info, ...)\
		Logger::GetInstance()->ConsoleLog(Warning,__FILENAME__,__LINE__,info,##__VA_ARGS__); \

#define CONSO_ERRORLOG(info, ...)\
		Logger::GetInstance()->ConsoleLog(Error,__FILENAME__,__LINE__,info,##__VA_ARGS__); \


enum LogLevel
{
	Info = 1,
	Warning = 2,
	Error = 3,
	Debug = 4
};

class Logger
{
public:
	DECLARE_SINGLEINSTANCE(Logger);
	~Logger();
	
public:
	/*********************************************************
	 ** FileName  目标文件名称：没有则自动创建，有则追加写
	 ** ModelName 需要打印日志的模块名称
	 ** LogInfo   日志信息
	 ** LogLevel  日志级别	
	 **********************************************************/
	bool WriteLogFile(std::string ModelName, LogLevel lv,const char* file,size_t line, std::string LogInfo, ...);
	
	/********************************************************
	 ** lv		日志级别
	 ** LogInfo	日志信息
	 *********************************************************/
	bool ConsoleLog(LogLevel lv, const char* file, size_t line, std::string LogInfo, ...);

	void SetFileName(std::string filename);


private:
	std::shared_ptr<spdlog::logger> m_logger ;
	std::string m_FileName;
	std::string GetTime();

};
