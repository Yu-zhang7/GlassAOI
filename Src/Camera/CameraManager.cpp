#include <thread>
#include <chrono>
#include <algorithm>
#include "CameraManager.h"
#include "Log.hpp"
#include "Global.h"
#include "imageSaver.h"             //独立线程保存图片
#include "ResultBuffer.h"   //用于保存拼接后的批量图像缓存

// 辅助函数：截断字符串并添加省略号
static std::string truncateString(const std::string& str, size_t maxLength) {
    if (str.length() <= maxLength) {
        return str;
    }
    if (maxLength <= 3) {
        return str.substr(0, maxLength);
    }
    return str.substr(0, maxLength - 3) + "...";
}

int GetCameraId(std::string serialNum)
{
    // 删除serialNum的空字符，再进行匹配，更精准
    // 和全局变量进行比较，返回相机的id
    if (serialNum.find(Camera1) != std::string::npos)
    {
        return 0;
    }
    else if (serialNum.find(Camera2) != std::string::npos)
    {
        return 1;
    }
    else if (serialNum.find(Camera3) != std::string::npos)
    {
        return 2;
    }
    else if (serialNum.find(Camera4) != std::string::npos)
    {
        return 3;
    }
    else if (serialNum.find(Camera5) != std::string::npos)
    {
        return 4;
    }
    else if (serialNum.find(Camera6) != std::string::npos)
    {
        return 5;
    }
    else if (serialNum.find(Camera7) != std::string::npos)
    {
        return 6;
    }
    else if (Camera8 != "" && serialNum.find(Camera8) != std::string::npos)
    {
        return 7;
    }
    else if (Camera9 != "" && serialNum.find(Camera9) != std::string::npos)
    {
        return 8;
    }
    else
    {
        return -1;
    }
}


/**
 * @brief 高效拼接多个相机的前n帧图像
 * @param camera_vectors 各相机的图像vector集合
 * @param n 每个相机取前n帧（取最小可用值）
 * @param result 输出拼接结果
 * @return 实际使用的帧数（最小可用n）
 */
size_t efficientMultiCameraMerge(
    const std::vector<std::vector<cv::Mat>>& camera_vectors,
    size_t requested_n,
    cv::Mat& result
) {
    // 1. 确定实际可用的n（所有相机的最小size）
    size_t actual_n = requested_n;
    for (const auto& vec : camera_vectors) {
        if (vec.size() < actual_n) {
            actual_n = vec.size();
        }
        if (actual_n == 0) break;
    }

    if (actual_n == 0 || camera_vectors.empty()) {
        result.release();
        return 0;
    }

    // 2. 验证图像尺寸一致性
    const int ref_height = camera_vectors[0][0].rows;
    const int ref_width = camera_vectors[0][0].cols;
    const int ref_type = camera_vectors[0][0].type();

    for (const auto& vec : camera_vectors) {
        for (size_t i = 0; i < actual_n; i++) {
            if (vec[i].rows != ref_height ||
                vec[i].cols != ref_width ||
                vec[i].type() != ref_type) {
                std::cerr << u8"图像尺寸或类型不匹配!" << std::endl;
                result.release();
                return 0;
            }
        }
    }

    // 3. 预分配结果内存
    const int total_height = ref_height * actual_n;
    const int total_width = ref_width * camera_vectors.size();

    if (result.empty() ||
        result.rows != total_height ||
        result.cols != total_width ||
        result.type() != ref_type) {
        result.create(total_height, total_width, ref_type);
        if (result.empty()) {
            std::cerr << u8"内存分配失败!" << std::endl;
            return 0;
        }
    }

    // 4. 并行填充数据
#pragma omp parallel for
    for (int cam_idx = 0; cam_idx < static_cast<int>(camera_vectors.size()); cam_idx++) {
        const auto& cam_images = camera_vectors[cam_idx];
        const int x_offset = cam_idx * ref_width;

        for (size_t frame_idx = 0; frame_idx < actual_n; frame_idx++) {
            const int y_offset = frame_idx * ref_height;

            // 获取目标ROI
            cv::Mat roi = result(cv::Rect(x_offset, y_offset, ref_width, ref_height));

            // 复制数据
            cam_images[frame_idx].copyTo(roi);
        }
    }

    return actual_n;
}

CameraManager::CameraManager()
{
    // 默认使用海康相机
    m_cameraBrand = CameraBrand::eHikvision;
}

CameraManager::~CameraManager()
{
    if (!m_cameraList.empty())
    {
        this->StopGrabbing();
        this->Close();
        CameraManager::FinalizeSDK(m_cameraBrand);
    }

    if (!m_dataCache.empty())
    {
        m_dataCache.clear();
    }
    //FILE_LOG_INFO("[Cameras] ~CameraManager");
    //std::cout << "CameraManager::~CameraManager()" << std::endl;
}

bool CameraManager::InitializeCameras()
{
    FILE_LOG_INFO("[Cameras] Initializing cameras with brand: %d", static_cast<int>(m_cameraBrand));
    if (IS_READ_MODE == 0)
    {
        // 枚举设备
        auto devices = CameraFactory::EnumDevices(m_cameraBrand);

        if (devices.empty())
        {
            FILE_LOG_DEBUG("[Cameras] No devices found for brand: %d", static_cast<int>(m_cameraBrand));
            return false;
        }

        FILE_LOG_INFO("[Cameras] Found %d devices", devices.size());

        // 清空现有相机列表
        m_cameraList.clear();

        // 创建相机对象
        for (const auto& deviceInfo : devices)
        {
            int id = GetCameraId(deviceInfo.serialNum);
            if (id == -1)
            {
                FILE_LOG_INFO("[Cameras] Camera SN %s not found in configuration", deviceInfo.serialNum.c_str());
                continue;
            }

            // 使用工厂创建相机对象
            auto camera = CameraFactory::CreateCamera(m_cameraBrand, id, (void*)&deviceInfo);
            if (camera)
            {
                m_cameraList.push_back(camera);
                FILE_LOG_INFO("[Cameras] Created camera %d with SN: %s", id, deviceInfo.serialNum.c_str());
            }
            else
            {
                FILE_LOG_INFO("[Cameras] Failed to create camera %d with SN: %s", id, deviceInfo.serialNum.c_str());
            }
        }
    }
    else if (IS_READ_MODE == 1)
    {
        for (size_t i = 0; i < CameraCount; i++)
        {
            // 使用工厂创建相机对象
            auto camera = CameraFactory::CreateCamera(m_cameraBrand, i, nullptr);
            if (camera)
            {
                m_cameraList.push_back(camera);
                FILE_LOG_INFO("[Cameras_READ-MODE] Created camera %d");
            }
            else
            {
                FILE_LOG_INFO("[Cameras_READ-MODE] Failed to create camera %d ", i);
            }
        }
    }
    else
    {
        return true;
    }
    // 按相机ID排序
    std::sort(m_cameraList.begin(), m_cameraList.end(),
        [](const std::shared_ptr<LineScanCamera_Base>& a, const std::shared_ptr<LineScanCamera_Base>& b) {
            return a->GetCameraId() < b->GetCameraId();
        });

    FILE_LOG_INFO("[Cameras] Initialized %d Cameras Completed.", m_cameraList.size());
    return !m_cameraList.empty();
}

bool CameraManager::GetImage(cv::Mat& img)
{
    //if (!img.empty())
    //{
    //    img.release();
    //}

    //if (m_cameraList.empty())
    //{
    //    FileLogPrintf("[Cameras] GetImage Error: CameraList is empty! GetImage Failed!");
    //    return false;
    //}

    //// 计算最小触发次数
    //int min = 0;
    //for (auto& camera : m_cameraList)
    //{
    //    if (min == 0)
    //    {
    //        min = camera->GetTriggerNum();
    //    }
    //    else
    //    {
    //        min = std::min(min, camera->GetTriggerNum());
    //    }
    //}

    //FileLogPrintf("[Cameras] GetImage: Min trigger number: %d", min);

    //std::vector<cv::Mat> imgVec(m_cameraList.size());

    //for (auto& camera : m_cameraList)
    //{
    //    std::vector<cv::Mat> tempVec;
    //    camera->GetMatListRange(tempVec, min);

    //    FileLogPrintf("[Cameras] GetImage: Camera[%d] has %d images", camera->GetCameraId(), tempVec.size());

    //    if (tempVec.empty())
    //    {
    //        continue;
    //    }

    //    cv::Mat temp;
    //    cv::vconcat(tempVec, temp);

    //    if (temp.empty())
    //    {
    //        FileLogPrintf("[Cameras] GetImage Error: Camera[%d] concatenated image is empty!", camera->GetCameraId());
    //        return false;
    //    }

    //    imgVec[camera->GetCameraId()] = temp.clone();
    //}

    //cv::hconcat(imgVec, m_totalImg);

    //if (m_totalImg.empty())
    //{
    //    FileLogPrintf("[Cameras] GetImage Error: Final concatenated image is empty!");
    //    return false;
    //}

    //img = m_totalImg.clone();
    //m_totalImg.release();

    return true;
}

void CameraManager::NotifyGrabFrameData(int num)
{
    if (num >= m_cameraList.size())
    {
        FILE_LOG_INFO("Num is too Large!");
        return;
    }

    auto& camera = m_cameraList[num];

    std::unique_lock<std::mutex> lock(m_mutex);
    cv::Mat img;
    camera->GetMat(img);
    m_dataCache.emplace(std::make_pair(num, img));
    lock.unlock();
    m_cv.notify_one();
}

CameraInfoList CameraManager::EnumDeivces(CameraBrand brand)
{
    return CameraFactory::EnumDevices(brand);
}

bool CameraManager::InitSDK(CameraBrand brand)
{
    return CameraFactory::InitSDK(brand);
}

bool CameraManager::FinalizeSDK(CameraBrand brand)
{
    return CameraFactory::FinalizeSDK(brand);
}

bool CameraManager::Open()
{
    FILE_LOG_INFO("CameraList Open Begin.");
    if (m_cameraList.empty() && IS_READ_MODE == 0)
    {
        FILE_LOG_INFO("CameraList is empty! Open Failed!");
        return false;
    }

    bool bRst = true;
    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        if (IS_READ_MODE == 0)
        {
            if (m_cameraList[i])
            {
                bRst = m_cameraList[i]->Open();
                if (!bRst) // 假设0表示成功
                {
                    FILE_LOG_INFO("Open camera[%d] Failed! CameraId = %d",
                        i, m_cameraList[i]->GetCameraId());
                    return false;
                }
                else
                {
                    FILE_LOG_INFO("camera[%d] exposure time = %f",
                        m_cameraList[i]->GetCameraId(), m_cameraList[i]->GetExposureTime());
                }
            }
        }
        //if (m_cameraList[i] != nullptr)
        //{
        //    m_cameraList[i]->SetmaxQueueSize(1000); //设置相机采图的缓存队列大小。仅用于采图，不包括后续合并帧。
        //}
    }

    FILE_LOG_INFO("CameraList Open Completed.");

    if (IS_READ_MODE == 0)
    {
        RegisterImageCallBack();
    }
    return true;
}

bool CameraManager::Close()
{
    FILE_LOG_INFO("[Cameras] Close Begin.");
    if (m_cameraList.empty())
    {
        FILE_LOG_INFO("[Cameras] Close Error: CameraList is empty! Close Failed!");
        return false;
    }

    bool bRst = true;
    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        m_cameraList[i]->ClearQueue();
        if (!m_cameraList[i]->Close())
        {
            FILE_LOG_INFO("[Cameras] Close Error: Close camera[%d] Failed!", i);
            bRst = false;
        }
    }

    FILE_LOG_INFO("[Cameras] Close Completed.");
    return bRst;
}

bool CameraManager::StartGrabbing()
{
    FILE_LOG_INFO("[Cameras] StartGrabbing Begin.");
    if (m_cameraList.empty() && IS_READ_MODE == 0)
    {
        FILE_LOG_INFO("[Cameras] StartGrabbing Error: cameraList is empty! Return.");
        return false;
    }

    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        if (!m_cameraList[i]->StartGrabbing()) // 假设0表示成功
        {
            FILE_LOG_INFO("[Cameras] StartGrabbing Error: camera[%d] StartGrabbing Failed! Return.", i);
            return false;
        }
    }

    FILE_LOG_INFO("[Cameras] StartGrabbing Completed.");
    return true;
}

bool CameraManager::StopGrabbing()
{
    FILE_LOG_INFO("[Cameras] StopGrabbing Begin.");
    if (m_cameraList.empty() && IS_READ_MODE == 0)
    {
        FILE_LOG_INFO("[Cameras] StopGrabbing Error: cameraList is empty! Return.");
        return false;
    }

    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        if (!m_cameraList[i]->StopGrabbing())
        {
            FILE_LOG_INFO("[Cameras] StopGrabbing Error: Camera[%d] StopGrabbing Failed!", i);
            return false;
        }
    }

    FILE_LOG_INFO("[Cameras] StopGrabbing Completed.");
    return true;
}

bool CameraManager::StartProcessImages()
{
    FILE_LOG_INFO("[Cameras] StartProcessImages Begin.");
    if (m_cameraList.empty() && IS_READ_MODE == 0)
    {
        FILE_LOG_INFO("[Cameras] StartProcessImages Error: cameraList is empty! Return.");
        return false;
    }
    ////清理缓存    (任何硬件触发已经启动，然后清理缓存的操作，都可能造成开头帧丢失)
    //for (int i = 0; i < GetCameraNum(); ++i)
    //{
    //    auto camera = GetCamera(i);
    //    camera->DataRelease();
    //}
    FILE_LOG_INFO("[Process]StartProcess: Camers data release.");


    if (IS_READ_MODE == 1)
    {
        for (size_t i = 0; i < CameraCount; i++)
        {
            std::thread  readImagesThread(&CameraManager::ReadFrameImagesFromFile_ThreadFunction, this, i);
            readImagesThread.detach();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    m_ImageProcessState = false;    //重置图像处理状态。在得到一组批次图像后，再置为true。用于给算法线程传信号
    m_ImageProcessRunning = true;   //启动图像处理循环的标志
    m_ExitPromise = std::promise<void>{}; // 重置promise
    m_ExitFuture = m_ExitPromise.get_future();
    //m_ImageProcessingThread = std::thread(&CameraManager::FrameStitchingThreadFunction, this);
    m_ImageProcessingThread = std::thread(&CameraManager::FrameStitchingThreadFunction_new, this);
    m_ImageProcessingThread.detach();

    FILE_LOG_INFO("[Cameras] StartProcessImages Completed. GetBatchImageThread is running.");
    return true;
}

bool CameraManager::StopProcessImages()
{
    FILE_LOG_INFO("[Cameras] StopProcessImages Begin.");
    if (m_cameraList.empty() && IS_READ_MODE == 0)
    {
        FILE_LOG_INFO("[Cameras] StopProcessImages Error: cameraList is empty! Return.");
        return false;
    }

    if (!m_ImageProcessRunning)
    {
        return true;
    }

    m_ImageProcessRunning = false;

    FILE_LOG_INFO("[Cameras] StopProcessImages Completed.");
    return true;
}

//读图模式1下，同时获取各个相机的读图数据
void CameraManager::ReadFrameImagesFromFile_ThreadFunction(int camId)
{
    while (true)
    {
        if (m_ImageProcessRunning)  //用于模拟多个相机同时开始触发获取图像
        {
            break;
        }
    }
    std::string path = "D:/Test/TestImage2";
    for (int i = 0; i < m_cameraList.size(); i++)
    {
        m_cameraList[camId]->ClearQueue();
    }
    m_cameraList[camId]->ReadFrameImageFromFile(camId, path);
}

bool CameraManager::SetFrameBrustStartTrigger()
{
    FILE_LOG_INFO("CameraList SetFrameBrustStartTrigger Begin.");

    if (m_cameraList.empty())
    {
        FILE_LOG_INFO("cameraList is empty! SetFrameBrustStartTrigger Failed!");
        return false;
    }

    bool bRst = true;
    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        int nRet = m_cameraList[i]->SetFrameBrustStartTrigger();
        if (nRet != 0) // 假设0表示成功
        {
            FILE_LOG_INFO("SetFrameBrustStartTrigger camera[%d] Failed! ErrorCode = %d", i, nRet);
            bRst = false;
        }
    }
    if (bRst)
    {
        FILE_LOG_INFO("CameraList SetFrameBrustStartTrigger Completed.");
    }
    
    return bRst;
}

bool CameraManager::SetLineStartTrigger()
{
    if (m_cameraList.empty())
    {
        FILE_LOG_INFO("cameraList is empty! SetLineStartTrigger Failed!");
        return false;
    }

    bool bRst = true;
    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        int nRet = m_cameraList[i]->SetLineStartTrigger();
        if (nRet != 0) // 假设0表示成功
        {
            FILE_LOG_INFO("SetLineStartTrigger camera[%d] Failed! ErrorCode = %d", i, nRet);
            bRst = false;
        }
    }
    if (bRst)
    {
        FILE_LOG_INFO("CameraList SetFrameBrustStartTrigger Completed.");
    }
    
    return bRst;
}

bool CameraManager::SetTriggerSource(TriggerSource source)
{
    FILE_LOG_INFO("CameraList SetTriggerSource Begin.");
    if (m_cameraList.empty())
    {
        FILE_LOG_INFO("cameraList is empty! SetTriggerSource Failed!");
        return false;
    }

    bool bRst = true;
    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        bool nRet = m_cameraList[i]->SetTriggerSource(source);
        if (!nRet) // 假设0表示成功
        {
            FILE_LOG_INFO("SetTriggerSource camera[%d] Failed! ErrorCode = %d", i, nRet);
            bRst = false;
        }
    }
    if (!bRst)
    {
        FILE_LOG_INFO("CameraList SetTriggerSource Completed.");
    }
    return bRst;
}

bool CameraManager::SetTriggerSwitch(bool mode)
{
    FILE_LOG_INFO("CameraList SetTriggerSwitch Begin. Value = %s", (mode ? "true" : "false"));
    if (m_cameraList.empty())
    {
        FILE_LOG_INFO("cameraList is empty! SetTriggerSwitch Failed!");
        return false;
    }

    bool bRst = true;
    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        bool nRet = m_cameraList[i]->SetTriggerSwitch(mode);
        if (!nRet) // 假设0表示成功
        {
            FILE_LOG_INFO("SetTriggerSwitch camera[%d] Failed! ErrorCode = %d", i, nRet);
            bRst = false;
        }
    }
    if (bRst)
    {
        FILE_LOG_INFO("CameraList SetTriggerSwitch Completed.");
    }

    return bRst;
}

bool CameraManager::SetExposureTime(float exp)
{
    FILE_LOG_INFO("CameraList SetExposureTime Begin. Value = %f", exp);
    if (m_cameraList.empty())
    {
        FILE_LOG_INFO("cameraList is empty! SetExposureTime Failed!");
        return false;
    }

    bool bRst = true;
    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        bool nRet = m_cameraList[i]->SetExposureTime(exp);
        if (!nRet) // 假设0表示成功
        {
            FILE_LOG_INFO("SetExposureTime camera[%d] Failed! ErrorCode = %d", i, nRet);
            bRst = false;
        }
    }
    if (bRst)
    {
        FILE_LOG_INFO("CameraList SetExposureTime Completed.");
    }
    
    return bRst;
}

bool CameraManager::SetAacquisitionLineRateEnable(bool mode)
{
    FILE_LOG_INFO("CameraList SetAacquisitionLineRateEnable Begin. Value =%s", (mode ? "true" : "false"));
    if (m_cameraList.empty())
    {
        FILE_LOG_INFO("cameraList is empty! SetAacquisitionLineRateEnable Failed!");
        return false;
    }

    bool bRst = true;
    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        bool nRet = m_cameraList[i]->SetAacquisitionLineRateEnable(mode);
        if (!nRet) // 假设0表示成功
        {
            FILE_LOG_INFO("SetAacquisitionLineRateEnable camera[%d] Failed! ErrorCode = %d", i, nRet);
            bRst = false;
        }
    }
    if (bRst)
    {
        FILE_LOG_INFO("CameraList SetAacquisitionLineRateEnable Completed.");
    }
    
    return bRst;
}

bool CameraManager::SetAcquisitionLineRate(int lineRate)
{
    FILE_LOG_INFO("CameraList SetAcquisitionLineRate Begin. Value = %d", lineRate);
    if (m_cameraList.empty())
    {
        FILE_LOG_INFO("cameraList is empty! SetAcquisitionLineRate Failed!");
        return false;
    }

    bool bRst = true;
    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        bool nRet = m_cameraList[i]->SetAcquisitionLineRate(lineRate);
        if (!nRet) // 假设0表示成功
        {
            FILE_LOG_INFO("SetAcquisitionLineRate camera[%d] Failed! ErrorCode = %d", i, nRet);
            bRst = false;
        }
    }
    if (bRst)
    {
        FILE_LOG_INFO("CameraList SetAcquisitionLineRate Completed.");
    }
    return bRst;
}

bool CameraManager::ImageHeight(int height)
{
    FILE_LOG_INFO("CameraList SetImageHeight Begin. Value = %d", height);
    if (m_cameraList.empty())
    {
        FILE_LOG_INFO("CameraList is empty! SetImageHeight Failed!");
        return false;
    }

    bool bRst = true;
    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        bool bRet = m_cameraList[i]->SetImageHeight(height);
        if (!bRet) // 假设0表示成功
        {
            FILE_LOG_INFO("SetImageHeight camera[%d] Failed! ", i);
            bRst = false;
        }
    }
    if (bRst)
    {
        FILE_LOG_INFO("CameraList SetImageHeight Completed.");
    }
    return bRst;
}

bool CameraManager::SetImageWidth(int width)
{
    FILE_LOG_INFO("CameraList SetImageWidth Begin. Value = %d", width);
    if (m_cameraList.empty())
    {
        FILE_LOG_INFO("CameraList is empty! SetImageWidth Failed!");
        return false;
    }

    bool bRst = true;
    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        bool nRet = m_cameraList[i]->SetImageWidth(width);
        if (!nRet)
        {
            FILE_LOG_INFO("SetImageWidth camera[%d] Failed!", i);
            bRst = false;
        }
    }
    if (bRst)
    {
        FILE_LOG_INFO("CameraList SetImageWidth Completed.");
    }
    
    return bRst;
}

bool CameraManager::RegisterImageCallBack()
{
    FILE_LOG_INFO("RegisterImageCallBack begin");
    if (m_cameraList.empty())
    {
        FILE_LOG_INFO("cameraList is empty! RegisterImageCallBack Failed!");
        return false;
    }

    bool bRst = true;
    int failedNum = 0;
    for (int i = 0; i < m_cameraList.size(); ++i)
    {
        if (!m_cameraList[i]->RegisterImageCallBack())
        {
            FILE_LOG_INFO("RegisterImageCallBack camera[%d] Failed!", i);
            bRst = false;
            failedNum++;
        }
    }

    if (bRst)
    {
        FILE_LOG_INFO("RegisterImageCallBack success.");
    }
    else
    {
        FILE_LOG_INFO("RegisterImageCallBack Completed.Register failed number = %d", failedNum);
    }

    return bRst;
}
//
//void CameraManager::StitchFrameFunc(bool* run)
//{
//    if (m_dataCache.empty())
//    {
//        return;
//    }
//
//    std::unique_lock<std::mutex> lock(m_mutex);
//    m_cv.wait(lock, [=]() -> bool {
//        return (m_dataCache.size() == m_cameraList.size()) || !(*run);
//        });
//
//    if (!(*run))
//    {
//        FileLogPrintf("Thread exit!");
//        return;
//    }
//
//    FileLogPrintf("m_dataCache.size() = %d", m_dataCache.size());
//
//    // 获取每个相机的图像，并使用cv::Mat进行水平拼接
//    cv::Mat line;
//    for (auto item : m_dataCache)
//    {
//        if (line.empty())
//        {
//            line = item.second.clone();
//        }
//        else
//        {
//            cv::hconcat(line, item.second, line);
//        }
//    }
//
//    if (m_totalImg.empty())
//    {
//        m_totalImg = line.clone();
//    }
//    else
//    {
//        cv::vconcat(m_totalImg, line, m_totalImg);
//    }
//
//    m_dataCache.clear();
//}

void CameraManager::SetStopGrabbingCallback(std::function<void()> callback)
{
    m_stopGrabbingCallback = callback;
}

void CameraManager::FrameStitchingThreadFunction()
{
    //REGISTER_THREAD_NAME("FrameStitchingThreadFunction");
    
    FILE_LOG_INFO("[Cameras] FrameStitching: Begin.");
    ResultBuffer& buffer = ResultBuffer::GetInstance();
    //设置状态为BatchImage图像处理中。用于向Alogrithm模块发送信号
   /********* 使用RAII确保promise被设置 *********/
    struct PromiseSetter {
        std::promise<void>& promise;
        ~PromiseSetter() {
            try {
                promise.set_value();
            }
            catch (...) {
                // 忽略set_value可能抛出的异常（例如promise已被设置）
            }
        }
    } setter{ m_ExitPromise };
    /********************************************/


    int camCount = m_cameraList.size();
    std::vector<cv::Mat> frames(camCount);
    std::vector<std::vector<cv::Mat>> all_camera_images(camCount, std::vector<cv::Mat>(1));

    // 使用非阻塞方式检查所有相机是否有帧可用
    auto allCamerasHaveFrames = [this, camCount]() {
        for (size_t i = 0; i < camCount; ++i) {
            if (m_cameraList[i]->IsQueueEmpty()) {
                return false;
            }
        }
        return true;
        };

    int index = 0;
    while (m_ImageProcessRunning)
    {
        

        // 等待所有相机都有帧可用
        if (!allCamerasHaveFrames()) {
            //FileLogPrintf("[Cameras] FrameStitching: LOOP[%d] Not all cameras have frames. Waiting...", index);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP[%d] Begin.", index);
        all_camera_images.assign(camCount, std::vector<cv::Mat>(1));
        // 从每个相机获取一帧
        bool allFramesAvailable = true;
        std::string emptyFrame_camId;
        for (size_t i = 0; i < camCount; ++i)
        {
            // 使用基类的公共方法获取图像
            cv::Mat frame;
            if (!m_cameraList[i]->PopFrame(frame, 30))
            {
                allFramesAvailable = false;
                emptyFrame_camId += std::to_string(m_cameraList[i]->GetCameraId())+ ", ";
            }
            else
            {
                all_camera_images[i][0] = frame;
            }
        }
        cv::Mat stitchedImage;
        if (allFramesAvailable)
        {
            FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP[%d] efficientMultiCameraMerge Begin.", index);
            //size_t actual_used = efficientMultiCameraMerge(all_camera_images, 1, stitchedImage);
                       
            std::vector<cv::Mat> stitched_images;
            for (int i = 0; i < all_camera_images.size(); i++) {
                stitched_images.push_back(all_camera_images[i][0]);
            }
            cv::hconcat(stitched_images, stitchedImage);

            
            // 将结果放入输出缓冲区
            if (stitchedImage.empty())
            {
                FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP[%d] efficientMultiCameraMerge failed.CONTINUE.", index);
                index++;
                continue;
            }
            buffer.PushFrame(std::move(stitchedImage));
            //buffer.PushFrame(stitchedImage.clone());
            stitchedImage.release();
            FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP[%d] efficientMultiCameraMerge Completed. BufferFrameCount=%d", index, buffer.GetFrameCount());
        }
        else {
            // 帧不完整，等待下一轮
            FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP[%d] Waiting for Next Cameras PopFrame . Camera that needs to wait for frames: ", index, emptyFrame_camId);
            std::this_thread::yield();
            continue;
        }
        FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP[%d] Completed.", index);
        index++;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    FILE_LOG_INFO("[Cameras] FrameStitching: Loop_1st Completed.");

    //获得剩余最小帧数
    int lastFrameCount = 100;
    for (size_t i = 0; i < camCount; i++)
    {
        lastFrameCount = std::min(lastFrameCount, (int)m_cameraList[i]->GetQueueSize());
    }
    FILE_LOG_INFO("[Cameras] FrameStitching: Last frames min count = %d", lastFrameCount);
    //按照最小帧数进行剩余帧的拼接
    for (size_t i = 0; i < lastFrameCount; ++i)
    {
        // 从每个相机获取一帧
        FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP_LAST[%d] Begin.", index);
        bool allFramesAvailable = true;
        for (size_t i = 0; i < camCount; ++i)
        {
            // 使用基类的公共方法获取图像
            if (!m_cameraList[i]->PopFrame(all_camera_images[i][0], 500)) // 10ms超时
            {
                allFramesAvailable = false;
                break;
            }
        }
        cv::Mat stitchedImage;
        if (allFramesAvailable)
        {
            size_t actual_used = efficientMultiCameraMerge(all_camera_images, 1, stitchedImage);
            // 将结果放入输出缓冲区
            if (stitchedImage.empty())
            {
                FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP_LAST[%d] efficientMultiCameraMerge failed.stitchedImage is empty. CONTINUE.", index);
                index++;
                continue;
            }
            buffer.PushFrame(std::move(stitchedImage));
            //buffer.PushFrame(stitchedImage.clone());
            stitchedImage.release();
            FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP_LAST[%d] efficientMultiCameraMerge Completed. BufferFrameCount=%d", index, buffer.GetFrameCount());

        }
        else {
            // 帧不完整，等待下一轮
            FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP_LAST[%d] PopFrame failed, Waiting for Next Cameras PopFrame . ", index);
            std::this_thread::yield();
            continue;
        }
        FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP_LAST[%d] Completed.", index);
        index++;
    }
    FILE_LOG_INFO("[Cameras] FrameStitching: Loop_2nd Completed.");

    //清理残留废帧
    for (size_t i = 0; i < camCount; ++i)
    {
        // 清理残留废帧
        m_cameraList[i]->ClearQueue();
    }
    FILE_LOG_INFO("[Cameras] FrameStitching: ClearQueue Completed.");
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    FILE_LOG_INFO("[Cameras] FrameStitching: End. ");
}

void CameraManager::FrameStitchingThreadFunction_new()
{
    //REGISTER_THREAD_NAME("FrameStitchingThreadFunction");

    FILE_LOG_INFO("[Cameras] FrameStitching: Begin.");
    ResultBuffer& buffer = ResultBuffer::GetInstance();
    //设置状态为BatchImage图像处理中。用于向Alogrithm模块发送信号
   /********* 使用RAII确保promise被设置 *********/
    struct PromiseSetter {
        std::promise<void>& promise;
        ~PromiseSetter() {
            try {
                promise.set_value();
            }
            catch (...) {
                // 忽略set_value可能抛出的异常（例如promise已被设置）
            }
        }
    } setter{ m_ExitPromise };
    /********************************************/


    int camCount = m_cameraList.size();
    std::vector<cv::Mat> frames(camCount);
    std::vector<std::vector<cv::Mat>> all_camera_images(camCount, std::vector<cv::Mat>(1));

    // 使用非阻塞方式检查所有相机是否有帧可用
    auto allCamerasHaveFrames = [this, camCount]() {
        for (size_t i = 0; i < camCount; ++i) {
            if (m_cameraList[i]->IsQueueEmpty()) {
                return false;
            }
        }
        return true;
        };

    buffer.PushFrame(std::move(cv::Mat(1, 1, CV_8UC1)));    //推送开始帧到算法队列。

    int index = 0;
    while (true)
    {
		if (!m_ImageProcessRunning && !allCamerasHaveFrames())  //收到停止信号且所有相机无帧可用时，退出循环
        {
            break;
        }

        // 等待所有相机都有帧可用
        if (!allCamerasHaveFrames()) {
            //FileLogPrintf("[Cameras] FrameStitching: LOOP[%d] Not all cameras have frames. Waiting...", index);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP[%d] Begin.", index);
        all_camera_images.assign(camCount, std::vector<cv::Mat>(1));
        // 从每个相机获取一帧
        bool allFramesAvailable = true;
        std::string emptyFrame_camId;
        for (size_t i = 0; i < camCount; ++i)
        {
            // 使用基类的公共方法获取图像
            cv::Mat frame;
            if (!m_cameraList[i]->PopFrame(frame, 30))
            {
                allFramesAvailable = false;
                emptyFrame_camId += std::to_string(m_cameraList[i]->GetCameraId()) + ", ";
            }
            else
            {
                all_camera_images[i][0] = frame;
            }
        }
        cv::Mat stitchedImage;
        if (allFramesAvailable)
        {
            FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP[%d] efficientMultiCameraMerge Begin.", index);
            //size_t actual_used = efficientMultiCameraMerge(all_camera_images, 1, stitchedImage);

            std::vector<cv::Mat> stitched_images;
            for (int i = 0; i < all_camera_images.size(); i++) {
                stitched_images.push_back(all_camera_images[i][0]);
            }
            cv::hconcat(stitched_images, stitchedImage);


            // 将结果放入输出缓冲区
            if (stitchedImage.empty())
            {
                FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP[%d] efficientMultiCameraMerge failed.CONTINUE.", index);
                index++;
                continue;
            }
            buffer.PushFrame(std::move(stitchedImage));
            //buffer.PushFrame(stitchedImage.clone());
            stitchedImage.release();
            FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP[%d] efficientMultiCameraMerge Completed. BufferFrameCount=%d", index, buffer.GetFrameCount());
        }
        else {
            // 帧不完整，等待下一轮
            FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP[%d] Waiting for Next Cameras PopFrame . Camera that needs to wait for frames: ", index, emptyFrame_camId);
            std::this_thread::yield();
            continue;
        }
        FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP[%d] Completed.", index);
        index++;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    FILE_LOG_INFO("[Cameras] FrameStitching: Loop_1st Completed.");

    //获得剩余最小帧数
    int lastFrameCount = 100;
    for (size_t i = 0; i < camCount; i++)
    {
        lastFrameCount = std::min(lastFrameCount, (int)m_cameraList[i]->GetQueueSize());
    }
    FILE_LOG_INFO("[Cameras] FrameStitching: Last frames min count = %d", lastFrameCount);
    //按照最小帧数进行剩余帧的拼接
    for (size_t i = 0; i < lastFrameCount; ++i)
    {
        // 从每个相机获取一帧
        FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP_LAST[%d] Begin.", index);
        bool allFramesAvailable = true;
        for (size_t i = 0; i < camCount; ++i)
        {
            // 使用基类的公共方法获取图像
            if (!m_cameraList[i]->PopFrame(all_camera_images[i][0], 500)) // 10ms超时
            {
                allFramesAvailable = false;
                break;
            }
        }
        cv::Mat stitchedImage;
        if (allFramesAvailable)
        {
            size_t actual_used = efficientMultiCameraMerge(all_camera_images, 1, stitchedImage);
            // 将结果放入输出缓冲区
            if (stitchedImage.empty())
            {
                FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP_LAST[%d] efficientMultiCameraMerge failed.stitchedImage is empty. CONTINUE.", index);
                index++;
                continue;
            }
            buffer.PushFrame(std::move(stitchedImage));
            //buffer.PushFrame(stitchedImage.clone());
            stitchedImage.release();
            FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP_LAST[%d] efficientMultiCameraMerge Completed. BufferFrameCount=%d", index, buffer.GetFrameCount());

        }
        else {
            // 帧不完整，等待下一轮
            FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP_LAST[%d] PopFrame failed, Waiting for Next Cameras PopFrame . ", index);
            std::this_thread::yield();
            continue;
        }
        FILE_LOG_DEBUG("[Cameras] FrameStitching: LOOP_LAST[%d] Completed.", index);
        index++;
    }
    FILE_LOG_INFO("[Cameras] FrameStitching: Loop_2nd Completed.");

    //清理残留废帧
    for (size_t i = 0; i < camCount; ++i)
    {
        // 清理残留废帧
        m_cameraList[i]->ClearQueue();
    }
    buffer.PushFrame(std::move(cv::Mat(2, 2, CV_8UC1)));    //推送结束帧到算法队列。
    FILE_LOG_INFO("[Cameras] FrameStitching: ClearQueue Completed.");
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    FILE_LOG_INFO("[Cameras] FrameStitching: End. ");
}

void CameraManager::ClearBuffer()
{
    //清理每个相机的图像缓存
    for (size_t i = 0; i < m_cameraList.size(); i++)
    {
        m_cameraList[i]->ClearQueue();
    }
    //清理拼接图像缓存
    ResultBuffer::GetInstance().ClearQueue();
}

void CameraManager::PushGlassInOutStatus(bool status)
{
	cv::Mat inMat(1, 1, CV_8UC1);
	cv::Mat outMat(2, 2, CV_8UC1);
    for (size_t i = 0; i < m_cameraList.size(); i++)
    {
        if (status)
        {
            m_cameraList[i]->PushFrame(inMat.clone());
        }
        else
        {
            m_cameraList[i]->PushFrame(outMat.clone());
        }
    }
}