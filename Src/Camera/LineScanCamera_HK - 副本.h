#ifndef LINESCANCAMERA_HK_H
#define LINESCANCAMERA_HK_H

#include "LineScanCamera_Base.h"
#include <string>
#include <list>
#include <functional>

// 海康相机
#include <MvCameraControl.h>

//#include "ImageData.h"

// 测试需要的内容
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

#ifndef MV_NULL
#define MV_NULL    0
#endif

// 前置声明
void ImageGrabCallBack(unsigned char* pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser);

class LineScanCamera_HK : public LineScanCamera_Base
{
public:
    /* 构造函数 */
    LineScanCamera_HK();

    /* 有参构造函数，传入相机序号 */
    LineScanCamera_HK(int num);

    /* 有参构造函数，传入相机序号和相机信息 */
    LineScanCamera_HK(int num, MV_CC_DEVICE_INFO* cameraInfo);

    LineScanCamera_HK(const LineScanCamera_HK& other);

    LineScanCamera_HK& operator=(const LineScanCamera_HK& other);

    /* 析构函数 */
    ~LineScanCamera_HK();

    //CameraInfoList EnumDeivces();

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
    int GetCameraId();

    void StartSaveImageThread() override;

    void StopSaveImageThread() override;

    void SaveImageProcess() override;

    //void AddImageToQueue(cv::Mat& img) override;

    // 友元函数
    friend void ImageGrabCallBack(unsigned char* pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser);

    //bool GetReadyBatch(std::vector<cv::Mat>& batch);
    //int GetReadyBatchCount();
    //void GetMatListRange(std::vector<cv::Mat>& imgList, int num);
    //void ClearBuffers();
    //bool GetLastBatch(std::vector<cv::Mat>& lastBatch);
    //void DataRelease();
    //int GetTriggerNum();
    
    //int getImageCount();
    //bool getImageFromList(cv::Mat& outImage);
private:
    MV_CC_DEVICE_INFO* m_deviceInfo;           // 设备信息
    MV_FRAME_OUT_INFO_EX* m_frameInfo;         // 帧信息
    void* m_hDevHandle;                        // 设备句柄

    // 图像保存相关
    std::atomic<bool> m_running{ false };
									
    std::thread m_saveThread;
									
										 
    std::mutex m_saveMutex;
    std::condition_variable m_saveCv;
    std::queue<cv::Mat> m_saveQueue;

    int m_number;                              // 相机编号

    // 静态成员
    static std::mutex m_fileLock;
    static std::string m_dirName;
								 

								   

};
#endif  //LINESCANCAMERA_HK_H