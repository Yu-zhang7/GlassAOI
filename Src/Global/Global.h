#ifndef GLOBAL_H
#define GLOBAL_H

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <QCoreApplication>
#include <QTranslator>
#include <QUuid>
#include <QMetaType>
#include <QImage>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <QMutex>
#include <QMap>
#include <QByteArray>
#include <string>
#include "RecipeConfig.h"
#include <iomanip>
#include <sstream>
#include <string_view>
#include <iostream>

//使用SQL_QUERY方式
#define SQL_QUERY

#define LOOPTEST 0


// 定义状态枚举
enum class GlassStatus
{
	GlassIn,
	GlassOut,
	NoGlass,
	Error
};
Q_DECLARE_METATYPE(GlassStatus);


struct drawInformation
{
	int									DefectId					= -1;					//瑕疵Id。用于使用小图存图时，与瑕疵信息对应。
	QUuid								uuid;											// 数据索引									  
	cv::Rect							rect;											//像素上对应的位置
	cv::Rect							realRect;										//玻璃上实际对应的位置（mm）
	DefectLevel							ErrorType;
	DefectType							DefectType;
	std::vector<float>					Errorinfo;
	int									camera_id;
	double								confidence;
};

/* 显示图像和瑕疵的结构体*/
struct QueueDefectItem
{
	cv::Mat								image_0;										//原始通道0图像
	cv::Mat								image_1;										//原始通道1图像
	cv::Mat								image_2;										//原始通道2图像
	cv::Mat								backgroundImage;								//背景图像
	cv::Mat								pseudoColorImage;								//伪彩图像

	std::vector<cv::Mat>				defectImages_ch0;								//瑕疵小图通道0的items
	std::vector<cv::Mat>				defectImages_ch1;								//瑕疵小图通道1的items
	std::vector<cv::Mat>				defectImages_ch2;								//瑕疵小图通道2的items

	std::vector<drawInformation>		drawInfo;										//json信息
	std::string							timeProduce;									//生产时间。对应数据记录的time_produce
	double								scale;											//缩放比例
	float								glassPhysicalWidth = 1000;						//玻璃宽度。毫米单位
	float								glassPhysicalHeight = 1000;						//玻璃高度。毫米单位
	float								glassPixelWidth = 1000;							//玻璃宽度。像素单位
	float								glassPixelHeight = 1000;						//玻璃高度。像素单位

	int									NgFlag;												//判断玻璃是否Ng
	std::string							resultPath;										//结果保存路径
	int									glassIndex;										//玻璃编号
};

Q_DECLARE_METATYPE(QueueDefectItem)	//注册元类型以便在信号槽中使用
Q_DECLARE_METATYPE(drawInformation)


struct RankJudgment
{
	//轻微缺陷 中等缺陷  严重缺陷 三种缺陷 四个分界点  ，其中上限应无，下限为0

	std::vector<float> areaRank;//面积判断   		轻微缺陷 中等缺陷  严重缺陷

	std::vector<float> LengthRank;//长度判断   		轻微缺陷 中等缺陷  严重缺陷

	std::vector<float> PerimeterRank;//周长判断   		轻微缺陷 中等缺陷  严重缺陷

	~RankJudgment()
	{
		areaRank.clear();
		LengthRank.clear();
		PerimeterRank.clear();
	}
};

//用于存放传入算法内部的参数，主要与配方有关
struct PrescriptionParameter
{
	double x_pixel2mm;//x方向像素与mm的转换比例
	double y_pixel2mm;//y方向像素与mm的转换比例

	double thresholdDetect_1;//检测阈值1
	double thresholdDetect_2;//检测阈值2

	std::vector<double> thresholdsCls;	//存放每类的分类置信度阈值

	int grayDifference;					// 灰度差值.用于水渍和脏污的区分

	//六类缺陷存放等级参数
	RankJudgment PoorCoating;			// 0 镀膜不良
	RankJudgment scratch;				// 1 划伤
	RankJudgment calculus;				// 2 结石
	RankJudgment bubble;				// 3 气泡
	RankJudgment Trademark;				// 4 商标
	RankJudgment WaterStain;			// 5 水渍
	RankJudgment smudge;				// 6 脏污
	RankJudgment ScreenPrinting;		// 7 丝印
	RankJudgment ChippedEdge;			// 8 崩边
	RankJudgment Pitting;				// 9 麻点
	RankJudgment GlassCullet;			// 10 玻渣
	RankJudgment Waviness;				// 11 云朵
	RankJudgment Other;					// 12 其他

};

struct Point {
	int x;
	int y;

};

struct DefectInfo {
	std::string							name;
	std::string							value;
	std::string							unit;
	double								confidence;
	double								minorValue;
	double								moderateValue;
	double								majorValue;
	bool								isEnabled;
	// 其他可能需要的字段
};
/*************** 各项文件的路径 **************************************/

//配置文件路径
inline std::string						ConfigPath				= "./Conifg/config.ini";

//配方文件主路径
inline std::string						RecipeFolderPath			= "./Recipes";

////缺陷配方0路径
//inline std::string RecipePath_0										= "./Conifg/Recipe0.json";
//
////缺陷配方1路径
//inline std::string RecipePath_1										= "./Conifg/Recipe1.json";
//
////缺陷配方2路径
//inline std::string RecipePath_2										= "./Conifg/Recipe2.json";

//数据库路径
inline std::string						DataBasePath				= "./DataBase/glassAOI.db";

//粗筛算法模型路径
inline std::string						ModelPath_cu				= "./Detect/model.engine";

//精筛算法模型路径
inline std::string						ModelPath_jing				= "./Detect/model2.engine";

//英文语言包
inline QString							lang_English				= "./Translation/GlassInspection_en_US.qm";

//俄文语言包
inline QString							lang_Russian				= "./Translation/GlassInspection_ru_RU.qm";

//中文语言包
inline QString							lang_Chinese				= "./Translation/GlassInspection_zh_CN.qm";

//西班牙文语言包
inline QString							lang_Spanish				= "./Translation/GlassInspection_es_ES.qm";

//葡萄牙文语言包
inline QString							lang_Portuguese				= "./Translation/GlassInspection_pt_BR.qm";

/**********************************************************************/
//玻璃图像旋转方向
inline int								RotationDirectionFlag		= 0; //0-顺时针旋转90度，1-逆时针旋转90度

//生成虚拟图是否使用不规则图像的方法
inline bool								UsedIrregularImage			= true;

inline bool								isGlassStopGrabbing			= true;		//是否停止采集图像

//是否保存合并大图(未分割)
inline bool								IsSaveLargeImage			= false;

//保存日志级别
inline int								LogFileLevel				= 2;	

//控制台日志级别
inline int								LogConsoleLevel				= 2;

//视图模式单双视图切换
inline bool								IsDoubleImage				= true;	

//伪彩图像切换
inline bool								IsPseudoColorImage			= true;

//当前缺陷配方
inline std::string						DefaultRecipeName			= "";

inline RecipeConfig::Config				RecipeInfo;

//界面语言
inline std::string						Language					= "";

//是否启用对丝印缺陷的检测
//inline bool								IsScreenPrintingUsed_Global		= false;
//inline bool								m_defectTypeUsability[DefectType::TYPE_SCREENPRINTING] = false;	//在RcipeConfig.h中声明

/* 不同相机的序列号 */
inline std::string						Camera1						= "";
inline std::string						Camera2						= "";
inline std::string						Camera3						= "";
inline std::string						Camera4						= "";
inline std::string						Camera5						= "";
inline std::string						Camera6						= "";
inline std::string						Camera7						= "";
inline std::string						Camera8						= "";
inline std::string						Camera9						= "";

//使用的相机数量
inline int								CameraCount					= 9;

inline int								CameraBrandIndex			= 0;				//相机品牌 0：海康 1华睿	

inline int								LightCount					= 3;				//光源通道数

/* 客户信息 */
inline std::string						CustomerName				= "";

inline std::vector<std::vector<float>>	camerainfo;

//踏板踩踏时间间隔
inline int								PedalTimeInterval			= 2;				//单位：秒。踏板踩踏的防护时间间隔，避免频繁踩踏产生的流程问题

//当次玻璃序号
inline int								glassIndex					= 0;


//像素到物理单位的转换比例
////inline double PIXEL_X_MM												= 0.109977;
////inline double PIXEL_Y_MM												= 0.198901;
////
////
////inline float GlassX;	//
////inline float GlassY;

inline double							Pixle2MM_X					= 0.109977;			//像素到mm的转换比例
inline double							Pixle2MM_Y					= 0.198901;			//像素到mm的转换比例

inline int								GrayReal					= 128;				//用于判断显示三通道中哪一个亮度(场)的图像

inline int								heightSingle				= 256;
inline double							glassThreshold_val			= 25.0;
inline double							range_val					= 25.0;

inline int								glassThreshold_display		= 20;				//界面显示用的玻璃区域灰度值阈值，默认为20.0

//长宽比例调整
inline double							widthScale					= 1.749962995;
inline double							heightScale					= 1.0;


//是否读图模式
inline int								IS_READ_MODE				= 0;

inline int								READ_MODE_PORCESS_STATUS	= -1;

//是否发送检测结果给控制板
inline bool								SendResults					= false;

//Real玻璃尺寸
inline float							GlassWidth;
inline float							GlassHeight;

//History玻璃尺寸
inline float							GlassWidth_His;
inline float							GlassHeight_His;


//暗场玻璃区域灰度值,最大值阈值
inline int								Light0GlassGrayValue		= 15;

//暗场丝印区域灰度值 最小值阈值
inline int								Light0SilkGrayValue			= 110;

//丝印占比判断
inline float							SilkAreaThreshold			= 0.001;


/* 历史图像自动清理 */
//是否自动清理历史图像
inline bool								IsAutoCleanup				= true;

//历史图像保留天数
inline int								ImagesRetentionDays			= 15;

//每日自动清理的时段
inline int								AutoCleanupTime				= 0;

//紧急清理磁盘空间的判断阈值
inline int								EmergencyThreshol			= 20;

//紧急清理磁盘空间的目标阈值
inline int								EmergencyTarget				= 30;

/* 图像参数 */
//曝光时间
inline int								ExposureTime				= 27;

//图像高度
inline int								ImageHeight					= 512;

//延迟停止采集
inline float							DelayStopTime				= 2.0;	//单位：秒

//延迟开始采集
inline float							DelayStartTime				= 1.0;	//单位：秒

inline bool								IsRealTimeChange_Main			= true; //表格数据是否实时刷新
inline bool								IsRealTimeChange_Tab			= true; //表格数据是否实时刷新

/***********************************************************************/
inline std::atomic<bool>				m_isWorking					= false;			//工作状态
inline std::atomic<bool>				m_isReady					= false;			//准备就绪状态

inline std::atomic<bool>				m_ImageProcessState			= false;			//图像处理流程状态。

inline std::atomic<bool>                m_isSignalRunning			= false;			//视觉流程运行状态

inline QDateTime						m_SignalDateTime;								//单次流程启动时间。界面显示时间

inline QDateTime						m_FilterDateTim_Start;							//历史记录过滤时间段开始时间

inline QDateTime						m_FilterDateTim_End;							//历史记录过滤时间段结束时间

//inline QVector<QString>					m_DefectTypeNames;								// 存储缺陷名称.用于界面动态展示

inline QMutex							m_DefectNamesMutex;								// 用于刷新缺陷名称线程安全访问

inline QMutex							m_DefectNamesMutex_his;								// 用于刷新缺陷名称线程安全访问

inline std::vector<drawInformation>		m_results_His;									//显示的历史记录的瑕疵信息Vector
inline std::vector<drawInformation>		m_results_Last;									//显示的最后一组记录的瑕疵信息Vector
inline std::vector<drawInformation>		m_results_Real;									//显示的当前记录的瑕疵信息Vector

inline std::string						m_time_produce_His;								//选择的历史记录的唯一标识
inline std::string						m_time_produce_Last;							//最后一组记录(左场景)的唯一标识
//inline std::string						m_time_produce_Real;							//当前记录(右场景)的唯一标识

inline QDateTime						m_dateTime_His;									//选择的历史记录的显示到界面上的时间
inline QDateTime						m_dateTime_Last;								//最后一组记录(左场景)的显示到界面上的时间
inline QDateTime						m_dateTime_Real;								//当前记录(右场景)的显示到界面上的时间

inline float							m_glassPixelWidth_Real		= 0.0f;
inline float							m_glassPixelHeight_Real		= 0.0f;

inline float							m_glassPixelWidth_Last		= 0.0f;
inline float							m_glassPixelHeight_Last		= 0.0f;

inline float							m_glassPixelWidth_His		= 0.0f;
inline float							m_glassPixelHeight_His		= 0.0f;

inline float							m_glassPhysicalWidth_Real	= 0.0f;
inline float							m_glassPhysicalHeight_Real	= 0.0f;

inline float							m_glassPhysicalWidth_Last	= 0.0f;
inline float							m_glassPhysicalHeight_Last	= 0.0f;

inline float							m_glassPhysicalWidth_His	= 0.0f;
inline float							m_glassPhysicalHeight_His	= 0.0f;

inline int								DefectMarkerSize_Max		= 26;	//图例最大像素尺寸	
inline int								DefectMarkerSize_Min		= 22;	//图例最大像素尺寸	

inline QImage Mat2QImage(const cv::Mat& mat)
{
	// 	QImage qImg;
	// 	if (cvImg.channels() == 3)                             //三通道彩色图像
	// 	{
	// 		//CV_BGR2RGB
	// 		cv::cvtColor(cvImg, cvImg, cv::COLOR_RGB2BGR);
	// 		qImg = QImage((const unsigned char*)(cvImg.data), cvImg.cols, cvImg.rows, cvImg.cols * cvImg.channels(), QImage::Format_RGB888);
	// 	}
	// 	else if (cvImg.channels() == 1)                    //单通道（灰度图）
	// 	{
	// 		qImg = QImage((const unsigned char*)(cvImg.data), cvImg.cols, cvImg.rows, cvImg.cols * cvImg.channels(), QImage::Format_Indexed8);
	// 
	// 		QVector<QRgb> colorTable;
	// 		for (int k = 0; k < 256; ++k)
	// 		{
	// 			colorTable.push_back(qRgb(k, k, k));
	// 		}
	// 		qImg.setColorTable(colorTable);//把qImg的颜色按像素点的颜色给设置
	// 	}
	// 	else
	// 	{
	// 		qImg = QImage((const unsigned char*)(cvImg.data), cvImg.cols, cvImg.rows, cvImg.cols * cvImg.channels(), QImage::Format_RGB888);
	// 	}
	// 	return qImg;

	if (mat.empty())
	{
		return QImage();
	}
	QImage image;
	switch (mat.type())
	{
	case CV_8UC1:
	{
		image = QImage((const uchar*)(mat.data),
			mat.cols, mat.rows, mat.step,
			QImage::Format_Grayscale8);
		return image.copy();
	}
	case CV_8UC2:
	{
		mat.convertTo(mat, CV_8UC1);
		image = QImage((const uchar*)(mat.data),
			mat.cols, mat.rows, mat.step,
			QImage::Format_Grayscale8);
		return image.copy();
	}
	case CV_8UC3:
	{
		// Copy input Mat
		const uchar* pSrc = (const uchar*)mat.data;
		// Create QImage with same dimensions as input Mat
		QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
		return image.rgbSwapped();
	}
	case CV_8UC4:
	{
		// Copy input Mat
		const uchar* pSrc = (const uchar*)mat.data;
		// Create QImage with same dimensions as input Mat
		QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
		return image.copy();
	}
	case CV_32FC1:
	{
		cv::Mat normalize_mat;
		normalize(mat, normalize_mat, 0, 255, cv::NORM_MINMAX, -1);
		mat.convertTo(normalize_mat, CV_8U);
		const uchar* pSrc = (const uchar*)normalize_mat.data;
		QImage image(pSrc, normalize_mat.cols, normalize_mat.rows, normalize_mat.step, QImage::Format_Grayscale8);
		return image.copy();
	}
	case CV_32FC3:
	{
		cv::Mat normalize_mat;
		normalize(mat, normalize_mat, 0, 255, cv::NORM_MINMAX, -1);
		mat.convertTo(normalize_mat, CV_8U);
		const uchar* pSrc = (const uchar*)normalize_mat.data;
		// Create QImage with same dimensions as input Mat
		QImage image(pSrc, normalize_mat.cols, normalize_mat.rows, normalize_mat.step, QImage::Format_RGB888);
		return image.rgbSwapped();
	}
	case CV_64FC1:
	{
		cv::Mat normalize_mat;
		normalize(mat, normalize_mat, 0, 255, cv::NORM_MINMAX, -1);
		normalize_mat.convertTo(normalize_mat, CV_8U);
		const uchar* pSrc = (const uchar*)normalize_mat.data;
		QImage image(pSrc, normalize_mat.cols, normalize_mat.rows, normalize_mat.step, QImage::Format_Grayscale8);
		return image.copy();
	}
	case CV_64FC3:
	{
		cv::Mat normalize_mat;
		normalize(mat, normalize_mat, 0, 255, cv::NORM_MINMAX, -1);
		normalize_mat.convertTo(normalize_mat, CV_8U);
		const uchar* pSrc = (const uchar*)normalize_mat.data;
		// Create QImage with same dimensions as input Mat
		QImage image(pSrc, normalize_mat.cols, normalize_mat.rows, normalize_mat.step, QImage::Format_RGB888);
		return image.rgbSwapped();
	}
	case CV_32SC1:
	{
		cv::Mat normalize_mat;
		normalize(mat, normalize_mat, 0, 255, cv::NORM_MINMAX, -1);
		normalize_mat.convertTo(normalize_mat, CV_8U);
		const uchar* pSrc = (const uchar*)normalize_mat.data;
		QImage image(pSrc, normalize_mat.cols, normalize_mat.rows, normalize_mat.step, QImage::Format_Grayscale8);
		return image.copy();
	}
	case CV_32SC3:
	{
		cv::Mat normalize_mat;
		normalize(mat, normalize_mat, 0, 255, cv::NORM_MINMAX, -1);
		normalize_mat.convertTo(normalize_mat, CV_8U);
		const uchar* pSrc = (const uchar*)normalize_mat.data;
		// Create QImage with same dimensions as input Mat
		QImage image(pSrc, normalize_mat.cols, normalize_mat.rows, normalize_mat.step, QImage::Format_RGB888);
		return image.rgbSwapped();
	}
	case CV_16SC1:
	{
		cv::Mat normalize_mat;
		normalize(mat, normalize_mat, 0, 255, cv::NORM_MINMAX, -1);
		normalize_mat.convertTo(normalize_mat, CV_8U);
		const uchar* pSrc = (const uchar*)normalize_mat.data;
		QImage image(pSrc, normalize_mat.cols, normalize_mat.rows, normalize_mat.step, QImage::Format_Grayscale8);
		return image.copy();
	}
	case CV_16SC3:
	{
		cv::Mat normalize_mat;
		normalize(mat, normalize_mat, 0, 255, cv::NORM_MINMAX, -1);
		normalize_mat.convertTo(normalize_mat, CV_8U);
		const uchar* pSrc = (const uchar*)normalize_mat.data;
		// Create QImage with same dimensions as input Mat
		QImage image(pSrc, normalize_mat.cols, normalize_mat.rows, normalize_mat.step, QImage::Format_RGB888);
		return image.rgbSwapped();
	}
	case CV_8SC1:
	{
		//Mat normalize_mat;
		//normalize(mat, normalize_mat, 0, 255, NORM_MINMAX, -1);
		mat.convertTo(mat, CV_8U);
		const uchar* pSrc = (const uchar*)mat.data;
		QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
		return image.copy();
	}
	case CV_8SC3:
	{
		mat.convertTo(mat, CV_8U);
		const uchar* pSrc = (const uchar*)mat.data;
		QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
		return image.rgbSwapped();
	}
	default:
	{
		mat.convertTo(mat, CV_8UC3);
		QImage image((const uchar*)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
		return image.rgbSwapped();
	}
	}
}

//计算各种语言的字符显示宽度
inline int EstimateDisplayWidth(const QString& str)
{
	int width = 0;
	for (QChar c : str)
	{
		uint ucs4 = c.unicode();
		//std::cout<<ucs4<<std::endl;
		if (ucs4 >= 0x0400 && ucs4 <= 0x04FF)
		{
			// 西里尔字母（俄文）→ 视为 2 格（全角）
			width += 2;
		}
		else if (ucs4 >= 0x4E00 && ucs4 <= 0x9FFF)
		{
			// 中文汉字 → 2 格
			width += 2;
		}
		else if (ucs4 >= 0x3040 && ucs4 <= 0x309F)
		{
			// 日文平假名 → 2 格
			width += 2;
		}
		else if (ucs4 >= 0x30A0 && ucs4 <= 0x30FF)
		{
			// 日文片假名 → 2 格
			width += 2;
		}
		else if ((ucs4 == 0x00B2 || ucs4 == 0x00B3 || ucs4 == 0x00B9))
		{
			// 上标数字：² U+00B2, ³ U+00B3, ¹ 0x00B9 → 半角，占 1 格
			width += 1;
		}
		else {
			// ASCII 英文、数字、标点 → 1 格
			width += 1;
		}
	}
	return width;
}

// 补全到指定 UTF-8 字符宽度，右侧补空格
inline std::string PadOrTruncate(const QString& str, int targetWidth) {
	QString result = str;
	int width = EstimateDisplayWidth(result);

	// 如果太长，截断（从末尾开始删）
	while (width > targetWidth) {
		if (result.isEmpty()) break;
		result.chop(1);
		width = EstimateDisplayWidth(result);
	}

	// 不足则补空格
	while (width < targetWidth) {
		result += ' ';
		width++;
	}

	return result.toUtf8().toStdString();
}

// 格式化浮点数：固定 4 位小数，整数部分至少 3 位，不足左侧补空格
inline std::string FormatFixedWidthFloat(double value, int integerWidth = 3, int decimalWidth = 4)
{
	// 格式化为固定小数
	QString qstr = QString::number(value, 'f', decimalWidth);

	// 分离整数和小数部分
	int dotPos = qstr.indexOf('.');
	QString integerPart = dotPos > 0 ? qstr.left(dotPos) : qstr;
	QString decimalPart = dotPos > 0 ? qstr.mid(dotPos) : "";

	// 补齐整数部分：左侧补空格
	while (integerPart.length() < integerWidth) {
		integerPart = ' ' + integerPart;
	}

	return (integerPart + decimalPart).toUtf8().toStdString();
}
#endif // GLOBAL_H