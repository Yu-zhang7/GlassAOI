//#include <QApplication>
#include "DataSave.h"
#include "Log.hpp"
#include <QDebug>

bool DataSave::operator()(QSqlDatabase& db, QueueDefectItem& resultItem, const std::string& customer)
{
    FILE_LOG_INFO("[DataSave] Begin!");
    /****** STEP0: Check 数据**********/
    if (!db.open())
    {
        FILE_LOG_ERROR("[DataSave] Check Error: Open db Failed! Save data Failed!");
        return false;
    }

    if ((resultItem.image_0.empty() || resultItem.image_1.empty() || resultItem.image_2.empty()) /*&& mergeInfo.size()==0*/)
    {
        FILE_LOG_ERROR("[DataSave] Check Error: largeImgs is empty! 1");
        return false;
    }

    if (resultItem.backgroundImage.empty())
    {
        FILE_LOG_ERROR("[DataSave] Check Error: backGroundImage is empty! 2");
        return false;
    }

    FILE_LOG_INFO("[DataSave] Check: Check invalid data successfully!");

    /****** STEP1: 创建记录数据对应的文件夹**********/
        //m_num = glassIndex;
        GlassInfoProduce tempProduce;
        GlassErrorInfoRecords tempRecords;
        //std::string rootPath/* = QApplication::applicationDirPath().toStdString()*/;
        //rootPath = RecipeInfo.global.dataSavePath.toStdString();

        std::string nameSave = resultItem.timeProduce + "_" + std::to_string(resultItem.glassIndex);
        std::string recordPath = resultItem.resultPath;
        //对保存路径进行二次校验
        if (!std::filesystem::exists(resultItem.resultPath))
        {
            FILE_LOG_ERROR(u8"[DataSave] Check Error: Error checking record directory: %s", resultItem.resultPath);
            return false;
        }
        FILE_LOG_INFO("[DataSave] Check: Check directory successfully!");

    /****** STEP4: 保存图像PNG/H5、瑕疵json等数据**********/
    
        ImageSaver* m_imgSaver = nullptr;
        if (m_imgSaver == nullptr)
        {
            m_imgSaver = new ImageSaver();
        }
        //connect(m_imgSaver, &ImageSaver::Signal_saved, this, &DataSave::handleSaveResult, Qt::UniqueConnection);
        //if (!LOOPTEST)   //测试时，不保存三通道大图
        {

            if (!resultItem.image_0.empty())
            {
                std::string saveImage = recordPath + "/" + nameSave + "_0.png";
                //cv::imwrite(saveImage, largeImgs[0]);
                m_imgSaver->saveImageAsync(saveImage, resultItem.image_0);       // 异步保存图像
            }
            if (!resultItem.image_1.empty())
            {
                std::string saveImage = recordPath + "/" + nameSave + "_1.png";
                //cv::imwrite(saveImage, largeImgs[1]);
                m_imgSaver->saveImageAsync(saveImage, resultItem.image_1);       // 异步保存图像
            }
            if (!resultItem.image_2.empty())
            {
                std::string saveImage = recordPath + "/" + nameSave + "_2.png";
                //cv::imwrite(saveImage, largeImgs[2]);
                m_imgSaver->saveImageAsync(saveImage, resultItem.image_2);       // 异步保存图像
            }
        }
        if (!resultItem.defectImages_ch0.empty())
        {
            std::string saveDetectImage = recordPath + "/" + nameSave + "_detectImages_0.h5";
            m_imgSaver->saveHDF5(saveDetectImage, resultItem.defectImages_ch0);
        }
        if (!resultItem.defectImages_ch1.empty())
        {
            std::string saveDetectImage = recordPath + "/" + nameSave + "_detectImages_1.h5";
            m_imgSaver->saveHDF5(saveDetectImage, resultItem.defectImages_ch1);
        }
        if (!resultItem.defectImages_ch2.empty())
        {
            std::string saveDetectImage = recordPath + "/" + nameSave + "_detectImages_2.h5";
            m_imgSaver->saveHDF5(saveDetectImage, resultItem.defectImages_ch2);
        }
        if (!resultItem.backgroundImage.empty())
        {
            std::string saveImage = recordPath + "/" + nameSave + "backgroundImage.jpg";
            m_imgSaver->saveImageAsync(saveImage, resultItem.backgroundImage);
        }
        if (!resultItem.pseudoColorImage.empty())
        {
            std::string saveImage = recordPath + "/" + nameSave + "pseudoColorImage.jpg";
            m_imgSaver->saveImageAsync(saveImage, resultItem.pseudoColorImage);
        }
        //std::filesystem::create_directories(errInfo+"/write/");
        //for (size_t i = 0; i < detectImages[0].size(); i++)
        //{
        //    if (!detectImages[0][i].empty())
        //    {
        //        std::string saveImage = errInfo + "/write/"+std::to_string(i)+"_0.png";
        //        //cv::imwrite(saveImage, largeImgs[2]);
        //        m_imgSaver->saveImageAsync(saveImage, detectImages[0][i]);       // 异步保存图像
        //    }
        //}
        //for (size_t i = 0; i < detectImages[1].size(); i++)
        //{
        //    if (!detectImages[1].empty())
        //    {
        //        std::string saveImage = errInfo + "/write/" + std::to_string(i) + "_1.png";
        //        //cv::imwrite(saveImage, largeImgs[2]);
        //        m_imgSaver->saveImageAsync(saveImage, detectImages[1][i]);       // 异步保存图像
        //    }
        //}
        //for (size_t i = 0; i < detectImages[2].size(); i++)
        //{
        //    if (!detectImages[2][i].empty())
        //    {
        //        std::string saveImage = errInfo + "/write/" + std::to_string(i) + "_1.png";
        //        //cv::imwrite(saveImage, largeImgs[2]);
        //        m_imgSaver->saveImageAsync(saveImage, detectImages[2][i]);       // 异步保存图像
        //    }
        //}
        delete m_imgSaver;
        m_imgSaver = nullptr;
        std::string saveJson = recordPath + "/" + nameSave + ".json";
        int glassLevel = 0;
        int camera_id = 1;
        std::vector<std::string> ErrorInfoData = { "area","height","width" };

        r2jUtils.saveData2Json(resultItem.drawInfo, glassLevel, resultItem.glassPixelWidth, resultItem.glassPixelHeight, saveJson);
        FILE_LOG_INFO("[DataSave] Save: SaveData2Json successfully!");
    /******************************************************/



    /****** STEP2: 计算是否为瑕疵玻璃**********/
    //int errrank_produce = 1;    //1：正常，0：NG
    //resultItem.NgFlag = 1;
    //if (resultItem.drawInfo.size() > 0)
    //{

    //    for (int i = 0; i < resultItem.drawInfo.size(); i++)
    //    {
    //        //20250705: 按照新规则，有严重缺陷的才视为NG
    //        if (resultItem.drawInfo[i].ErrorType == DefectLevel::SERIOUS)
    //        {
    //            //errrank_produce = 0;
    //            resultItem.NgFlag = 0;
    //            break;
    //        }
    //    }

    //}
    //tempProduce.errrank_produce = errrank_produce;
    //tempProduce.errrank_produce = resultItem.NgFlag;
    //FILE_LOG_INFO("[DataSave] Database: Set errorType successfully!");



    /****** STEP3: 整理数据并写入数据库**********/
    //整理数据
    tempProduce.errrank_produce = resultItem.NgFlag;
    if (resultItem.drawInfo.empty())
    {
        sqlUtils.setProduceInfo(tempProduce, resultItem.timeProduce, std::to_string(resultItem.glassIndex), 1, resultItem.drawInfo.size(), tempProduce.errrank_produce,
            std::to_string(resultItem.glassPhysicalHeight), std::to_string(resultItem.glassPhysicalWidth), "120", "ReasonExample", customer);
        sqlUtils.setRecordsInfo(tempRecords, resultItem.timeProduce + std::to_string(resultItem.glassIndex), resultItem.timeProduce, recordPath, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    else
    {
        sqlUtils.setProduceInfo(tempProduce, resultItem.timeProduce, std::to_string(resultItem.glassIndex), 1, resultItem.drawInfo.size(), tempProduce.errrank_produce,
            std::to_string(resultItem.glassPhysicalHeight), std::to_string(resultItem.glassPhysicalWidth), "120", "ReasonExample", customer);

        std::vector<int> TypeNum = GetDefectTypeNum(resultItem.drawInfo);   //获得每种瑕疵的个数

        sqlUtils.setRecordsInfo(tempRecords, resultItem.timeProduce + std::to_string(resultItem.glassIndex), resultItem.timeProduce, recordPath,
            TypeNum[0], TypeNum[1], TypeNum[2], TypeNum[3], TypeNum[4],
            TypeNum[5], TypeNum[6], TypeNum[7], TypeNum[8], TypeNum[9],
            TypeNum[10], TypeNum[11], TypeNum[12]);
    }

    // 写入数据库
        /******************保存数据到数据库********************/
    if (!StartTransaction(db))   // 开始事务
    {
        return false;
    }
    try
    {
        int flag = InsertGlassInfo(db, tempProduce);
        if (!flag)
        {
            //FILE_LOG_WARN("[DataSave] Database Error: Insert GlassInfo failed: \n%s",);
            return false;
        }
        if (!InsertGlassErrorInfo(db, tempRecords))
        {
            //FILE_LOG_WARN("[DataSave] Database Error: Insert GlassErrorInfo failed.");
            return false;
        }
        // 提交事务
        if (!CommitTransaction(db))
        {
            FILE_LOG_WARN("[DataSave] Database Error: CommitTransaction. Transaction rolled back.");
            return false;
        }
        QSqlQuery query(db);
        query.exec("PRAGMA wal_checkpoint(PASSIVE);");
    }
    catch (const std::exception& e)
    {
        RollbackTransaction(db);
        FILE_LOG_WARN("[DataSave] Database Error: Transaction rolled back. Exception occurred - %s", e.what());
        return false;
    }
    catch (...)
    {
        RollbackTransaction(db);
        FILE_LOG_ERROR("[DataSave] Database Error: Unknown exception occurred. Transaction rolled back.");
        return false;
    }
    FILE_LOG_INFO("[DataSave] Database : Data saved to database successfully!");

    //glassIndex++;
    FILE_LOG_INFO("[DataSave] Successfully!");
    return true;
}

std::string DataSave::GetTimeProduce()
{
   // std::string time_ = std::string(sqlUtils.getCurrentTime());
    return std::string(sqlUtils.getCurrentTime());
}

bool DataSave::saveData2Files(QueueDefectItem& resultItem)
{
    FILE_LOG_INFO("[DataSave] Begin!");
    /****** STEP0: Check 数据**********/

    if ((resultItem.image_0.empty() || resultItem.image_1.empty() || resultItem.image_2.empty()) /*&& mergeInfo.size()==0*/)
    {
        FILE_LOG_ERROR("largeImgs is empty!");
        return false;
    }

    if (resultItem.backgroundImage.empty())
    {
        FILE_LOG_ERROR("backGroundImage is empty!");
        return false;
    }

    FILE_LOG_INFO(" Check invalid data successfully!");

    /****** STEP1: 创建记录数据对应的文件夹**********/
    GlassInfoProduce tempProduce;
    GlassErrorInfoRecords tempRecords;

    std::string nameSave = resultItem.timeProduce + "_" + std::to_string(resultItem.glassIndex);
    std::string recordPath = resultItem.resultPath;
    //对保存路径进行二次校验
    if (!std::filesystem::exists(resultItem.resultPath))
    {
        FILE_LOG_ERROR(u8"Error checking record directory: %s", resultItem.resultPath);
        return false;
    }
    FILE_LOG_INFO("Check directory successfully!");

    /****** STEP4: 保存图像PNG/H5、瑕疵json等数据**********/

    ImageSaver* m_imgSaver = nullptr;
    if (m_imgSaver == nullptr)
    {
        m_imgSaver = new ImageSaver();
    }

    //if (!LOOPTEST)   //测试时，不保存三通道大图
    {

        if (!resultItem.image_0.empty())
        {
            std::string saveImage = recordPath + "/" + nameSave + "_0.png";
            //cv::imwrite(saveImage, largeImgs[0]);
            m_imgSaver->saveImageAsync(saveImage, resultItem.image_0);       // 异步保存图像
        }
        if (!resultItem.image_1.empty())
        {
            std::string saveImage = recordPath + "/" + nameSave + "_1.png";
            //cv::imwrite(saveImage, largeImgs[1]);
            m_imgSaver->saveImageAsync(saveImage, resultItem.image_1);       // 异步保存图像
        }
        if (!resultItem.image_2.empty())
        {
            std::string saveImage = recordPath + "/" + nameSave + "_2.png";
            //cv::imwrite(saveImage, largeImgs[2]);
            m_imgSaver->saveImageAsync(saveImage, resultItem.image_2);       // 异步保存图像
        }
    }
    if (!resultItem.defectImages_ch0.empty())
    {
        std::string saveDetectImage = recordPath + "/" + nameSave + "_detectImages_0.h5";
        m_imgSaver->saveHDF5(saveDetectImage, resultItem.defectImages_ch0);
    }
    if (!resultItem.defectImages_ch1.empty())
    {
        std::string saveDetectImage = recordPath + "/" + nameSave + "_detectImages_1.h5";
        m_imgSaver->saveHDF5(saveDetectImage, resultItem.defectImages_ch1);
    }
    if (!resultItem.defectImages_ch2.empty())
    {
        std::string saveDetectImage = recordPath + "/" + nameSave + "_detectImages_2.h5";
        m_imgSaver->saveHDF5(saveDetectImage, resultItem.defectImages_ch2);
    }
    if (!resultItem.backgroundImage.empty())
    {
        std::string saveImage = recordPath + "/" + nameSave + "backgroundImage.jpg";
        m_imgSaver->saveImageAsync(saveImage, resultItem.backgroundImage);
    }
    if (!resultItem.pseudoColorImage.empty())
    {
        std::string saveImage = recordPath + "/" + nameSave + "pseudoColorImage.jpg";
        m_imgSaver->saveImageAsync(saveImage, resultItem.pseudoColorImage);
    }
    delete m_imgSaver;
    m_imgSaver = nullptr;
    FILE_LOG_INFO("Save images after!");

    std::string saveJson = recordPath + "/" + nameSave + ".json";
    int glassLevel = 0;

    r2jUtils.saveData2Json(resultItem.drawInfo, glassLevel, resultItem.glassPixelWidth, resultItem.glassPixelHeight, saveJson);
    FILE_LOG_INFO("SaveData2Json successfully!");
    /******************************************************/

    FILE_LOG_INFO("Successfully!");
    return true;
}


bool DataSave::saveData2Database(QSqlDatabase& db, QueueDefectItem& resultItem, const std::string& customer)
{
    FILE_LOG_INFO("Begin!");
    if (!db.open())
    {
        FILE_LOG_ERROR("Check Error: Open db Failed! Save data Failed!");
        return false;
    }

    GlassInfoProduce tempProduce;
    GlassErrorInfoRecords tempRecords;

    /****** STEP3: 整理数据并写入数据库**********/
    //整理数据
    tempProduce.errrank_produce = resultItem.NgFlag;
    if (resultItem.drawInfo.empty())
    {
        sqlUtils.setProduceInfo(tempProduce, resultItem.timeProduce, std::to_string(resultItem.glassIndex), 1, resultItem.drawInfo.size(), tempProduce.errrank_produce,
            std::to_string(resultItem.glassPhysicalHeight), std::to_string(resultItem.glassPhysicalWidth), "120", "ReasonExample", customer);
        sqlUtils.setRecordsInfo(tempRecords, resultItem.timeProduce + std::to_string(resultItem.glassIndex), resultItem.timeProduce, resultItem.resultPath, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    else
    {
        sqlUtils.setProduceInfo(tempProduce, resultItem.timeProduce, std::to_string(resultItem.glassIndex), 1, resultItem.drawInfo.size(), tempProduce.errrank_produce,
            std::to_string(resultItem.glassPhysicalHeight), std::to_string(resultItem.glassPhysicalWidth), "120", "ReasonExample", customer);

        std::vector<int> TypeNum = GetDefectTypeNum(resultItem.drawInfo);   //获得每种瑕疵的个数

        sqlUtils.setRecordsInfo(tempRecords, resultItem.timeProduce + std::to_string(resultItem.glassIndex), resultItem.timeProduce, resultItem.resultPath,
            TypeNum[0], TypeNum[1], TypeNum[2], TypeNum[3], TypeNum[4],
            TypeNum[5], TypeNum[6], TypeNum[7], TypeNum[8], TypeNum[9],
            TypeNum[10], TypeNum[11], TypeNum[12]);
    }

    // 写入数据库
        /******************保存数据到数据库********************/
    if (!StartTransaction(db))   // 开始事务
    {
        return false;
    }
    try
    {
        int flag = InsertGlassInfo(db, tempProduce);
        if (!flag)
        {
            //FILE_LOG_WARN("[DataSave] Database Error: Insert GlassInfo failed: \n%s",);
            return false;
        }
        if (!InsertGlassErrorInfo(db, tempRecords))
        {
            //FILE_LOG_WARN("[DataSave] Database Error: Insert GlassErrorInfo failed.");
            return false;
        }
        // 提交事务
        if (!CommitTransaction(db))
        {
            FILE_LOG_WARN("CommitTransaction. Transaction rolled back.");
            return false;
        }
        QSqlQuery query(db);
        query.exec("PRAGMA wal_checkpoint(PASSIVE);");
    }
    catch (const std::exception& e)
    {
        RollbackTransaction(db);
        FILE_LOG_WARN("Transaction rolled back. Exception occurred - %s", e.what());
        return false;
    }
    catch (...)
    {
        RollbackTransaction(db);
        FILE_LOG_ERROR("Unknown exception occurred. Transaction rolled back.");
        return false;
    }
    FILE_LOG_INFO("Data saved to database successfully!");

    return true;
}

bool DataSave::CreatResultDirectory(const std::string& path, std::string& outPath, int& outGlassIndex)
{
    std::string rootPath/* = QApplication::applicationDirPath().toStdString()*/;
    rootPath = RecipeInfo.global.dataSavePath.toStdString();
    //std::string time_ = sqlUtils.getCurrentTime();
    std::string time_ = path;
    outGlassIndex = glassIndex;
    std::string nameSave = time_ + "_" + std::to_string(outGlassIndex);
    //std::string mainPath = rootPath + "/SaveData/";
    std::string mainPath = rootPath + "/";
    outPath = mainPath + nameSave;
    try
    {
        if (!std::filesystem::exists(outPath) || !std::filesystem::is_directory(outPath))
        {
            std::filesystem::create_directories(outPath);
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        FILE_LOG_ERROR(u8"[DataSave] Check Error: Error checking record directory: %s", e.what());
        return false;
    }
    //对保存路径进行二次校验
    if (!std::filesystem::exists(outPath))
    {
        FILE_LOG_ERROR(u8"[DataSave] Check Error: Error checking record directory: %s", outPath);
        return false;
    }
    glassIndex++;
    return true;
}

void DataSave::handleSaveResult(bool success, const QString& path)
{
    if (success)
    {
        //std::cout << "Save images success." << std::endl;
        FILE_LOG_INFO("Save images success：%s",path.toStdString().c_str());
    }
    else
    {
        //std::cout << "Save images failed." << std::endl;
        //qDebug() << u8"保存失败：" << path;
        FILE_LOG_INFO("Save images failed：%s", path.toStdString().c_str());
    }
}

//更新数据库。用于调整单个瑕疵信息。
bool DataSave::operator()(QSqlDatabase& db, std::vector<drawInformation>& mergeInfo, std::string& primaryKey
    ,float GlassPixelWidth, float GlassPixelHeight)
{
    FILE_LOG_INFO("DataSave: Start update!");
    if (!db.isOpen())
    {
        if (!db.open())
        {
            FILE_LOG_INFO("DataSave: Open db Failed! Save data Failed!");
            return false;
        }
    }
    if (!StartTransaction(db))   // 开始事务
    {
        return false;
    }
    try {
        GlassInfoProduce        tempProduce;
        GlassErrorInfoRecords   tempRecords;

        //1.查找对应的数据库信息
        QSqlQuery query(db);
        QString sql = QString("select * from glassinfo_produce where time_produce = '%1'").arg(QString::fromStdString(primaryKey));
        query.exec(sql);
        if (query.next())
        {
            //?  ?GlassInfoProduce ?  
            SetProduceInfo(query, tempProduce);
            tempProduce.errnum_produce = mergeInfo.size();
            int errrank_produce = 1;
            for (int i = 0; i < mergeInfo.size(); i++)
            {
                //20250705: 按照新规则，有严重缺陷的才视为NG
                if (mergeInfo[i].ErrorType == DefectLevel::SERIOUS)
                {
                    errrank_produce = 0;
                    break;
                }
            }
            tempProduce.errrank_produce = errrank_produce;

            //tempProduce.errnum_produce = mergeInfo.size();
            //tempProduce.errrank_produce = mergeInfo.size() > 0 ? 0 : 1;
        }

        QString sql_errorInfo = QString("select * from glasserrorinfo_records where id_records = '%1'").arg(QString::fromStdString(primaryKey) + QString::fromStdString(tempProduce.id_produce));
        query.clear();
        query.exec(sql_errorInfo);
        if (query.next())
        {
            SetRecordInfo(query, tempRecords);
            auto typeNum = GetDefectTypeNum(mergeInfo);
            tempRecords.errnum0_records  = typeNum.at(0);
            tempRecords.errnum1_records  = typeNum.at(1);
            tempRecords.errnum2_records  = typeNum.at(2);
            tempRecords.errnum3_records  = typeNum.at(3);
            tempRecords.errnum4_records  = typeNum.at(4);
            tempRecords.errnum5_records  = typeNum.at(5);
            tempRecords.errnum6_records  = typeNum.at(6);
            tempRecords.errnum7_records  = typeNum.at(7);
            tempRecords.errnum8_records  = typeNum.at(8);
            tempRecords.errnum9_records  = typeNum.at(9);
            tempRecords.errnum10_records = typeNum.at(10);
            tempRecords.errnum11_records = typeNum.at(11);
            tempRecords.errnum12_records = typeNum.at(12);
        }

        /* 更新数据库 */
        updataGlassInfo(db, tempProduce, tempRecords);

        /* 更新json文件 */
        std::string nameSave = tempProduce.time_produce + "_" + tempProduce.id_produce;
        std::filesystem::path path = tempRecords.detail_records + "/" + nameSave + ".json";
        if (std::filesystem::exists(path))
        {
            std::filesystem::remove(path);
        }
        float GlassPixelWidth = 0.0f;
        float GlassPixelHeight = 0.0f;
        r2jUtils.saveData2Json(mergeInfo, 0, GlassPixelWidth, GlassPixelHeight, path.string());
        // 提交事务
        if (!CommitTransaction(db))
        {
            return false;
        }
    }
    catch (const std::exception& e)
    {
        RollbackTransaction(db);
        FILE_LOG_ERROR("DataSave: Update failed.Transaction rolled back:   %s. ", e.what());
        return false;
    }
    catch (...)
    {
        //回滚事务
        RollbackTransaction(db);
        FILE_LOG_ERROR("DataSave: Update failed. Transaction rolled back.");
        return false;
    }
    FILE_LOG_INFO("DataSave: Start update success!");
    return true;
}

bool DataSave::StartTransaction(QSqlDatabase& db)
{
    if (m_transactionAction)
    {
        return true; // 避免重复开启
    }

    if (!db.isOpen())
    {
        FILE_LOG_ERROR("DataSave Error: Database is not open. Cannot start transaction.");
        return false;
    }

    if (!db.transaction())
    {
        FILE_LOG_ERROR("DataSave Error: Failed to start transaction. ErrorMessage: %s", db.lastError().text().toStdString().c_str());
        m_transactionAction = false;
        return false;
    }
    m_transactionAction = true;
    return true;
}

bool DataSave::CommitTransaction(QSqlDatabase& db)
{
    if (!m_transactionAction) {
        return true; // 没有事务就直接返回成功
    }

    if (!db.isOpen()) {
        FILE_LOG_ERROR("DataSave Error: Database is not open. Cannot commit transaction.");
        return false;
    }

    if (!db.commit()) {
        FILE_LOG_ERROR("DataSave Error: Commit failed. ErrorMessage: %s", db.lastError().text().toStdString().c_str());
        db.rollback(); // 自动回滚
        m_transactionAction = false;
        return false;
    }

    m_transactionAction = false;
    return true;
}

void DataSave::RollbackTransaction(QSqlDatabase& db)
{
    if (m_transactionAction && db.isOpen())
    {
        db.rollback();
        m_transactionAction = false;
        FILE_LOG_INFO("DataSave: Transaction rolled back.");
    }
}

void DataSave::SaveDefectInfoToJson(std::vector<drawInformation>& mergeInfo,int Level, float glassPixelWidth, float glassPixelHeight, std::string savePath)
{
    r2jUtils.saveData2Json(mergeInfo, Level, glassPixelWidth, glassPixelHeight, savePath);
    FILE_LOG_INFO("[DataSave] Save: SaveData2Json successfully!");
}

int DataSave::InsertGlassInfo(QSqlDatabase& db, GlassInfoProduce& glassInfo)
{
    if (!db.isOpen())
    {
        FILE_LOG_ERROR("DataSave: Open db Failed! Insert GlassInfo Failed!");
        return false;
    }

    std::string time_produce = glassInfo.time_produce;
    std::string id = glassInfo.id_produce;
    int num = glassInfo.num_produce;
    int errnum = glassInfo.errnum_produce;
    int errrank = glassInfo.errrank_produce;
    std::string length = glassInfo.length_produce;
    std::string width = glassInfo.width_produce;
    std::string diagonal = glassInfo.diagonal_produce;
    std::string reason = glassInfo.reason_produce;
    std::string customer = glassInfo.customer_produce;

    QString sql = QString("INSERT INTO glassinfo_produce(time_produce,id_produce,num_produce,errnum_produce,errrank_produce,length_produce,width_produce,diagonal_produce,reason_produce,customer_produce) VALUES('%1','%2',%3,%4,%5,'%6','%7','%8','%9','%10')")
        .arg(time_produce.c_str()).arg(id.c_str()).arg(num).arg(errnum).arg(errrank).arg(length.c_str()).arg(width.c_str()).arg(diagonal.c_str()).arg(QString::fromLocal8Bit(reason.c_str())).arg(QString::fromLocal8Bit(customer.c_str()));

    QSqlQuery query(db);
    if (!query.exec(sql))
    {
        FILE_LOG_ERROR("DataSave: Insert GlassInfo Failed!\n%s",sql.toStdString().c_str());
        return false;
    }

    return true;
}

int DataSave::InsertGlassErrorInfo(QSqlDatabase& db, GlassErrorInfoRecords& glassErrorInfo)
{
    if (!db.isOpen())
    {
        FILE_LOG_ERROR("DataSave: Open db Failed! Insert GlassErrorInfo Failed!");
        return false;
    }
    std::string id_records = glassErrorInfo.id_records;
    std::string time_produce = glassErrorInfo.time_produce;
    std::string detail_records = glassErrorInfo.detail_records;
    int errnum0  = glassErrorInfo.errnum0_records;
    int errnum1  = glassErrorInfo.errnum1_records;
    int errnum2  = glassErrorInfo.errnum2_records;
    int errnum3  = glassErrorInfo.errnum3_records;
    int errnum4  = glassErrorInfo.errnum4_records;
    int errnum5  = glassErrorInfo.errnum5_records;
    int errnum6  = glassErrorInfo.errnum6_records;
    int errnum7  = glassErrorInfo.errnum7_records;
    int errnum8  = glassErrorInfo.errnum8_records;
    int errnum9  = glassErrorInfo.errnum9_records;
    int errnum10 = glassErrorInfo.errnum10_records;
    int errnum11 = glassErrorInfo.errnum11_records;
    int errnum12 = glassErrorInfo.errnum12_records;

    QString sql = QString("INSERT INTO glasserrorinfo_records("
        "id_records,time_produce,detail_records,errnum0_records,errnum1_records,errnum2_records,errnum3_records,errnum4_records,errnum5_records,errnum6_records, errnum7_records"
        ",errnum8_records,errnum9_records,errnum10_records,errnum11_records, errnum12_records)"
        " VALUES('%1','%2','%3',%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16)")
        .arg(id_records.c_str()).arg(time_produce.c_str()).arg(detail_records.c_str())
        .arg(errnum0).arg(errnum1).arg(errnum2).arg(errnum3).arg(errnum4)
        .arg(errnum5).arg(errnum6).arg(errnum7).arg(errnum8).arg(errnum9)
        .arg(errnum10).arg(errnum11).arg(errnum12);

    QSqlQuery query(db);
    if (!query.exec(sql))
    {
        FILE_LOG_ERROR("DataSave: Insert GlassErrorInfo Failed!\n%s", sql.toStdString().c_str());
        return false;
    }
    return true;
}

/*===========================20250214 wang          -------   ?      ? +json    ==============================================*/

int DataSave::updateGlassInfoProduce(QSqlDatabase& db, const GlassInfoProduce& glassInfo) {
    if (!db.isOpen())
    {
        FILE_LOG_ERROR("DataSave: Open db Failed! Insert GlassInfo Failed!");
        return false;
    }

    std::string time_produce = glassInfo.time_produce;//    
    std::string id = glassInfo.id_produce;
    int num = glassInfo.num_produce;
    int errnum = glassInfo.errnum_produce;
    int errrank = glassInfo.errrank_produce;
    std::string length = glassInfo.length_produce;
    std::string width = glassInfo.width_produce;
    std::string diagonal = glassInfo.diagonal_produce;
    std::string reason = glassInfo.reason_produce;
    std::string customer = glassInfo.customer_produce;

    QString sql = QString("UPDATE glassinfo_produce SET "
        "num_produce = %1, "
        "errnum_produce = %2, "
        "errrank_produce = %3, "
        "length_produce = '%4', "
        "width_produce = '%5', "
        "diagonal_produce = '%6', "
        "reason_produce = '%7', "
        "customer_produce = '%8' "
        "WHERE time_produce = '%9' AND id_produce = '%10'")
        .arg(num)
        .arg(errnum)
        .arg(errrank)
        .arg(length.c_str())
        .arg(width.c_str())
        .arg(diagonal.c_str())
        .arg(QString::fromLocal8Bit(reason.c_str()))
        .arg(QString::fromLocal8Bit(customer.c_str()))
        .arg(time_produce.c_str())
        .arg(id.c_str());
    QSqlQuery query(db);
    if (!query.exec(sql))
    {
        FILE_LOG_ERROR("DataSave: Insert GlassInfo Failed!");
        return false;
    }
}

int DataSave::updateGlassErrorInfoRecords(QSqlDatabase& db, const GlassErrorInfoRecords& glassErrorInfo) {

    if (!db.isOpen())
    {
        FILE_LOG_ERROR("DataSave: Open db Failed! Update GlassErrorInfo Failed!");
        return false;
    }

    std::string id_records = glassErrorInfo.id_records;//    
    std::string detail_records = glassErrorInfo.detail_records;
    int errnum0  = glassErrorInfo.errnum0_records;
    int errnum1  = glassErrorInfo.errnum1_records;
    int errnum2  = glassErrorInfo.errnum2_records;
    int errnum3  = glassErrorInfo.errnum3_records;
    int errnum4  = glassErrorInfo.errnum4_records;
    int errnum5  = glassErrorInfo.errnum5_records;
    int errnum6  = glassErrorInfo.errnum6_records;
    int errnum7  = glassErrorInfo.errnum7_records;
    int errnum8  = glassErrorInfo.errnum8_records;
    int errnum9  = glassErrorInfo.errnum9_records;
    int errnum10 = glassErrorInfo.errnum10_records;
    int errnum11 = glassErrorInfo.errnum11_records;
    int errnum12 = glassErrorInfo.errnum12_records;

    QString sql = QString("UPDATE glasserrorinfo_records SET detail_records='%1',"
        " errnum0_records=%2, errnum1_records=%3, errnum2_records=%4, errnum3_records=%5, errnum4_records=%6, "
        " errnum5_records=%7, errnum6_records=%8, errnum7_records=%9, errnum8_records=%10, errnum9_records=%11,"
        " errnum10_records=%12, errnum11_records=%13, errnum12_records=%14 "
        "WHERE id_records='%15'")
        .arg(detail_records.c_str()).arg(errnum0).arg(errnum1).arg(errnum2).arg(errnum3)
        .arg(errnum4).arg(errnum5).arg(errnum6).arg(errnum7).arg(errnum8)
        .arg(errnum9).arg(errnum10).arg(errnum11).arg(errnum12)
        .arg(id_records.c_str());

    QSqlQuery query(db);
    if (!query.exec(sql))
    {
        FILE_LOG_ERROR("DataSave: Update GlassErrorInfo Failed!");
        return false;
    }
    return true;
}

//  更新glassinfo_produce数据表和glasserrorinfo_records数据表
int DataSave::updataGlassInfo(QSqlDatabase& db, GlassInfoProduce& glassInfo, GlassErrorInfoRecords& glassErrorInfo)
{
    //StartTransaction(db);
    QSqlQuery query(db);
    //1. 查找数据表中对应记录
    QString selectSql = QString("select * from glasserrorinfo_records where time_produce = '%1'").arg(glassErrorInfo.time_produce.c_str());
    query.exec(selectSql);
    if (query.size() == 0)
    {

        
        QSqlQuery query_(db);
        QString selectSql_ = QString("select * from glassinfo_produce where time_produce = '%1'").arg(glassErrorInfo.time_produce.c_str());//  ?    
        query_.exec(selectSql_);

        //2.更新获插入数据到glassinfo_produce数据表和glasserrorinfo_records中  
        if (query_.size() == 0)
        {
            int flag = InsertGlassInfo(db, glassInfo);

            if (flag)
            {
                InsertGlassErrorInfo(db, glassErrorInfo);
            }
        }
        else
        {
            updateGlassInfoProduce(db, glassInfo);
            InsertGlassErrorInfo(db, glassErrorInfo);
        }
    }
    else
    {
        updateGlassInfoProduce(db, glassInfo);
        updateGlassErrorInfoRecords(db, glassErrorInfo);
    }

    //CommitTransaction(db);

    return true;
}

void DataSave::SetProduceInfo(QSqlQuery& query, GlassInfoProduce& glassInfo)
{
    glassInfo.time_produce = query.value("time_produce").toString().toStdString();
    glassInfo.id_produce = query.value("id_produce").toString().toStdString();
    glassInfo.num_produce = query.value("num_produce").toInt();
    glassInfo.errnum_produce = query.value("errnum_produce").toInt();
    glassInfo.errrank_produce = query.value("errrank_produce").toInt();
    glassInfo.length_produce = query.value("length_produce").toString().toStdString();
    glassInfo.width_produce = query.value("width_produce").toString().toStdString();
    glassInfo.diagonal_produce = query.value("diagonal_produce").toString().toStdString();
    glassInfo.reason_produce = std::string(query.value("reason_produce").toString().toLocal8Bit());
    QString temp = query.value("customer_produce").toString();
    std::string temp_ =std::string(temp.toLocal8Bit());
    glassInfo.customer_produce = temp_;
}

void DataSave::SetRecordInfo(QSqlQuery& query, GlassErrorInfoRecords& glassErrorInfo)
{
    glassErrorInfo.id_records = query.value("id_records").toString().toStdString();
    glassErrorInfo.errnum0_records  = query.value("errnum0_records"). toInt();
    glassErrorInfo.errnum1_records  = query.value("errnum1_records"). toInt();
    glassErrorInfo.errnum2_records  = query.value("errnum2_records"). toInt();
    glassErrorInfo.errnum3_records  = query.value("errnum3_records"). toInt();
    glassErrorInfo.errnum4_records  = query.value("errnum4_records"). toInt();
    glassErrorInfo.errnum5_records  = query.value("errnum5_records"). toInt();
    glassErrorInfo.errnum6_records  = query.value("errnum6_records"). toInt();
    glassErrorInfo.errnum7_records  = query.value("errnum7_records"). toInt();
    glassErrorInfo.errnum8_records  = query.value("errnum8_records"). toInt();
    glassErrorInfo.errnum9_records  = query.value("errnum9_records"). toInt();
    glassErrorInfo.errnum10_records = query.value("errnum10_records").toInt();
    glassErrorInfo.errnum11_records = query.value("errnum11_records").toInt();
    glassErrorInfo.errnum12_records = query.value("errnum12_records").toInt();
    glassErrorInfo.detail_records = query.value("detail_records").toString().toStdString();
    glassErrorInfo.time_produce = query.value("time_produce").toString().toStdString();
}

std::vector<int> DataSave::GetDefectTypeNum(const std::vector<drawInformation>& mergeInfo)
{
    std::vector<int> typeNum(13);
    for (int tn = 0; tn < mergeInfo.size(); tn++)
    {
        if (mergeInfo[tn].DefectType == DefectType::TYPE_POORCOATING) {

            //typeName = "  ?    ";
            typeNum[0]++;
        }
        else if (mergeInfo[tn].DefectType == DefectType::TYPE_SCRATCH) {

            //typeName = "    ";
            typeNum[1]++;

        }
        else if (mergeInfo[tn].DefectType == DefectType::TYPE_CALCULUS) {

            //typeName = "  ?";
            typeNum[2]++;
        }
        else if (mergeInfo[tn].DefectType == DefectType::TYPE_BUBBLE) {

            //typeName = "    ";
            typeNum[3]++;

        }
        else if (mergeInfo[tn].DefectType == DefectType::TYPE_TRADEMARK) {

            //typeName = " ? ";
            typeNum[4]++;

        }
        else if (mergeInfo[tn].DefectType == DefectType::TYPE_WATERSTAIN) {

            //typeName = "?  ";
            typeNum[5]++;
        }
        else if (mergeInfo[tn].DefectType == DefectType::TYPE_SMUDGE) {

            //typeName = "    ";
            typeNum[6]++;
        }
        else if (mergeInfo[tn].DefectType == DefectType::TYPE_SCREENPRINTING) {

            //typeName = "    ";
            typeNum[7]++;
        }
        else if (mergeInfo[tn].DefectType == DefectType::TYPE_CHIPPED_EDGE) {

            //typeName = "    ";
            typeNum[8]++;
        }
        else if (mergeInfo[tn].DefectType == DefectType::TYPE_PITTING) {

            //typeName = "    ";
            typeNum[9]++;
        }
        else if (mergeInfo[tn].DefectType == DefectType::TYPE_GLASS_CULLET) {

            //typeName = "    ";
            typeNum[10]++;
        }
        else if (mergeInfo[tn].DefectType == DefectType::TYPE_WAVINESS) {

            //typeName = "    ";
            typeNum[11]++;
        }
        else if (mergeInfo[tn].DefectType == DefectType::TYPE_OTHER) {

            //typeName = "    ";
            typeNum[12]++;
        }
    }

    return typeNum;
}
