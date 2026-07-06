#pragma once
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>  // 确保包含这个头文件
#include <thread>
#include <vector>

// glassinfo_produce 表的结构体
struct GlassInfoProduce {
    std::string time_produce;
    std::string id_produce;
    int num_produce;
    int errnum_produce;
    int errrank_produce;
    std::string length_produce;
    std::string width_produce;
    std::string diagonal_produce;
    std::string reason_produce;
    std::string customer_produce;
};

// glasserrorinfo_records 表的结构体
struct GlassErrorInfoRecords {
    std::string id_records;
    int errnum0_records;
    int errnum1_records;
    int errnum2_records;
    int errnum3_records;
    int errnum4_records;
    int errnum5_records;
    int errnum6_records;
    int errnum7_records;
    int errnum8_records;
    int errnum9_records;
    int errnum10_records;
    int errnum11_records;
    int errnum12_records;
    std::string detail_records;
    std::string time_produce;
};

class SQLiteUtils
{
public:
	SQLiteUtils();
	~SQLiteUtils();

	std::string getCurrentTime();

    void setProduceInfo(GlassInfoProduce& proudceInfo, const std::string& time_produce, const std::string& id, int num, int errnum, int errrank, const std::string& length, const std::string& width,
        const std::string& diagonal, const std::string& reason, const std::string& customer);

    void setRecordsInfo(GlassErrorInfoRecords& recordsInfo, const std::string& id_records, const std::string& time_produce, const std::string& detail_records,
        int errnum0, int errnum1, int errnum2, int errnum3, int errnum4, 
        int errnum5, int errnum6, int errnum7, int errnum8, int errnum9, 
        int errnum10, int errnum11, int errnum12);

	void mainTest();
};

