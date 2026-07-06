#ifndef INFERTT_HPP
#define INFERTT_HPP

#ifdef INFERTRT_EXPORTS
#define INFERTRT_API __declspec(dllexport)
#else
#define INFERTRT_API __declspec(dllimport)
#endif


#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <cuda_runtime.h>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <string>
#include <memory>
#include <stdexcept>

// === 模型参数（根据实际模型调整）===
constexpr int MAX_BATCH_SIZE = 8;
constexpr int MODEL_HEIGHT = 256;
constexpr int MODEL_WIDTH = 256;
constexpr int NUM_CLASSES = 13;
constexpr int NUM_BOXES = 1344;
constexpr float COMMON_CONFIDENCE = 0.25f;

struct Object {
    cv::Rect2f box;
    int label;
    float confidence;
};

// TensorRT 对象智能指针删除器（TRT 8 / 10 兼容）
struct TrtDeleter {
    template<typename T>
    void operator()(T* obj) const {
        if (obj) {
#if NV_TENSORRT_MAJOR >= 10
            delete obj;
#else
            obj->destroy();
#endif
        }
    }
};

template<typename T>
using TrtUniquePtr = std::unique_ptr<T, TrtDeleter>;

class MyLogger : public nvinfer1::ILogger {
public:
    void log(Severity severity, const char* msg) noexcept override {
        switch (severity) {
        case Severity::kINTERNAL_ERROR:
        case Severity::kERROR:
            std::cerr << "[TRT ERROR] " << msg << std::endl;
            break;
        case Severity::kWARNING:
            std::cout << "[TRT WARNING] " << msg << std::endl;
            break;
        default:
            // 忽略 INFO/VERBOSE
            break;
        }
    }
};

class INFERTRT_API GLASSAOI {
public:
    GLASSAOI();
    ~GLASSAOI() = default; // RAII 自动释放

    void init(const std::string& onnxPath, const std::string& enginePath);
    void detect(const std::vector<cv::Mat>& input_images, std::vector<std::vector<Object>>& outputs);

private:
    void buildEngineFromOnnx(const std::string& onnxFile, const std::string& engineFile, size_t workspaceSize);
    float iou_of(const Object& a, const Object& b);
    void hardNMS(std::vector<Object>& input, std::vector<Object>& output,
        float iou_threshold, unsigned int topk);

    // TensorRT 核心对象（RAII）
    TrtUniquePtr<nvinfer1::IRuntime> runtime_;
    TrtUniquePtr<nvinfer1::ICudaEngine> engine_;
    TrtUniquePtr<nvinfer1::IExecutionContext> context_;

    // GPU 内存（device）
    void* d_input_ = nullptr;
    void* d_output_ = nullptr;

    // Host 内存（pinned memory 提升 H2D/D2H 速度）
    float* h_input_pinned_ = nullptr;
    float* h_output_pinned_ = nullptr;

    size_t input_size_bytes_ = 0;
    size_t output_size_bytes_ = 0;

    cudaStream_t stream_ = 0;

#if NV_TENSORRT_MAJOR >= 10
    std::string input_name_;
    std::string output_name_;
#endif
};

#endif // INFERTT_HPP