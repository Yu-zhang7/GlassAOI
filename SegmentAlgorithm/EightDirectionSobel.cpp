//#include "EightDirectionSobel.h"
//#include <opencv2/opencv.hpp>
//#include <iostream>
//#include <vector>
//#include <cmath>
//#include <algorithm>
//
//EightDirectionSobel::EightDirectionSobel()
//{
//}
//
//EightDirectionSobel::~EightDirectionSobel()
//{
//}
//
//cv::Mat EightDirectionSobel::edgeDetection8Direction(const cv::Mat& src, int output_type, double threshold) {
//    cv::Mat gray;
//    if (src.channels() == 3) {
//        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
//    }
//    else {
//        gray = src.clone();
//    }
//
//    // 定义八个方向的Sobel算子
//    std::vector<cv::Mat> kernels = create8DirectionKernels();
//    std::vector<cv::Mat> direction_results;
//
//    // 对每个方向进行卷积
//    for (const auto& kernel : kernels) {
//        cv::Mat result;
//        cv::filter2D(gray, result, CV_32F, kernel);
//        cv::convertScaleAbs(result, result);
//        direction_results.push_back(result);
//    }
//
//    // 根据输出类型返回结果
//    switch (output_type) {
//    case 0: return mergeAllDirections(direction_results, threshold);
//    case 1: return createDirectionMap(direction_results);
//    case 2: return getMaxResponse(direction_results, threshold);
//    default: return mergeAllDirections(direction_results, threshold);
//    }
//}
//
//std::vector<cv::Mat> EightDirectionSobel::create8DirectionKernels() {
//    std::vector<cv::Mat> kernels;
//
//    // 0°方向
//    kernels.push_back((cv::Mat_<float>(3, 3) <<
//        -1, 0, 1,
//        -2, 0, 2,
//        -1, 0, 1));
//
//    // 45°方向
//    kernels.push_back((cv::Mat_<float>(3, 3) <<
//        0, 1, 2,
//        -1, 0, 1,
//        -2, -1, 0));
//
//    // 90°方向
//    kernels.push_back((cv::Mat_<float>(3, 3) <<
//        -1, -2, -1,
//        0, 0, 0,
//        1, 2, 1));
//
//    // 135°方向
//    kernels.push_back((cv::Mat_<float>(3, 3) <<
//        -2, -1, 0,
//        -1, 0, 1,
//        0, 1, 2));
//
//    // 180°方向 (与0°方向相反)
//    kernels.push_back((cv::Mat_<float>(3, 3) <<
//        1, 0, -1,
//        2, 0, -2,
//        1, 0, -1));
//
//    // 225°方向 (与45°方向相反)
//    kernels.push_back((cv::Mat_<float>(3, 3) <<
//        0, -1, -2,
//        1, 0, -1,
//        2, 1, 0));
//
//    // 270°方向 (与90°方向相反)
//    kernels.push_back((cv::Mat_<float>(3, 3) <<
//        1, 2, 1,
//        0, 0, 0,
//        -1, -2, -1));
//
//    // 315°方向 (与135°方向相反)
//    kernels.push_back((cv::Mat_<float>(3, 3) <<
//        2, 1, 0,
//        1, 0, -1,
//        0, -1, -2));
//
//    return kernels;
//}
//
//cv::Mat EightDirectionSobel::mergeAllDirections(const std::vector<cv::Mat>& direction_results, double threshold) {
//    cv::Mat merged = cv::Mat::zeros(direction_results[0].size(), CV_8UC1);
//
//    for (int y = 0; y < merged.rows; y++) {
//        for (int x = 0; x < merged.cols; x++) {
//            float max_response = 0;
//            for (const auto& result : direction_results) {
//                uchar response = result.at<uchar>(y, x);
//                if (response > max_response) {
//                    max_response = response;
//                }
//            }
//            if (max_response > threshold) {
//                merged.at<uchar>(y, x) = 255;
//            }
//        }
//    }
//
//    return merged;
//}
//
//cv::Mat EightDirectionSobel::getMaxResponse(const std::vector<cv::Mat>& direction_results, double threshold) {
//    cv::Mat max_response = cv::Mat::zeros(direction_results[0].size(), CV_8UC1);
//
//    for (int y = 0; y < max_response.rows; y++) {
//        for (int x = 0; x < max_response.cols; x++) {
//            uchar max_val = 0;
//            for (const auto& result : direction_results) {
//                uchar val = result.at<uchar>(y, x);
//                if (val > max_val) {
//                    max_val = val;
//                }
//            }
//            max_response.at<uchar>(y, x) = max_val;
//        }
//    }
//
//    // 应用阈值
//    cv::threshold(max_response, max_response, threshold, 255, cv::THRESH_BINARY);
//    return max_response;
//}
//
//cv::Mat EightDirectionSobel::createDirectionMap(const std::vector<cv::Mat>& direction_results) {
//    int rows = direction_results[0].rows;
//    int cols = direction_results[0].cols;
//
//    // 创建大画布 (2行4列)
//    cv::Mat direction_map(rows * 2, cols * 4, CV_8UC1, cv::Scalar(0));
//
//    std::vector<std::string> direction_names = {
//        "0°", "45°", "90°", "135°",
//        "180°", "225°", "270°", "315°"
//    };
//
//    // 放置各方向结果
//    for (int i = 0; i < 8; i++) {
//        int row = i / 4;
//        int col = i % 4;
//        cv::Rect roi(col * cols, row * rows, cols, rows);
//        direction_results[i].copyTo(direction_map(roi));
//
//        // 添加方向标签
//        cv::putText(direction_map, direction_names[i],
//            cv::Point(col * cols + 10, row * rows + 30),
//            cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255), 2);
//    }
//
//    return direction_map;
//}
//
//void EightDirectionSobel::visualizeResults(const cv::Mat& src, const cv::Mat& edges, const std::string& output_path) {
//    // 在原图上绘制边缘
//    cv::Mat result_with_edges = src.clone();
//    if (result_with_edges.channels() == 1) {
//        cv::cvtColor(result_with_edges, result_with_edges, cv::COLOR_GRAY2BGR);
//    }
//
//    // 查找轮廓
//    std::vector<std::vector<cv::Point>> contours;
//    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
//
//    // 绘制轮廓
//    cv::drawContours(result_with_edges, contours, -1, cv::Scalar(0, 255, 0), 2);
//
//    // 创建对比图像
//    cv::Mat comparison = createComparisonImage(src, edges, result_with_edges);
//
//    // 显示结果
//    cv::imshow("Original", src);
//    cv::imshow("8-Direction Sobel Edges", edges);
//    cv::imshow("Result with Edges", result_with_edges);
//    cv::imshow("Comparison", comparison);
//
//    // 保存结果
//    if (!output_path.empty()) {
//        cv::imwrite(output_path, comparison);
//        std::cout << "结果已保存: " << output_path << std::endl;
//    }
//
//    cv::waitKey(0);
//}
//
//cv::Mat EightDirectionSobel::createComparisonImage(const cv::Mat& original, const cv::Mat& edges, const cv::Mat& result) {
//    int width = original.cols;
//    int height = original.rows;
//    int separator = 5;
//
//    cv::Mat comparison(height, width * 3 + separator * 2, CV_8UC3, cv::Scalar(40, 40, 40));
//
//    // 放置原图
//    cv::Mat roi1 = comparison(cv::Rect(0, 0, width, height));
//    if (original.channels() == 1) {
//        cv::cvtColor(original, roi1, cv::COLOR_GRAY2BGR);
//    }
//    else {
//        original.copyTo(roi1);
//    }
//
//    // 放置边缘图
//    cv::Mat roi2 = comparison(cv::Rect(width + separator, 0, width, height));
//    if (edges.channels() == 1) {
//        cv::cvtColor(edges, roi2, cv::COLOR_GRAY2BGR);
//    }
//    else {
//        edges.copyTo(roi2);
//    }
//
//    // 放置结果图
//    cv::Mat roi3 = comparison(cv::Rect(width * 2 + separator * 2, 0, width, height));
//    result.copyTo(roi3);
//
//    // 添加标题
//    cv::putText(comparison, "Original", cv::Point(10, 30),
//        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
//    cv::putText(comparison, "8-Direction Sobel", cv::Point(width + separator + 10, 30),
//        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
//    cv::putText(comparison, "Result", cv::Point(width * 2 + separator * 2 + 10, 30),
//        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
//
//    return comparison;
//}



#include "EightDirectionSobel.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// 初始化静态成员变量
std::vector<cv::Mat> EightDirectionSobel::kernels;
bool EightDirectionSobel::kernels_initialized = false;

EightDirectionSobel::EightDirectionSobel()
{
    // 确保卷积核被初始化
    if (!kernels_initialized) {
        initializeKernels();
    }
}

EightDirectionSobel::~EightDirectionSobel()
{
}

void EightDirectionSobel::initializeKernels() {
    if (kernels_initialized) {
        return;
    }

    // 0°方向
    kernels.push_back((cv::Mat_<float>(3, 3) <<
        -1, 0, 1,
        -2, 0, 2,
        -1, 0, 1));

    // 45°方向
    kernels.push_back((cv::Mat_<float>(3, 3) <<
        0, 1, 2,
        -1, 0, 1,
        -2, -1, 0));

    // 90°方向
    kernels.push_back((cv::Mat_<float>(3, 3) <<
        -1, -2, -1,
        0, 0, 0,
        1, 2, 1));

    // 135°方向
    kernels.push_back((cv::Mat_<float>(3, 3) <<
        -2, -1, 0,
        -1, 0, 1,
        0, 1, 2));

    // 180°方向 (与0°方向相反)
    kernels.push_back((cv::Mat_<float>(3, 3) <<
        1, 0, -1,
        2, 0, -2,
        1, 0, -1));

    // 225°方向 (与45°方向相反)
    kernels.push_back((cv::Mat_<float>(3, 3) <<
        0, -1, -2,
        1, 0, -1,
        2, 1, 0));

    // 270°方向 (与90°方向相反)
    kernels.push_back((cv::Mat_<float>(3, 3) <<
        1, 2, 1,
        0, 0, 0,
        -1, -2, -1));

    // 315°方向 (与135°方向相反)
    kernels.push_back((cv::Mat_<float>(3, 3) <<
        2, 1, 0,
        1, 0, -1,
        0, -1, -2));

    kernels_initialized = true;
}

cv::Mat EightDirectionSobel::edgeDetection8Direction(const cv::Mat& src, int output_type, double threshold) {
    cv::Mat gray;
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    }
    else {
        gray = src.clone();
    }

    // 确保卷积核已初始化
    if (!kernels_initialized) {
        initializeKernels();
    }

    std::vector<cv::Mat> direction_results;

    // 对每个方向进行卷积
    for (const auto& kernel : kernels) {
        cv::Mat result;
        cv::filter2D(gray, result, CV_32F, kernel);
        cv::convertScaleAbs(result, result);
        direction_results.push_back(result);
    }

    // 根据输出类型返回结果
    switch (output_type) {
    case 0: return mergeAllDirections(direction_results, threshold);
    case 1: return createDirectionMap(direction_results);
    case 2: return getMaxResponse(direction_results, threshold);
    default: return mergeAllDirections(direction_results, threshold);
    }
}

std::vector<cv::Mat> EightDirectionSobel::create8DirectionKernels() {
    // 确保卷积核已初始化
    if (!kernels_initialized) {
        initializeKernels();
    }
    return kernels;
}

// 以下其他函数保持不变...
cv::Mat EightDirectionSobel::mergeAllDirections(const std::vector<cv::Mat>& direction_results, double threshold) {
    cv::Mat merged = cv::Mat::zeros(direction_results[0].size(), CV_8UC1);

    for (int y = 0; y < merged.rows; y++) {
        for (int x = 0; x < merged.cols; x++) {
            float max_response = 0;
            for (const auto& result : direction_results) {
                uchar response = result.at<uchar>(y, x);
                if (response > max_response) {
                    max_response = response;
                }
            }
            if (max_response > threshold) {
                merged.at<uchar>(y, x) = 255;
            }
        }
    }

    return merged;
}

cv::Mat EightDirectionSobel::getMaxResponse(const std::vector<cv::Mat>& direction_results, double threshold) {
    cv::Mat max_response = cv::Mat::zeros(direction_results[0].size(), CV_8UC1);

    for (int y = 0; y < max_response.rows; y++) {
        for (int x = 0; x < max_response.cols; x++) {
            uchar max_val = 0;
            for (const auto& result : direction_results) {
                uchar val = result.at<uchar>(y, x);
                if (val > max_val) {
                    max_val = val;
                }
            }
            max_response.at<uchar>(y, x) = max_val;
        }
    }

    // 应用阈值
    cv::threshold(max_response, max_response, threshold, 255, cv::THRESH_BINARY);
    return max_response;
}

cv::Mat EightDirectionSobel::createDirectionMap(const std::vector<cv::Mat>& direction_results) {
    int rows = direction_results[0].rows;
    int cols = direction_results[0].cols;

    // 创建大画布 (2行4列)
    cv::Mat direction_map(rows * 2, cols * 4, CV_8UC1, cv::Scalar(0));

    std::vector<std::string> direction_names = {
        "0°", "45°", "90°", "135°",
        "180°", "225°", "270°", "315°"
    };

    // 放置各方向结果
    for (int i = 0; i < 8; i++) {
        int row = i / 4;
        int col = i % 4;
        cv::Rect roi(col * cols, row * rows, cols, rows);
        direction_results[i].copyTo(direction_map(roi));

        // 添加方向标签
        cv::putText(direction_map, direction_names[i],
            cv::Point(col * cols + 10, row * rows + 30),
            cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255), 2);
    }

    return direction_map;
}

void EightDirectionSobel::visualizeResults(const cv::Mat& src, const cv::Mat& edges, const std::string& output_path) {
    // 在原图上绘制边缘
    cv::Mat result_with_edges = src.clone();
    if (result_with_edges.channels() == 1) {
        cv::cvtColor(result_with_edges, result_with_edges, cv::COLOR_GRAY2BGR);
    }

    // 查找轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 绘制轮廓
    cv::drawContours(result_with_edges, contours, -1, cv::Scalar(0, 255, 0), 2);

    // 创建对比图像
    cv::Mat comparison = createComparisonImage(src, edges, result_with_edges);

    // 显示结果
    cv::imshow("Original", src);
    cv::imshow("8-Direction Sobel Edges", edges);
    cv::imshow("Result with Edges", result_with_edges);
    cv::imshow("Comparison", comparison);

    // 保存结果
    if (!output_path.empty()) {
        cv::imwrite(output_path, comparison);
        std::cout << "结果已保存: " << output_path << std::endl;
    }

    cv::waitKey(0);
}

cv::Mat EightDirectionSobel::createComparisonImage(const cv::Mat& original, const cv::Mat& edges, const cv::Mat& result) {
    int width = original.cols;
    int height = original.rows;
    int separator = 5;

    cv::Mat comparison(height, width * 3 + separator * 2, CV_8UC3, cv::Scalar(40, 40, 40));

    // 放置原图
    cv::Mat roi1 = comparison(cv::Rect(0, 0, width, height));
    if (original.channels() == 1) {
        cv::cvtColor(original, roi1, cv::COLOR_GRAY2BGR);
    }
    else {
        original.copyTo(roi1);
    }

    // 放置边缘图
    cv::Mat roi2 = comparison(cv::Rect(width + separator, 0, width, height));
    if (edges.channels() == 1) {
        cv::cvtColor(edges, roi2, cv::COLOR_GRAY2BGR);
    }
    else {
        edges.copyTo(roi2);
    }

    // 放置结果图
    cv::Mat roi3 = comparison(cv::Rect(width * 2 + separator * 2, 0, width, height));
    result.copyTo(roi3);

    // 添加标题
    cv::putText(comparison, "Original", cv::Point(10, 30),
        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
    cv::putText(comparison, "8-Direction Sobel", cv::Point(width + separator + 10, 30),
        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
    cv::putText(comparison, "Result", cv::Point(width * 2 + separator * 2 + 10, 30),
        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);

    return comparison;
}