#include "defectLevel.h"
#include "Log.hpp"

defectLevel::defectLevel()
{

	//初始化缺陷等级参数
	initJudgmentRank();

	//uniSeg.init("./models/Seg.engine");
	//std::string enginePath = "./models/Seg.engine";
	//cv::Mat img = cv::imread("E:/code/C++/GlassDefectDetection/GlassDefectDetection/models/test.bmp", 0);
	//cv::Mat mask = uniSeg.seg_img(img, enginePath);

}

defectLevel::~defectLevel()
{


}

bool contains(const cv::Rect& outer, const cv::Rect& inner) {
	return outer.x <= inner.x &&
		(outer.x + outer.width) >= (inner.x + inner.width) &&
		outer.y <= inner.y &&
		(outer.y + outer.height) >= (inner.y + inner.height);
}

// 计算两个矩形中心点的欧几里得距离
double calculateRectDistance(const cv::Rect& rect1, const cv::Rect& rect2) {
	cv::Point center1(rect1.x + rect1.width / 2, rect1.y + rect1.height / 2);
	cv::Point center2(rect2.x + rect2.width / 2, rect2.y + rect2.height / 2);
	return std::sqrt(std::pow(center1.x - center2.x, 2) + std::pow(center1.y - center2.y, 2));
}

// 合并两个矩形，考虑相交和包含的情况
cv::Rect mergeRects(const cv::Rect& rect1, const cv::Rect& rect2) {
	cv::Rect unionRect = rect1 | rect2;  // 使用 OR 运算符合并矩形（考虑相交和不相交的情况）
	
	if (contains(rect1,rect2) || contains(rect1, rect2)) {
		// 如果一个矩形包含另一个，则使用较大的矩形
		cv::Rect largerRect = (rect1.area() > rect2.area()) ? rect1 : rect2;
		return largerRect & unionRect;  // 通常这里 unionRect 已经包含了 largerRect，但这个操作确保了结果的正确性（虽然在这个特定情况下是多余的）
	}
	else {
		// 否则，使用合并后的矩形（已经考虑了相交的情况）
		return unionRect;
	}
}

// 合并两个 drawInformation 对象，考虑矩形的关系和 Errorinfo 的合并
drawInformation mergeDrawInfo(const drawInformation& info1, const drawInformation& info2, double distanceThreshold) {
	if (info1.DefectType != info2.DefectType) {
		throw std::invalid_argument("Cannot merge drawInformation with different DefectType");
	}

	cv::Rect mergedRect = mergeRects(info1.rect, info2.rect);

	// 如果两个矩形不相交也不在距离阈值内，则不应该合并它们（这里假设已经通过其他逻辑确保了它们在阈值内）
	// 但为了完整性，我们可以添加一个检查来确保合并是有意义的
	// (然而，在当前的上下文中，这个检查是不必要的，因为调用此函数的代码应该已经过滤了这些情况)

	drawInformation mergedInfo;
	mergedInfo.rect = mergedRect;
	mergedInfo.ErrorType = (info1.ErrorType < info2.ErrorType) ? info1.ErrorType : info2.ErrorType;
	mergedInfo.DefectType = info1.DefectType;

	// 合并 Errorinfo，这里简单地将它们连接起来，可能需要根据实际情况调整
	mergedInfo.Errorinfo.insert(mergedInfo.Errorinfo.end(), info1.Errorinfo.begin(), info1.Errorinfo.end());
	mergedInfo.Errorinfo.insert(mergedInfo.Errorinfo.end(), info2.Errorinfo.begin(), info2.Errorinfo.end());

	return mergedInfo;
}


// 合并同类缺陷,并根据结果进行缺陷等级调整
void mergeCloseInfo(std::vector<drawInformation>& saveInformations, std::vector<drawInformation>& outputInfo) {
	int distanceThreshold = 50;
	std::vector<drawInformation> mergedInformations;
	std::vector<bool> visited(saveInformations.size(), false);

	for (size_t i = 0; i < saveInformations.size(); ++i) {
		if (visited[i]) continue;

		std::vector<drawInformation> cluster;
		cluster.push_back(saveInformations[i]);
		visited[i] = true;

		for (size_t j = i + 1; j < saveInformations.size(); ++j) {
			if (!visited[j] && saveInformations[i].DefectType == saveInformations[j].DefectType) {
				cv::Rect combinedRect = mergeRects(saveInformations[i].rect, saveInformations[j].rect);
				if (combinedRect.area() != 0 &&  // 确保合并后的矩形不是空的（虽然在这个特定情况下通常不会发生）
					(saveInformations[i].rect & saveInformations[j].rect).area() > 0 ||  // 相交
					contains(saveInformations[i].rect,saveInformations[j].rect) ||  // 包含
					contains(saveInformations[j].rect,saveInformations[i].rect) ||  // 被包含
					calculateRectDistance(saveInformations[i].rect, saveInformations[j].rect) <= distanceThreshold) {  // 不相交但在距离阈值内

					cluster.push_back(saveInformations[j]);
					visited[j] = true;
				}
			}
		}

		if (cluster.size() > 1) {
			drawInformation mergedInfo = cluster[0];
			for (size_t k = 1; k < cluster.size(); ++k) {
				mergedInfo = mergeDrawInfo(mergedInfo, cluster[k], distanceThreshold);
			}
			mergedInformations.push_back(mergedInfo);
		}
		else {
			mergedInformations.push_back(cluster[0]);
		}
	}

}



/// <summary>
/// 初始化各缺陷等级参数
/// </summary>
void defectLevel::initJudgmentRank()
{

}



/// <summary>
///缺陷等级判断参数，当前判断参数包含：面积、最长长度，周长，其中长度用于判断划痕，周长用于判断崩边
/// </summary>
/// <param name="rankJudgment"></param>
/// <param name="SetAreaRank"></param>
/// <param name="SetLengthRank"></param>
/// <param name="SetPerimeterRank"></param>
void defectLevel::setRankJudgment(RankJudgment& rankJudgment, std::vector<float>& SetAreaRank, std::vector<float>& SetLengthRank, std::vector<float>& SetPerimeterRank)
{
	rankJudgment.areaRank = SetAreaRank;

	rankJudgment.LengthRank = SetLengthRank;

	rankJudgment.PerimeterRank = SetPerimeterRank;

}


//特征计算
/// <summary>
/// 面积
/// </summary>
/// <param name="contour"></param>
/// <returns></returns>
float getContourArea(std::vector<cv::Point>& contour) {
	return cv::contourArea(contour,true);
}

/// <summary>
/// 获取mask区域的像素面积
/// </summary>
/// <param name="mask"></param>
/// <returns></returns>
float defectLevel::getMaskArea(cv::Mat& mask) {

	float area = 0;
	for (int i = 0; i < mask.rows; i++) {

		for (int j = 0; j < mask.cols; j++) {

			if (mask.at<uchar>(i, j) == 255) {

				area++;

			}

		}

	}

	return area;
}

/// <summary>
/// 周长
/// </summary>
/// <param name="contour"></param>
/// <returns></returns>
float getContourLength(std::vector<cv::Point>& contour) {
	return cv::arcLength(contour, true);
}

/// <summary>
/// 圆形度计算
/// </summary>
/// <param name="contour"></param>
/// <returns></returns>
float getContourRoundness(std::vector<cv::Point>& contour) {

	float area = getContourArea(contour);//计算轮廓面积

	float len = getContourLength(contour);//计算轮廓周长

	return  (4 * CV_PI * area) / (len * len);
}


/// <summary>
/// 矩形度计算
/// </summary>
/// <param name="contour"></param>
/// <returns></returns>
float getContourRectangularity(std::vector<cv::Point>& contour) {
	float rectangularity;
	//先计算最小外接矩形的面积：
	cv::RotatedRect minrect = cv::minAreaRect(contour);    //最小外接矩形
	float area = getContourArea(contour);//计算轮廓面积
	int minrectmianji = minrect.size.height * minrect.size.width;
	if (minrectmianji == 0)rectangularity = 0;
	else rectangularity = area / minrectmianji;
	return rectangularity;
}



float getScratchLenght(cv::Mat& maskImage) {

	cv::Mat morph_image;
	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3), cv::Point(-1, -1));
	morphologyEx(maskImage, morph_image, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 1);

	//查找所有外轮廓
	std::vector< std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hireachy;
	findContours(morph_image, contours, hireachy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE, cv::Point());

	float sumLength = 0.0;
	for (size_t t = 0; t < contours.size(); t++)
	{

		float L = getContourLength(contours[t]);
		sumLength += L;

	}
	//std::cout << sumLength << std::endl;
	return sumLength;
}

/// <summary>
/// 计算图像对角线长度
/// </summary>
/// <param name="maskImage"></param>
/// <returns></returns>
float getDiagonalLength(cv::Mat& maskImage) {

	int width = maskImage.cols;  // 图像宽度
	int height = maskImage.rows; // 图像高度

	return sqrtf(width * width + height * height);
}

/// <summary>
/// <summary>
/// 计算非rect区域的平均灰度值
/// </summary>
/// <param name="img"></param>
/// <param name="rect"></param>
/// <param name="threshold"></param>
/// <returns></returns>
double calculateEdgeOrOuterMean(const cv::Mat& img, const cv::Rect& rect, uchar threshold = 100) {
	CV_Assert(img.type() == CV_8UC1); // 确保是单通道图像
	CV_Assert(!img.empty());

	int rows = img.rows;
	int cols = img.cols;

	cv::Mat mask = cv::Mat::zeros(img.size(), CV_8U);

	if (rect.x <= 0 && rect.y <= 0 && rect.width >= cols && rect.height >= rows) {
		//std::cout << "Rect covers the entire image, sampling inner edge." << std::endl;

		// 只保留图像最外层的一圈像素
		cv::rectangle(mask, cv::Rect(0, 0, cols, 1), cv::Scalar(255), -1);         // 上边
		cv::rectangle(mask, cv::Rect(0, rows - 1, cols, 1), cv::Scalar(255), -1); // 下边
		cv::rectangle(mask, cv::Rect(0, 0, 1, rows), cv::Scalar(255), -1);         // 左边
		cv::rectangle(mask, cv::Rect(cols - 1, 0, 1, rows), cv::Scalar(255), -1); // 右边
	}
	else {
		//std::cout << "Sampling outside of rect." << std::endl;

		// 创建掩膜：全图设置为白色
		mask.setTo(cv::Scalar(255));

		// 将 rect 区域设为黑色（排除）
		cv::rectangle(mask, rect, cv::Scalar(0), -1);
	}

	// 应用灰度阈值限制：只保留大于 threshold 的像素
	cv::Mat binaryMask;
	cv::threshold(img, binaryMask, threshold, 255, cv::THRESH_BINARY);

	// 组合两个掩膜：只保留“mask & binaryMask”部分
	cv::Mat finalMask;
	cv::bitwise_and(mask, binaryMask, finalMask);

	// 计算最终掩膜区域内像素的平均灰度值
	double meanVal = cv::mean(img, finalMask)[0];

	return meanVal;
}


/// <summary>
/// 计算mask区域的平均灰度值
/// </summary>
/// <param name="grayImage"></param>
/// <param name="binaryMask"></param>
/// <param name="meanA"></param>
/// <param name="meanB"></param>
void calculateMeansUsingBinaryMask(
	const cv::Mat& grayImage,
	const cv::Mat& binaryMask,
	double& meanA,
	double& meanB)
{
	CV_Assert(grayImage.type() == CV_8UC1 && binaryMask.type() == CV_8UC1);
	CV_Assert(grayImage.size() == binaryMask.size());

	// 创建掩膜：Region A (mask=255) 和 Region B (mask=0)
	cv::Mat maskA = binaryMask.clone();
	cv::Mat maskB;
	cv::bitwise_not(binaryMask, maskB); // 取反得到 Region B 的掩膜

	// 计算 Region A 平均值图中255
	meanA = cv::mean(grayImage, maskA)[0];

	// 计算 Region B 平均值
	meanB = cv::mean(grayImage, maskB)[0];
}

// 根据 ConnectedComponentItem 获取最大连通域的矩形
cv::Rect getMaxConnectedComponentRect(const ConnectedComponentItem& ccAnalysis, int minArea = 0) {
	cv::Rect maxRect;
	int maxArea = 0;
	int maxIndex = -1;

	// 遍历所有连通域（从1开始，0是背景）
	for (int i = 1; i < ccAnalysis.stats.rows; ++i) {
		int area = ccAnalysis.stats.at<int>(i, cv::CC_STAT_AREA);

		// 检查是否满足最小面积要求且面积最大
		if (area >= minArea && area > maxArea) {
			maxArea = area;
			maxIndex = i;
		}
	}

	// 如果找到了最大连通域
	if (maxIndex != -1) {
		maxRect = cv::Rect(
			ccAnalysis.stats.at<int>(maxIndex, cv::CC_STAT_LEFT),
			ccAnalysis.stats.at<int>(maxIndex, cv::CC_STAT_TOP),
			ccAnalysis.stats.at<int>(maxIndex, cv::CC_STAT_WIDTH),
			ccAnalysis.stats.at<int>(maxIndex, cv::CC_STAT_HEIGHT)
		);
	}

	return maxRect;
}

///对二值化图像进行连通域分析
/// </summary>
/// <param name="bw">输入二值化图像</param>
/// <param name="minArea">剔除面积较小的连通域</param>
/// <param name="ccAnalysis">连通域分析结果</param>
/// <param name="flag">是否返回包含所有连通域结果的mask</param>
/// <returns> 0 正确计算 -1 错误计算</returns>
int defectLevel::ConnectedComponentAnalysis(cv::Mat& bw, int minArea, ConnectedComponentItem& ccAnalysis,int flag) {

	try
	{
		ccAnalysis.numComponent = connectedComponentsWithStats(bw, ccAnalysis.labels, ccAnalysis.stats, ccAnalysis.centroids);

		if (flag == 1) {

			//这部分得到所有连通域融合的maks可以不要
			cv::Mat filteredBW(bw.rows, bw.cols, CV_8UC1, cv::Scalar(0));
			for (int i = 1; i < ccAnalysis.stats.rows; ++i) {
				int a = ccAnalysis.stats.at<int>(i, cv::CC_STAT_AREA);
				if (a >= minArea) {
					cv::Mat mask = (ccAnalysis.labels == i) * 255;
					bitwise_or(filteredBW, mask, filteredBW);
				}
			}
			ccAnalysis.mask = filteredBW;

		}

	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}
	catch (...) {
		std::cerr << "未知异常" << std::endl;
		return -1;
	}
	return 0;
}



/// <summary>
/// 仅获取缺陷信息
/// </summary>
/// <param name="inputMaskImage"></param>
/// <param name="type"></param>
/// <param name="errInfo"></param>
/// <returns></returns>
//int defectLevel::GetLevelInfo(cv::Mat& inputMaskImage, DefectType type, std::vector<float>& errInfo) {
//
//	DefectLevel levelJud = ABNORMAL;
//
//	ConnectedComponentItem ccAnalysis;
//	int minArea = 5;
//	int AnalysisFlag = ConnectedComponentAnalysis(inputMaskImage, minArea, ccAnalysis, 1);
//
//	float area = 0.0; // 面积
//	float length = 0.0;//长度
//	float perimeter = 0.0;//对角线长度
//
//	//RankJudgment PoorCoating;//镀膜不良 0    判断标准：面积
//	//RankJudgment scratch;//划伤 1			   判断标准：对角线长度
//	//RankJudgment calculus;//结石 2		   判断标准：对角线长度
//	//RankJudgment bubble;//气泡3			   判断标准：对角线长度
//	//RankJudgment WaterStain;//水渍 5         判断标准：面积
//	//RankJudgment smudge;//脏污 6			   判断标准：面积
//
//	if (type == DefectType::TYPE_POORCOATING) {			//		typeName = "镀膜不良";  面积两种判断
//
//		area = getMaskArea(inputMaskImage);
//		perimeter = getDiagonalLength(inputMaskImage);
//
//	}
//	else if (type == DefectType::TYPE_SCRATCH) {		//		typeName = "划伤";	单划痕时长度判断	多划痕时，对角线判断
//
//		//if (ccAnalysis.numComponent > 2) {//非单个划痕,采用对角线判断
//		//if (false) {//非单个划痕,采用对角线判断,由于软件为进行接口流出，暂不启用
//		perimeter = getDiagonalLength(inputMaskImage);
//		area = getMaskArea(inputMaskImage);
//
//	}
//	else if (type == DefectType::TYPE_CALCULUS) {		//		typeName = "结石";		对角线判断
//
//		//area = getMaskArea(inputMaskImage) * mappingScale * mappingScale;
//		perimeter = getDiagonalLength(inputMaskImage);
//		area = getMaskArea(inputMaskImage);
//
//	}
//	else if (type == DefectType::TYPE_BUBBLE) {			//		typeName = "气泡";		对角线判断
//		perimeter = getDiagonalLength(inputMaskImage);
//		area = getMaskArea(inputMaskImage);
//	}
//	else if (type == DefectType::TYPE_TRADEMARK) {		//		typeName = "商标";		无需判断
//
//		levelJud = DefectLevel::MINOR;//轻微 
//		area = getMaskArea(inputMaskImage);
//		perimeter = getDiagonalLength(inputMaskImage);
//	}
//	else if (type == DefectType::TYPE_WATERSTAIN) {		//		typeName = "水渍";		面积或单边长度判断
//
//		area = getMaskArea(inputMaskImage);
//		perimeter = getDiagonalLength(inputMaskImage);
//
//	}
//	else if (type == DefectType::TYPE_SMUDGE || type == DefectType::TYPE_SCREENPRINTING) {			//		typeName = "脏污";		面积判断
//
//		area = getMaskArea(inputMaskImage);
//		perimeter = getDiagonalLength(inputMaskImage);
//
//	}
//
//	//将数据（依次为 面积 长轴  周长） 依次存入vector中用于图像上绘制显示
//	errInfo.push_back(area);
//
//	///TODO判断缺陷类型，选择合适的度量标准
//	errInfo.push_back(length);
//	errInfo.push_back(perimeter);
//
//	return levelJud;
//}

/// <summary>
/// 通用版本缺陷信息计算，除商标外，商标单独处理，其余所有参数都进行计算
/// </summary>
/// <param name="inputMaskImage"></param>
/// <param name="type"></param>
/// <param name="errInfo"></param>
/// <returns></returns>
int defectLevel::GetLevelInfo(cv::Mat& inputMaskImage, DefectType type, std::vector<float>& errInfo) {

	DefectLevel levelJud = ABNORMAL;

	ConnectedComponentItem ccAnalysis;
	int minArea = 5;
	int AnalysisFlag = ConnectedComponentAnalysis(inputMaskImage, minArea, ccAnalysis, 1);

	float area = 0.0; // 面积
	float length = 0.0;//长度
	float perimeter = 0.0;//对角线长度

	//RankJudgment PoorCoating;//镀膜不良 0    判断标准：面积
	//RankJudgment scratch;//划伤 1			   判断标准：对角线长度
	//RankJudgment calculus;//结石 2		   判断标准：对角线长度
	//RankJudgment bubble;//气泡3			   判断标准：对角线长度
	//RankJudgment WaterStain;//水渍 5         判断标准：面积
	//RankJudgment smudge;//脏污 6			   判断标准：面积


	if (type == DefectType::TYPE_TRADEMARK) {		//		typeName = "商标";		无需判断

		levelJud = DefectLevel::MINOR;//轻微 
		area = getMaskArea(inputMaskImage);
		length = getDiagonalLength(inputMaskImage);//长度暂时都由对角线长度替代
		perimeter = getDiagonalLength(inputMaskImage);
	}else if (type == DefectType::TYPE_CALCULUS|| type == DefectType::TYPE_BUBBLE) {//20251210气泡结石 以最大连通域rect求解
		area = getMaskArea(inputMaskImage);

		cv::Rect maxRect = getMaxConnectedComponentRect(ccAnalysis, 0);
		length = sqrtf(maxRect.width * maxRect.width + maxRect.height * maxRect.height);//长度暂时都由对角线长度替代
		perimeter = sqrtf(maxRect.width * maxRect.width + maxRect.height * maxRect.height);
		if (length == 0 || perimeter == 0) {
			length = getDiagonalLength(inputMaskImage);//长度暂时都由对角线长度替代
			perimeter = getDiagonalLength(inputMaskImage);
		}
	}
	else {
		area = getMaskArea(inputMaskImage);
		length = getDiagonalLength(inputMaskImage);//长度暂时都由对角线长度替代
		perimeter = getDiagonalLength(inputMaskImage);
	}

	//将数据（依次为 面积 长轴  周长） 依次存入vector中用于图像上绘制显示
	errInfo.push_back(area);

	///TODO判断缺陷类型，选择合适的度量标准
	errInfo.push_back(length);
	errInfo.push_back(perimeter);

	return levelJud;
}


/// <summary>
/// 
/// </summary>
/// <param name="inputImage"></param>
/// <param name="rect"></param>
/// <param name="AnalysisInfo"></param>
/// <param name="TypeValue"></param>
/// <param name="levelType"></param>
/// <param name="errInfo"></param>
/// <returns></returns>
int defectLevel::secondDetect(cv::Mat& inputImage, cv::Rect& rect, ConnectedComponentItem& AnalysisInfo, DefectType& TypeValue, DefectLevel& levelType,std::vector<float>& errInfo)
{
	//判断缺陷的亮度，若较亮则通过宽高比，若宽高比接近1:1,则为镀膜不良，若不接近则为划伤
	//若较暗则为脏污、气泡，若宽高有一个小于5个像素则直接按照脏污处理

	cv::Mat roiImg = inputImage(rect);
	double avgGary = calculateEdgeOrOuterMean(inputImage,rect,30);
	double meanA, meanB;
	calculateMeansUsingBinaryMask(roiImg, AnalysisInfo.mask, meanA, meanB);

	//if ((meanA > 200 && meanA > avgGary) || (meanB > 200 && meanB > avgGary)) {
	//	TypeValue = DefectType::TYPE_SCRATCH;//优先划伤
	//	return 1;
	//}
	for (int i = 0; i < AnalysisInfo.stats.rows; i++) {

		if (AnalysisInfo.stats.at<int32_t>(i, 0) == 0 && AnalysisInfo.stats.at<int32_t>(i, 1) == 0) {
			continue;
		}
		else {

            if ((TypeValue!= DefectType::TYPE_SMUDGE) && (std::min<int>(AnalysisInfo.stats.at<int32_t>(i, 2), AnalysisInfo.stats.at<int32_t>(i, 3)) < 5)) {
				TypeValue = DefectType::TYPE_SMUDGE;
				return 1;
			}
		}
	}

	return 0;
}




/// <summary>
/// 针对缺陷特性进行二次错误类型判断
/// 缺陷判断标准
//1.针对白色点
//镀膜不良判断条件：
//(1)其亮度过亮灰度值255且占比60 % 以上
//(2)其尺寸大小不能过小，小于4个像素
//满足上述两点可认为是镀膜不良
//不满足上述任意一条均判别为脏污
//2.对于气泡、结石、水渍这一类特征相似，若其置信度小于0.8直接将其判别为脏污，若面积小于4个像素也判断为脏污
//3.为避免干扰线被误判为划伤，对划伤进行二次判断，若其高度较小
//4. 针对颜色较浅的脏污、
/// </summary>
/// <param name="inputImage"></param>
/// <param name="rect"></param>
/// <param name="AnalysisInfo"></param>
/// <param name="TypeValue"></param>
/// <param name="levelType"></param>
/// <param name="errInfo"></param>
/// <param name="confidence"></param>
/// <returns></returns>
int defectLevel::DoubleChangeErrorType(cv::Mat& inputImage, cv::Rect& rect, ConnectedComponentItem& AnalysisInfo, DefectType& TypeValue, DefectLevel& levelType, std::vector<float>& errInfo,double confidence)
{

	return 0;

}

/// <summary>
/// 针对缺陷特性进行二次错误类型判断
/// 
/// 西班牙现场：
/// 1.存在干水渍，其颜色非常浅但由于区域过大，导致其面积较大严重程度升高
/// 
/// </summary>
/// <param name="channel0"></param>
/// <param name="channel1"></param>
/// <param name="channel2"></param>
/// <param name="rect"></param>
/// <param name="AnalysisInfo"></param>
/// <param name="TypeValue"></param>
/// <param name="levelType"></param>
/// <param name="errInfo"></param>
/// <param name="confidence"></param>
/// <returns></returns>
int defectLevel::DoubleChangeErrorType(cv::Mat& channel0, cv::Mat& channel1, cv::Mat& channel2, cv::Rect& rect, ConnectedComponentItem& AnalysisInfo, DefectType& TypeValue, DefectLevel& levelType, std::vector<float>& errInfo, double confidence,int DifGrayVal)
{

	if (TypeValue == DefectType::TYPE_SMUDGE) {//仅对脏污进行二次判断，其他缺陷暂不进行二次判断，调整脏污等级

		// 检查输入参数
		if (channel2.empty() || channel0.empty() || channel1.empty()) {
			//std::cout << "错误：channel2为空" << std::endl;
			return -1;
		}

		// 确保rect在图像范围内
		cv::Rect safeRect = rect & cv::Rect(0, 0, channel2.cols, channel2.rows);
		if (safeRect.area() <= 0) {
			//std::cout << "错误：rect区域无效" << std::endl;
			return -1;
		}

		// 计算区域内平均灰度
		cv::Mat roiRegion0 = channel0(safeRect);
		cv::Mat roiRegion1 = channel1(safeRect);
		cv::Mat roiRegion2 = channel2(safeRect);
		cv::Scalar meanInside0 = cv::mean(roiRegion0);
		cv::Scalar meanInside1 = cv::mean(roiRegion1);
		cv::Scalar meanInside2 = cv::mean(roiRegion2);
		double meanInsideVal0 = meanInside0[0];
		double meanInsideVal1 = meanInside1[0];
		double meanInsideVal2 = meanInside2[0];

		// 创建掩码：区域内为0，区域外为1
		cv::Mat mask = cv::Mat::ones(channel2.size(), CV_8UC1) * 255;
		mask(safeRect) = 0;  // 区域内设为0（不参与计算）

		double meanOutsideVal0 = cv::mean(channel0, mask)[0];

		double meanInsideVal;//区域内灰度平均值
		double meanOutsideVal;//区域外灰度平均值
		// 根据玻璃类型选择使用的亮场通道
		if (meanInsideVal0 > meanInsideVal1) {  // 白玻/无膜
			meanInsideVal = meanInsideVal1;      // 使用亮场1的区域内值
			meanOutsideVal = cv::mean(channel1, mask)[0];  // 使用亮场1的区域外值
			//std::cout << "白玻/无膜玻璃，使用channel1" << std::endl;
		}
		else {  // 镀膜玻璃
			meanInsideVal = meanInsideVal2;      // 使用亮场2的区域内值
			meanOutsideVal = cv::mean(channel2, mask)[0];  // 使用亮场2的区域外值
			//std::cout << "镀膜玻璃，使用channel2" << std::endl;
		}


		// 计算灰度差距
		double grayDiff = std::abs(meanInsideVal - meanOutsideVal);
		double grayDiff0 = std::abs(meanInsideVal0 - meanOutsideVal0);
		if ((grayDiff < static_cast<double>(DifGrayVal)) && (grayDiff0 < static_cast<double>(DifGrayVal))) {

			levelType = DefectLevel::MINOR; // 调整为轻微
			
			// 保存前两个元素的值（如果存在）
			float val0 = errInfo.size() > 0 ? errInfo[0] : 0.0f;
			float val1 = errInfo.size() > 1 ? errInfo[1] : 0.0f;

			// 重置为3个元素，保留前两个值，第三个设为-1
			errInfo = { val0, val1, -1 };

		}

	}

	return 0;

}

/// <summary>
/// 
/// </summary>
/// <param name="inputImage"></param>
/// <param name="rect"></param>
/// <param name="type"></param>
/// <param name="AnalysisInfo"></param>
/// <param name="errInfo"></param>
/// <returns></returns>
DefectLevel defectLevel::defectLevelProcessSingle(cv::Mat& inputImage, cv::Rect& rect, DefectType& type, ConnectedComponentItem& AnalysisInfo, std::vector<float>& errInfo) {
	//RankJudgment PoorCoating;//镀膜不良 0    判断标准：面积
	//RankJudgment scratch;//划伤 1			   判断标准：对角线长度
	//RankJudgment calculus;//结石 2		   判断标准：对角线长度
	//RankJudgment bubble;//气泡3			   判断标准：对角线长度
	//RankJudgment WaterStain;//水渍 5         判断标准：面积
	//RankJudgment smudge;//脏污 6			   判断标准：面积

	DefectLevel levelJud = ABNORMAL;

	float area = 0.0; // 面积
	float length = 0.0;//长度
	float perimeter = 0.0;//对角线长度
	if (errInfo.size() == 0) {
		errInfo.push_back(area);
		errInfo.push_back(length);
		errInfo.push_back(perimeter);
	}
	if (type == DefectType::TYPE_SCRATCH || type == DefectType::TYPE_CALCULUS || type == DefectType::TYPE_BUBBLE) {
		//对角线判断使用rect矩形框的对角线
		int width = rect.width;
		int height = rect.height;
		double diagonalLength = std::sqrt(width * width + height * height);

		if (type == DefectType::TYPE_SCRATCH) {		//		typeName = "划伤";	单划痕时长度判断	多划痕时，对角线判断

			perimeter = diagonalLength;

		}
		else if (type == DefectType::TYPE_CALCULUS) {		//		typeName = "结石";		对角线判断

			//area = getMaskArea(inputMaskImage) * mappingScale * mappingScale;
			perimeter = diagonalLength;
		}
		else if (type == DefectType::TYPE_BUBBLE) {			//		typeName = "气泡";		对角线判断
			perimeter = diagonalLength;
		}
		errInfo[1] = perimeter;

	}
	else {
		cv::Mat roiImg = inputImage(rect);
		double avgGary = calculateEdgeOrOuterMean(inputImage, rect, 30);
		
		cv::Mat binaryImg;

		// 设置阈值
		double maxValue = 255;
		// 二值化
		cv::threshold(roiImg, binaryImg, avgGary, maxValue, cv::THRESH_BINARY);
		double meanA, meanB;
		calculateMeansUsingBinaryMask(roiImg, binaryImg,meanA,meanB);
		// 计算白色区域像素个数
		int whitePixels = cv::countNonZero(binaryImg);
		if (abs(avgGary - meanA) < abs(avgGary - meanB)) {
			area = whitePixels;
		}
		else {
			area = rect.width* rect.height;
		}
		errInfo[0] = area;
	}

	return NORMAL;
}

DefectLevel defectLevel::defectLevelProcessByEdge(cv::Mat& inputImage, cv::Rect& rect, DefectType& type, ConnectedComponentItem& AnalysisInfo, std::vector<float>& errInfo)
{
	cv::Mat img = inputImage.clone();
	cv::Mat maskImage;
	segUtils.GetCalculateResult(img, maskImage,25);
	//Roi区域提取
	//cv::Mat roiImage = img(rect);

	cv::Mat thresholdImg = maskImage(rect);


	int minArea = 5;
	ConnectedComponentItem ccAnalysis;
	int AnalysisFlag = ConnectedComponentAnalysis(thresholdImg, minArea, ccAnalysis, 1);

	if (AnalysisFlag != 0) {

		std::cerr << "Connected Component Analysis Error." << std::endl;
		errInfo.push_back(0.0);
		errInfo.push_back(0.0);
		errInfo.push_back(0.0);

		return AREAERROR;

	}
	//根据类别对分割图像进行分析，得到缺陷等级
	//DefectLevel resultLevel = TypeLevelJudgment(ccAnalysis.mask, type, errInfo);
	int flag = GetLevelInfo(ccAnalysis.mask, type, errInfo);

	AnalysisInfo = ccAnalysis;

	////绘制分割结果至Roi图像上
	//drawRoiImage = roiImage.clone();
	//gmUtils.drawDefectImage(drawRoiImage, ccAnalysis.mask, resultLevel);

	return DefectLevel::NORMAL;
}


std::vector<drawInformation> defectLevel::GetDefectLevel(std::vector<drawInformation>& data, PrescriptionParameter& levelRankInfo) {

	//像素转mm
	for (int i = 0; i < data.size(); i++) {
		data[i].Errorinfo[0] = data[i].Errorinfo[0] * levelRankInfo.x_pixel2mm * levelRankInfo.y_pixel2mm;
		data[i].Errorinfo[1] = data[i].Errorinfo[1] * levelRankInfo.x_pixel2mm;
		data[i].Errorinfo[2] = data[i].Errorinfo[2] * levelRankInfo.x_pixel2mm;
	}

	std::vector<int> removeIndexNumber;
	for (int i = 0; i < data.size(); i++) {

		//RankJudgment PoorCoating;//镀膜不良 0    判断标准：面积
		//RankJudgment scratch;//划伤 1			   判断标准：对角线长度
		//RankJudgment calculus;//结石 2		   判断标准：对角线长度
		//RankJudgment bubble;//气泡3			   判断标准：对角线长度
		//RankJudgment WaterStain;//水渍 5         判断标准：面积
		//RankJudgment smudge;//脏污 6			   判断标准：面积
		DefectType type = data[i].DefectType;

		//if (type > DefectType::TYPE_SMUDGE) {//20251204避免模型类别在软件中未定义，仅测试时使用
		//	type = DefectType::TYPE_SMUDGE;
		//	data[i].DefectType = DefectType::TYPE_SMUDGE;
		//}

		float area = 0.0; // 面积
		float length = 0.0;//长度
		float perimeter = 0.0;//对角线长度
		area = data[i].Errorinfo[0];
		length = data[i].Errorinfo[1];
		perimeter = data[i].Errorinfo[2];
		if (type == DefectType::TYPE_POORCOATING) {			//		typeName = "镀膜不良";  面积两种判断

			if (area < levelRankInfo.PoorCoating.areaRank[1] || levelRankInfo.PoorCoating.areaRank[1] == 0.0) {
				removeIndexNumber.push_back(i);
			}
			if (area >= levelRankInfo.PoorCoating.areaRank[1] && area < levelRankInfo.PoorCoating.areaRank[2]) {
				data[i].ErrorType = DefectLevel::MINOR;//轻微
			}
			if (area >= levelRankInfo.PoorCoating.areaRank[2] && area < levelRankInfo.PoorCoating.areaRank[3]) {

				data[i].ErrorType = DefectLevel::MEDIUM;//警告
			}
			if (area >= levelRankInfo.PoorCoating.areaRank[3]) {

				data[i].ErrorType = DefectLevel::SERIOUS;//严重
			}

		}
		else if (type == DefectType::TYPE_SCRATCH) {		//		typeName = "划伤";	单划痕时长度判断	多划痕时，对角线判断

			if (perimeter < levelRankInfo.scratch.PerimeterRank[1] || levelRankInfo.scratch.PerimeterRank[1] == 0.0) {
				removeIndexNumber.push_back(i);
			}
			if (perimeter >= levelRankInfo.scratch.PerimeterRank[1] && perimeter < levelRankInfo.scratch.PerimeterRank[2]) {
				data[i].ErrorType = DefectLevel::MINOR;//轻微
			}
			if (perimeter >= levelRankInfo.scratch.PerimeterRank[2] && perimeter < levelRankInfo.scratch.PerimeterRank[3]) {

				data[i].ErrorType = DefectLevel::MEDIUM;//警告
			}
			if (perimeter >= levelRankInfo.scratch.PerimeterRank[3]) {

				data[i].ErrorType = DefectLevel::SERIOUS;//严重
			}

		}
		else if (type == DefectType::TYPE_CALCULUS) {		//		typeName = "结石";		对角线判断
			if (perimeter < levelRankInfo.calculus.PerimeterRank[1] || levelRankInfo.calculus.PerimeterRank[1] == 0.0) {
				removeIndexNumber.push_back(i);
			}
			if (perimeter >= levelRankInfo.calculus.PerimeterRank[1] && perimeter < levelRankInfo.calculus.PerimeterRank[2]) {
				data[i].ErrorType = DefectLevel::MINOR;//轻微
			}
			if (perimeter >= levelRankInfo.calculus.PerimeterRank[2] && perimeter < levelRankInfo.calculus.PerimeterRank[3]) {

				data[i].ErrorType = DefectLevel::MEDIUM;//警告
			}
			if (perimeter >= levelRankInfo.calculus.PerimeterRank[3]) {

				data[i].ErrorType = DefectLevel::SERIOUS;//严重
			}
		}
		else if (type == DefectType::TYPE_BUBBLE) {			//		typeName = "气泡";		对角线判断
			if (perimeter < levelRankInfo.bubble.PerimeterRank[1] || levelRankInfo.bubble.PerimeterRank[1] == 0.0) {
				removeIndexNumber.push_back(i);
			}
			if (perimeter >= levelRankInfo.bubble.PerimeterRank[1] && perimeter < levelRankInfo.bubble.PerimeterRank[2]) {
				data[i].ErrorType = DefectLevel::MINOR;//轻微
			}
			if (perimeter >= levelRankInfo.bubble.PerimeterRank[2] && perimeter < levelRankInfo.bubble.PerimeterRank[3]) {

				data[i].ErrorType = DefectLevel::MEDIUM;//警告
			}
			if (perimeter >= levelRankInfo.bubble.PerimeterRank[3]) {

				data[i].ErrorType = DefectLevel::SERIOUS;//严重
			}
		}
		else if (type == DefectType::TYPE_TRADEMARK) {		//		typeName = "商标";		无需判断

			data[i].ErrorType = DefectLevel::MINOR;//轻微 

		}
		else if (type == DefectType::TYPE_WATERSTAIN) {		//		typeName = "水渍";		面积或单边长度判断
			if (area < levelRankInfo.WaterStain.areaRank[1] || levelRankInfo.WaterStain.areaRank[1] == 0.0) {
				removeIndexNumber.push_back(i);
			}
			if (area >= levelRankInfo.WaterStain.areaRank[1] && area < levelRankInfo.WaterStain.areaRank[2]) {
				data[i].ErrorType = DefectLevel::MINOR;//轻微
			}
			if (area >= levelRankInfo.WaterStain.areaRank[2] && area < levelRankInfo.WaterStain.areaRank[3]) {

				data[i].ErrorType = DefectLevel::MEDIUM;//警告
			}
			if (area >= levelRankInfo.WaterStain.areaRank[3]) {

				data[i].ErrorType = DefectLevel::SERIOUS;//严重
			}

		}
		else if (type == DefectType::TYPE_SMUDGE) {			//		typeName = "脏污";		面积判断

			if (area < levelRankInfo.smudge.areaRank[1] || levelRankInfo.smudge.areaRank[1] == 0.0) {
				removeIndexNumber.push_back(i);
			}

			//if (perimeter > 0) {//西班牙二次判断中将颜色较浅缺陷，最后一个值赋值为-1.0作为二次判断依据
			if (area >= levelRankInfo.smudge.areaRank[1] && area < levelRankInfo.smudge.areaRank[2]) {
				data[i].ErrorType = DefectLevel::MINOR;//轻微
			}
			if (area >= levelRankInfo.smudge.areaRank[2] && area < levelRankInfo.smudge.areaRank[3]) {

				data[i].ErrorType = DefectLevel::MEDIUM;//警告
			}
			if (area >= levelRankInfo.smudge.areaRank[3]) {

				data[i].ErrorType = DefectLevel::SERIOUS;//严重
			}
			//}
			//else {
			//	data[i].ErrorType = DefectLevel::MINOR;//轻微
			//}
		}
		else if (type == DefectType::TYPE_SCREENPRINTING) {//7
			if (area < levelRankInfo.ScreenPrinting.areaRank[1] || levelRankInfo.ScreenPrinting.areaRank[1] == 0.0) {
				removeIndexNumber.push_back(i);
			}
			if (area >= levelRankInfo.ScreenPrinting.areaRank[1] && area < levelRankInfo.ScreenPrinting.areaRank[2]) {
				data[i].ErrorType = DefectLevel::MINOR;//轻微
			}
			if (area >= levelRankInfo.ScreenPrinting.areaRank[2] && area < levelRankInfo.ScreenPrinting.areaRank[3]) {

				data[i].ErrorType = DefectLevel::MEDIUM;//警告
			}
			if (area >= levelRankInfo.ScreenPrinting.areaRank[3]) {

				data[i].ErrorType = DefectLevel::SERIOUS;//严重
			}

		}
		else if (type == DefectType::TYPE_CHIPPED_EDGE) {//8
			if (area < levelRankInfo.ChippedEdge.areaRank[1] || levelRankInfo.ChippedEdge.areaRank[1] == 0.0) {
				removeIndexNumber.push_back(i);
			}
			if (area >= levelRankInfo.ChippedEdge.areaRank[1] && area < levelRankInfo.ChippedEdge.areaRank[2]) {
				data[i].ErrorType = DefectLevel::MINOR;//轻微
			}
			if (area >= levelRankInfo.ChippedEdge.areaRank[2] && area < levelRankInfo.ChippedEdge.areaRank[3]) {

				data[i].ErrorType = DefectLevel::MEDIUM;//警告
			}
			if (area >= levelRankInfo.ChippedEdge.areaRank[3]) {

				data[i].ErrorType = DefectLevel::SERIOUS;//严重
			}
		}
		else if (type == DefectType::TYPE_PITTING) {//9
			if (area < levelRankInfo.Pitting.areaRank[1] || levelRankInfo.Pitting.areaRank[1] == 0.0) {
				removeIndexNumber.push_back(i);
			}
			if (area >= levelRankInfo.Pitting.areaRank[1] && area < levelRankInfo.Pitting.areaRank[2]) {
				data[i].ErrorType = DefectLevel::MINOR;//轻微
			}
			if (area >= levelRankInfo.Pitting.areaRank[2] && area < levelRankInfo.Pitting.areaRank[3]) {

				data[i].ErrorType = DefectLevel::MEDIUM;//警告
			}
			if (area >= levelRankInfo.Pitting.areaRank[3]) {

				data[i].ErrorType = DefectLevel::SERIOUS;//严重
			}
		}
		else if (type == DefectType::TYPE_GLASS_CULLET) {//10
			if (area < levelRankInfo.GlassCullet.areaRank[1] || levelRankInfo.GlassCullet.areaRank[1] == 0.0) {
				removeIndexNumber.push_back(i);
			}
			if (area >= levelRankInfo.GlassCullet.areaRank[1] && area < levelRankInfo.GlassCullet.areaRank[2]) {
				data[i].ErrorType = DefectLevel::MINOR;//轻微
			}
			if (area >= levelRankInfo.GlassCullet.areaRank[2] && area < levelRankInfo.GlassCullet.areaRank[3]) {

				data[i].ErrorType = DefectLevel::MEDIUM;//警告
			}
			if (area >= levelRankInfo.GlassCullet.areaRank[3]) {

				data[i].ErrorType = DefectLevel::SERIOUS;//严重
			}
		}
		else if (type == DefectType::TYPE_WAVINESS) {//11
				if (area < levelRankInfo.Waviness.areaRank[1] || levelRankInfo.Waviness.areaRank[1] == 0.0) {
					removeIndexNumber.push_back(i);
				}
				if (area >= levelRankInfo.Waviness.areaRank[1] && area < levelRankInfo.Waviness.areaRank[2]) {
					data[i].ErrorType = DefectLevel::MINOR;//轻微
				}
				if (area >= levelRankInfo.Waviness.areaRank[2] && area < levelRankInfo.Waviness.areaRank[3]) {

					data[i].ErrorType = DefectLevel::MEDIUM;//警告
				}
				if (area >= levelRankInfo.Waviness.areaRank[3]) {

					data[i].ErrorType = DefectLevel::SERIOUS;//严重
				}
		}
		else if (type == DefectType::TYPE_OTHER) {//12
			if (area < levelRankInfo.Other.areaRank[1] || levelRankInfo.Other.areaRank[1] == 0.0) {
				removeIndexNumber.push_back(i);
			}
			if (area >= levelRankInfo.Other.areaRank[1] && area < levelRankInfo.Other.areaRank[2]) {
				data[i].ErrorType = DefectLevel::MINOR;//轻微
			}
			if (area >= levelRankInfo.Other.areaRank[2] && area < levelRankInfo.Other.areaRank[3]) {

				data[i].ErrorType = DefectLevel::MEDIUM;//警告
			}
			if (area >= levelRankInfo.Other.areaRank[3]) {

				data[i].ErrorType = DefectLevel::SERIOUS;//严重
			}
		}

	}

	// ===== 剔除无效缺陷项 =====
	std::vector<drawInformation> result;
	std::set<int> removeSet(removeIndexNumber.begin(), removeIndexNumber.end());

	for (size_t i = 0; i < data.size(); ++i) {
		if (removeSet.find(static_cast<int>(i)) == removeSet.end()) {
			result.push_back(data[i]);
		}
	}

	return result;
}

void defectLevel::mainTest() {

	
}
