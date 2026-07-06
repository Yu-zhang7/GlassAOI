//


#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <highfive/H5File.hpp>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <H5public.h> // 添加HDF5初始化头文件
#include <highfive/H5DataType.hpp>
namespace fs = std::filesystem;

class HDF5Utils {
public:
    HDF5Utils();
    ~HDF5Utils();

    // 添加静态互斥锁
    static std::mutex h5_mutex;

    std::vector<cv::Mat> loadImagesFromHDF5(const std::string& filename);
    bool saveImagesToHDF5(const std::vector<cv::Mat>& images,
        const std::string& filename);
    int mainTest();

private:
    // 添加HDF5库初始化状态
    static bool hdf5_initialized;
    static void initialize_hdf5();
};