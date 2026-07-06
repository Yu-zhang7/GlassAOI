#include "AlgorithmProcess.h"
#include "CameraManager.h"          //相机管理类
#include "Json/glassData2Json.h"    //瑕疵数据管理类(Json读写))
#include "DataSave.h"       //数据保存类(保存图像文件、保存H5文件、瑕疵数据json文件、数据库记录等)
#include "Log.hpp"
#include "GeneralMethodTuning.h"

#include "ResultBuffer.h"   //用于保存拼接后的批量图像缓存

#define GRAY 2 //当前计算场


/*=========按时间戳保存图像==============*/

// 创建保存图像的目录
bool createDirectory(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            return std::filesystem::create_directories(path);
        }
        return true;
    }
    catch (...) {
        std::cerr << "创建目录失败: " << path << std::endl;
        return false;
    }
}

// 保存带时间戳的图像
std::string saveImageWithTimestamp(cv::Mat image,
    const std::string& saveDir = "captured_images",
    const std::string& prefix = "img") {

    // 创建保存目录
    if (!createDirectory(saveDir)) {
        return "";
    }

    // 获取当前时间戳
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);

    // 获取毫秒
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;

    // 格式化时间字符串
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y%m%d_%H%M%S")
        << "_" << std::setfill('0') << std::setw(3) << ms.count();

    // 构建完整路径
    std::string filename = saveDir + "/" + prefix + "_" + ss.str() + ".jpg";

    // 保存图像
    if (cv::imwrite(filename, image)) {
        std::cout << "图像保存成功: " << filename << std::endl;
        return filename;
    }
    else {
        std::cerr << "图像保存失败!" << std::endl;
        return "";
    }
}
/*=========按时间戳保存图像==============*/

/*===临时加入镀膜不良判断======*/
// 在 AlgorithmProcess.cpp 中实现函数
void redefineCoatingDefects(const cv::Mat& largeImage, std::vector<drawInformation>& defects) {
    if (largeImage.empty()) {
        FILE_LOG_INFO("[Algotithm] redefineCoatingDefects: largeImage is empty");
        return;
    }

    if (largeImage.channels() != 1) {
        FILE_LOG_INFO("[Algotithm] redefineCoatingDefects: largeImage must be single channel");
        return;
    }

    //FILE_LOG_INFO("[Algotithm] redefineCoatingDefects: Processing %d defects", static_cast<int>(defects.size()));

    //int redefinedCount = 0;

    for (auto& defect : defects) {
        // 只处理原来就是镀膜不良类型的缺陷
        if (defect.DefectType == DefectType::TYPE_SMUDGE || defect.DefectType == DefectType::TYPE_BUBBLE || defect.DefectType == DefectType::TYPE_CALCULUS || defect.DefectType == DefectType::TYPE_WATERSTAIN) {
            // 确保矩形在图像范围内
            cv::Rect validRect = defect.rect & cv::Rect(0, 0, largeImage.cols, largeImage.rows);

            if (validRect.width > 0 && validRect.height > 0) {
                try {
                    // 提取ROI区域
                    cv::Mat roiImg = largeImage(validRect);

                    // 创建掩码，查找灰度值为255的像素
                    cv::Mat mask;
                    cv::inRange(roiImg, cv::Scalar(255), cv::Scalar(255), mask);

                    // 统计255像素数量
                    int count = cv::countNonZero(mask);

                    // 如果255像素数量超过阈值，则确认为镀膜不良
                    if (count > 5) {
                        // 保持为镀膜不良类型
                        defect.DefectType = TYPE_POORCOATING;
                        defect.ErrorType = DefectLevel::MEDIUM;
                        // 可以在这里添加其他属性的更新
                        //redefinedCount++;
                    }
                    else {
                        // 如果不满足条件，可以考虑改为其他类型或保持原类型
                        // 这里根据你的需求决定是否需要重新分类
                        // defect.DefectType = TYPE_OTHER; // 示例：改为其他类型
                    }
                }
                catch (const cv::Exception& e) {
                    FILE_LOG_ERROR("[Algotithm] redefineCoatingDefects: OpenCV exception - %s", e.what());
                    continue;
                }
                catch (...) {
                    FILE_LOG_ERROR("[Algotithm] redefineCoatingDefects: Unknown exception occurred");
                    continue;
                }
            }
        }
    }

}
/*===临时加入镀膜不良判断======*/


std::string getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(localtime(&time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

void AlgorithmProcess::InitAlgorithmProcess()
{

}

AlgorithmProcess& AlgorithmProcess::GetInstance()
{
    static AlgorithmProcess instance;
    return instance;
}

void AlgorithmProcess::DestroyInstance()
{
    //instance.reset();
}


AlgorithmProcess::AlgorithmProcess()
{
}
/*=========计算玻璃高度尺寸=============*/
std::vector<double> calculateROIRatios(
    const cv::Rect& roi,
    const std::vector<int>& cameraWidths)
{
    std::vector<double> ratios;
    if (cameraWidths.empty() || roi.width <= 0) return ratios;

    ratios.reserve(cameraWidths.size());

    int currentX = 0;  // 当前相机区域在大图中的起始X坐标
    int roiStartX = roi.x;
    int roiEndX = roi.x + roi.width;

    for (int camWidth : cameraWidths) {
        int camEndX = currentX + camWidth;

        // 计算当前相机区域与ROI的重叠部分
        int overlapStart = std::max(roiStartX, currentX);
        int overlapEnd = std::min(roiEndX, camEndX);
        int overlapWidth = std::max(0, overlapEnd - overlapStart);

        // 计算当前相机区域中ROI的占比
        double ratio = (camWidth > 0) ?
            static_cast<double>(overlapWidth) / camWidth : 0.0;

        ratios.push_back(ratio);
        currentX = camEndX;  // 移动到下一个相机区域
    }

    return ratios;
}

/*=========计算玻璃高度尺寸=============*/

cv::Mat drawResultInfoImage(const cv::Mat& img, const std::vector<drawInformation>& batchDrawInfo) {

    cv::Mat drawImage = img.clone();    
    cv::cvtColor(drawImage, drawImage, cv::COLOR_BGR2RGB);
    if (batchDrawInfo.empty())
        return drawImage;
    for (int i = 0; i < batchDrawInfo.size(); i++) {

        cv::rectangle(drawImage, batchDrawInfo[i].rect, cv::Scalar(0, 0, 255), 2);



        // 标注类别 (0-7)
        //std::string label = std::to_string(static_cast<int>(saveInformations[i].DefectType));
        // 标注类别 (0-7)
        std::string label;
        if (batchDrawInfo[i].DefectType == TYPE_POORCOATING) {
            label = "0";
        }
        else if (batchDrawInfo[i].DefectType == TYPE_SCRATCH) {
            label = "1";
        }
        else if (batchDrawInfo[i].DefectType == TYPE_CALCULUS) {
            label = "2";
        }
        else if (batchDrawInfo[i].DefectType == TYPE_BUBBLE) {
            label = "3";
        }
        else if (batchDrawInfo[i].DefectType == TYPE_TRADEMARK) {
            label = "4";
        }
        else if (batchDrawInfo[i].DefectType == TYPE_WATERSTAIN) {
            label = "5";
        }
        else if (batchDrawInfo[i].DefectType == TYPE_SMUDGE) {
            label = "6";
        }
        //else if (batchDrawInfo[i].DefectType == TYPE_ScreenPrintingDefects) {
        //    label = "7";
        //}
        else {
            label = "8";
        }

        // 计算文本位置（矩形框左上角上方）
        cv::Point textOrg(
            batchDrawInfo[i].rect.x,
            batchDrawInfo[i].rect.y - 5
        );

        // 如果矩形框太靠上，将文本放在框内
        if (batchDrawInfo[i].rect.y < 20) {
            textOrg.y = batchDrawInfo[i].rect.y + 15;
        }

        // 绘制类别文本
        cv::putText(
            drawImage,
            label,
            textOrg,
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            cv::Scalar(0, 0, 255),
            1
        );
    }
    return drawImage;
}

bool AlgorithmProcess::DetectImages(QueueDefectItem& resultItem, const std::string& resultPath)
{
    FILE_LOG_INFO("[Algotithm] DetectImages: BEGIN");

    ResultBuffer& buffer = ResultBuffer::GetInstance();

    //读取网络置信度，并进行算法计算
    std::vector<drawInformation> results_beforeSort;
    std::vector<drawInformation> results_;//根据缩放比例存放新的值

   //旋转方向
    int RotationDirectionFlag = 0; //0-顺时针旋转90度，1-逆时针旋转90度
    RotationDirectionFlag = 0;//俄罗斯现场是逆时针 取1															
    cv::Mat largeImage_C1, largeImage_C2, largeImage_C3;
    //实时采图+算法模式和读图+算法模式
    if (IS_READ_MODE == 0 || IS_READ_MODE == 1)
    {
          FILE_LOG_INFO("[Algotithm] DetectImages: %s，RotationDirectionFlag:%d", (IS_READ_MODE == 0 ? "REAL_MODE BEGIN" : "READ_MODE_1 BEGIN"), RotationDirectionFlag);
        //if (IS_READ_MODE == 1)
        //{
        //    ReadBatchImagesFromPath(); 
        //    m_readBatchImageIndex = 0;
        //}
        cv::Mat batchImg;
        std::vector<drawInformation> batchDrawInfo;
        std::vector<std::vector<drawInformation>> batchDrawInfoArray;

        //std::vector<cv::Mat> batchImgItem;//存放未拆分三通道的数据，用于分析错场问题
        std::vector<cv::Mat> showImage_Batch;

        std::vector<cv::Mat> showImage_ch0;
        std::vector<cv::Mat> showImage_ch1;
        std::vector<cv::Mat> showImage_ch2;

        //std::vector<cv::Mat> drawResultImage;


        size_t total = 0;   //记录drawInformation的总数
        int LoopCount = 0;  //循环计数
        int GetImageErrorCount = 0; //批次采图错误计数。用于20信号异常未接收到时，自行跳出。

        bool isStart = false;
        FILE_LOG_INFO("[Algotithm] DetectImages: Loop begin.");
        while (m_isSignalRunning.load())
        {
            //Sleep(1000);
            //循环等待第一组图像获取完成
            //if (!m_ImageProcessState)   //等待图像处理信号启动。(信号来自CameraManager)
            //{
            //    std::this_thread::sleep_for(std::chrono::milliseconds(50));
            //    continue;
            //}

            //防护动作。避免循环死锁     //新硬件，暂时不考虑硬件信号不稳定情况。暂时注释防死锁逻辑。
            if (GetImageErrorCount >= 30)
            {
                //emit StopProcessSinals(false);
                break;
            }

            if (buffer.IsQueueEmpty())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            FILE_LOG_DEBUG("[Algotithm] DetectImages: LOOP[%d] Begin.", LoopCount);
            // 1. 获取批次图像
            batchImg.release();
            showImage_Batch.clear();
            std::vector<cv::Mat> batchEntireImage;
            std::vector<drawInformation> batchDrawInfo;
            cv::Rect batchRect;

            bool bRet = false;
            bRet = buffer.PopFrame(batchImg, 5);   //等待合并7个相机的帧图
            FILE_LOG_DEBUG("[Algotithm] DetectImages: PopFrame .");
            if (!bRet)
            {
                GetImageErrorCount++;
                FILE_LOG_DEBUG("[Algotithm] DetectImages: LOOP[%d] ComputeImage_Signal Failed! LOOP NEXT. EmptyCount = %d", LoopCount, GetImageErrorCount);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }
            FILE_LOG_DEBUG("[Algotithm] DetectImages: ComputeImage_Signal begin.");
            try
            {
                bRet = AlgorithmProcess::GetInstance().ComputeImage_Signal(batchImg, batchEntireImage, batchDrawInfo);
                //batchImgItem.push_back(batchImg);

                if (!bRet)
                {
                    GetImageErrorCount++;
                    FILE_LOG_DEBUG("[Algotithm] DetectImages: LOOP[%d] ComputeImage_Signal Failed! LOOP NEXT. EmptyCount = %d", LoopCount, GetImageErrorCount);
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    continue;
                }
            }
            catch (const std::exception&)
            {
                FILE_LOG_DEBUG("[Algotithm] DetectImages: ComputeImage_Signal Error.");
                continue;
            }
            FILE_LOG_DEBUG("[Algotithm] DetectImages: ComputeImage_Signal after.");
            /*=====绘制单帧计算结果=======*/
            //cv::Mat drawImageBatch  = drawResultInfoImage(batchEntireImage[2], batchDrawInfo);
            //drawResultImage.push_back(drawImageBatch);
            /*=====绘制单帧计算结果=======*/


            /****保存每批次拆分三通道图*****/
            ImageSaver* m_imgSaver = nullptr;
            if (m_imgSaver == nullptr)
            {
                m_imgSaver = new ImageSaver();
            }
            ////////for (size_t i = 0; i < batchEntireImage.size(); i++)
            ////////{
            ////////    if (!batchEntireImage[i].empty())
            ////////    {
            ////////        std::string saveImage = resultPath + "/" + std::to_string(LoopCount) + "_" + std::to_string(i) + ".png";
            ////////        //cv::imwrite(saveImage, batchEntireImage[i]);
            ////////        m_imgSaver->saveImageAsync(saveImage, batchEntireImage[i]);       // 异步保存图像
            ////////    }
            ////////}
            /*******************************/

            GetImageErrorCount = 0; //不连续的拿图失败时，重置错误计数
            FILE_LOG_DEBUG("[Algotithm] DetectImages: LOOP[%d] ComputeImage_Signal Completed!", LoopCount);


            showImage_ch0.push_back(batchEntireImage[0]);
            showImage_ch1.push_back(batchEntireImage[1]);
            showImage_ch2.push_back(batchEntireImage[2]);
            batchDrawInfoArray.emplace_back(batchDrawInfo);


            //cv::imwrite("./1/batch_" + std::to_string(LoopCount) + "_0.jpg", matsEntire[0]);
            //cv::imwrite("./1/batch_" + std::to_string(LoopCount) + "_1.jpg", matsEntire[1]);
            //cv::imwrite("./1/batch_" + std::to_string(LoopCount) + "_2.jpg", matsEntire[2]);


            total += batchDrawInfo.size();
            FILE_LOG_DEBUG("[Algotithm] LOOP[%d] Completed.", LoopCount);
            LoopCount++;
        }
        FILE_LOG_DEBUG("[Algotithm] Loop 1st Completed!!--------------------");

        /***********处理剩余BatchImage*****************************************/

        int lastCount = 0;
        while (buffer.GetQueueSize() > 0)
        {
            FILE_LOG_DEBUG("[Algotithm] DetectImages: LOOP_LAST[%d] Begin.", LoopCount);
            // 1. 获取批次图像
            batchImg.release();
            showImage_Batch.clear();
            std::vector<cv::Mat> batchEntireImage;
            std::vector<drawInformation> batchDrawInfo;
            cv::Rect batchRect;

            bool bRet = false;
            bRet = buffer.PopFrame(batchImg, 5);   //等待合并帧图，超时5秒
            if (!bRet)
            {
                GetImageErrorCount++;
                FILE_LOG_DEBUG("[Algotithm] DetectImages: LOOP_LAST[%d] ComputeImage_Signal Failed! LOOP NEXT. EmptyCount = %d", LoopCount, GetImageErrorCount);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            try
            {
                bRet = AlgorithmProcess::GetInstance().ComputeImage_Signal(batchImg, batchEntireImage, batchDrawInfo);
                //batchImgItem.push_back(batchImg);

                // /*=====保存单帧计算结果=======*/
                //std::string saveJsonPath = resultPath + "/tmp_frame_source" + std::to_string(LoopCount) + ".json";
                //DataSave dataSave;
                //dataSave.SaveDefectInfoToJson(batchDrawInfo, resultItem.NgFlag, resultItem.glassPixelWidth, resultItem.glassPixelHeight, saveJsonPath);
                ///*=====保存单帧计算结果=======*/
            }
            catch (const std::exception&)
            {
                FILE_LOG_DEBUG("[Algotithm] DetectImages: ComputeImage_Signal Error.");
                continue;
            }


            /*=====绘制单帧计算结果=======*/
            //cv::Mat drawImageBatch = drawResultInfoImage(batchEntireImage[2], batchDrawInfo);
            //drawResultImage.push_back(drawImageBatch);
            /*=====绘制单帧计算结果=======*/




            if (!bRet)
            {
                GetImageErrorCount++;
                FILE_LOG_DEBUG("[Algotithm] DetectImages: LOOP_LAST[%d] ComputeImage_Signal Failed! LOOP NEXT. EmptyCount = %d", LoopCount, GetImageErrorCount);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }
            GetImageErrorCount = 0; //不连续的拿图失败时，重置错误计数
            FILE_LOG_DEBUG("[Algotithm] DetectImages: LOOP_LAST[%d] GetImageOnce Completed!", LoopCount);


            showImage_ch0.push_back(batchEntireImage[0]);
            showImage_ch1.push_back(batchEntireImage[1]);
            showImage_ch2.push_back(batchEntireImage[2]);
            batchDrawInfoArray.emplace_back(batchDrawInfo);

            //cv::imwrite("./1/batch_" + std::to_string(LoopCount) + "_0.jpg", matsEntire[0]);
            //cv::imwrite("./1/batch_" + std::to_string(LoopCount) + "_1.jpg", matsEntire[1]);
            //cv::imwrite("./1/batch_" + std::to_string(LoopCount) + "_2.jpg", matsEntire[2]);


            total += batchDrawInfo.size();
            FILE_LOG_DEBUG("[Algotithm] LOOP_LAST[%d] Completed.", LoopCount);
            LoopCount++;
        }
        /**********************************************************************/
        FILE_LOG_DEBUG("[Algotithm] DetectImages: heightSingle value %d!!---------------------------", heightSingle);

        std::vector<drawInformation> mergedDefects;
        try
        {
            mergedDefects = m_Compute.mergeDefectsToLargeImage(batchDrawInfoArray, heightSingle);
        }
        catch (const std::exception&)
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages: mergeDefectsToLargeImage Error!!---------------------------");
        }

        FILE_LOG_INFO("[Algotithm] DetectImages: mergeDefectsToLargeImage Completed!");

        //边采边计算多帧图像合并(约30ms-100ms)
        //cv::vconcat(showImage_ch0, largeImage_C1);
        //cv::vconcat(showImage_ch1, largeImage_C2);
        //cv::vconcat(showImage_ch2, largeImage_C3);
   

        cv::vconcat(showImage_ch0, largeImage_C1);
        cv::vconcat(showImage_ch1, largeImage_C2);
        cv::vconcat(showImage_ch2, largeImage_C3);

        //cv::imwrite("largeImage_C1.png", largeImage_C1);
        //cv::imwrite("largeImage_C2.png", largeImage_C2);
        //cv::imwrite("largeImage_C3.png", largeImage_C3);

        //cv::Mat channel3Image;
        //cv::vconcat(batchImgItem, channel3Image);
        //cv::imwrite("channel3Image.png", channel3Image);
        //cv::Mat drawInfoImage;
        //cv::vconcat(drawResultImage, drawInfoImage);
        //cv::Mat drawInfoImage2 = drawResultInfoImage(largeImage_C3, mergedDefects);

        //三通道未拆分图像保存
        {
          /*  cv::Mat channel3Data;
            cv::vconcat(batchImgItem, channel3Data);
            std::string base_path = "D:/channel3Data/";
            namespace fs = std::filesystem;
            fs::path dir_path(base_path);

            if (!fs::exists(dir_path)) {
                if (fs::create_directories(dir_path)) {
                    std::cout << "目录创建成功: " << base_path << std::endl;
                }
                else {
                    std::cout << "目录创建失败，使用当前目录" << std::endl;
                    base_path = "./";
                }
            }
            else if (!fs::is_directory(dir_path)) {
                std::cout << "路径存在但不是目录，使用当前目录" << std::endl;
                base_path = "./";
            }
            else {
                std::cout << "目录已存在: " << base_path << std::endl;
            }

            if (!channel3Data.empty()) {
                std::string filename = base_path + "capture_" + getCurrentTimeString() + ".png";

                if (imwrite(filename, channel3Data)) {
                    std::cout << "SaveSucess: " << filename << std::endl;
                }
                else {
                    std::cout << "SaveFailed: " << filename << std::endl;
                }
            }*/

        }
 


        FILE_LOG_INFO("[Algotithm] DetectImages: Vconcat largeImage Completed! width:%d,height:%d", largeImage_C2.cols, largeImage_C2.rows);
        //获取玻璃区域
        cv::Rect mergedRect;
        std::vector<cv::Mat> showImage;

#if 0  //使用老版算法
        int ret = m_Compute.getGlassRoi(largeImage_C2, mergedRect, glassThreshold_val);
        //int ret = m_Compute.getGlassRoiSample(largeImage_C2, mergedRect, glassThreshold_val,0.1);
        if (ret == -1) {

            //std::cout << "No Glass Area" << std::endl;
            FileLogPrintf("[Algotithm] DetectImages: No Glass Area!!---------------------------");

            std::string base_path = "D:/captureError/";
            namespace fs = std::filesystem;
            fs::path dir_path(base_path);
            if (!fs::exists(dir_path)) {
                if (fs::create_directories(dir_path)) {
                    std::cout << "目录创建成功: " << base_path << std::endl;
                }
                else {
                    std::cout << "目录创建失败，使用当前目录" << std::endl;
                    base_path = "./";
                }
            }
            else if (!fs::is_directory(dir_path)) {
                std::cout << "路径存在但不是目录，使用当前目录" << std::endl;
                base_path = "./";
            }
            else {
                std::cout << "目录已存在: " << base_path << std::endl;
            }
            if (!largeImage_C1.empty()) {
                std::string filename = base_path + "capture_" + getCurrentTimeString() + "_1.png";

                if (imwrite(filename, largeImage_C1)) {
                    std::cout << "保存成功: " << filename << std::endl;
                }
            }
            if (!largeImage_C2.empty()) {
                std::string filename = base_path + "capture_" + getCurrentTimeString() + "_2.png";

                if (imwrite(filename, largeImage_C2)) {
                    std::cout << "保存成功: " << filename << std::endl;
                }
            }
            if (!largeImage_C3.empty()) {
                std::string filename = base_path + "capture_" + getCurrentTimeString() + "_3.png";

                if (imwrite(filename, largeImage_C3)) {
                    std::cout << "保存成功: " << filename << std::endl;
                }
            }
            return false;
        }
        FILE_LOG_INFO("[Algotithm] DetectImages: getGlassRoi Completed! Roi:(%d,%d,%d,%d)", mergedRect.x, mergedRect.y, mergedRect.width, mergedRect.height);
#else
        int showGrayValue = 100;//玻璃区域显示灰度值
        int BackgroundColor = 200;//背景区域显示灰度值
        cv::Mat glassMaskImage;
        std::vector<drawInformation> drawInfosFilter;
        //cv::Mat glassMaskImageVirtual;
        bool ret = m_showVirtualInfo.GetGlassAreaAndResult(largeImage_C2, glassThreshold_val, showGrayValue, mergedDefects, 180,
            mergedRect, glassMaskImage, drawInfosFilter);
        
   //     {//临时存图，判断玻璃裁剪不全问题
			//saveImageWithTimestamp(largeImage_C2, "D:/ErrorImage", "glassMask");
   //     }

        if (!ret)
        {
            FILE_LOG_INFO("[Algotithm] DetectImages: No Glass Area!!---------------------------");

           /* std::string base_path = "D:/captureError/";
            namespace fs = std::filesystem;
            fs::path dir_path(base_path);
            if (!fs::exists(dir_path)) {
                if (fs::create_directories(dir_path)) {
                    std::cout << "目录创建成功: " << base_path << std::endl;
                }
                else {
                    std::cout << "目录创建失败，使用当前目录" << std::endl;
                    base_path = "./";
                }
            }
            else if (!fs::is_directory(dir_path)) {
                std::cout << "路径存在但不是目录，使用当前目录" << std::endl;
                base_path = "./";
            }
            else {
                std::cout << "目录已存在: " << base_path << std::endl;
            }
            if (!largeImage_C1.empty()) {
                std::string filename = base_path + "capture_" + getCurrentTimeString() + "_1.png";

                if (imwrite(filename, largeImage_C1)) {
                    std::cout << "保存成功: " << filename << std::endl;
                }
            }
            if (!largeImage_C2.empty()) {
                std::string filename = base_path + "capture_" + getCurrentTimeString() + "_2.png";

                if (imwrite(filename, largeImage_C2)) {
                    std::cout << "保存成功: " << filename << std::endl;
                }
            }
            if (!largeImage_C3.empty()) {
                std::string filename = base_path + "capture_" + getCurrentTimeString() + "_3.png";

                if (imwrite(filename, largeImage_C3)) {
                    std::cout << "保存成功: " << filename << std::endl;
                }
            }*/
            return false;
        }
        mergedDefects.clear();
        //mergedDefects = std::move(drawInfosFilter);
        mergedDefects = drawInfosFilter;
        FILE_LOG_INFO("[Algotithm] DetectImages: GetGlassAreaAndResult Completed! Roi:(%d,%d,%d,%d)", mergedRect.x, mergedRect.y, mergedRect.width, mergedRect.height);
        //cv::Size virtualSize = cv::Size(glassMaskImage.cols / 10, glassMaskImage.rows / 10);
        //cv::Mat glassMaskImageVirtual = m_showVirtualInfo.generateVirtualImage(glassMaskImage, virtualSize, showGrayValue, BackgroundColor, 10);
        ////虚拟图顺时针旋转
        //transpose(glassMaskImageVirtual, glassMaskImageVirtual); // 先转置
        //flip(glassMaskImageVirtual, glassMaskImageVirtual, 1);   // 再水平翻转（与逆时针的区别）
        //resultItem.backgroundImage = glassMaskImageVirtual.clone();   //传递背景图(虚拟图)
        //FILE_LOG_INFO("[Algotithm] DetectImages: generateVirtualImage Completed!");
#endif
        //showImage.emplace_back(largeImage_C1);
        //showImage.emplace_back(largeImage_C2);
        //showImage.emplace_back(largeImage_C3);
        showImage.emplace_back(largeImage_C1(mergedRect));
        showImage.emplace_back(largeImage_C2(mergedRect));
        showImage.emplace_back(largeImage_C3(mergedRect));

        largeImage_C1.release();
		largeImage_C2.release();
		largeImage_C3.release();

        //获取玻璃高度值
        // 计算裁剪后的总宽度
        int total_cropped_width = 0;
        std::vector<int> cameraHightValue;//记录每个相机对应的像素数
        for (int cam = 1; cam <= CameraCount; cam++) {
            total_cropped_width += static_cast<int>(camerainfo[cam][1] - camerainfo[cam][0]);
            cameraHightValue.push_back(static_cast<int>(camerainfo[cam][1] - camerainfo[cam][0]));
        }

        //获取每个相机中玻璃的像素数
        std::vector<double> glassPartHeight = calculateROIRatios(mergedRect, cameraHightValue);
        double glassHeight = 0.0;//实际高度值（mm），直接显示使用
        for (int p = 0; p < glassPartHeight.size(); p++) {

            glassHeight = glassHeight + camerainfo[p + 1][2] * glassPartHeight[p];

        }
        resultItem.glassPhysicalHeight = glassHeight;   //得到玻璃实际高度(MM)
        FILE_LOG_INFO("[Algotithm] DetectImages: Get glassPhysicalHeight Completed!");

        //转换坐标，将大图中的坐标转换到裁剪后的玻璃区域
        std::vector<drawInformation> roiDrawInfo;
        try
        {
            //roiDrawInfo = m_Compute.transformToROICoordinates(mergedDefects, mergedRect);
            roiDrawInfo = m_Compute.transformToROICoordinates(mergedDefects, mergedRect);
        }
        catch (const std::exception& e)
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages: transformToROICoordinates Error!!---------------------------");
            buffer.ClearQueue();
            return false;
        }
        FILE_LOG_INFO("[Algotithm] DetectImages: transformToROICoordinates Completed! Number:%d.", roiDrawInfo.size());



        /*=========特殊区域二次检测============*/
        std::vector<drawInformation> partDetectInfo;
        {
            //partDetectInfo = m_Compute.ComputeProcessPart(showImage, m_Parameter, glassThreshold_val, range_val, 256, 256);

        }


        /*=========特殊区域二次检测============*/

        // 旋转检测结果
        // 获取ROI图像的尺寸（旋转前）
        int roi_width = showImage[0].cols;
        int roi_height = showImage[0].rows;
        try
        {
        
            //resultItem.drawInfo = m_Compute.rotateDrawInfoCCW90(roiDrawInfo, roi_width, roi_height);
            //resultItem.drawInfo = m_Compute.rotateDrawInfoCCW90(roiDrawInfo, roi_width, roi_height);
            //std::vector<drawInformation> drawInfo_;
            //if (RotationDirectionFlag == 0) {
            //    drawInfo_ = m_Compute.rotateDrawInfoCW90(roiDrawInfo, roi_width, roi_height);//顺时针旋转90度，合肥现场使用的是顺时针
            //}
            //else {																												 
            //    drawInfo_ = m_Compute.rotateDrawInfoCCW90(roiDrawInfo, roi_width, roi_height);    //逆时针旋转90度 ，俄罗斯现场视野逆时针
            //}

            //////////////std::string saveJsonPath = resultPath + "/tmp.json";
            //////////////DataSave dataSave;
            //////////////dataSave.SaveDefectInfoToJson(drawInfo_, resultItem.NgFlag, resultItem.glassPixelWidth, resultItem.glassPixelHeight, saveJsonPath);

            ////对检测结果进行边界剔除
            //std::vector<drawInformation> filterInfo_ =  m_GeneralMethod.filterDefectsByBorderRegion(drawInfo_, roi_height, roi_width, 0.04, 0.035);

            //if (partDetectInfo.size() > 0) {

            //    //对特殊区域二次检测结果进行旋转
            //    std::vector<drawInformation> partInfo_ = m_Compute.rotateDrawInfoCW90(partDetectInfo, roi_width, roi_height);

            //    filterInfo_.insert(filterInfo_.end(), partInfo_.begin(), partInfo_.end());
            //
            //}
            ////剔除贴边坐标
            //// width : 15000 600  height :11420  400  0.034
            //resultItem.drawInfo = filterInfo_;

            //resultItem.drawInfo = drawInfo_;//不进行任何剔除操作
            resultItem.drawInfo = roiDrawInfo;//不进行任何剔除操作

            FILE_LOG_INFO("[Algotithm] DetectImages: rotateDrawInfoCW90 Number drawInfo:%d,drawInfo2:%d", roiDrawInfo.size(), resultItem.drawInfo.size());
        }
        catch (const std::exception& e)
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages: rotateDrawInfoCCW90 Error!!---------------------------");
            buffer.ClearQueue();
            return false;
        }
        FILE_LOG_INFO("[Algotithm] DetectImages: rotateDrawInfoCW90 Completed!");
        try
        {
            ////图像旋转顺时针
            //for (size_t i = 0; i < showImage.size(); i++)
            //{
            //    // 顺时针旋转 90 度
            //    transpose(showImage[i], showImage[i]); // 先转置
            //    flip(showImage[i], showImage[i], 1);   // 再水平翻转（与逆时针的区别）

            //}
            ////// 顺时针旋转 90 度
            ////transpose(resultItem.backgroundImage, resultItem.backgroundImage); // 先转置
            ////flip(resultItem.backgroundImage, resultItem.backgroundImage, 1);   // 再水平翻转（与逆时针的区别）
            //resultItem.image_0 = std::move(showImage[0]);
            //resultItem.image_1 = std::move(showImage[1]);
            //resultItem.image_2 = std::move(showImage[2]);

   //         // 1. 清空目标
   //         resultItem.image_0.release();
   //         resultItem.image_1.release();
   //         resultItem.image_2.release();
   //         cv::Mat bgTempImage;
   //         // 2. 直接旋转到目标

   //         if (RotationDirectionFlag == 0) {
   //             //顺时针旋转90度
   //             m_GeneralMethod.blockRotate90Clockwise(showImage[0], resultItem.image_0,1024);
   //             m_GeneralMethod.blockRotate90Clockwise(showImage[1], resultItem.image_1,1024);
   //             m_GeneralMethod.blockRotate90Clockwise(showImage[2], resultItem.image_2,1024);
   //             m_GeneralMethod.blockRotate90Clockwise(resultItem.backgroundImage, bgTempImage,1024);
   //         }
   //         else {
   //             //逆时针旋转90度
   //             m_GeneralMethod.blockRotate90CounterClockwise(showImage[0], resultItem.image_0, 1024);
   //             m_GeneralMethod.blockRotate90CounterClockwise(showImage[1], resultItem.image_1, 1024);
   //             m_GeneralMethod.blockRotate90CounterClockwise(showImage[2], resultItem.image_2, 1024);
   //             m_GeneralMethod.blockRotate90CounterClockwise(resultItem.backgroundImage, bgTempImage, 1024);
   //         }

   //         resultItem.backgroundImage = bgTempImage.clone();

   //         //wang 20251125 由于透场无法检出镀膜不良，临时加入高亮判断
   //         //redefineCoatingDefects(resultItem.image_1, resultItem.drawInfo);


   //         showImage[0].release();
   //         showImage[1].release();
			//showImage[2].release();
   //         bgTempImage.release();
            //qxz0428原代码-end
            //qxz0428修改: 直接move，不旋转
            resultItem.image_0 = std::move(showImage[0]);
            resultItem.image_1 = std::move(showImage[1]);
            resultItem.image_2 = std::move(showImage[2]);
        }
        catch (const std::exception& e)
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages: transpose Image Error!!---------------------------");
            buffer.ClearQueue();
            return false;
        }
        FILE_LOG_INFO("[Algotithm] DetectImages: transpose Image Completed!");
        //cv::Mat drawInfoImage3 = drawResultInfoImage(showImage[2].clone() , resultItem.drawInfo);
        //cv::imwrite("drawInfoImage3
        // ", drawInfoImage3);
        //清理残余帧
        buffer.ClearQueue();
        FILE_LOG_INFO("[Algotithm] DetectImages: GetLastResult Completed.");

    }
    //读图模式+json结果模式
    else if (IS_READ_MODE == 2)
    {
        FILE_LOG_INFO("[Algotithm] DetectImages: READ_MODE_2 BEGIN!!");
        std::string path = m_GeneralMethod.readFilePaths();
        if (path.empty())
        {
            //emit SignalGetImageOver();
            FILE_LOG_ERROR("[Algotithm] DetectImages Error: FilesPath is empty! Return.");

            return false;
        }
        FILE_LOG_INFO("READ: %s", path.c_str());
        std::string name = m_GeneralMethod.extract_directory_name(path);

        resultItem.image_0 = cv::imread(path + "/" + name + "_0.png", 0);
        resultItem.image_1 = cv::imread(path + "/" + name + "_1.png", 0);
        resultItem.image_2 = cv::imread(path + "/" + name + "_2.png", 0);

        //emit SignalGetImageOver();

        //防护动作
        if (resultItem.image_0.empty())
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages Error: image_0 is empty! Return.");
            //emit SignalGetImageOver();
            return false;
        }
        if (resultItem.image_1.empty())
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages Error: image_1 is empty! Return.");
            //emit SignalGetImageOver();
            return false;
        }
        if (resultItem.image_2.empty())
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages Error: image_2 is empty! Return.");
            //emit SignalGetImageOver();
            return false;
        }

        FILE_LOG_INFO("[Algotithm] DetectImages : GetImage Completed!");
        glassData2Json json;

        resultItem.drawInfo = json.parseJsonToDrawInfos(path + "/" + name + ".json", resultItem.glassPixelWidth, resultItem.glassPixelHeight);


    }
    //防护动作
    if (resultItem.glassPixelHeight == 0.0f)
    {
        resultItem.glassPixelHeight = resultItem.image_0.rows;
    }
    if (resultItem.glassPixelWidth == 0.0f)
    {
        resultItem.glassPixelWidth = resultItem.image_0.cols;
    }
    FILE_LOG_INFO("[Algotithm] DetectImages Completed!");

	return true;
}

bool AlgorithmProcess::DetectImages_new(QueueDefectItem& resultItem, const std::string& resultPath)
{
    FILE_LOG_INFO("[Algotithm] DetectImages: BEGIN");

    ResultBuffer& buffer = ResultBuffer::GetInstance();

    //旋转方向
    //int RotationDirectionFlag = 0; //0-顺时针旋转90度，1-逆时针旋转90度
    //RotationDirectionFlag = 0;//西班牙现场是顺时针 取0
    //读取网络置信度，并进行算法计算
    std::vector<drawInformation> results_beforeSort;
    std::vector<drawInformation> results_;//根据缩放比例存放新的值

    cv::Mat largeImage_C1, largeImage_C2, largeImage_C3;
    //实时采图+算法模式和读图+算法模式
    if (IS_READ_MODE == 0 || IS_READ_MODE == 1)
    {
		FILE_LOG_INFO("[Algotithm] DetectImages: %s,RotationDirectionFlag:%d", (IS_READ_MODE == 0 ? "REAL_MODE BEGIN" : "READ_MODE_1 BEGIN"), RotationDirectionFlag);
        //if (IS_READ_MODE == 1)
        //{
        //    ReadBatchImagesFromPath(); 
        //    m_readBatchImageIndex = 0;
        //}
        cv::Mat batchImg;
        std::vector<drawInformation> batchDrawInfo;
        std::vector<std::vector<drawInformation>> batchDrawInfoArray;

        //std::vector<cv::Mat> batchImgItem;//存放未拆分三通道的数据，用于分析错场问题
        std::vector<cv::Mat> showImage_Batch;

        std::vector<cv::Mat> showImage_ch0;
        std::vector<cv::Mat> showImage_ch1;
        std::vector<cv::Mat> showImage_ch2;

        //std::vector<cv::Mat> drawResultImage;


        size_t total = 0;   //记录drawInformation的总数
        int LoopCount = 0;  //循环计数
        int GetImageErrorCount = 0; //批次采图错误计数。用于20信号异常未接收到时，自行跳出。

        bool isStart = false;
        FILE_LOG_INFO("[Algotithm] DetectImages: Loop begin.");
        while (true)
        {
            //Sleep(1000);
            //循环等待第一组图像获取完成
            //if (!m_ImageProcessState)   //等待图像处理信号启动。(信号来自CameraManager)
            //{
            //    std::this_thread::sleep_for(std::chrono::milliseconds(50));
            //    continue;
            //}

            //防护动作。避免循环死锁     //新硬件，暂时不考虑硬件信号不稳定情况。暂时注释防死锁逻辑。
            if (GetImageErrorCount >= 30)
            {
                //emit StopProcessSinals(false);
                break;
            }

            if (buffer.IsQueueEmpty())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            //判断算法过程开始信号
            if (!isStart)
            {
                //等待当前玻璃的开始帧到达
                if (buffer.GetFrontImage().rows == 1)
                {
                    //开始帧出队
                    buffer.PopFrame(batchImg, 5);
                    FILE_LOG_INFO("DETECT_IMAGES_BEGIN.");
                    isStart = true;
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    continue;
                }
            }
            //判断算法过程结束信号
            else
            {
                //收到结束帧，跳出
                if (buffer.GetFrontImage().rows == 2)
                {
                    //结束帧出队
                    buffer.PopFrame(batchImg, 5);
                    FILE_LOG_INFO("DETECT_IMAGES_END.");
                    break;
                }
            }

            FILE_LOG_DEBUG("[Algotithm] DetectImages: LOOP[%d] Begin.", LoopCount);
            // 1. 获取批次图像
            batchImg.release();
            showImage_Batch.clear();
            std::vector<cv::Mat> batchEntireImage;
            std::vector<drawInformation> batchDrawInfo;
            cv::Rect batchRect;

            bool bRet = false;
            bRet = buffer.PopFrame(batchImg, 5);   //等待合并7个相机的帧图
            FILE_LOG_DEBUG("[Algotithm] DetectImages: PopFrame .");
            if (!bRet)
            {
                GetImageErrorCount++;
                FILE_LOG_DEBUG("[Algotithm] DetectImages: LOOP[%d] ComputeImage_Signal Failed! LOOP NEXT. EmptyCount = %d", LoopCount, GetImageErrorCount);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }
            if (batchImg.empty())
            {
                GetImageErrorCount++;
                FILE_LOG_DEBUG("[Algotithm] DetectImages: LOOP[%d] ComputeImage_Signal Failed! LOOP NEXT. EmptyCount = %d", LoopCount, GetImageErrorCount);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }
            if (batchImg.rows == 2)
            {
                FILE_LOG_DEBUG("[Algotithm] DetectImages: Received Stop Frame. Break.");
                break;
            }
            FILE_LOG_DEBUG("[Algotithm] DetectImages: ComputeImage_Signal begin.");
            try
            {
                bRet = AlgorithmProcess::GetInstance().ComputeImage_Signal(batchImg, batchEntireImage, batchDrawInfo);
                //batchImgItem.push_back(batchImg);

                if (!bRet)
                {
                    GetImageErrorCount++;
                    FILE_LOG_DEBUG("[Algotithm] DetectImages: LOOP[%d] ComputeImage_Signal Failed! LOOP NEXT. EmptyCount = %d", LoopCount, GetImageErrorCount);
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    continue;
                }
            }
            catch (const std::exception&)
            {
                FILE_LOG_DEBUG("[Algotithm] DetectImages: ComputeImage_Signal Error.");
                continue;
            }
            FILE_LOG_DEBUG("[Algotithm] DetectImages: ComputeImage_Signal after.");
            /*=====绘制单帧计算结果=======*/
            //cv::Mat drawImageBatch  = drawResultInfoImage(batchEntireImage[2], batchDrawInfo);
            //drawResultImage.push_back(drawImageBatch);
            /*=====绘制单帧计算结果=======*/


            /****保存每批次拆分三通道图*****/
            ////////ImageSaver* m_imgSaver = nullptr;
            ////////if (m_imgSaver == nullptr)
            ////////{
            ////////    m_imgSaver = new ImageSaver();
            ////////}
            ////////for (size_t i = 0; i < batchEntireImage.size(); i++)
            ////////{
            ////////    if (!batchEntireImage[i].empty())
            ////////    {
            ////////        std::string saveImage = resultPath + "/" + std::to_string(LoopCount) + "_" + std::to_string(i) + ".png";
            ////////        //cv::imwrite(saveImage, batchEntireImage[i]);
            ////////        m_imgSaver->saveImageAsync(saveImage, batchEntireImage[i]);       // 异步保存图像
            ////////    }
            ////////}
            /*******************************/

            GetImageErrorCount = 0; //不连续的拿图失败时，重置错误计数
            FILE_LOG_DEBUG("[Algotithm] DetectImages: LOOP[%d] ComputeImage_Signal Completed!", LoopCount);


            showImage_ch0.push_back(batchEntireImage[0]);
            showImage_ch1.push_back(batchEntireImage[1]);
            showImage_ch2.push_back(batchEntireImage[2]);
            batchDrawInfoArray.emplace_back(batchDrawInfo);


            //cv::imwrite("./1/batch_" + std::to_string(LoopCount) + "_0.jpg", matsEntire[0]);
            //cv::imwrite("./1/batch_" + std::to_string(LoopCount) + "_1.jpg", matsEntire[1]);
            //cv::imwrite("./1/batch_" + std::to_string(LoopCount) + "_2.jpg", matsEntire[2]);


            total += batchDrawInfo.size();
            FILE_LOG_DEBUG("[Algotithm] LOOP[%d] Completed.", LoopCount);
            LoopCount++;
        }
        FILE_LOG_DEBUG("[Algotithm] DetectImages: heightSingle value %d!!---------------------------", heightSingle);

        std::vector<drawInformation> mergedDefects;
        try
        {
            mergedDefects = m_Compute.mergeDefectsToLargeImage(batchDrawInfoArray, heightSingle);
        }
        catch (const std::exception&)
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages: mergeDefectsToLargeImage Error!!---------------------------");
        }

        FILE_LOG_INFO("[Algotithm] DetectImages: mergeDefectsToLargeImage Completed!");

        //边采边计算多帧图像合并(约30ms-100ms)
        //cv::vconcat(showImage_ch0, largeImage_C1);
        //cv::vconcat(showImage_ch1, largeImage_C2);
        //cv::vconcat(showImage_ch2, largeImage_C3);


        cv::vconcat(showImage_ch0, largeImage_C1);
        cv::vconcat(showImage_ch1, largeImage_C2);
        cv::vconcat(showImage_ch2, largeImage_C3);

        //cv::imwrite("largeImage_C1.png", largeImage_C1);
        //cv::imwrite("largeImage_C2.png", largeImage_C2);
        //cv::imwrite("largeImage_C3.png", largeImage_C3);

        //cv::Mat channel3Image;
        //cv::vconcat(batchImgItem, channel3Image);
        //cv::imwrite("channel3Image.png", channel3Image);
        //cv::Mat drawInfoImage;
        //cv::vconcat(drawResultImage, drawInfoImage);
        //cv::Mat drawInfoImage2 = drawResultInfoImage(largeImage_C3, mergedDefects);

        //三通道未拆分图像保存
        {
              /*cv::Mat channel3Data;
              cv::vconcat(batchImgItem, channel3Data);
              std::string base_path = "D:/channel3Data/";
              namespace fs = std::filesystem;
              fs::path dir_path(base_path);

              if (!fs::exists(dir_path)) {
                  if (fs::create_directories(dir_path)) {
                      std::cout << "目录创建成功: " << base_path << std::endl;
                  }
                  else {
                      std::cout << "目录创建失败，使用当前目录" << std::endl;
                      base_path = "./";
                  }
              }
              else if (!fs::is_directory(dir_path)) {
                  std::cout << "路径存在但不是目录，使用当前目录" << std::endl;
                  base_path = "./";
              }
              else {
                  std::cout << "目录已存在: " << base_path << std::endl;
              }

              if (!channel3Data.empty()) {
                  std::string filename = base_path + "capture_" + getCurrentTimeString() + ".png";

                  if (imwrite(filename, channel3Data)) {
                      std::cout << "SaveSucess: " << filename << std::endl;
                  }
                  else {
                      std::cout << "SaveFailed: " << filename << std::endl;
                  }
              }*/

        }



        FILE_LOG_INFO("[Algotithm] DetectImages: Vconcat largeImage Completed! width:%d,height:%d", largeImage_C2.cols, largeImage_C2.rows);
        //获取玻璃区域
        cv::Rect mergedRect;
        std::vector<cv::Mat> showImage;

#if 0  //使用老版算法
        int ret = m_Compute.getGlassRoi(largeImage_C2, mergedRect, glassThreshold_val);
        //int ret = m_Compute.getGlassRoiSample(largeImage_C2, mergedRect, glassThreshold_val,0.1);
        if (ret == -1) {

            //std::cout << "No Glass Area" << std::endl;
            FileLogPrintf("[Algotithm] DetectImages: No Glass Area!!---------------------------");

            std::string base_path = "D:/captureError/";
            namespace fs = std::filesystem;
            fs::path dir_path(base_path);
            if (!fs::exists(dir_path)) {
                if (fs::create_directories(dir_path)) {
                    std::cout << "目录创建成功: " << base_path << std::endl;
                }
                else {
                    std::cout << "目录创建失败，使用当前目录" << std::endl;
                    base_path = "./";
                }
            }
            else if (!fs::is_directory(dir_path)) {
                std::cout << "路径存在但不是目录，使用当前目录" << std::endl;
                base_path = "./";
            }
            else {
                std::cout << "目录已存在: " << base_path << std::endl;
            }
            if (!largeImage_C1.empty()) {
                std::string filename = base_path + "capture_" + getCurrentTimeString() + "_1.png";

                if (imwrite(filename, largeImage_C1)) {
                    std::cout << "保存成功: " << filename << std::endl;
                }
            }
            if (!largeImage_C2.empty()) {
                std::string filename = base_path + "capture_" + getCurrentTimeString() + "_2.png";

                if (imwrite(filename, largeImage_C2)) {
                    std::cout << "保存成功: " << filename << std::endl;
                }
            }
            if (!largeImage_C3.empty()) {
                std::string filename = base_path + "capture_" + getCurrentTimeString() + "_3.png";

                if (imwrite(filename, largeImage_C3)) {
                    std::cout << "保存成功: " << filename << std::endl;
                }
            }
            return false;
        }
        FILE_LOG_INFO("[Algotithm] DetectImages: getGlassRoi Completed! Roi:(%d,%d,%d,%d)", mergedRect.x, mergedRect.y, mergedRect.width, mergedRect.height);
#else
        int showGrayValue = 100;//玻璃区域显示灰度值
        int BackgroundColor = 200;//背景区域显示灰度值
        cv::Mat glassMaskImage;
        std::vector<drawInformation> drawInfosFilter;
        //cv::Mat glassMaskImageVirtual;
        bool ret = m_showVirtualInfo.GetGlassAreaAndResult(largeImage_C2, glassThreshold_val, showGrayValue, mergedDefects, 180,
            mergedRect, glassMaskImage, drawInfosFilter);

        //临时存图，判断玻璃裁剪不全问题
        if (IsSaveLargeImage)
        {
#if 1
            //首选保存方案：所有玻璃的合并后大图保存到同一个文件夹内
            saveImageWithTimestamp(largeImage_C2, "D:/ErrorImage", "glassMask");
#else
            //备选保存方案：异步保存，减少耗时。保存到对应玻璃的文件夹内
            ImageSaver* m_imgSaver = nullptr;
            if (m_imgSaver == nullptr)
            {
                m_imgSaver = new ImageSaver();
            }

            if (!resultItem.image_0.empty())
            {
                std::string saveImage = resultPath + "/C2_glassMask.png";
                //cv::imwrite(saveImage, largeImgs[0]);
                m_imgSaver->saveImageAsync(saveImage, largeImage_C2);       // 异步保存图像
            }
#endif
        }

        if (!ret)
        {
            FILE_LOG_INFO("[Algotithm] DetectImages: No Glass Area!!---------------------------");

            /* std::string base_path = "D:/captureError/";
             namespace fs = std::filesystem;
             fs::path dir_path(base_path);
             if (!fs::exists(dir_path)) {
                 if (fs::create_directories(dir_path)) {
                     std::cout << "目录创建成功: " << base_path << std::endl;
                 }
                 else {
                     std::cout << "目录创建失败，使用当前目录" << std::endl;
                     base_path = "./";
                 }
             }
             else if (!fs::is_directory(dir_path)) {
                 std::cout << "路径存在但不是目录，使用当前目录" << std::endl;
                 base_path = "./";
             }
             else {
                 std::cout << "目录已存在: " << base_path << std::endl;
             }
             if (!largeImage_C1.empty()) {
                 std::string filename = base_path + "capture_" + getCurrentTimeString() + "_1.png";

                 if (imwrite(filename, largeImage_C1)) {
                     std::cout << "保存成功: " << filename << std::endl;
                 }
             }
             if (!largeImage_C2.empty()) {
                 std::string filename = base_path + "capture_" + getCurrentTimeString() + "_2.png";

                 if (imwrite(filename, largeImage_C2)) {
                     std::cout << "保存成功: " << filename << std::endl;
                 }
             }
             if (!largeImage_C3.empty()) {
                 std::string filename = base_path + "capture_" + getCurrentTimeString() + "_3.png";

                 if (imwrite(filename, largeImage_C3)) {
                     std::cout << "保存成功: " << filename << std::endl;
                 }
             }*/
            return false;
        }
        mergedDefects.clear();
        //mergedDefects = std::move(drawInfosFilter);
        mergedDefects = drawInfosFilter;
        FILE_LOG_INFO("[Algotithm] DetectImages: GetGlassAreaAndResult Completed! Roi:(%d,%d,%d,%d)", mergedRect.x, mergedRect.y, mergedRect.width, mergedRect.height);
        //cv::Size virtualSize = cv::Size(glassMaskImage.cols / 10, glassMaskImage.rows / 10);
        //cv::Mat glassMaskImageVirtual = m_showVirtualInfo.generateVirtualImage(glassMaskImage, virtualSize, showGrayValue, BackgroundColor, 10);
        ////虚拟图顺时针旋转
        //transpose(glassMaskImageVirtual, glassMaskImageVirtual); // 先转置
        //flip(glassMaskImageVirtual, glassMaskImageVirtual, 1);   // 再水平翻转（与逆时针的区别）
        //resultItem.backgroundImage = glassMaskImageVirtual.clone();   //传递背景图(虚拟图)
        //FILE_LOG_INFO("[Algotithm] DetectImages: generateVirtualImage Completed!");
#endif
        //showImage.emplace_back(largeImage_C1);
        //showImage.emplace_back(largeImage_C2);
        //showImage.emplace_back(largeImage_C3);
        showImage.emplace_back(largeImage_C1(mergedRect));
        showImage.emplace_back(largeImage_C2(mergedRect));
        showImage.emplace_back(largeImage_C3(mergedRect));

        largeImage_C1.release();
        largeImage_C2.release();
        largeImage_C3.release();

        //获取玻璃高度值
        // 计算裁剪后的总宽度
        int total_cropped_width = 0;
        std::vector<int> cameraHightValue;//记录每个相机对应的像素数
        for (int cam = 1; cam <= CameraCount; cam++) {
            total_cropped_width += static_cast<int>(camerainfo[cam][1] - camerainfo[cam][0]);
            cameraHightValue.push_back(static_cast<int>(camerainfo[cam][1] - camerainfo[cam][0]));
        }

        //获取每个相机中玻璃的像素数
        std::vector<double> glassPartHeight = calculateROIRatios(mergedRect, cameraHightValue);
        double glassHeight = 0.0;//实际高度值（mm），直接显示使用
        for (int p = 0; p < glassPartHeight.size(); p++) {

            glassHeight = glassHeight + camerainfo[p + 1][2] * glassPartHeight[p];

        }
        resultItem.glassPhysicalHeight = glassHeight;   //得到玻璃实际高度(MM)
        FILE_LOG_INFO("[Algotithm] DetectImages: Get glassPhysicalHeight Completed!");

        //转换坐标，将大图中的坐标转换到裁剪后的玻璃区域
        std::vector<drawInformation> roiDrawInfo;
        try
        {
            //roiDrawInfo = m_Compute.transformToROICoordinates(mergedDefects, mergedRect);
            roiDrawInfo = m_Compute.transformToROICoordinates(mergedDefects, mergedRect);
        }
        catch (const std::exception& e)
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages: transformToROICoordinates Error!!---------------------------");
            buffer.ClearQueue();
            return false;
        }
        FILE_LOG_INFO("[Algotithm] DetectImages: transformToROICoordinates Completed! Number:%d.", roiDrawInfo.size());



        /*=========特殊区域二次检测============*/
        std::vector<drawInformation> partDetectInfo;
        {
            //partDetectInfo = m_Compute.ComputeProcessPart(showImage, m_Parameter, glassThreshold_val, range_val, 256, 256);

        }


        /*=========特殊区域二次检测============*/

        // 旋转检测结果
        // 获取ROI图像的尺寸（旋转前）
        int roi_width = showImage[0].cols;
        int roi_height = showImage[0].rows;
        try
        {
			
            //std::vector<drawInformation> drawInfo_;
            //if (RotationDirectionFlag == 0) {
            //    drawInfo_ = m_Compute.rotateDrawInfoCW90(roiDrawInfo, roi_width, roi_height);//顺时针旋转90度，合肥现场使用的是顺时针
            //}
            //else {																																							  
            //    drawInfo_ = m_Compute.rotateDrawInfoCCW90(roiDrawInfo, roi_width, roi_height);    //逆时针旋转90度 ，俄罗斯现场视野逆时针
            //}

            /////////*std::string saveJsonPath = resultPath + "/tmp.json";
            ////////DataSave dataSave;
            ////////dataSave.SaveDefectInfoToJson(drawInfo_, resultItem.NgFlag, resultItem.glassPixelWidth, resultItem.glassPixelHeight, saveJsonPath);*/

            ////对检测结果进行边界剔除
            //std::vector<drawInformation> filterInfo_ = m_GeneralMethod.filterDefectsByBorderRegion(drawInfo_, roi_height, roi_width, 0.04, 0.035);
            //std::vector<drawInformation> filterInfo_ = drawInfo_;

            //if (partDetectInfo.size() > 0) {

            //    //对特殊区域二次检测结果进行旋转
            //    std::vector<drawInformation> partInfo_ = m_Compute.rotateDrawInfoCW90(partDetectInfo, roi_width, roi_height);

            //    filterInfo_.insert(filterInfo_.end(), partInfo_.begin(), partInfo_.end());
            //
            //}
            ////剔除贴边坐标
            //// width : 15000 600  height :11420  400  0.034
            //resultItem.drawInfo = filterInfo_;
            //resultItem.drawInfo = drawInfo_;//进行任何剔除操作
            //FILE_LOG_INFO("[Algotithm] DetectImages: rotateDrawInfoCW90 Number drawInfo:%d,drawInfo2:%d", drawInfo_.size(), resultItem.drawInfo.size());
            std::vector<drawInformation> filterInfo_ = Tuning::filterDefectsByBorderRegion(roiDrawInfo, roi_width, roi_height, 0.04, 0.035);
            //std::vector<drawInformation> filterInfo_ = m_GeneralMethod.filterDefectsByBorderRegion(roiDrawInfo, roi_width, roi_height, 0.04, 0.035);
            resultItem.drawInfo = filterInfo_;

            FILE_LOG_INFO("[Algotithm] DetectImages: rotateDrawInfoCW90 Number drawInfo:%d,drawInfo2:%d", roiDrawInfo.size(), resultItem.drawInfo.size());
        }
        catch (const std::exception& e)
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages: rotateDrawInfoCCW90 Error!!---------------------------");
            buffer.ClearQueue();
            return false;
        }
        FILE_LOG_INFO("[Algotithm] DetectImages: rotateDrawInfoCW90 Completed!");
        try
        {
            ////图像旋转顺时针
            //for (size_t i = 0; i < showImage.size(); i++)
            //{
            //    // 顺时针旋转 90 度
            //    transpose(showImage[i], showImage[i]); // 先转置
            //    flip(showImage[i], showImage[i], 1);   // 再水平翻转（与逆时针的区别）

            //}
            ////// 顺时针旋转 90 度
            ////transpose(resultItem.backgroundImage, resultItem.backgroundImage); // 先转置
            ////flip(resultItem.backgroundImage, resultItem.backgroundImage, 1);   // 再水平翻转（与逆时针的区别）
            //resultItem.image_0 = std::move(showImage[0]);
            //resultItem.image_1 = std::move(showImage[1]);
            //resultItem.image_2 = std::move(showImage[2]);

            //// 1. 清空目标
            //resultItem.image_0.release();
            //resultItem.image_1.release();
            //resultItem.image_2.release();
            //cv::Mat bgTempImage;
            //// 2. 直接旋转到目标							 
            //if (RotationDirectionFlag == 0) {
            //    //顺时针旋转90度
            //    m_GeneralMethod.blockRotate90Clockwise(showImage[0], resultItem.image_0, 1024);
            //    m_GeneralMethod.blockRotate90Clockwise(showImage[1], resultItem.image_1, 1024);
            //    m_GeneralMethod.blockRotate90Clockwise(showImage[2], resultItem.image_2, 1024);
            //    m_GeneralMethod.blockRotate90Clockwise(resultItem.backgroundImage, bgTempImage, 1024);
            //}
            //else {
            //    //逆时针旋转90度
            //    m_GeneralMethod.blockRotate90CounterClockwise(showImage[0], resultItem.image_0, 1024);
            //    m_GeneralMethod.blockRotate90CounterClockwise(showImage[1], resultItem.image_1, 1024);
            //    m_GeneralMethod.blockRotate90CounterClockwise(showImage[2], resultItem.image_2, 1024);
            //    m_GeneralMethod.blockRotate90CounterClockwise(resultItem.backgroundImage, bgTempImage, 1024);
            //}

            //resultItem.backgroundImage = bgTempImage.clone();

            ////wang 20251125 由于透场无法检出镀膜不良，临时加入高亮判断
            ////redefineCoatingDefects(resultItem.image_1, resultItem.drawInfo);


            //showImage[0].release();
            //showImage[1].release();
            //showImage[2].release();
            //bgTempImage.release();
            //qxz0428原代码-end
            //qxz0428修改: 直接move，不旋转
            resultItem.image_0 = std::move(showImage[0]);
            resultItem.image_1 = std::move(showImage[1]);
            resultItem.image_2 = std::move(showImage[2]);
        }
        catch (const std::exception& e)
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages: transpose Image Error!!---------------------------");
            buffer.ClearQueue();
            return false;
        }

        int glassType= 0;
        {//玻璃类别判断，西班牙使用，白玻和镀膜玻璃区域
            
            // 自动调整大小以适应图像
            int desiredSize = 50;
            int safeSize = std::min(desiredSize, std::min(resultItem.image_0.cols, resultItem.image_0.rows));

            int x = (resultItem.image_0.cols - safeSize) / 2;
            int y = (resultItem.image_0.rows - safeSize) / 2;

            cv::Rect safeROI(x, y, safeSize, safeSize);
            // 提取ROI区域进行计算
            cv::Mat region0 = resultItem.image_0(safeROI);
            cv::Mat region1 = resultItem.image_1(safeROI);

            cv::Scalar meanValueChannel_0 = cv::mean(region0);
            cv::Scalar meanValueChannel_1 = cv::mean(region1);

            // 原有的判断逻辑
            if (meanValueChannel_0[0] > meanValueChannel_1[0]) {
                // 透场亮度较大，可能是白玻，使用透场和亮场1的组合
                //std::cout << "白玻检测结果 - ROI: " << safeROI << std::endl;

                glassType = 0;
            }
            else {
                // 透场亮度较小，可能是镀膜玻璃，使用透场和亮场2的组合
                //std::cout << "镀膜玻璃检测结果 - ROI: " << safeROI << std::endl;
                glassType = 1;
            }

        }

        FILE_LOG_INFO("[Algotithm] DetectImages: transpose Image Completed!,glass Type:%d", glassType);
        //cv::Mat drawInfoImage3 = drawResultInfoImage(showImage[2].clone() , resultItem.drawInfo);
        //cv::imwrite("drawInfoImage3
        // ", drawInfoImage3);
        //清理残余帧
        //buffer.ClearQueue();  //2026.02.03 使用启停帧Mat后，考虑到紧随而至的第二块玻璃，此处不再清理。
        FILE_LOG_INFO("[Algotithm] DetectImages: GetLastResult Completed.");

    }
    //读图模式+json结果模式
    else if (IS_READ_MODE == 2)
    {
        FILE_LOG_INFO("[Algotithm] DetectImages: READ_MODE_2 BEGIN!!");
        std::string path = m_GeneralMethod.readFilePaths();
        if (path.empty())
        {
            //emit SignalGetImageOver();
            FILE_LOG_ERROR("[Algotithm] DetectImages Error: FilesPath is empty! Return.");

            return false;
        }
        FILE_LOG_INFO("READ: %s", path.c_str());
        std::string name = m_GeneralMethod.extract_directory_name(path);

        resultItem.image_0 = cv::imread(path + "/" + name + "_0.png", 0);
        resultItem.image_1 = cv::imread(path + "/" + name + "_1.png", 0);
        resultItem.image_2 = cv::imread(path + "/" + name + "_2.png", 0);

        //emit SignalGetImageOver();

        //防护动作
        if (resultItem.image_0.empty())
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages Error: image_0 is empty! Return.");
            //emit SignalGetImageOver();
            return false;
        }
        if (resultItem.image_1.empty())
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages Error: image_1 is empty! Return.");
            //emit SignalGetImageOver();
            return false;
        }
        if (resultItem.image_2.empty())
        {
            FILE_LOG_ERROR("[Algotithm] DetectImages Error: image_2 is empty! Return.");
            //emit SignalGetImageOver();
            return false;
        }

        FILE_LOG_INFO("[Algotithm] DetectImages : GetImage Completed!");
        glassData2Json json;

        resultItem.drawInfo = json.parseJsonToDrawInfos(path + "/" + name + ".json", resultItem.glassPixelWidth, resultItem.glassPixelHeight);


    }
    //防护动作
    if (resultItem.glassPixelHeight == 0.0f)
    {
        resultItem.glassPixelHeight = resultItem.image_0.rows;
    }
    if (resultItem.glassPixelWidth == 0.0f)
    {
        resultItem.glassPixelWidth = resultItem.image_0.cols;
    }
    FILE_LOG_INFO("[Algotithm] DetectImages Completed!");

    return true;
}

bool AlgorithmProcess::ComputeImage_Signal(cv::Mat& batchSourceImg
    , std::vector<cv::Mat>& batchEntireImage
    , std::vector<drawInformation>& batchDrawInfo)
{
    bool bRet = false;

    if (batchSourceImg.empty() || batchSourceImg.data == nullptr)
    {
        //GetImageErrorCount++;
        FILE_LOG_ERROR("[Algotithm] ComputeImage_Signal Error: GetImage Failed! ");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return false;
    }
    FILE_LOG_DEBUG("[Algotithm] ComputeImage_Signal: GetImageOnce Completed!");

    //cv::imwrite("./1/batchImg" + std::to_string(LoopCount) + ".jpg", batchImg);

    // 2.三通道拆分
    cv::Rect roiRect;
    //batchEntireImage = m_Beautify.divImg3ChannelEntire(batchSourceImg, camerainfo, 4096, 20, batchRect);
    //bool is3Channel = true;

    //{
    //    // 1. 获取当前时间戳（毫秒级精度）
    //    auto now = std::chrono::system_clock::now();
    //    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    //    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    //        now.time_since_epoch()) % 1000;

    //    // 2. 格式化为字符串：年-月-日_时-分-秒-毫秒
    //    std::tm* tm_info = std::localtime(&now_time_t);
    //    std::stringstream ss;
    //    ss << std::put_time(tm_info, "%Y%m%d_%H%M%S");
    //    ss << "_" << std::setfill('0') << std::setw(3) << now_ms.count();

    //    std::string timestamp = ss.str();

    //    // 3. 指定保存路径（修改为你需要的路径）
    //    std::string save_dir = "D:/AOI/TestImage/";

    //    // 4. 创建目录（如果不存在）- C++17方式
    //    std::filesystem::create_directories(save_dir);

    //    // 5. 构建完整文件路径
    //    std::string filename = save_dir + timestamp + ".png";  // 也可用.jpg
    //    bool is_saved = cv::imwrite(filename, batchSourceImg);

    //}
    try
    {
        if (LightCount == 3)
        {
            FILE_LOG_DEBUG("[Algotithm] ComputeImage_Signal: divImgChannel3 Used!");
            batchEntireImage = m_Beautify.divImgChannel3(batchSourceImg, camerainfo, 4096);
            //batchEntireImage = m_Beautify.divImgChannel3OpenCV(batchSourceImg, camerainfo, 4096);//20251017 wang divImgChannel3函数内部增加日志输出
            if (batchEntireImage.size()==0)
            {
                FILE_LOG_ERROR("[Algotithm] ComputeImage_Signal: divImgChannel3 False!");
                return false;
            }
        }
        else if (LightCount == 2)
        {
            FILE_LOG_DEBUG("[Algotithm] ComputeImage_Signal: divImgChannel2 Used!");
            batchEntireImage = m_Beautify.divImgChannel2(batchSourceImg, camerainfo, 4096);
			//batchEntireImage = m_Beautify.divImgChannel2OpenCV(batchSourceImg, camerainfo, 4096);//20251017 wang 优化divImgChannel2函数内部的日志输出

            if (batchEntireImage.size() == 2)
            {
                batchEntireImage.push_back(batchEntireImage[1].clone());
            }
            else {
                FILE_LOG_ERROR("[Algotithm] ComputeImage_Signal: divImgChannel2 False!");
                return false;
            }
        }
		else if (LightCount == 1)
        {
            FILE_LOG_DEBUG("[Algotithm] ComputeImage_Signal: divImgChannel2 Used!");
            batchEntireImage = m_Beautify.divImgChannel1(batchSourceImg, camerainfo, 4096);
            
            if (batchEntireImage.size() == 1)
            {
                batchEntireImage.push_back(batchEntireImage[0].clone());
                batchEntireImage.push_back(batchEntireImage[0].clone());
            }
            else {
                FILE_LOG_ERROR("[Algotithm] ComputeImage_Signal: divImgChannel1 False!");
                return false;
			}
        }
        else
        {
            batchEntireImage.push_back(batchSourceImg.clone());
            batchEntireImage.push_back(batchSourceImg.clone());
            batchEntireImage.push_back(batchSourceImg.clone());
        }
        if (batchEntireImage.size() != 3)
        {

            FILE_LOG_ERROR("[Algotithm] ComputeImage_Signal Error: DivImg3Channel Failed,No GlassArea!");
            return false;
        }
    }
    catch (const std::exception& e)
    {
        FILE_LOG_ERROR("[Algorithm] ComputeImage_Signal: Error in divImgChannel: %s.",
            e.what());
        return false;
    }

    //{
    //    // 1. 获取当前时间戳（毫秒级精度）
    //    auto now = std::chrono::system_clock::now();
    //    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    //    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    //        now.time_since_epoch()) % 1000;

    //    // 2. 格式化为字符串：年-月-日_时-分-秒-毫秒
    //    std::tm* tm_info = std::localtime(&now_time_t);
    //    std::stringstream ss;
    //    ss << std::put_time(tm_info, "%Y%m%d_%H%M%S");
    //    ss << "_" << std::setfill('0') << std::setw(3) << now_ms.count();

    //    std::string timestamp = ss.str();

    //    // 3. 指定保存路径（修改为你需要的路径）
    //    std::string save_dir = "D:/AOI/TestImage2/";

    //    // 4. 创建目录（如果不存在）- C++17方式
    //    std::filesystem::create_directories(save_dir);

    //    // 5. 构建完整文件路径
    //    std::string filename0 = save_dir + timestamp + "_0.png";  // 也可用.jpg
    //    std::string filename1 = save_dir + timestamp + "_1.png";  // 也可用.jpg
    //    std::string filename2 = save_dir + timestamp + "_2.png";  // 也可用.jpg
    // 
    //    cv::imwrite(filename0, batchEntireImage[0]);
    //    cv::imwrite(filename1, batchEntireImage[1]);
    //    cv::imwrite(filename2, batchEntireImage[2]);

    //}
    //FileLogPrintf("[Algotithm] ComputeImage_Signal: divImgChannel Completed!");
    /**** 单次计算瑕疵DrawInfo的算法 ****/
    //std::vector<drawInformation> results_;//根据缩放比例存放新的值
    //2.对图像直接进行检测
    //std::vector<drawInformation> batchDrawInfo;
    
    if (batchEntireImage[2].empty()) {

        FILE_LOG_ERROR("[Algotithm] ComputeImage_Signal Error: batchEntireImage[2] is empty! Return.");
        return false;
    }
    FILE_LOG_DEBUG("[Algotithm] ComputeImage_Signal: ComputeProcess begin!");
    
    try
    {
        //batchDrawInfo = m_Compute.ComputeProcess(batchEntireImage[1], m_Parameter, glassThreshold_val, range_val, 256, 256);
        //batchDrawInfo = m_Compute.ComputeProcess(batchEntireImage[0], m_Parameter, glassThreshold_val, range_val, 256, 256);//采用透场进行检测

        batchDrawInfo = m_Compute.ComputeProcessMultiChannel(batchEntireImage, m_Parameter, glassThreshold_val, range_val, 256, 256);

        //Sleep(500);//模拟算法计算
    }
    catch (const std::exception& e)
    {
        //FileLogPrintf("[Algotithm] ComputeImage_Signal: ComputeProcess Error!");//20250917 wang 软件崩溃防护
        FILE_LOG_ERROR("[Algorithm] ComputeImage_Signal: Error in ComputeProcess: %s.",
            e.what());
        return false;
    }


    FILE_LOG_DEBUG("[Algotithm] ComputeImage_Signal: ComputeProcess Completed! batchDrawInfo In Number:%d", batchDrawInfo.size());
    /****************************************/
    return true;
}

bool AlgorithmProcess::GetLastResult(std::vector<cv::Mat>& showImage_ch0,
std::vector<cv::Mat>& showImage_ch1,
std::vector<cv::Mat>& showImage_ch2,
const std::vector<std::vector<drawInformation>>& batchDrawInfoArray,
const std::vector<cv::Rect>& batchRectArray,
std::vector<cv::Mat>& outImages,
std::vector<drawInformation> outDrawInfo)
{ 
    FILE_LOG_DEBUG("[Algotithm] GetLastResult: Begin.");

    // 水平拼接所有三通道图像
    cv::Mat stitchedImage_ch0, stitchedImage_ch1, stitchedImage_ch2;
    if (!showImage_ch0.empty() && !showImage_ch1.empty() && !showImage_ch2.empty())
    {
        cv::vconcat(showImage_ch0, stitchedImage_ch0);
        cv::vconcat(showImage_ch1, stitchedImage_ch1);
        cv::vconcat(showImage_ch2, stitchedImage_ch2);
    }
    else
    {
        FILE_LOG_ERROR("[Algotithm] GetLastResult Error: Return. BatchImgItem is empty.");
        //emit ChangeCardStatus(2);   //更新主界面顶部状态指示,2=无板，1=板出，0=板进、配合图像显示逻辑的调整，改为compute计算结束后显示“板出”

        //emit SignalCheckRecipeChanged();
        return false;
    }
    FILE_LOG_DEBUG("[Algotithm] GetLastResult: Vconcat showImage Completed.");


    // 步骤2：计算每个图像在拼接图中的起始Y坐标
    std::vector<int> startY;
    startY.push_back(0);  // 第一张图的起始Y=0
    for (size_t i = 0; i < showImage_ch0.size() - 1; ++i) {
        startY.push_back(startY[i] + showImage_ch0[i].rows);
    }

    // 步骤3：调整每个玻璃区域的位置并合并
    cv::Rect mergedRect;
    bool firstValid = true;

    for (size_t i = 0; i < batchRectArray.size(); ++i) {
        const cv::Rect& rect = batchRectArray[i];

        // 跳过无效矩形
        if (rect.width <= 0 || rect.height <= 0) continue;

        // 创建调整后的矩形（Y坐标偏移）
        cv::Rect adjustedRect = rect;
        adjustedRect.y += startY[i];

        // 首次有效矩形直接赋值
        if (firstValid) {
            mergedRect = adjustedRect;
            firstValid = false;
        }
        // 后续矩形使用并集合并
        else {
            // 计算合并后的矩形边界
            int x1 = std::min(mergedRect.x, adjustedRect.x);
            int y1 = std::min(mergedRect.y, adjustedRect.y);
            int x2 = std::max(mergedRect.x + mergedRect.width,
                adjustedRect.x + adjustedRect.width);
            int y2 = std::max(mergedRect.y + mergedRect.height,
                adjustedRect.y + adjustedRect.height);

            mergedRect = cv::Rect(x1, y1, x2 - x1, y2 - y1);
        }
    }
    FILE_LOG_DEBUG("[Algotithm] GetLastResult: MergedRect Completed. MergedRect：%d, %d, %d, %d,"
        , mergedRect.x
        , mergedRect.y
        , mergedRect.width
        , mergedRect.height);
    //获取大图中的rect结果
    std::vector<drawInformation> DrawInfoLarge = m_GeneralMethod.stitchImagesAndTransformRects(
        showImage_ch0, 
        stitchedImage_ch0, 
        batchDrawInfoArray);

    //对大图进行裁剪得到玻璃区域，并将所有检测结果也都转换到裁剪后的玻璃区域
    //std::vector<cv::Mat> showImage;
    outImages.clear();
    outImages.push_back(stitchedImage_ch0(mergedRect));
    outImages.push_back(stitchedImage_ch1(mergedRect));
    outImages.push_back(stitchedImage_ch2(mergedRect));
    //将图片进行旋转 将坐标也旋转
//4. 根据配置文件内容，判断是否需要对图像进行反转等处理
    int flipV = 0;
    for (size_t i = 0; i < outImages.size(); i++)
    {
        // 逆时针旋转 90 度
        transpose(outImages[i], outImages[i]); // 先转置
        flip(outImages[i], outImages[i], 0);   // 再垂直翻转

        if (flipV == 1) {
            // 水平翻转图片
            cv::flip(outImages[i], outImages[i], 0);
        }
    }

    FILE_LOG_DEBUG("[Algotithm] GetLastResult: Flip image Completed!");

    //转换坐标，将大图中的坐标转换到裁剪后的玻璃区域
    std::vector<drawInformation> roiDrawInfo = m_GeneralMethod.transformToROICoordinates(DrawInfoLarge, mergedRect);


    // 旋转检测结果
    // 获取ROI图像的尺寸（旋转前）
    int roi_width = outImages[0].cols;
    int roi_height = outImages[0].rows;
    //outDrawInfo = m_GeneralMethod.rotateDrawInfoCCW90(roiDrawInfo, roi_width, roi_height);
    std::vector<drawInformation> outDrawInfo_ = m_GeneralMethod.rotateDrawInfoCCW90(roiDrawInfo, roi_width, roi_height);
    FILE_LOG_DEBUG("[Algotithm] GetLastResult: RotateDrawInfo Completed!");

    // width : 15000 600  height :11420  400  0.034
    outDrawInfo = m_GeneralMethod.filterDefectsByBorderRegion(outDrawInfo_, roi_width, roi_height, 0.04, 0.035);


    FILE_LOG_DEBUG("[Algotithm] GetLastResult: Completed.");
    return true;
}



bool AlgorithmProcess::SetRecipeParameterToCompute()
{ 
    if (RecipeInfo.defects.size()<=0)
    {
        FILE_LOG_ERROR("[Algotithm] SetRecipeParameterToCompute error: RecipeInfo.defects.size()<=0.");

        return false;
    }
    // 初始化基本参数
    m_Parameter.x_pixel2mm = Pixle2MM_X;
    m_Parameter.y_pixel2mm = Pixle2MM_Y;
    m_Parameter.thresholdDetect_1 = RecipeInfo.global.thresholdDetect_1;
    m_Parameter.thresholdDetect_2 = RecipeInfo.global.thresholdDetect_2;

    m_Parameter.grayDifference = RecipeInfo.global.grayDifference;

    m_Parameter.thresholdsCls.resize(DefectType::DEFECT_TYPE_COUNT, 1.0f);
    for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
    {
        int id = RecipeInfo.defects[i].id;
        m_Parameter.thresholdsCls[id]= RecipeInfo.defects[i].confidence;

    }
    FILE_LOG_INFO("Confidences Completed.");

    // 使用默认阈值初始化各类缺陷参数
    defectLevel defectLevelInstance;
    int typeIndex = -1;
    // 镀膜不良 (PoorCoating) 面积
    {
        std::vector<float> poorCoatingArea = { 0.0f, 1.0f, 3.0f, 10.0f };
        std::vector<float> poorCoatingLength = { 0.0f, 0.0f, 3.0f, 10.0f };
        std::vector<float> poorCoatingPerimeter = { 0.0f, 1.0f, 3.0f, 10.0f };

        
        for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
        {
            if (RecipeInfo.defects[i].id == 0)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex >= 0)
        {
            poorCoatingArea[1] = RecipeInfo.defects[typeIndex].minorThreshold;
            poorCoatingArea[2] = RecipeInfo.defects[typeIndex].moderateThreshold;
            poorCoatingArea[3] = RecipeInfo.defects[typeIndex].majorThreshold;

            FILE_LOG_INFO("PoorCoating Completed. confidence=%f,confidence_src=%f", m_Parameter.thresholdsCls[0], RecipeInfo.defects[typeIndex].confidence);
        }
        defectLevelInstance.setRankJudgment(m_Parameter.PoorCoating, poorCoatingArea, poorCoatingLength, poorCoatingPerimeter);
    }


    // 划伤 (scratch)    周长
    {
        std::vector<float> scratchArea = { 0.0f, 1.0f, 3.0f, 15.0f };
        std::vector<float> scratchLength = { 0.0f, 1.0f, 3.0f, 15.0f };
        std::vector<float> scratchPerimeter = { 0.0f, 1.0f, 3.0f, 15.0f };

        typeIndex = -1;
        for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
        {
            if (RecipeInfo.defects[i].id == 1)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex >= 0)
        {
            scratchPerimeter[1] = RecipeInfo.defects[typeIndex].minorThreshold;
            scratchPerimeter[2] = RecipeInfo.defects[typeIndex].moderateThreshold;
            scratchPerimeter[3] = RecipeInfo.defects[typeIndex].majorThreshold;

            FILE_LOG_INFO("Scratch Completed. confidence=%f,confidence_src=%f", m_Parameter.thresholdsCls[1], RecipeInfo.defects[typeIndex].confidence);
        }
        defectLevelInstance.setRankJudgment(m_Parameter.scratch, scratchArea, scratchLength, scratchPerimeter);
    }


    // 结石 (calculus) 周长
    {
        std::vector<float> calculusArea = { 0.0f, 1.0f, 3.0f, 20.0f };
        std::vector<float> calculusLength = { 0.0f, 1.0f, 3.0f, 20.0f };
        std::vector<float> calculusPerimeter = { 0.0f, 1.0f, 3.0f, 20.0f };

        typeIndex = -1;
        for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
        {
            if (RecipeInfo.defects[i].id == 2)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex >= 0)
        {
            calculusPerimeter[1] = RecipeInfo.defects[typeIndex].minorThreshold;
            calculusPerimeter[2] = RecipeInfo.defects[typeIndex].moderateThreshold;
            calculusPerimeter[3] = RecipeInfo.defects[typeIndex].majorThreshold;

            FILE_LOG_INFO("Calculus Completed.. confidence=%f,confidence_src=%f", m_Parameter.thresholdsCls[2], RecipeInfo.defects[typeIndex].confidence);
        }
        defectLevelInstance.setRankJudgment(m_Parameter.calculus, calculusArea, calculusLength, calculusPerimeter);
    }


    // 气泡 (bubble) 周长
    {
        std::vector<float> bubbleArea = { 0.0f, 1.0f, 10.0f, 15.0f };
        std::vector<float> bubbleLength = { 0.0f, 1.0f, 10.0f, 15.0f };
        std::vector<float> bubblePerimeter = { 0.0f, 1.0f, 10.0f, 15.0f };

        typeIndex = -1;
        for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
        {
            if (RecipeInfo.defects[i].id == 3)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex >= 0)
        {
            bubblePerimeter[1] = RecipeInfo.defects[typeIndex].minorThreshold;
            bubblePerimeter[2] = RecipeInfo.defects[typeIndex].moderateThreshold;
            bubblePerimeter[3] = RecipeInfo.defects[typeIndex].majorThreshold;

            FILE_LOG_INFO("Bubble Completed. confidence=%f,confidence_src=%f", m_Parameter.thresholdsCls[3], RecipeInfo.defects[typeIndex].confidence);
        }
        defectLevelInstance.setRankJudgment(m_Parameter.bubble, bubbleArea, bubbleLength, bubblePerimeter);
    }
    

    // 商标TRADEMARK
    {
        std::vector<float> trademarkArea = { 0.0f, 1.0f, 10.0f, 20.0f };
        std::vector<float> trademarkLength = { 0.0f, 1.0f, 10.0f, 20.0f };
        std::vector<float> trademarkPerimeter = { 0.0f, 1.0f, 10.0f, 20.0f };

        typeIndex = -1;
        for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
        {
            if (RecipeInfo.defects[i].id == 4)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex >= 0)
        {
            trademarkArea[1] = RecipeInfo.defects[typeIndex].minorThreshold;
            trademarkArea[2] = RecipeInfo.defects[typeIndex].moderateThreshold;
            trademarkArea[3] = RecipeInfo.defects[typeIndex].majorThreshold;


            FILE_LOG_INFO("Trademark Completed. confidence=%f,confidence_src=%f", m_Parameter.thresholdsCls[4], RecipeInfo.defects[typeIndex].confidence);
        }
        defectLevelInstance.setRankJudgment(m_Parameter.Trademark, trademarkArea, trademarkLength, trademarkPerimeter);
    }

    // 水渍 (WaterStain) 面积
    {
        std::vector<float> waterStainArea = { 0.0f, 1.0f, 10.0f, 20.0f };
        std::vector<float> waterStainLength = { 0.0f, 1.0f, 10.0f, 20.0f };
        std::vector<float> waterStainPerimeter = { 0.0f, 1.0f, 10.0f, 20.0f };

        typeIndex = -1;
        for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
        {
            if (RecipeInfo.defects[i].id == 5)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex >= 0)
        {
            waterStainArea[1] = RecipeInfo.defects[typeIndex].minorThreshold;
            waterStainArea[2] = RecipeInfo.defects[typeIndex].moderateThreshold;
            waterStainArea[3] = RecipeInfo.defects[typeIndex].majorThreshold;


            FILE_LOG_INFO("WaterStain Completed. confidence=%f,confidence_src=%f", m_Parameter.thresholdsCls[5], RecipeInfo.defects[typeIndex].confidence);
        }
        defectLevelInstance.setRankJudgment(m_Parameter.WaterStain, waterStainArea, waterStainLength, waterStainPerimeter);
    }


    // 脏污 (smudge)  面积
    {
        std::vector<float> smudgeArea = { 0.0f, 1.0f, 15.0f, 20.0f };
        std::vector<float> smudgeLength = { 0.0f, 1.0f, 15.0f, 20.0f };
        std::vector<float> smudgePerimeter = { 0.0f, 1.0f, 15.0f, 20.0f };

        typeIndex = -1;
        for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
        {
            if (RecipeInfo.defects[i].id == 6)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex >= 0)
        {
            smudgeArea[1] = RecipeInfo.defects[typeIndex].minorThreshold;
            smudgeArea[2] = RecipeInfo.defects[typeIndex].moderateThreshold;
            smudgeArea[3] = RecipeInfo.defects[typeIndex].majorThreshold;


            FILE_LOG_INFO("Smudge Completed. confidence=%f,confidence_src=%f", m_Parameter.thresholdsCls[6], RecipeInfo.defects[typeIndex].confidence);
        }
        defectLevelInstance.setRankJudgment(m_Parameter.smudge, smudgeArea, smudgeLength, smudgePerimeter);
    }

    // 7 丝印 
    {
        std::vector<float> area         = { 0.0f, 999.0f, 999.0f, 999.0f };
        std::vector<float> length       = { 0.0f, 999.0f, 999.0f, 999.0f };
        std::vector<float> perimeter    = { 0.0f, 999.0f, 999.0f, 999.0f };

        typeIndex = -1;
        for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
        {
            if (RecipeInfo.defects[i].id == 7)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex >= 0)
        {
            area[1] = RecipeInfo.defects[typeIndex].minorThreshold;
            area[2] = RecipeInfo.defects[typeIndex].moderateThreshold;
            area[3] = RecipeInfo.defects[typeIndex].majorThreshold;


            FILE_LOG_INFO("ScreenPrinting Completed. confidence=%f,confidence_src=%f", m_Parameter.thresholdsCls[7], RecipeInfo.defects[typeIndex].confidence);
        }
        defectLevelInstance.setRankJudgment(m_Parameter.ScreenPrinting, area, length, perimeter);
    }

    //8 崩边
    //if (!IsChippedEdgeUsed)
    {
        // 崩边
        std::vector<float> area         = { 0.0f, 999.0f, 999.0f, 999.0f };
        std::vector<float> length       = { 0.0f, 999.0f, 999.0f, 999.0f };
        std::vector<float> perimeter    = { 0.0f, 999.0f, 999.0f, 999.0f };

        typeIndex = -1;
        for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
        {
            if (RecipeInfo.defects[i].id == 8)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex >= 0)
        {
            area[1] = RecipeInfo.defects[typeIndex].minorThreshold;
            area[2] = RecipeInfo.defects[typeIndex].moderateThreshold;
            area[3] = RecipeInfo.defects[typeIndex].majorThreshold;

            
            FILE_LOG_INFO("CHIPPEDEDGE Completed. confidence=%f,confidence_src=%f", m_Parameter.thresholdsCls[8], RecipeInfo.defects[typeIndex].confidence);
        }
        defectLevelInstance.setRankJudgment(m_Parameter.ChippedEdge, area, length, perimeter);
    }
    //9 麻点
    //if (!IsPittingUsed)
    {
        std::vector<float> area         = { 0.0f, 999.0f, 999.0f, 999.0f };
        std::vector<float> length       = { 0.0f, 999.0f, 999.0f, 999.0f };
        std::vector<float> perimeter    = { 0.0f, 999.0f, 999.0f, 999.0f };

        typeIndex = -1;
        for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
        {
            if (RecipeInfo.defects[i].id == 9)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex >= 0)
        {
            area[1] = RecipeInfo.defects[typeIndex].minorThreshold;
            area[2] = RecipeInfo.defects[typeIndex].moderateThreshold;
            area[3] = RecipeInfo.defects[typeIndex].majorThreshold;

            
            FILE_LOG_INFO("PITTING Completed. confidence=%f,confidence_src=%f", m_Parameter.thresholdsCls[9], RecipeInfo.defects[typeIndex].confidence);
        }
        defectLevelInstance.setRankJudgment(m_Parameter.Pitting, area, length, perimeter);
    }
    //10 玻渣
    //if (!IsGlassCulletUsed)
    { 
        std::vector<float> area         = { 0.0f, 999.0f, 999.0f, 999.0f };
        std::vector<float> length       = { 0.0f, 999.0f, 999.0f, 999.0f };
        std::vector<float> perimeter    = { 0.0f, 999.0f, 999.0f, 999.0f };

        typeIndex = -1;
        for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
        {
            if (RecipeInfo.defects[i].id == 10)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex >= 0)
        {
            area[1] = RecipeInfo.defects[typeIndex].minorThreshold;
            area[2] = RecipeInfo.defects[typeIndex].moderateThreshold;
            area[3] = RecipeInfo.defects[typeIndex].majorThreshold;

            
            FILE_LOG_INFO("GLASSCULET Completed. confidence=%f,confidence_src=%f", m_Parameter.thresholdsCls[10], RecipeInfo.defects[typeIndex].confidence);
        }
        defectLevelInstance.setRankJudgment(m_Parameter.GlassCullet, area, length, perimeter);
    }
    //11 云朵
    //if (!IsWavinessUsed)
    {
        //11 云朵
        std::vector<float> area         = { 0.0f, 999.0f, 999.0f, 999.0f };
        std::vector<float> length       = { 0.0f, 999.0f, 999.0f, 999.0f };
        std::vector<float> perimeter    = { 0.0f, 999.0f, 999.0f, 999.0f };

        typeIndex = -1;
        for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
        {
            if (RecipeInfo.defects[i].id == 11)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex >= 0)
        {
            area[1] = RecipeInfo.defects[typeIndex].minorThreshold;
            area[2] = RecipeInfo.defects[typeIndex].moderateThreshold;
            area[3] = RecipeInfo.defects[typeIndex].majorThreshold;

            
            FILE_LOG_INFO("WAVINESS Completed. confidence=%f,confidence_src=%f", m_Parameter.thresholdsCls[11], RecipeInfo.defects[typeIndex].confidence);
        }
        defectLevelInstance.setRankJudgment(m_Parameter.Waviness, area, length, perimeter);
    }
    //12 其他
    //if (!IsOtherUsed)
    {
        //12 其他
        std::vector<float> area         = { 0.0f, 999.0f, 999.0f, 999.0f };
        std::vector<float> length       = { 0.0f, 999.0f, 999.0f, 999.0f };
        std::vector<float> perimeter    = { 0.0f, 999.0f, 999.0f, 999.0f };

        typeIndex = -1;
        for (size_t i = 0; i < RecipeInfo.defects.size(); i++)
        {
            if (RecipeInfo.defects[i].id == 12)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex >= 0)
        {
            area[1] = RecipeInfo.defects[typeIndex].minorThreshold;
            area[2] = RecipeInfo.defects[typeIndex].moderateThreshold;
            area[3] = RecipeInfo.defects[typeIndex].majorThreshold;
            FILE_LOG_INFO("OTHER Completed. confidence=%f,confidence_src=%f", m_Parameter.thresholdsCls[12], RecipeInfo.defects[typeIndex].confidence);
        }

        defectLevelInstance.setRankJudgment(m_Parameter.Other, area, length, perimeter);
        
    }



    return true;

}