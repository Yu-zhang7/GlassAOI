#ifndef EIGHT_DIRECTION_SOBEL_H
#define EIGHT_DIRECTION_SOBEL_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

class EightDirectionSobel {
private:
    static std::vector<cv::Mat> kernels;
    static bool kernels_initialized;

    static void initializeKernels();

public:
    EightDirectionSobel();
    ~EightDirectionSobel();

    static cv::Mat edgeDetection8Direction(const cv::Mat& src, int output_type = 0, double threshold = 30);

    static std::vector<cv::Mat> create8DirectionKernels();

    static cv::Mat mergeAllDirections(const std::vector<cv::Mat>& direction_results, double threshold);

    static cv::Mat getMaxResponse(const std::vector<cv::Mat>& direction_results, double threshold);

    static cv::Mat createDirectionMap(const std::vector<cv::Mat>& direction_results);

    static void visualizeResults(const cv::Mat& src, const cv::Mat& edges, const std::string& output_path = "");

    static cv::Mat createComparisonImage(const cv::Mat& original, const cv::Mat& edges, const cv::Mat& result);
};

#endif // EIGHT_DIRECTION_SOBEL_H
