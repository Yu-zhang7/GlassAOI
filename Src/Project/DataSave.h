#ifndef DATASAVE_H
#define DATASAVE_H

#include <QObject>
#include <QtSql/qsqldatabase.h>
#include <QtSql/qsqlquery.h>
#include <QtSql/qsqlerror.h>
#include <QtSql/qsqlrecord.h>
#include <vector>
#include <opencv2/opencv.hpp>

#include "SQLiteUtils.h"            //数据整理类(整理后，通过DataSave类保存到数据库)
#include "Json/glassData2Json.h"    //瑕疵数据管理类(Json读写))
#include "imageSaver.h"             //独立线程保存图片

/* DataSave类，用以保存计算结果 */
class DataSave : public QObject
{
    Q_OBJECT
public:
    DataSave() = default;
    ~DataSave() = default;

    bool operator()(QSqlDatabase& db, QueueDefectItem& resultItem, const std::string& customer);

    /* 获得唯一的记录ID(基于时间))*/
    std::string GetTimeProduce();

    /* 保存数据到文件 */
    bool saveData2Files(QueueDefectItem& resultItem);

    /* 保存数据到数据库 */
    bool saveData2Database(QSqlDatabase& db, QueueDefectItem& resultItem, const std::string& customer);

    /* 创建保存结果的文件夹 */
    //bool CreatResultDirectory(const std::string& path, std::string& outPath);
    bool CreatResultDirectory(const std::string& path, std::string& outPath, int& outGlassIndex);


    /* 更新数据的接口 */
    bool operator()(QSqlDatabase& db, std::vector<drawInformation>& mergeInfo, std::string& primaryKey
        , float GlassPixelWidth, float GlassPixelHeight);

    /* 开始事务 */
    bool StartTransaction(QSqlDatabase& db);

    /* 提交事务 */
    bool CommitTransaction(QSqlDatabase& db);

    /* 回滚事务 */
    void RollbackTransaction(QSqlDatabase& db);

    /* 保存瑕疵数据到json */
    void SaveDefectInfoToJson(std::vector<drawInformation>& mergeInfo, int Level, float glassPixelWidth, float glassPixelHeight, std::string savePath);

private:
    /* 插入glassinfo_produce一条新信息 */
    int InsertGlassInfo(QSqlDatabase& db, GlassInfoProduce& glassInfo);

    /* 插入glasserrorinfo_records一条新信息 */
    int InsertGlassErrorInfo(QSqlDatabase& db, GlassErrorInfoRecords& glassErrorInfo);

    /* 更新glassinfo_produce信息 */
    int updateGlassInfoProduce(QSqlDatabase& db, const GlassInfoProduce& glassInfo);
    
    /* 更新glasserrorinfo_records信息 */
    int updateGlassErrorInfoRecords(QSqlDatabase& db, const GlassErrorInfoRecords& glassErrorInfo);

    /* 更新数据库信息 */
    int updataGlassInfo(QSqlDatabase& db, GlassInfoProduce& glassInfo, GlassErrorInfoRecords& glassErrorInfo);

    /* 数据库查询结果和实体类进行转换 */
    void SetProduceInfo(QSqlQuery& query, GlassInfoProduce& glassInfo);
    void SetRecordInfo(QSqlQuery& query, GlassErrorInfoRecords& glassErrorInfo);

    /* 获取各个类型缺陷的数量 */
    std::vector<int> GetDefectTypeNum(const std::vector<drawInformation>& mergeInfo);
private slots:
    void handleSaveResult(bool success, const QString& path); // 处理保存结果

private:
    glassData2Json r2jUtils;   //结果转json类
    SQLiteUtils sqlUtils;   //数据库操作类
    int         m_num = 0;  //记录数
    bool        m_transactionAction = false; //事务标志

    
};

#endif // !DATASAVE_h



