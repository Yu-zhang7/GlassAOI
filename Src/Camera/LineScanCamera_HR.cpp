#include "LineScanCamera_HR.h"
#include "Log.hpp"
#include "CameraManager.h"
#include <filesystem>

// 静态成员变量初始化
std::mutex LineScanCamera_HR::m_fileLock;
std::string LineScanCamera_HR::m_dirName = "";

static void displayDeviceInfo(IMV_DeviceList deviceInfoList)
{
    IMV_DeviceInfo* pDevInfo = NULL;
    unsigned int cameraIndex = 0;
    char vendorNameCat[11];
    char cameraNameCat[STR_LEN];

    // 打印Title行 
    printf("\nIdx Type Vendor     Model      S/N             DeviceUserID    IP Address    \n");
    printf("------------------------------------------------------------------------------\n");

    for (cameraIndex = 0; cameraIndex < deviceInfoList.nDevNum; cameraIndex++)
    {
        pDevInfo = &deviceInfoList.pDevInfo[cameraIndex];
        // 设备列表的相机索引  最大表示字数：3
        printf("%-3d", cameraIndex + 1);

        // 相机的设备类型（GigE，U3V，CL，PCIe）
        switch (pDevInfo->nCameraType)
        {
        case typeGigeCamera:printf(" GigE"); break;
        case typeU3vCamera:printf(" U3V "); break;
        case typeCLCamera:printf(" CL  "); break;
        case typePCIeCamera:printf(" PCIe"); break;
        default:printf("     "); break;
        }

        // 制造商信息  最大表示字数：10 
        if (strlen(pDevInfo->vendorName) > 10)
        {
            memcpy(vendorNameCat, pDevInfo->vendorName, 7);
            vendorNameCat[7] = '\0';
            strcat_s(vendorNameCat, "...");
            printf(" %-10.10s", vendorNameCat);
        }
        else
        {
            printf(" %-10.10s", pDevInfo->vendorName);
        }

        // 相机的型号信息 最大表示字数：10 
        printf(" %-10.10s", pDevInfo->modelName);

        // 相机的序列号 最大表示字数：15 
        printf(" %-15.15s", pDevInfo->serialNumber);

        // 自定义用户ID 最大表示字数：15 
        if (strlen(pDevInfo->cameraName) > 15)
        {
            memcpy(cameraNameCat, pDevInfo->cameraName, 12);
            cameraNameCat[12] = '\0';
            strcat_s(cameraNameCat, "...");
            printf(" %-15.15s", cameraNameCat);
        }
        else
        {
            printf(" %-15.15s", pDevInfo->cameraName);
        }

        // GigE相机时获取IP地址 
        if (pDevInfo->nCameraType == typeGigeCamera)
        {
            printf(" %s", pDevInfo->DeviceSpecificInfo.gigeDeviceInfo.ipAddress);
        }

        printf("\n");
    }

    return;
}

cv::Mat convertFrameToMat(IMV_Frame* frame)
{
    if (!frame || !frame->pData) {
        printf("Invalid frame data\n");
        return cv::Mat();
    }

    cv::Mat result;
    unsigned char* pDstData = nullptr;

    try {
        if (gvspPixelMono8 == frame->frameInfo.pixelFormat) {
            // 直接创建Mat并拷贝数据
            result = cv::Mat(
                frame->frameInfo.height,
                frame->frameInfo.width,
                CV_8UC1,
                frame->pData
            ).clone();  // 关键：深拷贝
        }
        else if (gvspPixelBGR8 == frame->frameInfo.pixelFormat) {
            result = cv::Mat(
                frame->frameInfo.height,
                frame->frameInfo.width,
                CV_8UC3,
                frame->pData
            ).clone();  // 关键：深拷贝
        }
        else {
            // 转换到BGR8格式
            pDstData = new unsigned char[frame->frameInfo.width * frame->frameInfo.height * 3];

            IMV_PixelConvertParam convertParam = { 0 };
            convertParam.nWidth = frame->frameInfo.width;
            convertParam.nHeight = frame->frameInfo.height;
            convertParam.ePixelFormat = frame->frameInfo.pixelFormat;
            convertParam.pSrcData = frame->pData;
            convertParam.nSrcDataLen = frame->frameInfo.size;
            convertParam.nPaddingX = frame->frameInfo.paddingX;
            convertParam.nPaddingY = frame->frameInfo.paddingY;
            convertParam.eBayerDemosaic = demosaicNearestNeighbor;
            convertParam.eDstPixelFormat = gvspPixelBGR8;
            convertParam.pDstBuf = pDstData;
            convertParam.nDstBufSize = frame->frameInfo.width * frame->frameInfo.height * 3;

            //if (IMV_OK != IMV_PixelConvert(devHandle, &convertParam)) {
            //    throw std::runtime_error("Pixel convert failed");
            //}

            result = cv::Mat(
                frame->frameInfo.height,
                frame->frameInfo.width,
                CV_8UC3,
                pDstData
            ).clone();  // 深拷贝转换后的数据
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Convert error: " << e.what() << std::endl;
        result.release();
    }

    // 清理临时缓冲区
    if (pDstData) {
        delete[] pDstData;
    }

    return result;
}

void ImageGrabCallBack(IMV_Frame* pFrame, void* pUser)
{
    if (!pFrame)
    {
        return;
    }

    auto camera = static_cast<LineScanCamera_HR*>(pUser);
    if (!camera)
    {
        return;
    }

    // 转换帧数据（无锁（包含深拷贝）
    cv::Mat img = convertFrameToMat(pFrame);
    //IMV_ReleaseFrame(camera->camHandle, pFrame);	//没有通过GetFrame获取帧。此处不需要。官方DEMO使用回调获取帧信息也无此调用。
    if (!camera->PushFrame(std::move(img)))
    {
        FILE_LOG_DEBUG("Camera[%d] frame queue is full, dropping frame", camera->GetCameraId());
    }

}

LineScanCamera_HR::LineScanCamera_HR()
{
    m_imgList.reserve(16);
}

LineScanCamera_HR::LineScanCamera_HR(int num)
{
    //m_number = num;
    m_imgList.reserve(16);
}

LineScanCamera_HR::LineScanCamera_HR(int num, IMV_DeviceInfo cameraInfo)
{
    //m_number = num;
    m_Key = cameraInfo.cameraKey;
    m_userId = cameraInfo.cameraName;
    FILE_LOG_INFO("m_index = %d , m_Key = %s, userid = %s", num, m_Key.c_str(), m_userId.c_str());

    m_imgList.reserve(16);
}

LineScanCamera_HR::LineScanCamera_HR(const LineScanCamera_HR& other)
{
    //m_number = other.m_number;
    m_Key = other.m_Key;
    m_userId = other.m_userId;
    m_imgList.reserve(16);
}

LineScanCamera_HR& LineScanCamera_HR::operator=(const LineScanCamera_HR& other)
{
    if (this == &other)
        return *this;

    //m_number = other.m_number;
    m_Key = other.m_Key;
    m_userId = other.m_userId;

    return *this;
}

LineScanCamera_HR::~LineScanCamera_HR()
{
    if (m_deviceInfo != nullptr)
    {
        delete m_deviceInfo;
        m_deviceInfo = nullptr;
    }
    std::cout << "LineScanCamera_HR::~LineScanCamera_HR()" << std::endl;
}

CameraInfoList LineScanCamera_HR::EnumDevices()
{
    IMV_DeviceList deviceList;
    CameraInfoList cameraList;

    int ret = IMV_EnumDevices(&deviceList, interfaceTypeGige | interfaceTypeUsb3);
    if (IMV_OK != ret)
    {
        printf("IMV_EnumDevices failed. ret:%d\r\n", ret);
        return cameraList;
    }

    printf("find device finished. device num:%d.\r\n", deviceList.nDevNum);

    if (deviceList.nDevNum < 1)
    {
        printf("no camera\n");
        return cameraList;
    }

    for (unsigned int i = 0; i < deviceList.nDevNum; ++i)
    {
        IMV_DeviceInfo* pDevInfo = NULL;
        pDevInfo = &deviceList.pDevInfo[i];
        CameraInfo cameraInfo;

        cameraInfo.serialNum = pDevInfo->serialNumber;

        if (pDevInfo->nCameraType == typeGigeCamera)
        {
            cameraInfo.IpAddr = pDevInfo->DeviceSpecificInfo.gigeDeviceInfo.ipAddress;
            cameraInfo.type = CameraType::eGige;
        }
        else if (pDevInfo->nCameraType == typeU3vCamera)
        {
            cameraInfo.type = CameraType::eUsb3;
        }
        else if (pDevInfo->nCameraType == typeCLCamera)
        {
            cameraInfo.type = CameraType::eCameraLink;
        }

        cameraInfo.deviceInfo_hr = pDevInfo;
        cameraList.emplace_back(cameraInfo);
    }

    displayDeviceInfo(deviceList);
    return cameraList;
}

bool LineScanCamera_HR::InitSDK()
{
    // 华睿相机SDK不需要显式初始化
    return true;
}

bool LineScanCamera_HR::FinalizeSDK()
{
    // 华睿相机SDK不需要显式反初始化
    return true;
}

bool LineScanCamera_HR::IsDeviceAccessable(unsigned int accessMode)
{
    // 华睿相机没有直接提供此功能，返回true
    return true;
}

bool LineScanCamera_HR::Open()
{
    int ret = IMV_OK;
    //FileLogPrintf("open:::: m_number = %d , m_Key = %s, userid = %s", m_number, m_Key.c_str(), m_userId.c_str());
    ret = IMV_CreateHandle(&camHandle, modeByCameraKey, (void*)m_Key.c_str());
    if (IMV_OK != ret)
    {
        printf("Create devHandle by CameraKey failed! Key is [%s], ErrorCode[%d]\n", m_Key.c_str(), ret);
        return false;
    }
    if (!camHandle)
    {
        return false;
    }

    ret = IMV_Open(camHandle);
    if (IMV_OK != ret)
    {
        printf("Open camera failed! cameraKey[%s], ErrorCode[%d]\n", m_Key.c_str(), ret);
        IMV_DestroyHandle(camHandle);
        return false;
    }

    return true;
}

bool LineScanCamera_HR::Close()
{
    int ret = IMV_OK;
    if (!camHandle)
    {
        return false;
    }

    ret = IMV_Close(camHandle);
    if (IMV_OK != ret)
    {
        printf("Close grabbing failed! EcameraKey[%s], ErrorCode[%d]\n", m_Key.c_str(), ret);
        return false;
    }

    ret = IMV_DestroyHandle(camHandle);
    if (IMV_OK != ret)
    {
        printf("Destroy device Handle failed! EcameraKey[%s], ErrorCode[%d]\n", m_Key.c_str(), ret);
        return false;
    }

    return true;
}

bool LineScanCamera_HR::IsDeviceConnect()
{
    if (!camHandle)
    {
        return false;
    }
    return IMV_IsOpen(camHandle);
}

bool LineScanCamera_HR::RegisterImageCallBack()
{
    if (!camHandle)
    {
        return false;
    }
    int nRet = IMV_AttachGrabbing(camHandle, ImageGrabCallBack, this);
    if (nRet != IMV_OK)
    {
        printf("Attach grabbing failed! cameraKey[%s], ErrorCode[%d]\n", m_Key.c_str(), nRet);
        return false;
    }
    return true;
}

bool LineScanCamera_HR::StartGrabbing()
{
    if (!camHandle)
    {
        return false;
    }
    printf("Start [%p] grabbing.\r\n", camHandle);
    IMV_ClearFrameBuffer(camHandle);
    IMV_ResetStatisticsInfo(camHandle);

    int nRet = IMV_StartGrabbing(camHandle);
    if (nRet != IMV_OK)
    {
        printf("Start grabbing failed! cameraKey[%s], ErrorCode[%d]\n", m_Key.c_str(), nRet);
        return false;
    }
    return true;
}

bool LineScanCamera_HR::StopGrabbing()
{
    if (!camHandle)
    {
        return false;
    }

    bool nRst = IMV_StopGrabbing(camHandle);
    if (nRst != IMV_OK)
    {
        return false;
    }
    IMV_ClearFrameBuffer(camHandle);
    IMV_ResetStatisticsInfo(camHandle);

    printf("Stop [%p] grabbing.\r\n", camHandle);
    return true;
}

bool LineScanCamera_HR::SetFrameBrustStartTrigger()
{
    // 华睿相机设置帧触发模式
    if (!camHandle)
    {
        return false;
    }

    int nRet = IMV_SetEnumFeatureSymbol(camHandle, "TriggerSelector", "FrameBurstStart");
    if (nRet != IMV_OK)
    {
        return false;
    }
    nRet = IMV_SetEnumFeatureSymbol(camHandle, "TriggerMode", "On");
    if (nRet != IMV_OK)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HR::SetLineStartTrigger()
{
    // 华睿相机设置行触发模式
    if (!camHandle)
    {
        return false;
    }

    int ret = IMV_SetEnumFeatureSymbol(camHandle, "TriggerSelector", "LineStart");
    if (ret != IMV_OK)
    {
        return false;
    }
    ret = IMV_SetEnumFeatureSymbol(camHandle, "TriggerMode", "On");
    if (ret != IMV_OK)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HR::SetTriggerSource(TriggerSource source)
{
    if (!camHandle)
    {
        return false;
    }

    const char* sourceStr = "";
    switch (source)
    {
    case TriggerSource::eLine0:
        sourceStr = "Line0";
        break;
    case TriggerSource::eLine1:
        sourceStr = "Line1";
        break;
    case TriggerSource::eLine2:
        sourceStr = "Line2";
        break;
    case TriggerSource::eLine3:
        sourceStr = "Line3";
        break;
    case TriggerSource::eCounter0:
        sourceStr = "Counter0";
        break;
    case TriggerSource::eSoftWare:
        sourceStr = "Software";
        break;
    case TriggerSource::eFrequencyConverter:
        sourceStr = "FrequencyConverter";
        break;
    default:
        return IMV_INVALID_PARAM;
    }
    int ret = IMV_SetEnumFeatureSymbol(camHandle, "TriggerSource", sourceStr);
    if (ret != IMV_OK)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HR::SetTriggerSwitch(bool mode)
{
    if (!camHandle)
    {
        return false;
    }

    int ret = IMV_SetEnumFeatureSymbol(camHandle, "TriggerMode", mode ? "On" : "Off");
    if (ret != IMV_OK)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HR::SetExposureTime(float& exp)
{
    if (!camHandle)
    {
        return false;
    }

    int ret = IMV_SetDoubleFeatureValue(camHandle, "ExposureTime", exp);
    if (ret != IMV_OK )
    {
        return false;
    }
    return true;
}

float LineScanCamera_HR::GetExposureTime()
{
    if (!camHandle)
    {
        return -1.0f;
    }

    double value = 0.0;
    int ret = IMV_GetDoubleFeatureValue(camHandle, "ExposureTime", &value);
    if (ret != IMV_OK)
    {
        return -1.0f;
    }

    return static_cast<float>(value);
}

bool LineScanCamera_HR::SetAacquisitionLineRateEnable(bool mode)
{
    if (!camHandle)
    {
        return false;
    }

    int nRet = IMV_SetBoolFeatureValue(camHandle, "AcquisitionLineRateEnable", mode);
    if (nRet != IMV_OK)
    {
        return false;
    }
    return true;
}

bool LineScanCamera_HR::GetAcquisitionLineRateEnable()
{
    if (!camHandle)
    {
        return false;
    }

    bool value = false;
    int nRet = IMV_GetBoolFeatureValue(camHandle, "AcquisitionLineRateEnable", &value);
    if (nRet != IMV_OK)
    {
        return false;
    }

    return value;
}

bool LineScanCamera_HR::SetAcquisitionLineRate(int& lineRate)
{
    if (!camHandle)
    {
        return false;
    }
    int nRet = IMV_SetIntFeatureValue(camHandle, "AcquisitionLineRate", lineRate);
    if (nRet != IMV_OK)
    {
        return false;
    }
    return true;
}

int LineScanCamera_HR::GetAcquisitionLineRate()
{
    if (!camHandle)
    {
        return -1;
    }

    int64_t value = 0;
    int ret = IMV_GetIntFeatureValue(camHandle, "AcquisitionLineRate", &value);
    if (IMV_OK != ret)
    {
        return -1;
    }

    return static_cast<int>(value);
}

bool LineScanCamera_HR::SetImageHeight(int height)
{
    if (!camHandle)
    {
        return IMV_INVALID_HANDLE;
    }

    int nRet = IMV_SetIntFeatureValue(camHandle, "Height", height);
    if (nRet != IMV_OK)
    {
        return false;
    }
    return true;
}

int LineScanCamera_HR::GetImageHeight()
{
    if (!camHandle)
    {
        return -1;
    }

    int64_t value = 0;
    int nRet = IMV_GetIntFeatureValue(camHandle, "Height", &value);
    if (nRet != IMV_OK)
    {
        return -1;
    }

    return static_cast<int>(value);
}

int LineScanCamera_HR::GetImageWidth()
{
    if (!camHandle)
    {
        return -1;
    }

    int64_t value = 0;
    int nRet = IMV_GetIntFeatureValue(camHandle, "Width", &value);
    if (nRet != IMV_OK)
    {
        return -1;
    }

    return static_cast<int>(value);
}

bool LineScanCamera_HR::SetImageWidth(int& width)
{
    if (!camHandle)
    {
        return false;
    }

    int nRet = IMV_SetIntFeatureValue(camHandle, "Width", width);
    if (nRet != IMV_OK)
    {
        return false;
    }
    return true;
}

void LineScanCamera_HR::StartSaveImageThread()
{
    m_running.store(true);
    m_saveThread = std::thread(&LineScanCamera_HR::SaveImageProcess, this);
}

void LineScanCamera_HR::StopSaveImageThread()
{
    m_running.store(false);
    m_saveCv.notify_one();
    if (m_saveThread.joinable())
        m_saveThread.join();
}

void LineScanCamera_HR::SaveImageProcess()
{

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

    ////判断总体文件夹是否存在,不存在则创建
    //auto savePath = (fs::current_path() / fs::path("Singal_Frame") / fs::path(m_dirName) / fs::path(std::to_string(m_number)));
    //if (!fs::exists(savePath))
    //{
    //    fs::create_directories(savePath);
    //}

    //int frameNum = 0;
    //std::vector<int> compression_params = { cv::IMWRITE_PNG_COMPRESSION, 9 };

    //while (m_running.load())
    //{
    //    std::unique_lock<std::mutex> lock(m_saveMutex);
    //    m_saveCv.wait_for(lock, std::chrono::milliseconds(1000), [&] {
    //        return m_running.load() || !m_saveQueue.empty();
    //        });

    //    if (!m_running.load() && m_saveQueue.empty())
    //    {
    //        break;
    //    }

    //    if (!m_saveQueue.empty())
    //    {
    //        auto image = m_saveQueue.front();
    //        std::string imagePath = (savePath / fs::path(std::to_string(frameNum))).string();
    //        cv::imwrite(imagePath + ".png", image, compression_params);
    //        m_saveQueue.pop();
    //        frameNum++;
    //    }
    //}
}

int LineScanCamera_HR::GetCameraId()
{
    return 0;
}

