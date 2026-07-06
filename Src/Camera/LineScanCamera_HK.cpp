#include "LineScanCamera_HK.h"
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <chrono>
#include <iostream>
#include "CameraManager.h"
#include "Log.hpp"

//静态成员变量初始化
std::mutex  LineScanCamera_HK::m_fileLock;
std::string LineScanCamera_HK::m_dirName = "";

void ImageGrabCallBack(unsigned char* pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser)
{
    if (pUser == nullptr || pData == nullptr || pFrameInfo == nullptr)
    {
        FILE_LOG_DEBUG("ImageGrabCallBack: Invalid parameters!");
        return;
    }

    auto camera = static_cast<LineScanCamera_HK*>(pUser);
    if (camera == nullptr)
    {
        FILE_LOG_DEBUG("static_cast failed! camera is empty!");
        return;
    }

    // 使用unique_ptr自动释放data内存
    std::unique_ptr<unsigned char[]> data(new unsigned char[pFrameInfo->nFrameLen]);
    std::memcpy(data.get(), pData, pFrameInfo->nFrameLen);

    // 创建cv::Mat（注意：这里Mat会引用data的内存，所以data要保持存在）
    cv::Mat tempMat(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, data.get());
    cv::Mat mat = tempMat.clone();  // clone创建独立拷贝

    // 更新frameInfo
    delete camera->m_frameInfo;
    camera->m_frameInfo = new MV_FRAME_OUT_INFO_EX;
    std::memcpy(camera->m_frameInfo, pFrameInfo, sizeof(MV_FRAME_OUT_INFO_EX));

    if (!camera->PushFrame(std::move(mat)))
    {
        FILE_LOG_DEBUG("Camera[%d] frame queue is full, dropping frame", camera->GetCameraId());
    }

    // data会在unique_ptr离开作用域时自动释放
}

LineScanCamera_HK::LineScanCamera_HK() :
    
    m_deviceInfo(nullptr),
    m_frameInfo(nullptr)
{
    m_number = -1;
}

LineScanCamera_HK::LineScanCamera_HK(int num) :

    m_frameInfo(nullptr),
    m_deviceInfo(nullptr)
{
    m_number = num;
}

LineScanCamera_HK::LineScanCamera_HK(int num, MV_CC_DEVICE_INFO* cameraInfo) :
    LineScanCamera_HK(num)
{
    if (cameraInfo != nullptr)
    {
        m_deviceInfo = new MV_CC_DEVICE_INFO;
        std::memcpy(m_deviceInfo, cameraInfo, sizeof(MV_CC_DEVICE_INFO));
    }
    else
    {
        m_deviceInfo = nullptr;
    }
}

LineScanCamera_HK::LineScanCamera_HK(const LineScanCamera_HK& other)
    
{
    m_number = other.m_number;
    if (other.m_frameInfo != nullptr)
    {
        m_frameInfo = new MV_FRAME_OUT_INFO_EX;
        memmove(m_frameInfo, other.m_frameInfo, sizeof(MV_FRAME_OUT_INFO_EX));
    }
    else
    {
        m_frameInfo = nullptr;
    }
    
    if (other.m_deviceInfo != nullptr)
    {
        m_deviceInfo = new MV_CC_DEVICE_INFO;
        memmove(m_deviceInfo, other.m_deviceInfo, sizeof(MV_CC_DEVICE_INFO));
    }
    else
    {
        m_deviceInfo = nullptr;
    }
}

LineScanCamera_HK& LineScanCamera_HK::operator=(const LineScanCamera_HK& other)
{
    if (this == &other)
    {
        return *this;
    }

    m_number        = other.m_number;

    if (m_deviceInfo != nullptr)
    {
        delete m_deviceInfo;
        m_deviceInfo = nullptr;
    }
    memmove(m_deviceInfo, other.m_deviceInfo, sizeof(MV_CC_DEVICE_INFO));

    if (m_frameInfo != nullptr)
    {
        delete m_frameInfo;
        m_frameInfo = nullptr;
    }
    memmove(m_frameInfo, other.m_frameInfo, sizeof(MV_FRAME_OUT_INFO_EX));

    //m_img = other.m_img.clone();
}

LineScanCamera_HK::~LineScanCamera_HK()
{
    //// 停止采集和关闭设备
    //StopGrabbing();
    //Close();

    // 释放资源
    if (m_deviceInfo != nullptr)
    {
        delete m_deviceInfo;
        m_deviceInfo = nullptr;
    }

    if (m_frameInfo != nullptr)
    {
        delete m_frameInfo;
        m_frameInfo = nullptr;
    }

    //// 停止保存线程
    //StopSaveImageThread();

    std::cout << "LineScanCamera_HK::~LineScanCamera_HK()" << std::endl;
}

CameraInfoList LineScanCamera_HK::EnumDevices()
{
    MV_CC_DEVICE_INFO_LIST mvDeviceList;
    CameraInfoList cameraList;
    memset(&mvDeviceList, 0x00, sizeof(MV_CC_DEVICE_INFO_LIST));
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE | MV_GENTL_GIGE_DEVICE | MV_GENTL_CAMERALINK_DEVICE |
        MV_GENTL_CXP_DEVICE | MV_GENTL_XOF_DEVICE, &mvDeviceList);
    if (nRet != MV_OK)
    {
        return cameraList;
    }
    if (mvDeviceList.nDeviceNum == 0)
    {
        return cameraList;
    }
    for (unsigned int i = 0; i < mvDeviceList.nDeviceNum; ++i)
    {
        CameraInfo cameraInfo;
        MV_CC_DEVICE_INFO* pDeviceInfo = mvDeviceList.pDeviceInfo[i];

        if (pDeviceInfo == nullptr)
        {
            continue;
        }
        if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
        {
            int nIp1 = ((mvDeviceList.pDeviceInfo[i]->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
            int nIp2 = ((mvDeviceList.pDeviceInfo[i]->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
            int nIp3 = ((mvDeviceList.pDeviceInfo[i]->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
            int nIp4 = (mvDeviceList.pDeviceInfo[i]->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);

            char ipAddr[100] = { 0 };
            std::snprintf(ipAddr, sizeof(ipAddr), "%d.%d.%d.%d", nIp1, nIp2, nIp3, nIp4);

            cameraInfo.IpAddr = std::string(ipAddr);

            cameraInfo.serialNum = std::string((char*)pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber,
                sizeof(pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber) / sizeof(pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber[0]));

            cameraInfo.type = CameraType::eGige;
        }
        else if (pDeviceInfo->nTLayerType == MV_USB_DEVICE)
        {
            cameraInfo.serialNum = std::string((char*)pDeviceInfo->SpecialInfo.stUsb3VInfo.chSerialNumber,
                sizeof(pDeviceInfo->SpecialInfo.stUsb3VInfo.chSerialNumber) / sizeof(pDeviceInfo->SpecialInfo.stUsb3VInfo.chSerialNumber[0]));

            cameraInfo.type = CameraType::eUsb3;
        }
        else if (pDeviceInfo->nTLayerType == MV_GENTL_CAMERALINK_DEVICE)
        {
            cameraInfo.serialNum = std::string((char*)pDeviceInfo->SpecialInfo.stCMLInfo.chSerialNumber,
                sizeof(pDeviceInfo->SpecialInfo.stCMLInfo.chSerialNumber) / sizeof(pDeviceInfo->SpecialInfo.stCMLInfo.chSerialNumber[0]));

            cameraInfo.type = CameraType::eCameraLink;
        }
        cameraInfo.deviceInfo = pDeviceInfo;
        cameraList.emplace_back(cameraInfo);
    }
    return cameraList;
}

bool LineScanCamera_HK::InitSDK()
{
    int nRet = MV_CC_Initialize();
    if (MV_OK != nRet)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HK::FinalizeSDK()
{
    int nRet = MV_CC_Finalize();
    if (MV_OK != nRet)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HK::IsDeviceAccessable(unsigned int accessMode)
{
    return MV_CC_IsDeviceAccessible(m_deviceInfo, accessMode);
}

bool LineScanCamera_HK::Open()
{
    if (MV_NULL == m_deviceInfo)
    {
        return false;
    }

    //if (m_hDevHandle)
    //{
    //    return false;
    //}

    int nRet = MV_CC_CreateHandle(&m_hDevHandle, m_deviceInfo);
    if (MV_OK != nRet)
    {
        m_hDevHandle = MV_NULL;
        return false;
    }

    nRet = MV_CC_OpenDevice(m_hDevHandle);
    if (MV_OK != nRet)
    {
        MV_CC_DestroyHandle(m_hDevHandle);
        m_hDevHandle = MV_NULL;
        return false;
    }

    return true;
}

bool LineScanCamera_HK::Close()
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }

    MV_CC_CloseDevice(m_hDevHandle);

    int nRet = MV_CC_DestroyHandle(m_hDevHandle);
    m_hDevHandle = MV_NULL;

    return true;
}

bool LineScanCamera_HK::IsDeviceConnect()
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }
    return MV_CC_IsDeviceConnected(m_hDevHandle);
}

bool LineScanCamera_HK::RegisterImageCallBack()
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }
    int nRet = MV_CC_RegisterImageCallBackEx(m_hDevHandle, ImageGrabCallBack, this);
    
    if (nRet != MV_OK)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HK::StartGrabbing()
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }

    int nRet = MV_CC_StartGrabbing(m_hDevHandle);

    if (nRet != MV_OK)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HK::StopGrabbing()
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }
    int nRet = MV_CC_StopGrabbing(m_hDevHandle);

    if (nRet != MV_OK)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HK::SetFrameBrustStartTrigger()
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }
    int nRet = SetTriggerSwitch(true);
    if (nRet != MV_OK)
    {
        return false;
    }

    nRet = MV_CC_SetEnumValue(m_hDevHandle, "TriggerSelector", (int)MV_CAM_TRIGGER_OPTION::FRAMEBURSTSTART);
    if (nRet != MV_OK)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HK::SetLineStartTrigger()
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }
    int nRet = SetTriggerSwitch(true);
    if (nRet != MV_OK)
    {
        return false;
    }
    nRet = MV_CC_SetEnumValue(m_hDevHandle, "TriggerSelector", (int)MV_CAM_TRIGGER_OPTION::LINESTART);
    if (nRet != MV_OK)
    {
        return false;
    }

    return true;
}

bool LineScanCamera_HK::SetTriggerSource(TriggerSource source)
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }
    int nRet = SetTriggerSwitch(true);
    if (nRet != MV_OK)
    {
        return false;
    }
    
    nRet = MV_CC_SetEnumValue(m_hDevHandle, "TriggerSource", (int)source);
    if (nRet != MV_OK)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HK::SetTriggerSwitch(bool mode)
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }
    int nRet = MV_CC_SetEnumValue(m_hDevHandle, "TriggerMode", (int)mode);
    if (nRet != MV_OK)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HK::SetExposureTime(float& exp)
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }
    int nRet = MV_CC_SetEnumValue(m_hDevHandle, "ExposureAuto", MV_EXPOSURE_AUTO_MODE_OFF);
    if (nRet != MV_OK)
    {
        return false;
    }
    nRet = MV_CC_SetFloatValue(m_hDevHandle, "ExposureTime", exp);
    if (nRet != MV_OK)
    {
        return false;
    }
    return true;
}

float LineScanCamera_HK::GetExposureTime()
{
    if (MV_NULL == m_hDevHandle)
    {
        return -1.0f;
    }
    MVCC_FLOATVALUE stFloatValue = { 0 };
    int nRet = MV_CC_GetFloatValue(m_hDevHandle, "ExposureTime", &stFloatValue);
    //int nRet = m_camera.GetFloatValue("ExposureTime", &stFloatValue);
    if (nRet != MV_OK)
    {
        return -1.0f;
    }

    return stFloatValue.fCurValue;
}

bool LineScanCamera_HK::SetAacquisitionLineRateEnable(bool mode)
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }
    int nRet = MV_CC_SetBoolValue(m_hDevHandle, "AcquisitionLineRateEnable", mode);
    if (nRet != MV_OK)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HK::GetAcquisitionLineRateEnable()
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }
    bool mode;
    int nRet = MV_CC_GetBoolValue(m_hDevHandle, "AcquisitionLineRateEnable", &mode);
    
    if (nRet != MV_OK)
    {
        return false;
    }

    return mode;
}

bool LineScanCamera_HK::SetAcquisitionLineRate(int& lineRate)
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }
    int nRet = MV_CC_SetIntValueEx(m_hDevHandle, "AcquisitionLineRate", lineRate);
    if (nRet != MV_OK)
    {
        return false;
    }
    return true;
}

int LineScanCamera_HK::GetAcquisitionLineRate()
{
    if (MV_NULL == m_hDevHandle)
    {
        return -1;
    }
    MVCC_INTVALUE_EX stIntValue = { 0 };
    int nRet = MV_CC_GetIntValueEx(m_hDevHandle, "AcquisitionLineRate", &stIntValue);
    if (nRet != MV_OK)
    {
        return -1;
    }

    return stIntValue.nCurValue;
}

bool LineScanCamera_HK::SetImageHeight(int height)
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }
    int nRet = MV_CC_SetIntValueEx(m_hDevHandle, "Height", height);
    if (nRet != MV_OK)
    {
        return false;
    }
    return true;
}

int LineScanCamera_HK::GetImageWidth()
{
    if (MV_NULL == m_hDevHandle)
    {
        return -1;
    }
    MVCC_INTVALUE_EX stIntValue = { 0 };
    int nRet = MV_CC_GetIntValueEx(m_hDevHandle, "Width", &stIntValue);
    if (nRet != MV_OK)
    {
        return -1;
    }
    return stIntValue.nCurValue;
}

int LineScanCamera_HK::GetImageHeight()
{
    if (MV_NULL == m_hDevHandle)
    {
        return -1;
    }
    MVCC_INTVALUE_EX stIntValue = { 0 };
    int nRet = MV_CC_GetIntValueEx(m_hDevHandle, "Height", &stIntValue);
    if (nRet != MV_OK)
    {
        return -1;
    }
    return stIntValue.nCurValue;
}

bool LineScanCamera_HK::SetImageWidth(int& width)
{
    if (MV_NULL == m_hDevHandle)
    {
        return false;
    }
    int nRet = MV_CC_SetIntValueEx(m_hDevHandle, "Width", width);
    if (nRet != MV_OK)
    {
        return false;
    }
    return true;
}

void LineScanCamera_HK::SaveImageProcess()
{
    //REGISTER_THREAD_NAME("SaveImageProcess"); //用于分析线程崩溃时的调用栈
    
    namespace fs = std::filesystem;
    {
        /* 先加锁，防止多个线程同时修改静态成员变量 */
        std::lock_guard<std::mutex> lock(m_fileLock);

        if (m_dirName.empty())
        {
            /* 获取系统时间 */
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm stTime;
            localtime_s(&stTime, &now_c);

            /* 将系统时间戳转换为字符串 */
            std::stringstream ss;
            ss << std::put_time(&stTime, "%Y%m%d_%H%M%S");
            m_dirName = ss.str();
        }
    }
    
    //判断总体文件夹是否存在,不存在则创建
    auto savePath = (fs::current_path() / fs::path("Singal_Frame") / fs::path(m_dirName) / fs::path(std::to_string(m_number)));
    if (!fs::exists(savePath))
    {
        fs::create_directories(savePath);
    }

    int frameNum = 0;
    //设置OpenCV无损保存的参数
    std::vector<int> compression_params = { cv::IMWRITE_PNG_COMPRESSION, 9 };
    while (m_running.load())
    {
        std::unique_lock<std::mutex> lock(m_saveMutex);
        m_saveCv.wait_for(lock, std::chrono::milliseconds(1000), [&] {
            return m_running.load() || !m_saveQueue.empty();
        });

        if (!m_running.load())
        {
            std::cout << "Begin to exit queue has :" << m_saveQueue.size() << std::endl;
            //这里将queue的中的图片都保存完，然后退出线程
            while (!m_saveQueue.empty())
            {
                auto image = m_saveQueue.front();
                std::string imagePath = (savePath / fs::path(std::to_string(frameNum))).string();
                cv::imwrite(imagePath + ".png", image, compression_params);
                m_saveQueue.pop();
                frameNum++;
            }
            break;
        }

        //这里保存图片,但是为了速度考虑，每次只保存一张图片，不一下全保存
        if (!m_saveQueue.empty())
        {
            auto image = m_saveQueue.front();
            std::string imagePath = (savePath / fs::path(std::to_string(frameNum))).string();
            cv::imwrite(imagePath + ".png", image, compression_params);
            m_saveQueue.pop();
            frameNum++;
        }
    }

    std::cout << "Save single data exit :" << m_saveQueue.size() << std::endl;
}

void LineScanCamera_HK::StartSaveImageThread()
{
    m_running.store(true);
    m_saveThread = std::thread(&LineScanCamera_HK::SaveImageProcess, this);
}

void LineScanCamera_HK::StopSaveImageThread()
{
    m_running.store(false);
    m_saveCv.notify_one();
    if (m_saveThread.joinable())
        m_saveThread.join();
}

int LineScanCamera_HK::GetCameraId()
{
    return m_number;
}
