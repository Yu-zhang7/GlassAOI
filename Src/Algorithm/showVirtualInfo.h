/*
	对玻璃区域进行提取，并将检测缺陷区域约束至玻璃区域内
*/
#pragma once

#include "Global.h"

class showVirtualInfo
{
public:
	showVirtualInfo();
	~showVirtualInfo();

	cv::Mat generateVirtualImage(const cv::Mat& legendImage, cv::Size virtualSize, int glassColor, int BackgroundColor, int threshold);

	bool GetGlassAreaAndResult(const cv::Mat& largeImage, int threshold, int showGrayValue, std::vector<drawInformation>& drawInfos, int minAreaThreshold, cv::Rect& glassRect, cv::Mat& glassMaskImageVirtual, std::vector<drawInformation>& drawInfosFilter);

	// 玻璃伪彩图生成（支持多个玻璃区域）
	cv::Mat generateGlassPseudoColor(
		const cv::Mat& inputImage,
		int glassThreshold,
		int regionSize
	);

	cv::Mat generateGlassPseudoColorChannel2(const cv::Mat& inputImage, const cv::Mat& inputImage2, int glassThreshold, int regionSize);

	cv::Mat createVirtualMap(const std::vector<cv::Mat>& images, int glassColor, int BackgroundColor, int thresholdValue);

private:
	// 辅助方法
	cv::Mat downsampleImage(const cv::Mat& inputImage, int regionSize);
	cv::Mat extractGlassRegions(const cv::Mat& image, int glassThreshold);
	std::vector<std::vector<cv::Point>> findGlassContours(const cv::Mat& binaryMask);
	cv::Mat createPseudoColorForRegions(const cv::Mat& downsampledImage, const std::vector<std::vector<cv::Point>>& contours);
};