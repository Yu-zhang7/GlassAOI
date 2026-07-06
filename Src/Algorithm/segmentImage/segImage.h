#pragma once

#ifndef SEGMENT_ALGO_API
#ifdef SEGMENT_ALGORITHM_EXPORTS
#define SEGMENT_ALGO_API __declspec(dllexport)
#else
#define SEGMENT_ALGO_API __declspec(dllimport)
#endif
#endif

#include <iostream>
#include <string>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>

#include <filesystem>

#include "EightDirectionSobel.h"

namespace fs = std::filesystem;

class SEGMENT_ALGO_API segImage
{
public:
	segImage();
	~segImage();

	cv::Mat drawEdgesOnImage(const cv::Mat& src, const cv::Mat& edges, cv::Rect rect, const cv::Scalar& color, int thickness);

	cv::Mat detectBlackPadding(const cv::Mat& src, int threshold_value);

	std::pair<double, double> GetCalculateResult(const cv::Mat& src, cv::Mat& maskImage, int threshold_value);
	cv::Mat GetDrawImage(const cv::Mat& src, int threshold_value, int level);

	int mainTest();
	int processAllImages(const std::string& inputDir, const std::string& outputDir);
private:
	void visualizeDefects(const cv::Mat& image, const cv::Mat& defect_mask, const cv::Mat& black_mask,
		const std::string& outputPath);
};