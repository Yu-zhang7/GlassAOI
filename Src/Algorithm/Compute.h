#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>

//#include "deploy/model.hpp"
//#include "deploy/option.hpp"
//#include "deploy/result.hpp"


#include "infertrt.hpp"//新版算法

//#include "header.h"
#include "Global.h"                 //全局变量类
#include "defectLevel.h"

#include "../Global/Log.hpp"

class Compute
{
public:
	Compute();
	~Compute();

	defectLevel levelUtils;

	int RotateImage(std::vector<cv::Mat>& images, int flipV);

	//逆时针旋转90度
	std::vector<drawInformation> rotateDrawInfoCCW90(
		const std::vector<drawInformation>& drawInfos,
		int width, int height);
	//顺时针旋转90度
	std::vector<drawInformation> rotateDrawInfoCW90(
		const std::vector<drawInformation>& drawInfos,
		int width, int height);

	std::vector<drawInformation> transformToROICoordinates(const std::vector<drawInformation>& largeDrawInfo, const cv::Rect& roiRect);

	int getGlassRoi(cv::Mat& inputGaryImg, cv::Rect& GlassRect, int threshold);

	int getGlassRoiSample(cv::Mat& inputGaryImg, cv::Rect& GlassRect, int threshold, double scale);

	std::vector<drawInformation> mergeDefectsToLargeImage(const std::vector<std::vector<drawInformation>>& batchDrawInfo, int rowHeight);

	std::vector<int> findGlassTilesFast(const std::vector<cv::Mat>& tiles, double threshold);



	std::vector<drawInformation> ComputeProcess(cv::Mat& largeImg,
		PrescriptionParameter& parameter, const double& glassThreshold_val, const double& range_val,
		int tileHeight, int tileWidth);

	cv::Mat ComputeProcessDraw(cv::Mat& largeImg, PrescriptionParameter& parameter, const double& glassThreshold_val, const double& range_val, int tileHeight, int tileWidth);

	void process_batch_m_imagesChannel(const std::vector<cv::Mat>& imageChannel0, const std::vector<cv::Mat>& imageChannel1, const std::vector<cv::Mat>& imageChannel2, 
		const std::vector<int>& indices, std::vector<int>& errIndexs, std::vector<std::vector<drawInformation>>& Informations, double threshold, int flag);

	//std::vector<drawInformation> ComputeProcessMultiChannel_old(std::vector<cv::Mat>& largeImgs, PrescriptionParameter& parameter, const double& glassThreshold_val, const double& range_val, int tileHeight, int tileWidth);

	std::vector<drawInformation> ComputeProcessMultiChannel(std::vector<cv::Mat>& largeImgs, PrescriptionParameter& parameter, const double& glassThreshold_val, const double& range_val, int tileHeight, int tileWidth);

	std::vector<drawInformation> ComputeProcessPart(std::vector<cv::Mat>& largeImgs, PrescriptionParameter& parameter, const double& glassThreshold_val, const double& range_val, int tileHeight, int tileWidth);

	std::pair<std::vector<cv::Mat>, std::vector<int>> detectDefectiveGlassTilesHorizontal(const std::vector<cv::Mat>& mats, 
		const std::vector<int>& glassIndices, const std::vector<std::vector<int>>& imgIndexs, 
		int tileWidth, int tileHeight, double range_val);
	std::pair<std::vector<cv::Mat>, std::vector<int>> detectDefectiveGlassTilesVertical(const std::vector<cv::Mat>& mats, 
		const std::vector<int>& glassIndices, const std::vector<std::vector<int>>& imgIndexs, 
		int tileWidth, int tileHeight, double range_val);


private:
	cv::Mutex mutex; // 用于线程同步

	//std::unique_ptr<deploy::DetectModel> model = nullptr;
	//std::unique_ptr<deploy::DetectModel> model2 = nullptr;

	// 新版算法
	GLASSAOI m_detector;
};

