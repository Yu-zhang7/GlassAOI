#include <atomic>
#include <vector>
#include <opencv2/opencv.hpp>

class FrameBuffer {
private:
    std::vector<cv::Mat> buffer;
    std::atomic<size_t> write_idx{ 0 };
    std::atomic<size_t> read_idx{ 0 };
    const size_t capacity;

public:
    FrameBuffer(size_t size = 1000) : capacity(size) {
        buffer.resize(size);
    }

    // 生产者方法 - 由frameCallback调用
    bool try_push(cv::Mat&& frame) {
        size_t current_write = write_idx.load(std::memory_order_relaxed);
        size_t next_write = (current_write + 1) % capacity;

        // 检查缓冲区是否已满
        if (next_write == read_idx.load(std::memory_order_acquire)) {
            return false;
        }

        // 使用std::move高效转移图像数据
        buffer[current_write] = std::move(frame);

        // 更新写索引
        write_idx.store(next_write, std::memory_order_release);
        return true;
    }

    // 消费者方法 - 由拼接线程调用
    bool try_pop(cv::Mat& frame) {
        size_t current_read = read_idx.load(std::memory_order_relaxed);

        // 检查缓冲区是否为空
        if (current_read == write_idx.load(std::memory_order_acquire)) {
            return false;
        }

        // 使用std::move高效获取图像数据
        frame = std::move(buffer[current_read]);

        // 更新读索引
        read_idx.store((current_read + 1) % capacity, std::memory_order_release);
        return true;
    }

    size_t size() const {
        size_t w = write_idx.load(std::memory_order_acquire);
        size_t r = read_idx.load(std::memory_order_acquire);

        if (w >= r) {
            return w - r;
        }
        else {
            return capacity - r + w;
        }
    }

    bool empty() const {
        return read_idx.load(std::memory_order_acquire) ==
            write_idx.load(std::memory_order_acquire);
    }

    bool full() const {
        size_t next_write = (write_idx.load(std::memory_order_relaxed) + 1) % capacity;
        return next_write == read_idx.load(std::memory_order_acquire);
    }
};