#include "infertrt.hpp"

GLASSAOI::GLASSAOI() {
    if (cudaStreamCreate(&stream_) != cudaSuccess) {
        throw std::runtime_error("Failed to create CUDA stream.");
    }
}

void GLASSAOI::buildEngineFromOnnx(const std::string& onnxFile, const std::string& engineFile, size_t workspaceSize) {
    MyLogger logger;
    auto builder = TrtUniquePtr<nvinfer1::IBuilder>(nvinfer1::createInferBuilder(logger));
    if (!builder) throw std::runtime_error("Failed to create TensorRT builder.");

    const auto explicitBatch = 1U << static_cast<uint32_t>(nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
    auto network = TrtUniquePtr<nvinfer1::INetworkDefinition>(builder->createNetworkV2(explicitBatch));
    auto parser = TrtUniquePtr<nvonnxparser::IParser>(nvonnxparser::createParser(*network, logger));

    if (!parser || !parser->parseFromFile(onnxFile.c_str(), static_cast<int>(nvinfer1::ILogger::Severity::kWARNING))) {
        throw std::runtime_error("Failed to parse ONNX file: " + onnxFile);
    }

    auto config = TrtUniquePtr<nvinfer1::IBuilderConfig>(builder->createBuilderConfig());
#if NV_TENSORRT_MAJOR >= 10
    config->setMemoryPoolLimit(nvinfer1::MemoryPoolType::kWORKSPACE, workspaceSize);
#else
    config->setMaxWorkspaceSize(workspaceSize);
#endif

    if (builder->platformHasFastFp16()) {
        config->setFlag(nvinfer1::BuilderFlag::kFP16);
        std::cout << "[INFO] FP16 precision enabled." << std::endl;
    }

    auto engine = TrtUniquePtr<nvinfer1::ICudaEngine>(
        builder->buildEngineWithConfig(*network, *config)
    );
    if (!engine) {
        throw std::runtime_error("Failed to build TensorRT engine from ONNX.");
    }

    auto modelStream = TrtUniquePtr<nvinfer1::IHostMemory>(engine->serialize());
    if (!modelStream) {
        throw std::runtime_error("Failed to serialize engine.");
    }

    std::ofstream ofs(engineFile, std::ios::binary);
    if (!ofs.write(static_cast<const char*>(modelStream->data()), modelStream->size())) {
        throw std::runtime_error("Failed to write engine file: " + engineFile);
    }
    ofs.close();

    std::cout << "[INFO] Engine saved to: " << engineFile << std::endl;
}

void GLASSAOI::init(const std::string& onnxPath, const std::string& enginePath) {
    // 检查 engine 是否存在
    std::ifstream file(enginePath, std::ios::binary);
    if (!file.good()) {
        std::cout << "[INFO] Engine not found. Building from ONNX..." << std::endl;
        buildEngineFromOnnx(onnxPath, enginePath, 1ULL << 30); // 1GB
        file.open(enginePath, std::ios::binary);
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> data(size);
    file.read(data.data(), size);
    file.close();

    MyLogger logger;
    runtime_ = TrtUniquePtr<nvinfer1::IRuntime>(nvinfer1::createInferRuntime(logger));
    if (!runtime_) throw std::runtime_error("Failed to create TensorRT runtime.");

#if NV_TENSORRT_MAJOR >= 10
    engine_ = TrtUniquePtr<nvinfer1::ICudaEngine>(runtime_->deserializeCudaEngine(data.data(), size));
#else
    engine_ = TrtUniquePtr<nvinfer1::ICudaEngine>(runtime_->deserializeCudaEngine(data.data(), size, nullptr));
#endif
    if (!engine_) throw std::runtime_error("Failed to deserialize engine.");

    context_ = TrtUniquePtr<nvinfer1::IExecutionContext>(engine_->createExecutionContext());
    if (!context_) throw std::runtime_error("Failed to create execution context.");

    // 分配 GPU 内存
    input_size_bytes_ = MAX_BATCH_SIZE * 3 * MODEL_HEIGHT * MODEL_WIDTH * sizeof(float);
    output_size_bytes_ = MAX_BATCH_SIZE * (4 + NUM_CLASSES) * NUM_BOXES * sizeof(float);

    if (cudaMalloc(&d_input_, input_size_bytes_) != cudaSuccess ||
        cudaMalloc(&d_output_, output_size_bytes_) != cudaSuccess) {
        throw std::runtime_error("Failed to allocate GPU memory.");
    }

    // 分配 pinned host 内存（提升传输速度）
    if (cudaMallocHost(&h_input_pinned_, input_size_bytes_) != cudaSuccess ||
        cudaMallocHost(&h_output_pinned_, output_size_bytes_) != cudaSuccess) {
        throw std::runtime_error("Failed to allocate pinned host memory.");
    }

#if NV_TENSORRT_MAJOR >= 10
    // TRT 10: 绑定 tensor 名称
    int32_t num_io_tensors = engine_->getNbIOTensors();
    if (num_io_tensors != 2) {
        throw std::runtime_error("Expected exactly 2 I/O tensors, got " + std::to_string(num_io_tensors));
    }
    input_name_ = engine_->getIOTensorName(0);
    output_name_ = engine_->getIOTensorName(1);

    context_->setInputTensorAddress(input_name_.c_str(), d_input_);
    context_->setOutputTensorAddress(output_name_.c_str(), d_output_);

    nvinfer1::Dims inDim = engine_->getTensorShape(input_name_.c_str());
    nvinfer1::Dims outDim = engine_->getTensorShape(output_name_.c_str());
#else
    // TRT 8: 使用 binding index
    if (engine_->getNbBindings() != 2) {
        throw std::runtime_error("Expected exactly 2 bindings.");
    }
    nvinfer1::Dims inDim = engine_->getBindingDimensions(0);
    nvinfer1::Dims outDim = engine_->getBindingDimensions(1);
#endif

    // 验证输入输出形状
    if (!(inDim.nbDims == 4 && inDim.d[0] == MAX_BATCH_SIZE && inDim.d[1] == 3 &&
        inDim.d[2] == MODEL_HEIGHT && inDim.d[3] == MODEL_WIDTH)) {
        throw std::runtime_error("Input shape mismatch!");
    }
    if (!(outDim.nbDims == 3 && outDim.d[0] == MAX_BATCH_SIZE &&
        outDim.d[1] == (4 + NUM_CLASSES) && outDim.d[2] == NUM_BOXES)) {
        throw std::runtime_error("Output shape mismatch!");
    }

    std::cout << "[INFO] TensorRT " << NV_TENSORRT_MAJOR << "." << NV_TENSORRT_MINOR
        << " initialized. Input: " << inDim.d[0] << "x" << inDim.d[1]
        << "x" << inDim.d[2] << "x" << inDim.d[3]
        << ", Output: " << outDim.d[0] << "x" << outDim.d[1] << "x" << outDim.d[2] << std::endl;
}

float GLASSAOI::iou_of(const Object& a, const Object& b) {
    float x1 = std::max(a.box.x, b.box.x);
    float y1 = std::max(a.box.y, b.box.y);
    float x2 = std::min(a.box.x + a.box.width, b.box.x + b.box.width);
    float y2 = std::min(a.box.y + a.box.height, b.box.y + b.box.height);

    float inter_w = std::max(0.0f, x2 - x1);
    float inter_h = std::max(0.0f, y2 - y1);
    float inter_area = inter_w * inter_h;

    float area_a = a.box.width * a.box.height;
    float area_b = b.box.width * b.box.height;
    float union_area = area_a + area_b - inter_area;

    return (union_area > 1e-6f) ? inter_area / union_area : 0.0f;
}

void GLASSAOI::hardNMS(std::vector<Object>& input, std::vector<Object>& output,
    float iou_threshold, unsigned int topk) {
    if (input.empty()) return;
    std::sort(input.begin(), input.end(),
        [](const Object& a, const Object& b) { return a.confidence > b.confidence; });

    std::vector<bool> suppressed(input.size(), false);
    output.clear();
    output.reserve(std::min(topk, static_cast<unsigned int>(input.size())));

    for (size_t i = 0; i < input.size() && output.size() < topk; ++i) {
        if (suppressed[i]) continue;
        output.push_back(input[i]);
        for (size_t j = i + 1; j < input.size(); ++j) {
            if (!suppressed[j] && iou_of(input[i], input[j]) > iou_threshold) {
                suppressed[j] = true;
            }
        }
    }
}

void GLASSAOI::detect(const std::vector<cv::Mat>& input_images, std::vector<std::vector<Object>>& outputs) {
    int N = static_cast<int>(input_images.size());
    if (N <= 0 || N > MAX_BATCH_SIZE) {
        throw std::invalid_argument("Batch size must be in [1, " + std::to_string(MAX_BATCH_SIZE) + "], got " + std::to_string(N));
    }

    outputs.assign(N, {});

    std::vector<float> ratios(N);
    std::vector<int> x_pads(N), y_pads(N);

    // 预处理：填充到固定尺寸
    for (int b = 0; b < N; ++b) {
        const cv::Mat& img = input_images[b];
        if (img.empty()) {
            std::cerr << "[WARNING] Empty input image at batch " << b << std::endl;
            continue;
        }

        cv::Mat resized;
        float ratio = std::min(static_cast<float>(MODEL_WIDTH) / img.cols,
            static_cast<float>(MODEL_HEIGHT) / img.rows);
        int new_w = static_cast<int>(img.cols * ratio);
        int new_h = static_cast<int>(img.rows * ratio);
        int x_pad = (MODEL_WIDTH - new_w) / 2;
        int y_pad = (MODEL_HEIGHT - new_h) / 2;

        cv::resize(img, resized, cv::Size(new_w, new_h));
        cv::copyMakeBorder(resized, resized, y_pad, y_pad, x_pad, x_pad,
            cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));

        ratios[b] = ratio;
        x_pads[b] = x_pad;
        y_pads[b] = y_pad;

        float* base = h_input_pinned_ + b * 3 * MODEL_HEIGHT * MODEL_WIDTH;
        for (int c = 0; c < 3; ++c) {
            float* ch = base + c * MODEL_HEIGHT * MODEL_WIDTH;
            for (int i = 0; i < MODEL_HEIGHT; ++i) {
                const cv::Vec3b* row = resized.ptr<cv::Vec3b>(i);
                for (int j = 0; j < MODEL_WIDTH; ++j) {
                    ch[i * MODEL_WIDTH + j] = row[j][2 - c] / 255.0f; // BGR -> RGB? adjust as needed
                }
            }
        }
    }

    // H2D
    if (cudaMemcpyAsync(d_input_, h_input_pinned_, input_size_bytes_, cudaMemcpyHostToDevice, stream_) != cudaSuccess) {
        throw std::runtime_error("Failed to copy input to device.");
    }

    // 推理
#if NV_TENSORRT_MAJOR >= 10
    if (!context_->enqueueV3(stream_)) {
        throw std::runtime_error("executeV3 failed!");
    }
#else
    void* bindings[2] = { d_input_, d_output_ };
    if (!context_->enqueueV2(bindings, stream_, nullptr)) {
        throw std::runtime_error("enqueueV2 failed!");
    }
#endif

    // D2H
    if (cudaMemcpyAsync(h_output_pinned_, d_output_, output_size_bytes_, cudaMemcpyDeviceToHost, stream_) != cudaSuccess) {
        throw std::runtime_error("Failed to copy output from device.");
    }
    if (cudaStreamSynchronize(stream_) != cudaSuccess) {
        throw std::runtime_error("CUDA stream synchronization failed.");
    }

    // 后处理
    for (int b = 0; b < N; ++b) {
        std::vector<Object> proposals;
        const float* out = h_output_pinned_ + b * (4 + NUM_CLASSES) * NUM_BOXES;

        for (int i = 0; i < NUM_BOXES; ++i) {
            float cx = out[i];
            float cy = out[i + NUM_BOXES];
            float w = out[i + 2 * NUM_BOXES];
            float h = out[i + 3 * NUM_BOXES];

            float max_conf = -1.0f;
            int cls_id = 0;
            for (int c = 0; c < NUM_CLASSES; ++c) {
                float conf = out[i + (4 + c) * NUM_BOXES];
                if (conf > max_conf) {
                    max_conf = conf;
                    cls_id = c;
                }
            }

            if (max_conf < COMMON_CONFIDENCE) continue;

            Object obj;
            obj.box.x = (cx - w * 0.5f - x_pads[b]) / ratios[b];
            obj.box.y = (cy - h * 0.5f - y_pads[b]) / ratios[b];
            obj.box.width = w / ratios[b];
            obj.box.height = h / ratios[b];
            obj.label = cls_id;
            obj.confidence = max_conf;
            proposals.push_back(obj);
        }

        hardNMS(proposals, outputs[b], 0.6f, 30);
    }
}