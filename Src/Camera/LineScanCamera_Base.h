// LineScanCamera_Base.h
#ifndef LINESCANCAMERA_BASE_H
#define LINESCANCAMERA_BASE_H


/******使用示例***********************************
* // 初始化SDK
CameraFactory::InitSDK(CameraBrand::eHikvision);

// 枚举设备
auto devices = CameraFactory::EnumDevices(CameraBrand::eHikvision);

// 创建相机
auto camera = CameraFactory::CreateCamera(CameraBrand::eHikvision, 0, &deviceInfo);

// 使用统一接口操作相机
camera->Open();
camera->StartGrabbing();

// 使用基类中实现的通用功能
std::vector<cv::Mat> batch;
if (camera->GetReadyBatch(batch)) {
    // 处理批次图像
}

int batchCount = camera->GetReadyBatchCount();
// ...
*****************************************************/

#include <string>
#include <list>
#include <vector>
#include <deque>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <MvCameraControl.h>
#include "IMV/IMVDefines.h"

#define STR_LEN 16

enum class CameraType
{
    eGige = 0,
    eUsb3 = 1,
    eCameraLink = 2
};

enum class TriggerSource
{
    eLine0 = 0,
    eLine1 = 1,
    eLine2 = 2,
    eLine3 = 3,
    eCounter0 = 4,
    eSoftWare = 7,
    eFrequencyConverter = 8
};

enum class MV_CAM_TRIGGER_OPTION
{
    FRAMEBURSTSTART = 6,
    LINESTART = 9
};

struct CameraInfo
{
    std::string serialNum;
    std::string IpAddr;
    CameraType  type;
    MV_CC_DEVICE_INFO* deviceInfo;
    IMV_DeviceInfo* deviceInfo_hr;
};
using CameraInfoList = std::list<CameraInfo>;

class LineScanCamera_Base
{
public:
    virtual ~LineScanCamera_Base();

    //测试函数。用于从文件中读取图像
    void ReadFrameImageFromFile(int cameraId, const std::string& path);

    


    // 静态方法
    static CameraInfoList EnumDevices();
    static bool InitSDK();
    static bool FinalizeSDK();

    // 实例方法 - 纯虚函数（需要派生类实现）
    virtual bool IsDeviceAccessable(unsigned int accessMode) = 0;
    virtual bool Open() = 0;
    virtual bool Close() = 0;
    virtual bool IsDeviceConnect() = 0;
    virtual bool RegisterImageCallBack() = 0;
    virtual bool StartGrabbing() = 0;
    virtual bool StopGrabbing() = 0;
    virtual bool SetFrameBrustStartTrigger() = 0;
    virtual bool SetLineStartTrigger() = 0;
    virtual bool SetTriggerSource(TriggerSource source) = 0;
    virtual bool SetTriggerSwitch(bool mode) = 0;
    virtual bool SetExposureTime(float& exp) = 0;
    virtual float GetExposureTime() = 0;
    virtual bool SetAacquisitionLineRateEnable(bool mode) = 0;
    virtual bool GetAcquisitionLineRateEnable() = 0;
    virtual bool SetAcquisitionLineRate(int& lineRate) = 0;
    virtual int GetAcquisitionLineRate() = 0;
    virtual bool SetImageHeight(int height) = 0;
    virtual int GetImageHeight() = 0;
    virtual int GetImageWidth() = 0;
    virtual bool SetImageWidth(int& width) = 0;
    virtual int GetCameraId() = 0;
    //virtual void GetMat(cv::Mat& img) = 0;
    //virtual void GetMatList(std::vector<cv::Mat>& imgList) = 0;
    virtual void StartSaveImageThread() = 0;
    virtual void StopSaveImageThread() = 0;
    virtual void SaveImageProcess() = 0;
    //virtual void AddImageToQueue(cv::Mat& img) = 0;

    //队列操作方法
    bool PushFrame(cv::Mat&& frame);
    bool PopFrame(cv::Mat& frame, int timeout_ms = 0);
    bool TryPopFrame(cv::Mat& frame); // 非阻塞版本

    void SetmaxQueueSize(int maxSize = 1500);

    /* 获取队列大小 */
    size_t GetQueueSize() const;

    /* 队列是否为空 */
    bool IsQueueEmpty() const;

    /* 队列是否已满 */
    bool IsQueueFull() const;

    /* 清空队列 */
    void ClearQueue();

    /* 获取丢帧数 */
    int GetDroppedFrames() const;

    /* 获取帧数 */
    int GetFrameCount() const;

    /* 等待队列非空 */
    bool WaitForFrame(int timeout_ms = -1);

    void GetMat(cv::Mat& img);
    void GetMatList(std::vector<cv::Mat>& imgList);
    void AddImageToQueue(cv::Mat& img);

    /* 判断玻璃是否开始 */
    cv::Mat GetFrontImage();

protected:
    // 保护成员 - 锁和条件变量
    mutable std::mutex queueMutex;
    std::condition_variable queueCV;

    // 图像队列和锁
    std::queue<cv::Mat> frameQueue;

    // 统计变量
    std::atomic<int> frameCounter{ 0 };
    std::atomic<int> droppedFrames{ 0 };

    // 队列最大大小
    size_t maxQueueSize = 1000;

}; 
#endif	//LINESCANCAMERA_BASE_H