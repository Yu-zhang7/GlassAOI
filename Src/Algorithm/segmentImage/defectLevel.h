#pragma once

#ifndef SEGMENT_ALGO_API
#ifdef SEGMENT_ALGORITHM_EXPORTS
#define SEGMENT_ALGO_API __declspec(dllexport)
#else
#define SEGMENT_ALGO_API __declspec(dllimport)
#endif
#endif

//#include "GeneralMethod.h"
#include "Global.h"
#include "segImage.h"

#include <limits>
#include <algorithm>
#include <vector>
#include <cmath>


//#include <opencv2/alphamat.hpp>//AlphaMatting infoFlow使用


struct ConnectedComponentItem
{
	/* data */
	int numComponent = 0;
	cv::Mat labels;// 输出标签图像，每个像素点对应一个连通域标签
	cv::Mat stats;// 输出统计信息，包括连通域的外接矩形、面积等
	cv::Mat centroids;// 输出连通域的质心坐标
	cv::Mat mask;//输出所有连通域对应mask

};

class SEGMENT_ALGO_API defectLevel
{

public:
	defectLevel();
	~defectLevel();

	segImage segUtils;

	float getMaskArea(cv::Mat& mask);

	int GetLevelInfo(cv::Mat& inputMaskImage, DefectType type, std::vector<float>& errInfo);

	//DefectLevel defectLevelProcess(cv::Mat& inputImage,cv::Rect& rect, DefectType& type,cv::Mat& drawRoiImage);

	void setRankJudgment(RankJudgment& rankJudgment, 
		std::vector<float>& SetAreaRank,
		std::vector<float>& SetLengthRank,
		std::vector<float>& SetPerimeterRank
		);

	int ConnectedComponentAnalysis(cv::Mat& bw, int minArea, ConnectedComponentItem& ccAnalysis, int flag = 0);


	DefectLevel defectLevelProcessSingle(cv::Mat& inputImage, cv::Rect& rect, DefectType& type, ConnectedComponentItem& AnalysisInfo, std::vector<float>& errInfo);
	
	DefectLevel defectLevelProcessByEdge(cv::Mat& inputImage, cv::Rect& rect, DefectType& type, ConnectedComponentItem& AnalysisInfo, std::vector<float>& errInfo);

	int secondDetect(cv::Mat& inputImage, cv::Rect& rect, ConnectedComponentItem& AnalysisInfo, DefectType& TypeValue, DefectLevel& levelType, std::vector<float>& errInfo);
	int DoubleChangeErrorType(cv::Mat& inputImage, cv::Rect& rect, ConnectedComponentItem& AnalysisInfo, DefectType& TypeValue, DefectLevel& levelType, std::vector<float>& errInfo, double confidence);

	int DoubleChangeErrorType(cv::Mat& channel0, cv::Mat& channel1, cv::Mat& channel2, cv::Rect& rect, ConnectedComponentItem& AnalysisInfo, DefectType& TypeValue, DefectLevel& levelType, std::vector<float>& errInfo, double confidence, int DifGrayVal);

	std::vector<drawInformation> GetDefectLevel(std::vector<drawInformation>& data, PrescriptionParameter& levelRankInfo);

	void mainTest();

public:

	////六类缺陷存放等级参数
	//RankJudgment PoorCoating;//镀膜不良 0
	//RankJudgment scratch;//划伤 1
	//RankJudgment calculus;//结石 2
	//RankJudgment bubble;//气泡3
	//RankJudgment WaterStain;//水渍 5
	//RankJudgment smudge;//脏污 6



private:
	void initJudgmentRank();

};



