#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <vector>
#include <map>
#include <condition_variable>
#include <mutex>
#include <opencv2/opencv.hpp>
#include "LineScanCamera_Base.h"
#include "CameraFactory.h"
#include <functional>
#include <memory>
#include <future> // 包含promise和future
#include <thread>
#include <iostream>
#include <atomic>

using CameraList = std::vector<std::shared_ptr<LineScanCamera_Base>>;
using DataCache = std::map<int, cv::Mat>;

/* 相机管理类，全局唯一，单例模式 */
class CameraManager
{
public:
    /* 全局访问入口 */
    static CameraManager* GetInstance()
    {
        static CameraManager manager;
        return &manager;
    }

    /* 析构函数 */
    ~CameraManager();

    /* 单例模式禁用以下函数 */
    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    // 通过循环遍历的方式，对相机列表进行设置

    /* 获取图片函数 */
    bool GetImage(cv::Mat& img);

    /* 通知函数 */
    void NotifyGrabFrameData(int num);

    /* 枚举相机列表 */
    static CameraInfoList EnumDeivces(CameraBrand brand);

    /* 初始化SDK */
    static bool InitSDK(CameraBrand brand);

    /* 反初始化SDK */
    static bool FinalizeSDK(CameraBrand brand);

    /* 打开设备 */
    bool Open();

    /* 关闭设备 */
    bool Close();

    /* 开始取流 */
    bool StartGrabbing();

    /* 停止取流 */
    bool StopGrabbing();

    /* 开始获取图像并处理图像 */
    bool StartProcessImages();

    /* 停止获取图像并处理图像 */
    bool StopProcessImages();

    /* 读图测试线程。配合IS_READ_MODE=1的模式来使用 */
    void ReadFrameImagesFromFile_ThreadFunction(int camId);

    /* 设置相机为帧触发模式 */
    bool SetFrameBrustStartTrigger();

    /* 设置相机为行触发模式 */
    bool SetLineStartTrigger();

    /* 设置相机触发源 */
    bool SetTriggerSource(TriggerSource source);

    /* 设置触发模式是否打开 */
    bool SetTriggerSwitch(bool mode);

    /* 设置相机曝光 */
    bool SetExposureTime(float exp);

    /* 设置行频使能 */
    bool SetAacquisitionLineRateEnable(bool mode);

    /* 设置行频 */
    bool SetAcquisitionLineRate(int lineRate);

    /* 设置图像高度 */
    bool ImageHeight(int height);

    /* 设置图像宽度 */
    bool SetImageWidth(int width);

    /* 获取相机单例接口 */
    std::shared_ptr<LineScanCamera_Base> GetCamera(int num)
    {
        if (num >= 0 && num < m_cameraList.size())
            return m_cameraList[num];
        return nullptr;
    }

    /* 获取相机个数 */
    int GetCameraNum()
    {
        return m_cameraList.size();
    }


    void SetStopGrabbingCallback(std::function<void()> callback);

    /* 图像拼接函数,运行在线程中 */
    void FrameStitchingThreadFunction();

    void FrameStitchingThreadFunction_new();

    /* 清理图像缓存 */
    void ClearBuffer();

	/* 推送玻璃进出状态 */
    void PushGlassInOutStatus(bool status);

    /* 拿图前清理函数 */
    void Clear()
    {
        m_dataCache.clear();
        m_height = 0;
        if (!m_totalImg.empty())
        {
            m_totalImg.release();
        }
    }

    /* 设置所有相机回调函数 */
    bool RegisterImageCallBack();

    /* 设置相机品牌 */
    void SetCameraBrand(CameraBrand brand) { m_cameraBrand = brand; }
    CameraBrand GetCameraBrand() const { return m_cameraBrand; }

    /* 初始化相机列表 */
    bool InitializeCameras();

private:
    /* 构造函数私有，单例模式 */
    CameraManager();

    // 主动stopGrabbing的回调
    std::function<void()> m_stopGrabbingCallback;

    // 相机品牌
    CameraBrand m_cameraBrand = CameraBrand::eHikvision;

public:


private:
    CameraList m_cameraList;              // 全局相机列表
    DataCache m_dataCache;                // 用来存放各个相机帧数据的缓存
    int m_height;                         // 记录行高

    /**** 管理图像处理线程相关的变量 *****/
    std::thread m_ImageProcessingThread;
    std::atomic<bool> m_ImageProcessRunning{ false };
    std::promise<void> m_ExitPromise;
    std::future<void> m_ExitFuture;
    /************************************/
    
    /******* 批次处理需要使用的变量 *****/
    std::mutex m_mutex;                   // 用来控制同步的变量
    std::condition_variable m_cv;         // 用来控制同步的条件变量
    std::thread m_thread;                 // 线程函数
    cv::Mat m_totalImg;                   // 用来存放图片的Mat

    // 批次同步状态
    std::mutex m_batchMutex;
    std::condition_variable m_batchCV;

    std::atomic<int> m_readyCount{ 0 };
    std::atomic<int> m_batchSize{ 15 };
    std::atomic<int> m_currentBatch{ 0 };

    std::queue<cv::Mat> m_allBatches;
    std::mutex allMutex;
    std::condition_variable allCV;

    int m_batchIndex = 0;
    /************************************/

    /********读帧图测试使用**********************/

    std::vector<std::deque<std::vector<cv::Mat>>> m_Read_ALL_CAM_batchQueue;
};

#endif  //CAMERAMANAGER_H