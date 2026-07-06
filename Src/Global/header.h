#pragma once
#include <iostream>

//opencv
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
//
//typedef enum DefectLevel
//{
//
//	NORMAL,										//0 正常
//	MINOR,										//1 轻微缺陷
//	MEDIUM,										//2 中等缺陷
//	SERIOUS,									//3 严重缺陷
//	ABNORMAL,									//4 异常
//	AREAERROR									//5 区域错误，非玻璃区域判断
//};
//
//
//typedef enum DefectType
//{
//
//	TYPE_POORCOATING,							//0 镀膜不良
//	TYPE_SCRATCH,								//1 划伤
//	TYPE_CALCULUS,								//2 结石
//	TYPE_BUBBLE,								//3 气泡
//	TYPE_TRADEMARK,								//4 商标
//	TYPE_WATERSTAIN,							//5 水渍
//	TYPE_SMUDGE,								//6 脏污
//	TYPE_ScreenPrintingDefects					//7 丝印缺陷
//};
//
//struct drawInformation
//{
//	int					DefectId = -1;			//瑕疵Id。用于使用小图存图时，与瑕疵信息对应。
//	QUuid				uuid;					// 数据索引									  
//	cv::Rect			rect;					//像素上对应的位置
//	cv::Rect			realRect;				//玻璃上实际对应的位置（mm）
//	DefectLevel			ErrorType;
//	DefectType			DefectType;
//	std::vector<float>	Errorinfo;
//	int					camera_id;
//	double				confidence;
//};