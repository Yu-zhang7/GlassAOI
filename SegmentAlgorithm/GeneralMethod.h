#pragma once

#include "header.h"
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>

#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <QUuid>
#include "Global.h"

#include <omp.h>

#include "segmentImage/segImage.h"

class GeneralMethod
{
public:
	segImage segUtils;
public:
	GeneralMethod();
	~GeneralMethod();

	std::vector<std::string> loadFileImage(std::string img_dir);
	std::vector<std::string> getImageFileNames(const std::string& directoryPath);
	cv::Mat convertTo3Channels(const cv::Mat& binImg);

	int drawDefectImage(cv::Mat& inputImage, cv::Mat& maskImage, DefectLevel ErrorType = DefectLevel::NORMAL);

	int mergedImage2One(std::vector<cv::Mat> images, cv::Mat& mergedImg);

	void drawRectangle(cv::Mat& image, const cv::Rect& rect, DefectLevel& ErrorType);

	void drawRectangleWithInfo(cv::Mat& image, const cv::Rect& rect, DefectLevel& ErrorType, DefectType& DefectType, std::vector<float>& Errorinfo, float scale);

	int divideImage(cv::Mat& largeImage,int imgWidth,int imgHeight,std::vector<cv::Mat>& mats, std::vector<std::vector<int>>& imgIndexs);

	int divideImageShallow(cv::Mat& largeImage, int imgWidth, int imgHeight, std::vector<cv::Mat>& tiles, std::vector<std::vector<int>>& imgIndexs);

	int MergeImage(std::vector<cv::Mat>& mats, std::vector<std::vector<int>>& imgIndexs, cv::Mat& mergeLarget);

	void mainTest();

	std::vector<drawInformation> filterDefectsByBorderRegion(const std::vector<drawInformation>& defects, int imageWidth, int imageHeight, float verticalRatio, float horizontalRatio);

	std::vector<drawInformation> mergeDrawInformations(const std::vector<drawInformation>& drawInfos);

	std::vector<drawInformation> mergeDrawInformationsWithDistance(const std::vector<drawInformation>& drawInfos, int Dis);

	void gray2ColorMap(cv::Mat& inputGaryImg, cv::Mat& colorMapImg);

	int getGlassRect(cv::Mat& inputGaryImg, cv::Rect& rect_, cv::Rect& GlassRect, int threshold);
	int OnlyGetGlassRect(cv::Mat& inputGaryImg, cv::Rect& rect_, int threshold);

	std::string readFilePaths();

	std::string gbk_to_utf8(const std::string& str_gbk);

	std::string extract_directory_name(const std::string& path_str);

	cv::Rect computeCropRect2Color(const cv::Rect& inputRect, int imageWidth, int imageHeight, int imageSize, cv::Rect* outRelativeRect);

	double getMaxGrayValue(const cv::Mat& image);

	cv::Mat drawSemiTransparentRedOverlayWithCircle(const cv::Mat& src, const cv::Rect& rect, double alpha = 0.5, int minRadius = 10, int maxRadius = 100);

	std::vector<drawInformation> filterResultsByEdgeDistance(const std::vector<drawInformation>& results_beforeSort, const std::vector<cv::Point>& edgePoints, double distanceThreshold);

	void drawResults(cv::Mat& image, const std::vector<drawInformation>& results);

	double getAverageOfTwoColumns(const cv::Mat& img, int col1, int col2);

	double getScale(int width, int height);

	std::vector<drawInformation> real2Virtual(std::vector<drawInformation> results, double scale);

	std::vector<drawInformation> real2Virtual_(std::vector<drawInformation> results, double widthScale, double heightScale);

	std::vector<drawInformation> real2VirtualResize(int width, int height, std::vector<drawInformation>& results, double widthScale, double heightScale, int box);

	std::vector<drawInformation> stitchImagesAndTransformRects(const std::vector<cv::Mat>& images, const cv::Mat& stitchedImage, const std::vector<std::vector<drawInformation>>& batchDrawInfo);

	std::vector<drawInformation> stitchImagesAndTransformRects(const std::vector<cv::Mat>& images, const cv::Mat& stitchedImage, const std::vector<std::vector<drawInformation>>& batchDrawInfo, std::vector<cv::Rect>& rects);

	std::vector<drawInformation> transformToROICoordinates(const std::vector<drawInformation>& largeDrawInfo, const cv::Rect& roiRect);

	cv::Rect rotateRectCCW90(const cv::Rect& rect, int width, int height);

	std::vector<drawInformation> rotateDrawInfoCCW90(const std::vector<drawInformation>& drawInfos, int width, int height);

	cv::Mat vconcatVariableWidth(const std::vector<cv::Mat>& images, const cv::Scalar& bgColor);

	void blockRotate90Clockwise(cv::Mat& src, cv::Mat& dst, int blockSize);

	void blockRotate90CounterClockwise(cv::Mat& src, cv::Mat& dst, int blockSize);

	bool efficientVerticalConcat(const std::vector<cv::Mat>& images, cv::Mat& result);

	cv::Mat efficientVerticalConcat(const std::vector<cv::Mat>& images);

	cv::Scalar getDefectColor(DefectLevel level);

	cv::Mat generateDefectThumbnail(const std::vector<drawInformation>& defects, const cv::Size& originalSize, const cv::Size& thumbnailSize, int canvasGrayValue);

	std::vector<drawInformation> filterAndSortDefects(const std::vector<drawInformation>& results);

	void drawRectCenter(cv::Mat& img, const cv::Rect& rect, const cv::Scalar& color);

	cv::Rect mergeRectanglesWithOffset(const std::vector<cv::Rect>& currentBatchRect, const std::vector<int>& startY);

	void rotatePointsClockwise90(const std::vector<cv::Point>& srcPoints, std::vector<cv::Point>& dstPoints, int width, int height);

	cv::Mat convertEdgePointsToMat(const std::vector<cv::Point>& edgePoints);

	int RotateImage(std::vector<cv::Mat>& images, int flipV);

	cv::Rect computeCropRect(const cv::Rect& inputRect, int imageWidth, int imageHeight, int imageSize);

private:
};
