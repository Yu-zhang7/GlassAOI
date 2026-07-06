#include "ResultBuffer.h"

ResultBuffer& ResultBuffer::GetInstance(size_t bufferSize)
{
    static ResultBuffer instance(bufferSize);
    return instance;
}

ResultBuffer::ResultBuffer(size_t bufferSize) :
    maxQueueSize(bufferSize)
{
    // 构造函数实现
}


bool ResultBuffer::PushFrame(cv::Mat&& frame)
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

bool ResultBuffer::PopFrame(cv::Mat& frame, int timeout_ms)
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
    std::unique_lock<std::mutex> lock(queueMutex);
    if (!queueCV.wait_for(lock, std::chrono::milliseconds(timeout_ms),
        [this]() { return !frameQueue.empty(); })) {
        return false; // 超时
    }

    if (frameQueue.empty())
    {
        return false;
    }
    frame = std::move(frameQueue.front());
    frameQueue.pop();
    return true;
}

bool ResultBuffer::TryPopFrame(cv::Mat& frame)
{
    std::unique_lock<std::mutex> lock(queueMutex);
    if (frameQueue.empty()) {
        return false;
    }

    frame = std::move(frameQueue.front());
    frameQueue.pop();
    return true;
}

size_t ResultBuffer::GetQueueSize() const
{
    std::unique_lock<std::mutex> lock(queueMutex);
    return frameQueue.size();
}

bool ResultBuffer::IsQueueEmpty() const
{
    std::unique_lock<std::mutex> lock(queueMutex);
    return frameQueue.empty();
}

bool ResultBuffer::IsQueueFull() const
{
    std::unique_lock<std::mutex> lock(queueMutex);
    return frameQueue.size() >= maxQueueSize;
}

void ResultBuffer::ClearQueue()
{
    std::unique_lock<std::mutex> lock(queueMutex);
    while (!frameQueue.empty()) {
        frameQueue.pop();
    }
    droppedFrames = 0;
    frameCounter = 0;
}

int ResultBuffer::GetDroppedFrames() const
{
    return droppedFrames.load(std::memory_order_relaxed);
}

int ResultBuffer::GetFrameCount() const
{
    return frameCounter.load(std::memory_order_relaxed);
}

bool ResultBuffer::WaitForFrame(int timeout_ms)
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

void ResultBuffer::GetMat(cv::Mat& img)
{
    if (!TryPopFrame(img)) // 非阻塞模式
    {
        img.release();
    }
}

void ResultBuffer::GetMatList(std::vector<cv::Mat>& imgList)
{
    imgList.clear();

    // 使用基类的公共方法获取所有可用图像
    cv::Mat frame;
    while (TryPopFrame(frame)) // 非阻塞模式
    {
        imgList.push_back(std::move(frame));
    }
}

void ResultBuffer::AddImageToQueue(cv::Mat& img)
{
    PushFrame(std::move(img));
}

cv::Mat ResultBuffer::GetFrontImage()
{
    std::unique_lock<std::mutex> lock(queueMutex);
    return frameQueue.front();
}