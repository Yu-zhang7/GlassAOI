#ifndef LINESCANCAMERA_HR_H
#define LINESCANCAMERA_HR_H

#include "LineScanCamera_Base.h"
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <chrono>
#include <iostream>
//#include "Device.h"
#include "Log.hpp"
//#include "ImageData.h"

// 华睿相机
#include "IMV/IMVApi.h"
#include "IMV/IMVDefines.h"

// 测试需要的内容
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <deque>

// 前置声明
void ImageGrabCallBack(IMV_Frame* pFrame, void* pUser);

class LineScanCamera_HR : public LineScanCamera_Base
{
public:
    /* 构造函数 */
    LineScanCamera_HR();

    /* 有参构造函数，传入相机序号 */
    LineScanCamera_HR(int num);

    /* 有参构造函数，传入相机序号和相机信息 */
    LineScanCamera_HR(int num, IMV_DeviceInfo cameraInfo);

    LineScanCamera_HR(const LineScanCamera_HR& other);

    LineScanCamera_HR& operator=(const LineScanCamera_HR& other);

    /* 析构函数 */
    ~LineScanCamera_HR();

    /* 枚举相机列表 */
    static CameraInfoList EnumDevices();

    /* 初始化SDK */
    static bool InitSDK();

    /* 反初始化SDK */
    static bool FinalizeSDK();

    /* 判断设备是否可达 */
    bool IsDeviceAccessable(unsigned int accessMode) override;

    /* 打开设备 */
    bool Open() override;

    /* 关闭设备 */
    bool Close() override;

    /* 判断相机是否处于连接状态 */
    bool IsDeviceConnect() override;

    /* 注册获取图像函数回调 */
    bool RegisterImageCallBack() override;

    /* 开始取流 */
    bool StartGrabbing() override;

    /* 停止取流 */
    bool StopGrabbing() override;

    /* 设置相机为帧触发模式 */
    bool SetFrameBrustStartTrigger() override;

    /* 设置相机为行触发模式 */
    bool SetLineStartTrigger() override;

    /* 设置相机触发源 */
    bool SetTriggerSource(TriggerSource source) override;

    /* 设置触发模式是否打开 */
    bool SetTriggerSwitch(bool mode) override;

    /* 设置相机曝光 */
    bool SetExposureTime(float& exp) override;

    /* 获取相机曝光 */
    float GetExposureTime() override;

    /* 设置行频使能 */
    bool SetAacquisitionLineRateEnable(bool mode) override;

    /* 获取行频使能 */
    bool GetAcquisitionLineRateEnable() override;

    /* 设置行频 */
    bool SetAcquisitionLineRate(int& lineRate) override;

    /* 获取行频 */
    int GetAcquisitionLineRate() override;

    /* 设置图像高度 */
    bool SetImageHeight(int height) override;

    /* 获取图像高度 */
    int GetImageHeight() override;

    /* 获取图像宽度 */
    int GetImageWidth() override;

    /* 设置图像宽度 */
    bool SetImageWidth(int& width) override;

    /* 获取相机ID */
    int GetCameraId() override;

    void StartSaveImageThread() override;

    void StopSaveImageThread() override;

    void SaveImageProcess() override;

    //void AddImageToQueue(cv::Mat& img) override;

    // 友元函数
    friend void ImageGrabCallBack(IMV_Frame* pFrame, void* pUser);

    IMV_HANDLE camHandle = NULL;
    std::string m_Key;
    std::string m_userId;

private:
    IMV_DeviceInfo* m_deviceInfo = nullptr;

    std::atomic<bool> m_running{ false };

    std::thread m_saveThread;

    std::mutex m_saveMutex;
    std::condition_variable m_saveCv;
    static std::string m_dirName;
    static std::mutex m_fileLock;


    std::vector<cv::Mat> m_imgList;
};

#endif  //LINESCANCAMERA_HR_H