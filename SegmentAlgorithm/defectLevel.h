#pragma once

//#include "GeneralMethod.h"
#include "Global.h"
#include "segImage.h"

#include <limits>
#include <algorithm>
#include <vector>
#include <cmath>

struct ConnectedComponentItem
{
	int numComponent = 0;
	cv::Mat labels;
	cv::Mat stats;
	cv::Mat centroids;
	cv::Mat mask;
};

class defectLevel
{
public:
	defectLevel();
	~defectLevel();

	segImage segUtils;

	float getMaskArea(cv::Mat& mask);

	int GetLevelInfo(cv::Mat& inputMaskImage, DefectType type, std::vector<float>& errInfo);

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

private:
	void initJudgmentRank();
};
