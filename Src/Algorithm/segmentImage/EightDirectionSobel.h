//#ifndef EIGHT_DIRECTION_SOBEL_H
//#define EIGHT_DIRECTION_SOBEL_H
//
//#include <opencv2/opencv.hpp>
//#include <vector>
//#include <string>
//
//class EightDirectionSobel {
//public:
//    EightDirectionSobel();
//    ~EightDirectionSobel();
//
//    std::vector<cv::Mat> kernels;
//
//    /**
//     * @brief 八方向Sobel边缘检测
//     * @param src 输入图像
//     * @param output_type 输出类型: 0=合并结果, 1=各方向结果, 2=最大响应
//     * @param threshold 阈值
//     * @return 边缘检测结果
//     */
//    static cv::Mat edgeDetection8Direction(const cv::Mat& src, int output_type = 0, double threshold = 30);
//
//    /**
//     * @brief 创建八方向Sobel算子
//     * @return 八个方向的卷积核
//     */
//    static std::vector<cv::Mat> create8DirectionKernels();
//
//    /**
//     * @brief 合并所有方向的响应
//     * @param direction_results 各方向的结果
//     * @param threshold 阈值
//     * @return 合并后的边缘图像
//     */
//    static cv::Mat mergeAllDirections(const std::vector<cv::Mat>& direction_results, double threshold);
//
//    /**
//     * @brief 获取最大响应
//     * @param direction_results 各方向的结果
//     * @param threshold 阈值
//     * @return 最大响应图像
//     */
//    static cv::Mat getMaxResponse(const std::vector<cv::Mat>& direction_results, double threshold);
//
//    /**
//     * @brief 创建方向映射图像
//     * @param direction_results 各方向的结果
//     * @return 包含所有方向的拼接图像
//     */
//    static cv::Mat createDirectionMap(const std::vector<cv::Mat>& direction_results);
//
//    /**
//     * @brief 可视化边缘检测结果
//     * @param src 原图
//     * @param edges 边缘图像
//     * @param output_path 输出路径
//     */
//    static void visualizeResults(const cv::Mat& src, const cv::Mat& edges, const std::string& output_path = "");
//
//    /**
//     * @brief 创建对比图像
//     * @param original 原图
//     * @param edges 边缘图
//     * @param result 结果图
//     * @return 拼接后的对比图像
//     */
//    static cv::Mat createComparisonImage(const cv::Mat& original, const cv::Mat& edges, const cv::Mat& result);
//};
//
//#endif // EIGHT_DIRECTION_SOBEL_H


#ifndef EIGHT_DIRECTION_SOBEL_H
#define EIGHT_DIRECTION_SOBEL_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

class EightDirectionSobel {
private:
    static std::vector<cv::Mat> kernels; // 改为静态类变量
    static bool kernels_initialized;     // 标记是否已初始化

    // 初始化卷积核
    static void initializeKernels();

public:
    EightDirectionSobel();
    ~EightDirectionSobel();

    /**
     * @brief 八方向Sobel边缘检测
     * @param src 输入图像
     * @param output_type 输出类型: 0=合并结果, 1=各方向结果, 2=最大响应
     * @param threshold 阈值
     * @return 边缘检测结果
     */
    static cv::Mat edgeDetection8Direction(const cv::Mat& src, int output_type = 0, double threshold = 30);

    /**
     * @brief 创建八方向Sobel算子
     * @return 八个方向的卷积核
     */
    static std::vector<cv::Mat> create8DirectionKernels();

    /**
     * @brief 合并所有方向的响应
     * @param direction_results 各方向的结果
     * @param threshold 阈值
     * @return 合并后的边缘图像
     */
    static cv::Mat mergeAllDirections(const std::vector<cv::Mat>& direction_results, double threshold);

    /**
     * @brief 获取最大响应
     * @param direction_results 各方向的结果
     * @param threshold 阈值
     * @return 最大响应图像
     */
    static cv::Mat getMaxResponse(const std::vector<cv::Mat>& direction_results, double threshold);

    /**
     * @brief 创建方向映射图像
     * @param direction_results 各方向的结果
     * @return 包含所有方向的拼接图像
     */
    static cv::Mat createDirectionMap(const std::vector<cv::Mat>& direction_results);

    /**
     * @brief 可视化边缘检测结果
     * @param src 原图
     * @param edges 边缘图像
     * @param output_path 输出路径
     */
    static void visualizeResults(const cv::Mat& src, const cv::Mat& edges, const std::string& output_path = "");

    /**
     * @brief 创建对比图像
     * @param original 原图
     * @param edges 边缘图
     * @param result 结果图
     * @return 拼接后的对比图像
     */
    static cv::Mat createComparisonImage(const cv::Mat& original, const cv::Mat& edges, const cv::Mat& result);
};

#endif // EIGHT_DIRECTION_SOBEL_H