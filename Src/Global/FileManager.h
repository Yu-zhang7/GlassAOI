#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include <mutex>
#include <map>

class CFileManager
{
public:
	/*__declspec(dllexport)*/ CFileManager();
	~CFileManager() = default;
	/*__declspec(dllexport)*/ //~CFileManager();
	/*					   */
	/*__declspec(dllexport)*/ static CFileManager* GetInstance();
	/*					   */
	/*__declspec(dllexport)*/ bool isContainFile(std::string fileName);
	/*__declspec(dllexport)*/ char* readFile(std::string fileName);
	/*__declspec(dllexport)*/ std::map<std::string, std::string> readConfigFile(std::string fileName,const char* separator);
	/*__declspec(dllexport)*/ void writeConfigFile(std::string fileName, std::map<std::string, std::string> data, const char* separator);
	/*__declspec(dllexport)*/ bool writeFile(std::string fileName, const char* data);
	/*__declspec(dllexport)*/ bool writeFile(std::string fileName, char* data);
	/*__declspec(dllexport)*/ bool appendFile(std::string fileName, const char* data);

private:
	void init();
	bool isNameNull(std::string fileName);
	bool isDataNull(const char* data);

private:
	static CFileManager* m_file;
	std::mutex m_mutex;
};

