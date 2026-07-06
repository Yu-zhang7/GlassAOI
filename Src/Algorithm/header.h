#pragma once
#include <iostream>

//opencv
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>

//typedef enum DefectLevel {
//
//	NORMAL,//正常  0
//	MINOR,//轻微缺陷  1
//	MEDIUM,//中等缺陷 2
//	SERIOUS,//严重缺陷 3
//	ABNORMAL,//异常 4
//	AREAERROR//区域错误，非玻璃区域判断5
//};
//
//
//typedef enum DefectType {
//
//	TYPE_POORCOATING,//镀膜不良 0
//	TYPE_SCRATCH,//划伤 1
//	TYPE_CALCULUS,//结石 2
//	TYPE_BUBBLE,//气泡3
//	TYPE_TRADEMARK,//商标 4
//	TYPE_WATERSTAIN,//水渍 5
//	TYPE_SMUDGE,//脏污 6
//	TYPE_ScreenPrintingDefects//丝印缺陷 7
//};
//
//struct drawInformation
//{
//	int					DefectId = -1;		//瑕疵Id。用于使用小图存图时，与瑕疵信息对应。
//	//QUuid				uuid;				// 数据索引									  
//	cv::Rect			rect;				//像素上对应的位置
//	cv::Rect			realRect;			//玻璃上实际对应的位置（mm）
//	DefectLevel			ErrorType;
//	DefectType			DefectType;
//	std::vector<float>	Errorinfo;
//	int					camera_id;
//	double				confidence;
//};
//
//struct RankJudgment
//{
//	//轻微缺陷 中等缺陷  严重缺陷 三种缺陷 四个分界点  ，其中上限应无，下限为0
//
//	std::vector<float> areaRank;//面积判断   		轻微缺陷 中等缺陷  严重缺陷
//
//	std::vector<float> LengthRank;//长度判断   		轻微缺陷 中等缺陷  严重缺陷
//
//	std::vector<float> PerimeterRank;//周长判断   		轻微缺陷 中等缺陷  严重缺陷
//
//};
//
////用于存放传入算法内部的参数，主要与配方有关
//struct PrescriptionParameter
//{
//	double x_pixel2mm;//x方向像素与mm的转换比例
//	double y_pixel2mm;//y方向像素与mm的转换比例
//
//	double thresholdDetect_1;//检测阈值1
//	double thresholdDetect_2;//检测阈值2
//
//	std::vector<double> thresholdsCls;//存放每类的分类置信度阈值
//
//	//六类缺陷存放等级参数
//	RankJudgment PoorCoating;//镀膜不良 0
//	RankJudgment scratch;//划伤 1
//	RankJudgment calculus;//结石 2
//	RankJudgment bubble;//气泡3
//	RankJudgment WaterStain;//水渍 5
//	RankJudgment smudge;//脏污 6
//
//
//};