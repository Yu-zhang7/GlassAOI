#pragma once
#include <opencv2/opencv.hpp>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>

#include "Log.hpp"
class ImageBeautify
{
public:
	ImageBeautify();
	~ImageBeautify();

	int getGlassRoi(cv::Mat& inputGaryImg, cv::Rect& GlassRect, cv::Mat& glassImage, int threshold, std::vector<cv::Point>& edgePoints);
	int getGlassRoi(cv::Mat& inputGaryImg, cv::Rect& GlassRect, cv::Mat& glassImage, int threshold);

	int getGlassRoi(cv::Mat& inputGaryImg, cv::Rect& GlassRect, int threshold);

	void MergeImage(cv::Mat& background, cv::Mat patch, cv::Rect roi, int colorFlag, int colorVal, double alpha);

	void MeanImage(std::vector<std::vector<cv::Mat>> matVector, cv::Mat& meanImage,int threshold);

	void getInfoImage(std::vector<std::vector<cv::Mat>> matVector, std::vector<int> lightChannel, int threshold, std::vector<cv::Mat>& mergeImage, cv::Point& sizeImage, int& grayVal);

	cv::Mat getColorLegend(cv::Mat legendImage, int colorFlag);

	cv::Mat getChannelRoiImg(cv::Mat& image, int startCol, int endCol, int startRow, int endRow);

	std::vector<cv::Mat> divImg3Channel(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol, int roiChannel, int threshold);
	std::vector<cv::Mat> divImg3Channel(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol, int roiChannel, int threshold, std::vector<cv::Point>& edgePoints);

	std::vector<cv::Mat> divImg3ChannelEntire(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol, int threshold, cv::Rect& rect);

	std::vector<cv::Mat> divImgChannel3(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol);


	//std::vector<cv::Mat> divImg3Channel(const cv::Mat& src, const std::vector<std::vector<float>>& start_end, int cameraCol, int roiChannel, int threshold);

	//std::vector<cv::Mat> divImg3Channel(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol, int roiChannel, int threshold);


	std::vector<cv::Mat> divImgChannel2(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol);

	std::vector<cv::Mat> divImgChannel1(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol);

	std::vector<cv::Mat> divImgChannel3OpenCV(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol);

	std::vector<cv::Mat> divImgChannel2OpenCV(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol);

	void mainTest();
private:

};

