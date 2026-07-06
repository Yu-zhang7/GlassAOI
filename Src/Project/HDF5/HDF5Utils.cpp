#include "HDF5Utils.h"

HDF5Utils::HDF5Utils()
{
}

HDF5Utils::~HDF5Utils()
{
}

std::vector<cv::Mat> HDF5Utils::loadImagesFromHDF5(const std::string& filename) {
    std::vector<cv::Mat> images;

    try {
        // 1. 基础验证
        if (!fs::exists(filename)) {
            throw std::runtime_error(u8"文件不存在");
        }

        // 2. 打开文件
        HighFive::File file(filename, HighFive::File::ReadOnly);

        // 3. 获取数据集
        std::string dataset_path = "/images";
        if (!file.exist(dataset_path)) {
            auto datasets = file.listObjectNames();
            if (!datasets.empty()) dataset_path = datasets[0];
        }

        auto dataset = file.getDataSet(dataset_path);
        auto dims = dataset.getSpace().getDimensions();

        //// 输出数据集信息用于调试
        //std::cout << "数据集维度: ";
        //for (size_t i = 0; i < dims.size(); ++i) {
        //    std::cout << dims[i] << " ";
        //}
        //std::cout << "\n数据类型: " << dataset.getDataType().string() << std::endl;

        // 4. 维度处理
        size_t num_images, height, width;
        if (dims.size() == 2) {
            num_images = 1;
            height = dims[0];
            width = dims[1];
        }
        else if (dims.size() == 3) {
            num_images = dims[0];
            height = dims[1];
            width = dims[2];
        }
        else {
            throw std::runtime_error(u8"不支持的数据维度: " + std::to_string(dims.size()));
        }

        // 5. 准备存储数据的容器
        std::vector<unsigned char> all_data(num_images * height * width);

        // 6. 使用与写入相同的底层API读取
        hid_t dataset_id = dataset.getId();
        hid_t mem_type_id = H5T_NATIVE_UCHAR;  // 与写入时保持一致

        herr_t status = H5Dread(
            dataset_id,
            mem_type_id,
            H5S_ALL,          // 内存空间
            H5S_ALL,          // 文件空间
            H5P_DEFAULT,      // 属性列表
            all_data.data()    // 数据指针
        );

        if (status < 0) {
            throw std::runtime_error(u8"HDF5底层读取失败");
        }

        // 7. 转换为Mat
        images.reserve(num_images);
        for (size_t i = 0; i < num_images; ++i) {
            cv::Mat img(height, width, CV_8UC1);
            size_t offset = i * height * width;
            if (offset + height * width > all_data.size()) {
                throw std::runtime_error(u8"数据越界");
            }
            std::memcpy(img.data, &all_data[offset], height * width);
            images.push_back(img);
        }

        std::cout << u8"成功加载 " << images.size() << u8" 张图像 ("
            << width << "x" << height << ")" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << u8"加载失败: " << e.what() << std::endl;
    }

    return images;
}

bool HDF5Utils::saveImagesToHDF5(const std::vector<cv::Mat>& images,
    const std::string& filename) {
    if (images.empty())
    {
        return false;
    }

    const size_t num_images = images.size();
    const size_t height = images[0].rows;
    const size_t width = images[0].cols;

    try {
        // 1. 严格验证所有图像
        for (const auto& img : images) {
            if (img.rows != static_cast<int>(height) ||
                img.cols != static_cast<int>(width) ||
                img.type() != CV_8UC1) {
                std::cerr << "图像验证失败: "
                    << "尺寸: " << img.rows << "x" << img.cols
                    << " 类型: " << img.type()
                    << " (应为: " << height << "x" << width << " CV_8UC1)"
                    << std::endl;
                throw std::runtime_error("图像属性不匹配");
                return false;
            }
        }

        // 2. 准备连续内存缓冲区
        std::vector<unsigned char> all_data(num_images * height * width);

        // 3. 并行复制数据
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(num_images); ++i) {
            const cv::Mat& img = images[i];
            size_t offset = i * height * width;

            if (img.isContinuous()) {
                std::memcpy(all_data.data() + offset, img.data, height * width);
            }
            else {
                // 如果图像不连续，创建临时连续副本
                cv::Mat temp = img.clone();
                std::memcpy(all_data.data() + offset, temp.data, height * width);
            }
        }

        // 4. 创建文件并写入
        HighFive::File file(filename, HighFive::File::Truncate);
        std::vector<size_t> dims = { num_images, height, width };
        HighFive::DataSpace dataspace(dims);

        // 创建数据集
        auto dataset = file.createDataSet<unsigned char>(
            "/images",
            dataspace
        );

        // 5. 使用底层HDF5 API写入
        hid_t dataset_id = dataset.getId();
        herr_t status = H5Dwrite(
            dataset_id,
            H5T_NATIVE_UCHAR,   // 数据类型
            H5S_ALL,            // 内存空间
            H5S_ALL,            // 文件空间
            H5P_DEFAULT,        // 属性列表
            all_data.data()      // 数据指针
        );

        if (status < 0) {
            throw std::runtime_error("HDF5底层写入失败");
            return false;
        }

        std::cout << "成功写入 " << num_images << " 张图像" << std::endl;
    }
    catch (const HighFive::Exception& err) {
        std::cerr << "HDF5错误: " << err.what() << std::endl;
        std::remove(filename.c_str());
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        std::remove(filename.c_str());
        return false;
    }
    return true;
}


int HDF5Utils::mainTest() {
    std::vector<cv::Mat> test_images;
    std::string folder_path = "./img/Test2";

    // 加载测试图像 (循环10次示例)
    for (int i = 0; i < 20; i++) {
        try {
            for (const auto& entry : fs::directory_iterator(folder_path)) {
                if (entry.is_regular_file()) {
                    std::string file_path = entry.path().string();
                    cv::Mat img = cv::imread(file_path, 0);
                    if (!img.empty()) {
                        cv::resize(img, img, cv::Size(256, 256), cv::INTER_LINEAR);
                        test_images.push_back(img);
                        //std::cout << "Loaded: " << file_path << std::endl;
                    }
                }
            }
        }
        catch (const fs::filesystem_error& e) {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
            return 1;
        }
    }

    // 测试存储性能
    auto start = std::chrono::high_resolution_clock::now();
    saveImagesToHDF5(test_images, "image_collection.h5");
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start);
    std::cout << "存储耗时: " << duration.count() << " 毫秒" << std::endl;

    // 测试读取性能
    start = std::chrono::high_resolution_clock::now();
    auto loaded_images = loadImagesFromHDF5("image_collection.h5");
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start);
    std::cout << "读取耗时: " << duration.count() << " 毫秒" << std::endl;

    // 验证数据一致性
    for (size_t i = 0; i < test_images.size(); ++i) {
        double diff = cv::norm(test_images[i], loaded_images[i], cv::NORM_L1);
        if (diff > 0) {
            std::cerr << "图像 " << i << " 内容不一致!" << std::endl;
        }
    }

    return 0;
}