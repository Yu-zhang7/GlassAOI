#include "SQLiteUtils.h"

SQLiteUtils::SQLiteUtils()
{
    
}

SQLiteUtils::~SQLiteUtils()
{
    
}

// 获取当前时间，精确到秒
std::string SQLiteUtils::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);

    tm time_tm;
    localtime_s(&time_tm, &now_time_t);

    // 格式化时间为 "yyyy-mm-dd hh:mm:ss"
    std::stringstream ss;
    ss << std::put_time(&time_tm, "%Y%m%d%H%M%S");

    return ss.str();
}

/*===========================数据库数据查询==============================================*/

void SQLiteUtils::setProduceInfo(GlassInfoProduce& proudceInfo, const std::string& time_produce, const std::string& id, 
    int num, int errnum, int errrank, const std::string& length, const std::string& width, 
    const std::string& diagonal, const std::string& reason, const std::string& customer)
{
    proudceInfo.time_produce = time_produce;
    proudceInfo.id_produce = id;
    proudceInfo.num_produce = num;
    proudceInfo.errnum_produce = errnum;
    proudceInfo.errrank_produce = errrank;
    proudceInfo.length_produce = length;
    proudceInfo.width_produce = width;
    proudceInfo.diagonal_produce = diagonal;
    proudceInfo.reason_produce = reason;
    proudceInfo.customer_produce = customer;
}

void SQLiteUtils::setRecordsInfo(GlassErrorInfoRecords& recordsInfo, const std::string& id_records, 
    const std::string& time_produce, const std::string& detail_records, int errnum0, int errnum1, 
    int errnum2, int errnum3, int errnum4, int errnum5, 
    int errnum6, int errnum7, int errnum8, int errnum9,
    int errnum10, int errnum11, int errnum12)
{
    recordsInfo.id_records = id_records;
    recordsInfo.time_produce = time_produce;
    recordsInfo.detail_records = detail_records;
    recordsInfo.errnum0_records = errnum0;
    recordsInfo.errnum1_records = errnum1;
    recordsInfo.errnum2_records = errnum2;
    recordsInfo.errnum3_records = errnum3;
    recordsInfo.errnum4_records = errnum4;
    recordsInfo.errnum5_records = errnum5;
    recordsInfo.errnum6_records = errnum6;
    recordsInfo.errnum7_records = errnum7;
    recordsInfo.errnum8_records = errnum8;
    recordsInfo.errnum9_records = errnum9;
    recordsInfo.errnum10_records = errnum10;
    recordsInfo.errnum11_records = errnum11;
    recordsInfo.errnum12_records = errnum12;
}