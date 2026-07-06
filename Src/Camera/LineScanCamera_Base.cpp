// LineScanCamera_Base.cpp
#include "LineScanCamera_Base.h"
#include <iostream>

LineScanCamera_Base::~LineScanCamera_Base()
{
    // 基类析构函数
}

bool LineScanCamera_Base::PushFrame(cv::Mat&& frame)
{
    std::unique_lock<std::mutex> lock(queueMutex);

    // 检查队列是否已满
    if (frameQueue.size() >= maxQueueSize) {
        droppedFrames.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    // 使用std::move高效转移图像数据
    frameQueue.push(std::move(frame));
    frameCounter.fetch_add(1, std::memory_order_relaxed);

    // 通知等待的消费者
    queueCV.notify_one();
    return true;
}

bool LineScanCamera_Base::PopFrame(cv::Mat& frame, int timeout_ms)
{
    if (timeout_ms == 0)
    {
        // 非阻塞模式
        return TryPopFrame(frame);
    }

    if (timeout_ms < 0)
    {
        // 无限等待模式
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCV.wait(lock, [this]() { return !frameQueue.empty(); });

        if (frameQueue.empty())
        {
            return false;
        }
        frame = std::move(frameQueue.front());
        frameQueue.pop();
        return true;
    }

    // 带超时的等待模式
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        auto status = queueCV.wait_for(lock, std::chrono::milliseconds(timeout_ms),
            [this]() { return !frameQueue.empty(); });
        if (!status)
        {
            return false;
        }

        if (frameQueue.empty())
        {
            return false;
        }
        frame = std::move(frameQueue.front());
        //frame = frameQueue.front().clone();
        frameQueue.pop();
    }
    //std::cout << u8"拿图完成：" << std::endl;;
    return true;
}

bool LineScanCamera_Base::TryPopFrame(cv::Mat& frame)
{
    std::unique_lock<std::mutex> lock(queueMutex);
    if (frameQueue.empty()) {
        return false;
    }

    frame = std::move(frameQueue.front());
    frameQueue.pop();
    return true;
}

void LineScanCamera_Base::SetmaxQueueSize(int maxSize)
{
    maxQueueSize = maxSize;
}

size_t LineScanCamera_Base::GetQueueSize() const
{
    std::unique_lock<std::mutex> lock(queueMutex);
    return frameQueue.size();
}

bool LineScanCamera_Base::IsQueueEmpty() const
{
    std::unique_lock<std::mutex> lock(queueMutex);
    return frameQueue.empty();
}

bool LineScanCamera_Base::IsQueueFull() const
{
    std::unique_lock<std::mutex> lock(queueMutex);
    return frameQueue.size() >= maxQueueSize;
}

void LineScanCamera_Base::ClearQueue()
{
    std::unique_lock<std::mutex> lock(queueMutex);
    while (!frameQueue.empty()) {
        frameQueue.pop();
    }
    droppedFrames = 0;
    frameCounter = 0;
}

int LineScanCamera_Base::GetDroppedFrames() const
{
    return droppedFrames.load(std::memory_order_relaxed);
}

int LineScanCamera_Base::GetFrameCount() const
{
    return frameCounter.load(std::memory_order_relaxed);
}

bool LineScanCamera_Base::WaitForFrame(int timeout_ms)
{
    if (timeout_ms < 0) {
        // 无限等待
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCV.wait(lock, [this]() { return !frameQueue.empty(); });
        return true;
    }

    if (timeout_ms == 0) {
        // 立即返回
        return !IsQueueEmpty();
    }

    // 带超时等待
    std::unique_lock<std::mutex> lock(queueMutex);
    return queueCV.wait_for(lock, std::chrono::milliseconds(timeout_ms),
        [this]() { return !frameQueue.empty(); });
}

void LineScanCamera_Base::GetMat(cv::Mat& img)
{
    if (!TryPopFrame(img)) // 非阻塞模式
    {
        img.release();
    }
}

void LineScanCamera_Base::GetMatList(std::vector<cv::Mat>& imgList)
{
    imgList.clear();

    // 使用基类的公共方法获取所有可用图像
    cv::Mat frame;
    while (TryPopFrame(frame)) // 非阻塞模式
    {
        imgList.push_back(std::move(frame));
    }
}

void LineScanCamera_Base::AddImageToQueue(cv::Mat& img)
{
    PushFrame(std::move(img));
}

// 静态方法的默认实现（可被派生类覆盖）
CameraInfoList LineScanCamera_Base::EnumDevices()
{
    return CameraInfoList();
}

//
//// 静态方法的默认实现（可被派生类覆盖）
//CameraInfoList LineScanCamera_Base::EnumDevices()
//{
//    return CameraInfoList();
//}

bool LineScanCamera_Base::InitSDK()
{
    return 0;
}

bool LineScanCamera_Base::FinalizeSDK()
{
    return 0;
}


void LineScanCamera_Base::ReadFrameImageFromFile(int cameraId, const std::string& path)
{
    static std::atomic<int> frameCounter{ 0 };
    //D:\YangFan_Work\GlassInspection_v3\TestImages
    frameCounter = 0;
    {
        std::vector<cv::Mat> cam_FrameImages;
        std::string folder_path = path + "/" + std::to_string(cameraId); // 替换为您的文件夹路径
        std::vector<std::string> image_paths;

        try {
            // 遍历文件夹中的所有文件
            for (const auto& entry : std::filesystem::directory_iterator(folder_path)) {
                if (entry.is_regular_file()) {
                    std::string extension = entry.path().extension().string();
                    // 检查是否为图像文件（支持常见格式）
                    if (extension == ".png" || extension == ".jpg" ||
                        extension == ".jpeg" || extension == ".bmp" ||
                        extension == ".tiff" || extension == ".tif") {
                        image_paths.push_back(entry.path().string());
                    }
                }
            }


            // 按文件名中的数字值排序（真正的递增顺序）
            std::sort(image_paths.begin(), image_paths.end(), [](const std::string& a, const std::string& b) {
                // 提取文件名（不含路径和扩展名）
                std::string filename_a = std::filesystem::path(a).stem().string(); // stem() 获取不带扩展名的文件名
                std::string filename_b = std::filesystem::path(b).stem().string();

                try {
                    // 转换为数字进行比较
                    long long num_a = std::stoll(filename_a);
                    long long num_b = std::stoll(filename_b);
                    return num_a < num_b;
                }
                catch (const std::exception& e) {
                    // 如果转换失败，回退到字符串比较
                    std::cerr << u8"警告: 无法将文件名转换为数字，使用字符串比较: "
                        << filename_a << ", " << filename_b << std::endl;
                    return filename_a < filename_b;
                }
                });
            std::deque<std::vector<cv::Mat>> batchQueue;
            // 读取图像并存入vector
            int count = 0;
            for (const auto& path : image_paths) {
                cv::Mat mat = cv::imread(path, 0);
                if (!mat.empty())
                {
                    count++;
                    /* 保存图像到队列 */
                    if (!PushFrame(std::move(mat))) // 使用基类的队列方法添加图像
                    //if (!PushFrame(mat.clone())) // 使用基类的队列方法添加图像
                    {
                        // 队列已满，记录丢帧
                        //LogPrintf("Camera[%d] frame queue is full, dropping frame", cameraId);
                    }

                    // 性能监控（每100帧记录一次）

                    //if (frameCounter++ % 100 == 0)
                    //{
                    //    FileLogPrintf("Camera[%d] Frames: %d, Dropped: %d, Queue: %d/%d",
                    //        GetCameraId(), frameCounter.load(),
                    //        GetDroppedFrames(), GetQueueSize(),
                    //        maxQueueSize);
                    //}
                }
                else {
                    std::cout << u8"无法读取: " << path << std::endl;
                }
            }
            //if (cam_FrameImages.size() > 0)
            //{
            //    batchQueue.emplace_back(
            //        std::move(cam_FrameImages));
            //    cam_FrameImages.clear();
            //}
            std::cout << u8"[相机" << cameraId << u8"]总共读取了 " << count << u8" 张图像" << std::endl;

        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << u8"文件系统错误: " << e.what() << std::endl;
            return;
        }
        catch (const std::exception& e) {
            std::cerr << u8"错误: " << e.what() << std::endl;
            return;
        }

    }


}

cv::Mat LineScanCamera_Base::GetFrontImage()
{
    std::unique_lock<std::mutex> lock(queueMutex);
    return frameQueue.front();
}