#pragma once
#include "Logger.h"
#include <Windows.h>
#include <iostream>
#include <locale>
#include <codecvt>

DEFINITION_IDLE_SINGLEINSTANCE(Logger);
bool Logger::WriteLogFile(std::string ModelName, LogLevel lv,const char* file, size_t line, std::string LogInfo, ...)
{


	try
	{
		spdlog::drop(ModelName);
		m_logger = spdlog::basic_logger_mt(ModelName, m_FileName);
	}
	catch (const spdlog::spdlog_ex& ex)
	{
		std::cout << "Log init failed: " << ex.what() << std::endl;
	};

	std::string filename = file;
	std::string m_Log = "[" + filename + ":" + std::to_string(line) + "]" + LogInfo;
	static char buf_content[LOG_BUFF_LENGTH] = { 0 };
	va_list ap;
	va_start(ap, LogInfo);

	//int sizelog = LogInfo.size();
	//if (sizelog % 2 > 0)
	//{
	//	LogInfo += u8"\0";
	//}
	//sizelog = LogInfo.size();
	//std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	//std::wstring str = converter.from_bytes(LogInfo);

	std::vsnprintf(buf_content, LOG_BUFF_LENGTH, m_Log.c_str(),ap);
	va_end(ap);
	
	bool result = true;
	switch (lv)
	{
	case Info:
		{
			m_logger->info(buf_content);
			break;
		}
	case Warning:
		{
			m_logger->warn(buf_content);
			break;
		}
	case Error:
		{
			m_logger->error(buf_content);
			break;
		}
	case Debug:
		{
			spdlog::set_level(spdlog::level::debug); // Set global log level to debug
			m_logger->debug(buf_content);
			break;
		}
	default:
		break;
	}
	return true;
}

bool Logger::ConsoleLog(LogLevel lv, const char* file, size_t line, std::string LogInfo, ...)
{
	static char buf_content[LOG_BUFF_LENGTH] = { 0 };
	std::string filename = file;
	std::string m_Log = "[" + filename + ":" + std::to_string(line) + "]" + LogInfo;
	va_list ap;
	va_start(ap, LogInfo);
	std::vsnprintf(buf_content, LOG_BUFF_LENGTH, m_Log.c_str(), ap);
	va_end(ap);
	switch (lv)
	{
	case Info:
		spdlog::info(buf_content);
		break;
	case Warning:
		
		spdlog::warn(buf_content);
		break;
	case Error:
		spdlog::error(buf_content);
		break;
	case Debug:
		spdlog::set_level(spdlog::level::debug);
		spdlog::debug(buf_content);
		break;
	default:
		break;
	}
	return true;
}

void Logger::SetFileName(std::string filename)
{
	m_FileName = filename;
}

std::string Logger::GetTime()
{
	std::string m_Time;
	struct tm* p;
	time_t timep;
	time(&timep);
	p = localtime(&timep);
	std::string year = std::to_string(p->tm_year + 1900);
	std::string month = std::to_string(p->tm_mon + 1);
	std::string day = std::to_string(p->tm_mday);
	std::string hour = std::to_string(p->tm_hour);
	std::string min = std::to_string(p->tm_min);
	std::string sec = std::to_string(p->tm_sec);
	if (month.size() < 2)
	{
		month = "0" + month;
	}
	if (day.size() < 2)
	{
		day = "0" + day;
	}
	if (hour.size() < 2)
	{
		hour = "0" + hour;
	}
	if (min.size() < 2)
	{
		min = "0" + min;
	}
	if (sec.size() < 2)
	{
		sec = "0" + sec;
	}
	m_Time = year + month + day + hour + min + sec;
	return m_Time;
}


Logger::~Logger()
{

}

void Logger::init()
{
	SetConsoleOutputCP(65001);

	std::string pathHead = GetTime();
	m_FileName = "./log/"+pathHead+".log";
}