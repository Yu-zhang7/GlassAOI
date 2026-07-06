#ifndef RESULTBUFFER_H
#define RESULTBUFFER_H

#include "LineScanCamera_Base.h"
class ResultBuffer
{
public:
    // 删除拷贝构造函数和赋值运算符
    ResultBuffer(const ResultBuffer&) = delete;
    ResultBuffer& operator=(const ResultBuffer&) = delete;

    // 获取单例实例
    static ResultBuffer& GetInstance(size_t bufferSize = 1000);
    //ResultBuffer(size_t bufferSize = 1000);
    //virtual ~ResultBuffer() = default;

    //队列操作方法
    bool PushFrame(cv::Mat&& frame);
    bool PopFrame(cv::Mat& frame, int timeout_ms = 0);
    bool TryPopFrame(cv::Mat& frame); // 非阻塞版本

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

private:
    // 构造函数私有化
    ResultBuffer(size_t bufferSize);
    ~ResultBuffer() = default;

    // 保护成员 - 锁和条件变量
    mutable std::mutex queueMutex;
    std::condition_variable queueCV;

    // 图像队列和锁
    std::queue<cv::Mat> frameQueue;

    // 统计变量
    std::atomic<int> frameCounter{ 0 };
    std::atomic<int> droppedFrames{ 0 };

    // 队列最大大小
    size_t maxQueueSize;
};

#endif // RESULTBUFFER_H