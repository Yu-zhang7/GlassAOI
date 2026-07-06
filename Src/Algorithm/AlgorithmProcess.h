#pragma once
#include "Global.h"                 //全局变量类
#include "GeneralMethod.h"          //通用方法类
#include "ImageBeautify.h"          // 图像处理类

#include "Compute.h"                //算法管理类

#include "showVirtualInfo.h"        //生成虚拟图类

class AlgorithmProcess
{
public:
    /* 删除拷贝构造函数和赋值运算符 */
    AlgorithmProcess(const AlgorithmProcess&) = delete;
    AlgorithmProcess& operator=(const AlgorithmProcess&) = delete;

    void InitAlgorithmProcess();
    /* 获取单例实例的静态方法 */
    static AlgorithmProcess& GetInstance();

    /* 清理单例资源 */
    static void DestroyInstance();

    bool DetectImages(QueueDefectItem& resultItem, const std::string& resultPath);
    bool DetectImages_new(QueueDefectItem& resultItem, const std::string& resultPath);
    bool ComputeImage_Signal(cv::Mat& batchSourceImg, std::vector<cv::Mat>& batchEntireImage,
        std::vector<drawInformation>& batchDrawInfo);

    bool GetLastResult(std::vector<cv::Mat>& showImage_ch0,
        std::vector<cv::Mat>& showImage_ch1,
        std::vector<cv::Mat>& showImage_ch2,
        const std::vector<std::vector<drawInformation>>& batchDrawInfoArray,
        const std::vector<cv::Rect>& batchRectArray,
        std::vector<cv::Mat>& outImages,
        std::vector<drawInformation> outDrawInfo);

    //传递配方参数给算法
    bool SetRecipeParameterToCompute();

private:
    /* 私有构造函数 */
    AlgorithmProcess();
    //void ReadBatchImagesFromPath();
    //bool GetImageOnceFromFile(cv::Mat& batchImage);
private:
    GeneralMethod           m_GeneralMethod;                //通用方法类

    ImageBeautify           m_Beautify;                     //图像美化类

    Compute                 m_Compute;                      //算法管理类

    PrescriptionParameter   m_Parameter;                    //算法用配方参数

    std::vector<cv::Mat>    m_readBatchImages;
    int                     m_readBatchImageIndex;  
    //int                   m_LoopCount;                    //循环计数

    //int                   m_GetImageErrorCount;           //获取图像错误计数

    showVirtualInfo         m_showVirtualInfo;              //显示虚拟图
};

