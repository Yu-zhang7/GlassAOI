#include "GeneralMethod.h"
#include "Log.hpp"


GeneralMethod::GeneralMethod()
{

}

GeneralMethod::~GeneralMethod()
{

}


std::vector<std::string> GeneralMethod::loadFileImage(std::string img_dir)
{
    std::vector<std::string> imgs;
    for (const auto& file : std::filesystem
        ::directory_iterator(img_dir)) {
        std::filesystem::path path = file.path();
        std::string filename = path.filename().string();  // 获取文件名  

        // 检查文件扩展名  
        if (filename.find(".png") != std::string::npos ||
            filename.find(".PNG") != std::string::npos ||
            filename.find(".jpeg") != std::string::npos ||
            filename.find(".jpg") != std::string::npos||
            filename.find(".bmp") != std::string::npos) {
            imgs.push_back(path.string());  // 使用 path.string() 获取完整的路径字符串  
        }
    }

	return imgs;
}


// Helper function to check if a file has a supported image extension
bool isSupportedImageFormat(const std::string& filename) {
    const std::vector<std::string> supportedExtensions = { ".png", ".jpg", ".jpeg", ".bmp", ".tiff", ".tif" };
    for (const auto& ext : supportedExtensions) {
        if (filename.size() >= ext.size() &&
            filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0) {
            return true;
        }
    }
    return false;
}

/// <summary>
/// 仅获取文件夹中图像的名称
/// </summary>
/// <param name="directoryPath"></param>
/// <returns></returns>
std::vector<std::string> GeneralMethod::getImageFileNames(const std::string& directoryPath) {
    std::vector<std::string> imageFileNames;
    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (entry.is_regular_file() && isSupportedImageFormat(entry.path().filename().string())) {
            imageFileNames.push_back(entry.path().filename().string());
        }
    }
    return imageFileNames;
}
cv::Mat GeneralMethod::convertTo3Channels(const cv::Mat& binImg)
{
    cv::Mat three_channel = cv::Mat::zeros(binImg.rows, binImg.cols, CV_8UC3);
    std::vector<cv::Mat> channels;
    for (int i = 0; i < 3; i++)
    {
        channels.push_back(binImg);
    }
    merge(channels, three_channel);
    return three_channel;
}

int GeneralMethod::drawDefectImage(cv::Mat& inputImage, cv::Mat& maskImage, DefectLevel ErrorType)
{
    cv::Scalar drawColor = cv::Scalar(0, 255, 0);
    if (inputImage.channels() == 1) {

        inputImage = convertTo3Channels(inputImage);
    }

    if (ErrorType == DefectLevel::NORMAL) { //正常  绿色

    }
    else if (ErrorType == DefectLevel::MINOR) {//轻微缺陷  蓝色

        drawColor = cv::Scalar(255, 0, 0);
    }
    else if (ErrorType == DefectLevel::MEDIUM) {//中等缺陷  黄色
        drawColor = cv::Scalar(0, 255, 255);
    }
    else if (ErrorType == DefectLevel::SERIOUS) {//严重缺陷  红色

        drawColor = cv::Scalar(0, 0, 255);

    }
    cv::Mat colorImage = cv::Mat::zeros(maskImage.size(), CV_8UC3);
    for (int i = 0; i < maskImage.rows; i++) {

        for (int j = 0; j < maskImage.cols; j++) {

            if (maskImage.at<uchar>(i, j) == 0) {

                //colorImage.at<cv::Vec3b>(i,j) = drawColor;
                colorImage.at<cv::Vec3b>(i,j) = cv::Vec3b(drawColor[0], drawColor[1], drawColor[2]);
            
            }
            else {

                colorImage.at<cv::Vec3b>(i, j) = inputImage.at<cv::Vec3b>(i, j);

            }

        }

    }
    double alpha = 0.5; // 第一幅图像的权重  
    double beta = 0.5;  // 第二幅图像的权重  
    double gamma = 0.0; // 加到加权和上的标量值 

    //膨胀系数
    int kernelSize = 3;
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(kernelSize, kernelSize));

    // 创建一个用于存储膨胀结果的Mat对象  
    cv::Mat dilated;

    // 对每个通道分别进行膨胀操作  
    // 因为OpenCV的膨胀操作是针对单通道图像的，所以我们需要对每个通道分别处理  
    std::vector<cv::Mat> channels(3);
    cv::split(colorImage, channels);

    for (int i = 0; i < 3; ++i) {
        cv::dilate(channels[i], channels[i], element);
    }

    // 合并处理后的通道  
    cv::merge(channels, dilated);

    cv::addWeighted(inputImage, alpha, dilated, beta, gamma, inputImage);

    return 0;
}


int GeneralMethod::mergedImage2One(std::vector<cv::Mat> images,cv::Mat& mergedImg) {

    if (images.size() <= 1) {

        return -1;

    }
    // 获取图像的宽度和高度
    int width = images[0].cols;
    int height = images[1].rows;
    int imageSize = images.size();
    // 创建一个新的图像矩阵，其宽度与原图相同，高度是原图的两倍
    cv::Mat mergedImage;
    cv::hconcat(images, mergedImage);
    mergedImg = mergedImage.clone();

    return 0;

}

// 函数定义

/// <summary>
/// 	 NORMAL,//正常  0
///      MINOR,//轻微缺陷  1
///      MEDIUM,//中等缺陷 2
///      SERIOUS,//严重缺陷 3
///      ABNORMAL//异常 4
/// </summary>
/// <param name="image"></param>
/// <param name="rect"></param>
/// <param name="ErrorType"></param>
void GeneralMethod::drawRectangle(cv::Mat& image, const cv::Rect& rect, DefectLevel& ErrorType) {
    cv::Scalar colorScalar;
    switch (ErrorType) {
    case NORMAL:
        colorScalar = cv::Scalar(0, 255, 0); // 绿色
        break;
    case MINOR:
        colorScalar = cv::Scalar(255, 0, 0); // 蓝色（注意：这里通常蓝色是BGR格式，即(255, 0, 0)）
        break;
    case MEDIUM:
        colorScalar = cv::Scalar(0, 255, 255); // 黄色
        break;
    case SERIOUS:
        colorScalar = cv::Scalar(0, 0, 255); // 红色
        break;
    default:
        colorScalar = cv::Scalar(125, 125, 125); // 红色
        //std::cerr << "Error: Invalid color!" << std::endl;
        break;
    }

    // 绘制矩形
    cv::rectangle(image, rect, colorScalar, 1); // 2是线条的粗细

}

/// <summary>
/// 
/// </summary>
/// <param name="image"></param>
/// <param name="rect"></param>
/// <param name="ErrorType"></param>
/// <param name="Errorinfo">
/// 	float area = 0.0; // 面积
//      float length = 0.0;//长度
//      float perimeter = 0.0;//周长
/// <param name="scale">标定的像素与实际距离的比值，即1个像素对应的长度mm</param>
/// </param>
void GeneralMethod::drawRectangleWithInfo(cv::Mat& image, const cv::Rect& rect, DefectLevel& ErrorType, DefectType& DefectType,std::vector<float>& Errorinfo,float scale) {
    cv::Scalar colorScalar;
    switch (ErrorType) {
    case NORMAL:
        colorScalar = cv::Scalar(255, 0, 0); // 蓝色
        break;
    case MINOR:
        colorScalar = cv::Scalar(0, 255, 0); // 绿色（注意：这里通常蓝色是BGR格式，即(255, 0, 0)）
        break;
    case MEDIUM:
        colorScalar = cv::Scalar(0, 255, 255); // 黄色
        break;
    case SERIOUS:
        colorScalar = cv::Scalar(0, 0, 255); // 红色
        break;
    default:
        colorScalar = cv::Scalar(255, 0, 0); // 红色
        //std::cerr << "Error: Invalid color!" << std::endl;
        break;
    }
    float width = rect.width > rect.height ? rect.height : rect.width;
    float height = rect.width < rect.height ? rect.height : rect.width;
    width = width * scale;
    height = height * scale;

    std::stringstream width2, height2, Area2,length2;
    width2 << std::fixed << std::setprecision(2) << width;
    height2 << std::fixed << std::setprecision(2) << height;
    Area2 << std::fixed << std::setprecision(2) << Errorinfo[0];
    length2 << std::fixed << std::setprecision(2) << Errorinfo[1];

    std::string widthString = width2.str();
    std::string heightString = height2.str();
    std::string AreaString = Area2.str();
    std::string lengthString = length2.str();


    std::string typeName = "";
    if (DefectType == DefectType::TYPE_POORCOATING) {

        typeName = "镀膜不良,Area:" + AreaString;

    }
    else if (DefectType == DefectType::TYPE_SCRATCH) {

        typeName = "划伤,L:" + lengthString;

    }
    else if (DefectType == DefectType::TYPE_CALCULUS) {

        typeName = "结石,Area:" + AreaString;

    }
    else if (DefectType == DefectType::TYPE_BUBBLE) {

        typeName = "气泡, Area:" + AreaString;

    }
    else if (DefectType == DefectType::TYPE_TRADEMARK) {

        typeName = "商标";
    }
    else if (DefectType == DefectType::TYPE_WATERSTAIN) {

        typeName = "水渍, Area:" + AreaString;
    }
    else if (DefectType == DefectType::TYPE_SMUDGE) {

        typeName = "脏污, Area:" + AreaString;

    }
    // 计算左下角坐标
    cv::Point bottomLeft(rect.x, rect.y + rect.height);

    // 定义文本的左下角起始位置（为了美观，可以稍微偏移一下），    //40大小字体 需要550宽度的像素  50的高度
    cv::Point textOrg(bottomLeft.x - 10, bottomLeft.y + 20);
    if (textOrg.x < 0) {

        textOrg.x = 0;

    }
    if (textOrg.y < 0) {
        textOrg.y = 0;
    }

    if ((textOrg.x + 280) > image.cols) {
        //超出右边

        textOrg.x = image.cols - 280;
    }
    if ((textOrg.y + 50) > image.rows) {
        //超出右边

        textOrg.y = image.rows - 50;
    }

    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    double fontScale = 0.5;
    int thickness = 1;

    // 绘制矩形
    cv::rectangle(image, rect, colorScalar, 1); // 2是线条的粗细

}		 
int GeneralMethod::divideImage(cv::Mat& largeImage, int imgWidth, int imgHeight, std::vector<cv::Mat>& mats,std::vector<std::vector<int>>& imgIndexs)
{
    if (largeImage.empty()) {
        std::cerr << "无法读取大图！" << std::endl;
        return -1;
    }

    // 计算需要添加的0像素数量
    int padWidth = (largeImage.cols % imgWidth == 0) ? 0 : imgWidth - (largeImage.cols % imgWidth);
    int padHeight = (largeImage.rows % imgHeight == 0) ? 0 : imgHeight - (largeImage.rows % imgHeight);

    // 创建一个尺寸调整后的图像，并填充0像素
    cv::Mat paddedImage = cv::Mat::zeros(largeImage.rows + padHeight, largeImage.cols + padWidth, largeImage.type());
    largeImage.copyTo(paddedImage(cv::Rect(0, 0, largeImage.cols, largeImage.rows)));

    // 计算分割后的小图数量
    int numTilesX = paddedImage.cols / imgWidth;
    int numTilesY = paddedImage.rows / imgHeight;

    for (int y = 0; y < numTilesY; ++y) {
        std::vector<int> imgIndex;
        for (int x = 0; x < numTilesX; ++x) {
        
            cv::Rect roi(x * imgWidth, y * imgHeight, imgWidth, imgHeight);

            cv::Mat tempImg = paddedImage(roi);

            imgIndex.push_back(x);
            mats.push_back(tempImg);
        
        }
        imgIndexs.push_back(imgIndex);
    }



    return 0;
}

int GeneralMethod::divideImageShallow(cv::Mat& largeImage, int imgWidth, int imgHeight,
    std::vector<cv::Mat>& tiles,
    std::vector<std::vector<int>>& imgIndexs) {
    if (largeImage.empty()) {
        std::cerr << "Input image is empty!" << std::endl;
        return -1;
    }

    // 计算分割后的块数（向上取整）
    int numTilesX = (largeImage.cols + imgWidth - 1) / imgWidth;
    int numTilesY = (largeImage.rows + imgHeight - 1) / imgHeight;

    // 预分配输出容器
    tiles.reserve(numTilesX * numTilesY);
    imgIndexs.resize(numTilesY);

    for (int y = 0; y < numTilesY; ++y) {
        int y_start = y * imgHeight;
        int actual_height = std::min(imgHeight, largeImage.rows - y_start);

        for (int x = 0; x < numTilesX; ++x) {
            int x_start = x * imgWidth;
            int actual_width = std::min(imgWidth, largeImage.cols - x_start);

            // 创建ROI（浅拷贝）
            cv::Mat tile(largeImage, cv::Rect(x_start, y_start, actual_width, actual_height));

            // 只有当块不完整时才需要补零
            if (actual_width == imgWidth && actual_height == imgHeight) {
                tiles.push_back(tile);
            }
            else {
                cv::Mat paddedTile = cv::Mat::zeros(imgHeight, imgWidth, largeImage.type());
                tile.copyTo(paddedTile(cv::Rect(0, 0, actual_width, actual_height)));
                tiles.push_back(paddedTile);
            }

            imgIndexs[y].push_back(x);
        }
    }

    return 0;
}

int GeneralMethod::MergeImage(std::vector<cv::Mat>& mats, std::vector<std::vector<int>>& imgIndexs,cv::Mat& mergeLarget) {

    int numTilesY = imgIndexs.size();
    int numTilesX = imgIndexs[0].size();
    int tileWidth = mats[0].cols; 
    int tileHeight = mats[0].rows; 
    cv::Mat resultImage = cv::Mat::zeros(numTilesY* tileHeight, numTilesX* tileWidth, mergeLarget.type());

    for (int y = 0; y < numTilesY; ++y) {
        for (int x = 0; x < numTilesX; ++x) {


            cv::Rect dstRect(x * tileWidth, y * tileHeight, tileWidth, tileHeight);
            int matIndex = imgIndexs[y].size() * y + x;
            if (mats[matIndex].channels() == 1) {

                mats[matIndex] = convertTo3Channels(mats[matIndex]);

            }
            mats[matIndex].copyTo(resultImage(dstRect));
        }
    }
     cv::Rect cropRect(0, 0, mergeLarget.cols, mergeLarget.rows);
     mergeLarget = resultImage(cropRect);
    return 0;
}
void GeneralMethod::mainTest()
{
    cv::Mat largetImage = cv::imread("F:/data/GAS/all/20241104/20241104_151120113_PLUG/0_envImage_Left.png",0);
    
    std::vector<cv::Mat> mats;
    std::vector<std::vector<int>> imgIndexs;
    
    divideImage(largetImage,100,100, mats, imgIndexs);

    //cv::Mat mergeLarget = cv::Mat::zeros(largetImage.size(), CV_8UC3);
    cv::Mat mergeLarget = cv::Mat::zeros(largetImage.size(), CV_8UC1);

    MergeImage(mats, imgIndexs, mergeLarget);

}


/// <summary>
/// 按照图像比例剔除边缘区域的缺陷，主要防止由于边缘存在胶印之类的造成误检
/// </summary>
/// <param name="defects"></param>
/// <param name="imageWidth"></param>
/// <param name="imageHeight"></param>
/// <param name="verticalRatio">垂直比例</param>
/// <param name="horizontalRatio">水平比例</param>
/// <returns></returns>
std::vector<drawInformation> GeneralMethod::filterDefectsByBorderRegion(
    const std::vector<drawInformation>& defects,
    int imageWidth,
    int imageHeight,
    float verticalRatio,
    float horizontalRatio)
{
    std::vector<drawInformation> filteredDefects;

    // 计算边界区域的坐标范围
    //int verticalBorder = static_cast<int>(imageHeight * horizontalRatio);
    //int horizontalBorder = static_cast<int>(imageWidth * verticalRatio);

    int verticalBorder = 300;
    int horizontalBorder = 300;

    // 定义边界区域
    int leftBorder = horizontalBorder;
    int rightBorder = imageWidth - horizontalBorder;
    int topBorder = verticalBorder;
    int bottomBorder = imageHeight - verticalBorder;

    // 遍历所有缺陷，只保留不在边界区域内的缺陷
    for (const auto& defect : defects) {
        cv::Rect rect = defect.rect;

        // 检查缺陷是否完全在边界区域内
        bool isInBorderRegion =
            (rect.x < leftBorder) ||                           // 在左边界
            (rect.x + rect.width > rightBorder) ||            // 在右边界  
            (rect.y < topBorder) ||                           // 在上边界
            (rect.y + rect.height > bottomBorder);            // 在下边界

        // 如果缺陷不在边界区域内，则保留
        //if (!isInBorderRegion) {
        //    filteredDefects.push_back(defect);
        //}

        if (!isInBorderRegion) {//非边界区域
			if (defect.DefectType == DefectType::TYPE_CHIPPED_EDGE) {//20260312把非贴边缺陷改为脏污，主要是因为贴边的缺陷比较特殊，容易出现误检，所以把它们单独分类出来，后续可以根据实际情况调整
                drawInformation tempDefect = defect;
                tempDefect.DefectType = DefectType::TYPE_SMUDGE;
                filteredDefects.push_back(tempDefect);
            }
            else {
                filteredDefects.push_back(defect);
            }

        }
        else {//在边界内,保持不变
            filteredDefects.push_back(defect);
        }

    }
    return filteredDefects;
}

/*===================================同类型接近类型缺陷合并====================================================*/
//// 检查两个矩形是否有重叠
bool doRectsOverlap(const cv::Rect& rect1, const cv::Rect& rect2) {
    return !(rect1.x + rect1.width <= rect2.x ||
        rect1.x >= rect2.x + rect2.width ||
        rect1.y + rect1.height <= rect2.y ||
        rect1.y >= rect2.y + rect2.height);
}

// 检查两个矩形是否有重叠
bool doRectsOverlap_(const cv::Rect& rect1, const cv::Rect& rect2,int dis) {
    // 扩展rect1的边界
    cv::Rect expandedRect1(
        rect1.x - dis,
        rect1.y - dis,
        rect1.width + 2 * dis,
        rect1.height + 2 * dis
    );

    // 扩展rect2的边界
    cv::Rect expandedRect2(
        rect2.x - dis,
        rect2.y - dis,
        rect2.width + 2 * dis,
        rect2.height + 2 * dis
    );

    // 判断扩展后的矩形是否重叠
    return !(expandedRect1.x + expandedRect1.width <= expandedRect2.x ||
        expandedRect1.x >= expandedRect2.x + expandedRect2.width ||
        expandedRect1.y + expandedRect1.height <= expandedRect2.y ||
        expandedRect1.y >= expandedRect2.y + expandedRect2.height);
}

// 合并两个drawInformation对象（假设它们的DefectType相同）
drawInformation mergeDrawInfo(const drawInformation& di1, const drawInformation& di2) {
    drawInformation merged;
    merged.Errorinfo.resize(3);
    merged.DefectType = di1.DefectType;
    merged.ErrorType = di1.ErrorType; // 你可以根据需要定义合并规则
    merged.Errorinfo[0] = 0.0;
    merged.Errorinfo[1] = 0.0;
    merged.Errorinfo[2] = 0.0;
    // 合并rect
    int x = di1.rect.x< di2.rect.x? di1.rect.x: di2.rect.x;
    int y = di1.rect.y < di2.rect.y ? di1.rect.y : di2.rect.y;
    int width = (di1.rect.x + di1.rect.width) >= (di2.rect.x + di2.rect.width) ? (di1.rect.x + di1.rect.width) : (di2.rect.x + di2.rect.width);
    width = width - x;
    int height = (di1.rect.y + di1.rect.height) >= (di2.rect.y + di2.rect.height) ? (di1.rect.y + di1.rect.height) : (di2.rect.y + di2.rect.height);
    height = height - y;
    merged.rect = cv::Rect(x, y, width, height);


    merged.Errorinfo[0] = di1.Errorinfo[0] + di2.Errorinfo[0];
    merged.Errorinfo[1] = di1.Errorinfo[1] + di2.Errorinfo[1];
    merged.Errorinfo[2] = di1.Errorinfo[2] + di2.Errorinfo[2];

    return merged;
}

// 合并两个drawInformation对象（假设它们的DefectType相同）
//取合并区域中各部分的最大值
drawInformation mergeDrawInfo_(const drawInformation& di1, const drawInformation& di2) {
    drawInformation merged;
    merged.Errorinfo.resize(3);
    merged.DefectType = di1.DefectType;
    merged.ErrorType = di1.ErrorType; // 你可以根据需要定义合并规则
    merged.Errorinfo[0] = 0.0;
    merged.Errorinfo[1] = 0.0;
    merged.Errorinfo[2] = 0.0;
    // 合并rect
    int x = di1.rect.x < di2.rect.x ? di1.rect.x : di2.rect.x;
    int y = di1.rect.y < di2.rect.y ? di1.rect.y : di2.rect.y;
    int width = (di1.rect.x + di1.rect.width) >= (di2.rect.x + di2.rect.width) ? (di1.rect.x + di1.rect.width) : (di2.rect.x + di2.rect.width);
    width = width - x;
    int height = (di1.rect.y + di1.rect.height) >= (di2.rect.y + di2.rect.height) ? (di1.rect.y + di1.rect.height) : (di2.rect.y + di2.rect.height);
    height = height - y;
    merged.rect = cv::Rect(x, y, width, height);

    if (di1.Errorinfo[0] > di2.Errorinfo[0]) {
        merged.Errorinfo[0] = di1.Errorinfo[0];
    }
    else
    {
        merged.Errorinfo[0] = di2.Errorinfo[0];
    }
    if (di1.Errorinfo[1] > di2.Errorinfo[1]) {

        merged.Errorinfo[1] = di1.Errorinfo[1];
    }
    else
    {
        merged.Errorinfo[1] = di2.Errorinfo[1];
    }

    if (di1.Errorinfo[2] > di2.Errorinfo[2]) {
        merged.Errorinfo[2] = di1.Errorinfo[2];
    }
    else
    {
        merged.Errorinfo[2] = di2.Errorinfo[2];
    }

    return merged;
}


drawInformation mergeDrawInfos(std::vector<drawInformation> currentGroup, std::vector<int> indexs) {

    drawInformation merged;
    merged.DefectType = currentGroup[indexs[0]].DefectType;
    merged.ErrorType = currentGroup[indexs[0]].ErrorType; // 你可以根据需要定义合并规则
    merged.Errorinfo[0] = 0.0;
    merged.Errorinfo[1] = 0.0;
    merged.Errorinfo[1] = 0.0;
    int minX = 100000;
    int minY = 100000;
    int maxX = -1;
    int maxY = -1;

    for (int i = 0; i < indexs.size(); i++) {

        if (minX < currentGroup[indexs[i]].rect.x) {

            minX = currentGroup[indexs[i]].rect.x;

        }
        if (minY < currentGroup[indexs[i]].rect.y) {

            minY = currentGroup[indexs[i]].rect.y;

        }
        if (maxX < currentGroup[indexs[i]].rect.x+ currentGroup[indexs[i]].rect.width) {

            maxX = currentGroup[indexs[i]].rect.x + currentGroup[indexs[i]].rect.width;

        }
        if (maxY < currentGroup[indexs[i]].rect.y + currentGroup[indexs[i]].rect.height) {

            maxY = currentGroup[indexs[i]].rect.y + currentGroup[indexs[i]].rect.height;

        }

        for (int n = 0; currentGroup[indexs[i]].Errorinfo.size();n++) {

            merged.Errorinfo[n] += currentGroup[indexs[i]].Errorinfo[n];
        }


    }
    merged.rect = cv::Rect(minX, minY, maxX- minX, maxY- minY);

    return merged;
}




std::vector<drawInformation> GeneralMethod::mergeDrawInformations(const std::vector<drawInformation>& drawInfos) {
    std::map<DefectType, std::vector<drawInformation>> groupedByDefectType;
    std::vector<drawInformation> mergedDrawInfos;

    // 按DefectType分组
    for (const auto& di : drawInfos) {
        groupedByDefectType[di.DefectType].push_back(di);
    }

    // 合并每组中的drawInformation
    for (const auto& kv : groupedByDefectType) {
        std::vector<drawInformation> currentGroup = kv.second;
        std::vector<drawInformation> mergedGroup;

        while (!currentGroup.empty()) {
            drawInformation first = currentGroup.front();
            currentGroup.erase(currentGroup.begin());
            mergedGroup.push_back(first);

            for (auto it = currentGroup.begin(); it != currentGroup.end(); /* no increment here */) {
                //if (doRectsOverlap(first.rect, it->rect)) {
                if (doRectsOverlap_(first.rect, it->rect, 150)) {
                    first = mergeDrawInfo(first, *it);
                    it = currentGroup.erase(it); // erase returns the next iterator
                }
                else {
                    ++it; // only increment if we didn't erase the element
                }
            }
        }

        mergedDrawInfos.insert(mergedDrawInfos.end(), mergedGroup.begin(), mergedGroup.end());
    }



    return mergedDrawInfos;
}


std::vector<drawInformation> GeneralMethod::mergeDrawInformationsWithDistance(const std::vector<drawInformation>& drawInfos,int Dis) {
    std::map<DefectType, std::vector<drawInformation>> groupedByDefectType;
    std::vector<drawInformation> mergedDrawInfos;

    // 按DefectType分组
    for (const auto& di : drawInfos) {
        groupedByDefectType[di.DefectType].push_back(di);
    }

    // 合并每组中的drawInformation
    for (const auto& kv : groupedByDefectType) {
        std::vector<drawInformation> currentGroup = kv.second;
        std::vector<drawInformation> mergedGroup;

        while (!currentGroup.empty()) {
            drawInformation first = currentGroup.front();
            currentGroup.erase(currentGroup.begin());
            mergedGroup.push_back(first);

            for (auto it = currentGroup.begin(); it != currentGroup.end(); /* no increment here */) {
                //if (doRectsOverlap(first.rect, it->rect)) {
                if (doRectsOverlap_(first.rect, it->rect, Dis)) {
                    first = mergeDrawInfo_(first, *it);
                    it = currentGroup.erase(it); // erase returns the next iterator
                }
                else {
                    ++it; // only increment if we didn't erase the element
                }
            }
        }

        mergedDrawInfos.insert(mergedDrawInfos.end(), mergedGroup.begin(), mergedGroup.end());
    }



    return mergedDrawInfos;
}

/*===================================同类型接近类型缺陷合并====================================================*/




/***************************************20250811 软件算法通用库整理 王***************************************/

/*===============文件======================*/

//规范化路径
std::string GeneralMethod::extract_directory_name(const std::string& path_str)
{
    std::filesystem::path p(path_str);

    // 规范化路径（处理多余的分隔符等）
    p = p.lexically_normal();

    // 如果路径以分隔符结尾，移除末尾的分隔符
    if (!p.empty() && p.has_filename() == false)
    {
        p = p.parent_path();
    }

    // 返回最后一级目录名
    return p.filename().string();
}

//读图模式下，读取文件的路径
std::string GeneralMethod::readFilePaths()
{
    const std::string filename = "FilesPath.txt";
    std::ifstream file(filename); // 创建输入文件流
    std::string paths = ""; // 存储路径的容器

    // 检查文件是否成功打开
    if (!file.is_open())
    {
        //FileLogPrintf("无法打开文件: %s", filename.c_str());
        return paths;
    }

    std::string line;
    // 逐行读取文件内容
    while (std::getline(file, line))
    {
        // 跳过空行
        if (!line.empty())
        {
            paths = line;
        }
    }

    std::filesystem::path p(paths);
    // 规范化路径（处理多余的分隔符等）
    p = p.lexically_normal();

    if (!p.empty() && p.has_filename() == false)
    {
        p = p.parent_path();
    }
    paths = p.string();
    return paths; // 返回路径列表
}

std::string GeneralMethod::gbk_to_utf8(const std::string& str_gbk) {
    int wide_len = MultiByteToWideChar(CP_ACP, 0, str_gbk.c_str(), -1, nullptr, 0);
    std::wstring wstr(wide_len, 0);
    MultiByteToWideChar(CP_ACP, 0, str_gbk.c_str(), -1, &wstr[0], wide_len);

    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str_utf8(utf8_len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str_utf8[0], utf8_len, nullptr, nullptr);

    return str_utf8;
}
/*===============文件======================*/

/*===============获取计算信息======================*/

/// <summary>
/// 20250306 修改，返回两个矩形，一个为扩充区域，一个为实际玻璃区域
/// </summary>
/// <param name="inputGaryImg"></param>
/// <param name="rect_"></param>
/// <param name="threshold"></param>
/// <returns></returns>
int GeneralMethod::getGlassRect(cv::Mat& inputGaryImg, cv::Rect& rect_, cv::Rect& GlassRect, int threshold) {

    // 阈值处理
    cv::Mat binaryImage;
    cv::threshold(inputGaryImg, binaryImage, threshold, 255, cv::THRESH_BINARY);

    // 找到所有白色区域的轮廓
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(binaryImage, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 找到最大的白色区域
    int maxArea = 0;
    int maxIndex = -1;
    for (size_t i = 0; i < contours.size(); i++) {
        int area = cv::contourArea(contours[i]);
        if (area > maxArea) {
            maxArea = area;
            maxIndex = i;
        }
    }

    if (maxIndex == -1) {
        std::cout << "No white region found" << std::endl;
        return -1;
    }

    // 计算最大白色区域向外扩展指定像素的矩形
    cv::Rect originalBoundingRect = cv::boundingRect(contours[maxIndex]);

    //int expandPixels = 200; // 指定扩展像素数
    //boundingRect.x = (boundingRect.x - expandPixels > 0) ? boundingRect.x - expandPixels : 0;
    //boundingRect.y = (boundingRect.y - expandPixels > 0) ? boundingRect.y - expandPixels : 0;
    //boundingRect.width = (boundingRect.width + 2 * expandPixels < inputGaryImg.cols - boundingRect.x) ? boundingRect.width + 2 * expandPixels : inputGaryImg.cols - boundingRect.x;
    //boundingRect.height = (boundingRect.height + 2 * expandPixels < inputGaryImg.rows - boundingRect.y) ? boundingRect.height + 2 * expandPixels : inputGaryImg.rows - boundingRect.y;

    //rect_ = boundingRect;



    int expandPixels = 200; // 指定扩展像素数
    cv::Rect expandedBoundingRect = originalBoundingRect;

    expandedBoundingRect.x = (expandedBoundingRect.x - expandPixels > 0) ? expandedBoundingRect.x - expandPixels : 0;
    expandedBoundingRect.y = (expandedBoundingRect.y - expandPixels > 0) ? expandedBoundingRect.y - expandPixels : 0;
    expandedBoundingRect.width = (expandedBoundingRect.width + 2 * expandPixels < inputGaryImg.cols - expandedBoundingRect.x) ? expandedBoundingRect.width + 2 * expandPixels : inputGaryImg.cols - expandedBoundingRect.x;
    expandedBoundingRect.height = (expandedBoundingRect.height + 2 * expandPixels < inputGaryImg.rows - expandedBoundingRect.y) ? expandedBoundingRect.height + 2 * expandPixels : inputGaryImg.rows - expandedBoundingRect.y;

    rect_ = expandedBoundingRect;

    // 原始矩形区域在扩展后图像中的位置
    cv::Rect originalRectInExpanded = cv::Rect(
        originalBoundingRect.x - expandedBoundingRect.x,
        originalBoundingRect.y - expandedBoundingRect.y,
        originalBoundingRect.width,
        originalBoundingRect.height);
    GlassRect = originalRectInExpanded;

    return 0;
}

int GeneralMethod::OnlyGetGlassRect(cv::Mat& inputGaryImg, cv::Rect& rect_, int threshold) {

    // 阈值处理
    cv::Mat binaryImage;
    cv::threshold(inputGaryImg, binaryImage, threshold, 255, cv::THRESH_BINARY);

    // 找到所有白色区域的轮廓
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(binaryImage, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 找到最大的白色区域
    int maxArea = 0;
    int maxIndex = -1;
    for (size_t i = 0; i < contours.size(); i++) {
        int area = cv::contourArea(contours[i]);
        if (area > maxArea) {
            maxArea = area;
            maxIndex = i;
        }
    }

    if (maxIndex == -1) {
        std::cout << "No white region found" << std::endl;
        return -1;
    }

    // 计算最大白色区域向外扩展指定像素的矩形
    cv::Rect boundingRect = cv::boundingRect(contours[maxIndex]);

    rect_ = boundingRect;


    return 0;
}

//获取玻璃中间两列的灰度值
double GeneralMethod::getAverageOfTwoColumns(const cv::Mat& img, int col1, int col2) {
    if (col1 < 0 || col2 >= img.cols || col1 >= img.cols || col2 < 0) {
        std::cerr << "Invalid column indices!" << std::endl;
        return -1;
    }

    cv::Mat twoCols;
    cv::hconcat(img.col(col1), img.col(col2), twoCols);
    cv::Scalar meanVal = cv::mean(twoCols);
    return meanVal[0];
}

/// <summary>
/// 获取示例图相对于原图的缩放比
/// </summary>
/// <param name="width"></param>
/// <param name="height"></param>
/// <returns></returns>
double GeneralMethod::getScale(int width, int height) {

    return 1000 / static_cast<double>(std::max(width, height));
}
double GeneralMethod::getMaxGrayValue(const cv::Mat& image) {
    if (image.empty()) {
        std::cerr << "Image is empty!" << std::endl;
        return -1;
    }

    // 检查图像是否为单通道图像
    if (image.channels() != 1) {
        // 将彩色图像转换为灰度图像
        cv::Mat grayImage;
        cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);
        return getMaxGrayValue(grayImage);
    }

    // 使用 cv::minMaxLoc 获取图像中的最小和最大值
    double minVal, maxVal;
    cv::minMaxLoc(image, &minVal, &maxVal);

    return maxVal;
}


std::vector<drawInformation> GeneralMethod::filterResultsByEdgeDistance(
    const std::vector<drawInformation>& results_beforeSort,
    const std::vector<cv::Point>& edgePoints,
    double distanceThreshold = 100.0) {

    if (edgePoints.empty()) return results_beforeSort;

    // 转换为 FLANN 可接受的 Mat 格式
    cv::Mat flannData = convertEdgePointsToMat(edgePoints);

    // 构建 FLANN 索引
    cv::flann::Index tree(flannData, cv::flann::KDTreeIndexParams(4));

    std::vector<drawInformation> filteredResults;

    for (const auto& info : results_beforeSort) {
        // 获取矩形中心点
        cv::Point center(info.rect.x + info.rect.width / 2, info.rect.y + info.rect.height / 2);

        // 查询最近邻点索引和距离
        std::vector<int> indices(1);
        std::vector<float> dists(1);
        cv::Mat queryPt = (cv::Mat_<float>(1, 2) << center.x, center.y);

        tree.knnSearch(queryPt, indices, dists, 1, cv::flann::SearchParams(32));

        // 如果最小距离大于阈值，则保留该缺陷
        if (std::sqrt(dists[0]) >= distanceThreshold) {


            filteredResults.push_back(info);
        }
    }

    return filteredResults;
}



/*===============边采边计算======================*/

/// <summary>
/// 合并多组图像中玻璃rect区域，获取大图中的rect值
/// </summary>
/// <param name="currentBatchRect"></param>
/// <param name="startY"></param>
/// <returns></returns>
cv::Rect GeneralMethod::mergeRectanglesWithOffset(const std::vector<cv::Rect>& currentBatchRect,
    const std::vector<int>& startY) {
    cv::Rect mergedRect;
    bool firstValid = true;

    // 检查输入大小是否一致
    if (currentBatchRect.size() != startY.size()) {
        return mergedRect; // 返回空矩形
    }

    for (size_t i = 0; i < currentBatchRect.size(); ++i) {
        const cv::Rect& rect = currentBatchRect[i];

        // 跳过无效矩形
        if (rect.width <= 0 || rect.height <= 0) continue;

        // 创建调整后的矩形（Y坐标偏移）
        cv::Rect adjustedRect = rect;
        adjustedRect.y += startY[i];

        // 首次有效矩形直接赋值
        if (firstValid) {
            mergedRect = adjustedRect;
            firstValid = false;
        }
        // 后续矩形使用并集合并
        else {
            // 计算合并后的矩形边界
            int x1 = std::min(mergedRect.x, adjustedRect.x);
            int y1 = std::min(mergedRect.y, adjustedRect.y);
            int x2 = std::max(mergedRect.x + mergedRect.width,
                adjustedRect.x + adjustedRect.width);
            int y2 = std::max(mergedRect.y + mergedRect.height,
                adjustedRect.y + adjustedRect.height);

            mergedRect = cv::Rect(x1, y1, x2 - x1, y2 - y1);
        }
    }

    return mergedRect;
}

// 将点集顺时针旋转90度
void GeneralMethod::rotatePointsClockwise90(const std::vector<cv::Point>& srcPoints,
    std::vector<cv::Point>& dstPoints,
    int width, int height)
{
    dstPoints.clear();
    for (const auto& pt : srcPoints) {
        int new_x = pt.y;
        int new_y = height - 1 - pt.x;
        dstPoints.push_back(cv::Point(new_x, new_y));
    }
}

// 将边缘点转换为 FLANN 所需格式
cv::Mat GeneralMethod::convertEdgePointsToMat(const std::vector<cv::Point>& edgePoints) {
    cv::Mat data(static_cast<int>(edgePoints.size()), 2, CV_32F);
    for (size_t i = 0; i < edgePoints.size(); ++i) {
        data.at<float>(i, 0) = static_cast<float>(edgePoints[i].x);
        data.at<float>(i, 1) = static_cast<float>(edgePoints[i].y);
    }
    return data;
}
/*===============获取计算信息======================*/

/*===============界面显示信息处理======================*/
/// <summary>
/// 缺陷等级排序功能，以实现界面显示顺序为 红 黄 绿
/// </summary>
/// <param name="results"></param>
/// <returns></returns>
std::vector<drawInformation> GeneralMethod::filterAndSortDefects(const std::vector<drawInformation>& results) {
    std::vector<drawInformation> filtered;

    // 过滤出 MINOR, MEDIUM, SERIOUS
    for (const auto& info : results) {
        if (info.ErrorType == DefectLevel::MINOR ||
            info.ErrorType == DefectLevel::MEDIUM ||
            info.ErrorType == DefectLevel::SERIOUS) {
            filtered.push_back(info);
        }
    }

    // 排序：MINOR(1) < MEDIUM(2) < SERIOUS(3)
    std::sort(filtered.begin(), filtered.end(),
        [](const drawInformation& a, const drawInformation& b) {
            return static_cast<int>(a.ErrorType) < static_cast<int>(b.ErrorType);
        });

    return filtered;
}

/// <summary>
/// 获取绘制圆点的rect
/// </summary>
/// <param name="inputRect"></param>
/// <param name="imageWidth"></param>
/// <param name="imageHeight"></param>
/// <param name="imageSize"></param>
/// <param name="outRelativeRect"></param>
/// <returns></returns>
cv::Rect GeneralMethod::computeCropRect2Color(const cv::Rect& inputRect, int imageWidth, int imageHeight, int imageSize, cv::Rect* outRelativeRect)
{
    // 计算输入 Rect 的中心点
    int centerX = inputRect.x + inputRect.width / 2;
    int centerY = inputRect.y + inputRect.height / 2;

    // 固定裁剪尺寸为 imageSize x imageSize（如640x640）
    int cropWidth = imageSize;
    int cropHeight = imageSize;

    // 计算裁剪区域的左上角坐标
    int x = centerX - cropWidth / 2;
    int y = centerY - cropHeight / 2;

    // 确保裁剪区域不超出图像边界
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + cropWidth > imageWidth) x = imageWidth - cropWidth;
    if (y + cropHeight > imageHeight) y = imageHeight - cropHeight;

    cv::Rect cropRect(x, y, cropWidth, cropHeight);

    // 如果提供了 outRelativeRect，则计算 inputRect 在 cropRect 中的相对位置
    if (outRelativeRect != nullptr)
    {
        int relativeX = inputRect.x - x;
        int relativeY = inputRect.y - y;
        int relativeWidth = inputRect.width;
        int relativeHeight = inputRect.height;

        // 确保 rect 不越界
        relativeX = std::max(0, relativeX);
        relativeY = std::max(0, relativeY);
        relativeWidth = std::min(relativeWidth, cropWidth - relativeX);
        relativeHeight = std::min(relativeHeight, cropHeight - relativeY);

        *outRelativeRect = cv::Rect(relativeX, relativeY, relativeWidth, relativeHeight);
    }

    return cropRect;
}


/// <summary>
/// 绘制展示缺陷效果图
/// </summary>
/// <param name="src"></param>
/// <param name="rect"></param>
/// <param name="alpha"></param>
/// <param name="minRadius"></param>
/// <param name="maxRadius"></param>
/// <returns></returns>
cv::Mat GeneralMethod::drawSemiTransparentRedOverlayWithCircle(const cv::Mat& src, const cv::Rect& rect, double alpha, int minRadius, int maxRadius)
{
    // 确保输入是4通道图像（BGRA）
    cv::Mat result;
    if (src.channels() == 3) {
        cv::cvtColor(src, result, cv::COLOR_BGR2BGRA);
    }
    else if (src.channels() == 1) {
        cv::cvtColor(src, result, cv::COLOR_GRAY2BGRA);
    }
    else {
        src.copyTo(result);
    }

    // 创建一个与原图相同大小的全黑图像，并在其上绘制不透明红色区域
    cv::Mat overlay = result.clone();
    overlay.setTo(cv::Scalar(0, 0, 0, 0));  // 全透明背景

    // 计算最大内切圆的半径
    int radius = std::min(rect.width, rect.height) / 2;

    // 应用最大最小半径约束
    radius = std::max(minRadius, std::min(radius, maxRadius));

    // 计算圆心位置
    cv::Point center(rect.x + rect.width / 2, rect.y + rect.height / 2);

    // 绘制填充的红色圆形
    cv::circle(overlay, center, radius, cv::Scalar(0, 0, 255, 255), cv::FILLED);

    // 将 overlay 与 result 进行融合，alpha 控制透明度
    cv::addWeighted(overlay, alpha, result, 1 - alpha, 0, result);

    return result;
}


void GeneralMethod::drawRectCenter(cv::Mat& img, const cv::Rect& rect, const cv::Scalar& color = cv::Scalar(0, 0, 255)) {
    int centerX = rect.x + rect.width / 2;
    int centerY = rect.y + rect.height / 2;
    cv::Point center(centerX, centerY);

    // 可选：绘制矩形框
    cv::rectangle(img, rect, cv::Scalar(0, 255, 0), 2);


    // 绘制中心点（红色圆点）
    cv::circle(img, center, 101, color, -1);
}

std::vector<drawInformation> GeneralMethod::real2Virtual(std::vector<drawInformation> results, double scale) {
    std::vector<drawInformation> result2Virtual;
    // 遍历 results 并调整 rect
    for (auto& info : results) {
        drawInformation  tempdraw;
        tempdraw = info;
        tempdraw.rect.x = static_cast<int>(info.rect.x * scale);
        tempdraw.rect.y = static_cast<int>(info.rect.y * scale);
        tempdraw.rect.width = static_cast<int>(info.rect.width * scale);
        tempdraw.rect.height = static_cast<int>(info.rect.height * scale);
        result2Virtual.push_back(tempdraw);
    }

    return result2Virtual;
}

std::vector<drawInformation> GeneralMethod::real2Virtual_(std::vector<drawInformation> results, double widthScale, double heightScale) {
    std::vector<drawInformation> result2Virtual;

    // 遍历 results 并调整 rect
    for (auto& info : results) {
        drawInformation  tempdraw;
        tempdraw = info;
        tempdraw.rect.x = static_cast<int>(info.rect.x * widthScale);
        tempdraw.rect.y = static_cast<int>(info.rect.y * heightScale);
        tempdraw.rect.width = static_cast<int>(info.rect.width * widthScale);
        tempdraw.rect.height = static_cast<int>(info.rect.height * heightScale);
        result2Virtual.push_back(tempdraw);
    }

    return result2Virtual;
}

/// <summary>
/// 20260212 修改，像素坐标转虚拟图上坐标
/// </summary>
/// <param name="width"></param>
/// <param name="height"></param>
/// <param name="results"></param>
/// <param name="widthScale"></param>
/// <param name="heightScale"></param>
/// <param name="box"></param>
/// <returns></returns>
std::vector<drawInformation> GeneralMethod::real2VirtualResize(int width, int height, std::vector<drawInformation>& results, double widthScale, double heightScale, int box) {
    std::vector<drawInformation> result2Virtual;

    // 遍历 results 并调整 rect
    int infoIndex = 0;
    for (auto& info : results) {
        info.uuid = QUuid::createUuid();
        drawInformation tempdraw = info;
        tempdraw.uuid = info.uuid;

        // 调整 rect 的位置和大小
        tempdraw.rect.x = static_cast<int>(info.rect.x * widthScale);
        tempdraw.rect.y = static_cast<int>(info.rect.y * heightScale);
        tempdraw.rect.width = static_cast<int>(info.rect.width * widthScale);
        tempdraw.rect.height = static_cast<int>(info.rect.height * heightScale);
        tempdraw.confidence = info.confidence;
        tempdraw.realRect = info.rect; // 保存原始的rect,像素单位。

        // 确保最小尺寸 - 即使为0也设为最小2
        if (tempdraw.rect.width <= 0) {
            tempdraw.rect.width = 2;
        }
        if (tempdraw.rect.height <= 0) {
            tempdraw.rect.height = 2;
        }

        // ========== 修改：保留所有缺陷，只做边界修正 ==========

        // 1. 处理完全在图像外的情况（x或y坐标完全超出边界）
        // 对于这种缺陷，我们可以尝试将它移到边界上
        if (tempdraw.rect.x >= width) {
            // x坐标完全在右边界外，移动到右边界
            tempdraw.rect.x = width - 1; // 确保在图像内
            // 如果宽度较大，可能需要调整宽度
            if (tempdraw.rect.x + tempdraw.rect.width > width) {
                tempdraw.rect.width = 1; // 最小宽度
            }
        }
        else if (tempdraw.rect.x + tempdraw.rect.width <= 0) {
            // 整个矩形在左边界外，移动到左边界
            tempdraw.rect.x = 0;
        }

        if (tempdraw.rect.y >= height) {
            // y坐标完全在下边界外，移动到下边界
            tempdraw.rect.y = height - 1;
            // 如果高度较大，可能需要调整高度
            if (tempdraw.rect.y + tempdraw.rect.height > height) {
                tempdraw.rect.height = 1; // 最小高度
            }
        }
        else if (tempdraw.rect.y + tempdraw.rect.height <= 0) {
            // 整个矩形在上边界外，移动到上边界
            tempdraw.rect.y = 0;
        }

        // 2. 修正负坐标（部分在图像外）
        if (tempdraw.rect.x < 0) {
            // 记录原始宽度
            int originalWidth = tempdraw.rect.width;
            // 调整宽度，减去越界部分
            tempdraw.rect.width = tempdraw.rect.width + tempdraw.rect.x; // x是负数，所以是减法
            // 移动到边界
            tempdraw.rect.x = 0;

            // 确保调整后的宽度有效
            if (tempdraw.rect.width <= 0) {
                tempdraw.rect.width = 2; // 最小宽度
            }
        }

        if (tempdraw.rect.y < 0) {
            // 记录原始高度
            int originalHeight = tempdraw.rect.height;
            // 调整高度，减去越界部分
            tempdraw.rect.height = tempdraw.rect.height + tempdraw.rect.y; // y是负数，所以是减法
            // 移动到边界
            tempdraw.rect.y = 0;

            // 确保调整后的高度有效
            if (tempdraw.rect.height <= 0) {
                tempdraw.rect.height = 2; // 最小高度
            }
        }

        // 3. 修正右/下边界越界（部分在图像外）
        if (tempdraw.rect.x + tempdraw.rect.width > width) {
            // 调整宽度，使其在边界内
            tempdraw.rect.width = width - tempdraw.rect.x;
            // 确保调整后的宽度有效
            if (tempdraw.rect.width <= 0) {
                // 如果调整后宽度无效，确保至少显示一个像素
                if (tempdraw.rect.x < width) {
                    tempdraw.rect.width = 1;
                }
                else {
                    // 如果x坐标已经在边界外，强制移到边界内
                    tempdraw.rect.x = width - 1;
                    tempdraw.rect.width = 1;
                }
            }
        }

        if (tempdraw.rect.y + tempdraw.rect.height > height) {
            // 调整高度，使其在边界内
            tempdraw.rect.height = height - tempdraw.rect.y;
            // 确保调整后的高度有效
            if (tempdraw.rect.height <= 0) {
                // 如果调整后高度无效，确保至少显示一个像素
                if (tempdraw.rect.y < height) {
                    tempdraw.rect.height = 1;
                }
                else {
                    // 如果y坐标已经在边界外，强制移到边界内
                    tempdraw.rect.y = height - 1;
                    tempdraw.rect.height = 1;
                }
            }
        }

        // 4. 最终确保矩形在图像边界内（防御性编程）
        // 确保x坐标在[0, width-1]范围内
        if (tempdraw.rect.x < 0) tempdraw.rect.x = 0;
        if (tempdraw.rect.x >= width) tempdraw.rect.x = width - 1;

        // 确保y坐标在[0, height-1]范围内
        if (tempdraw.rect.y < 0) tempdraw.rect.y = 0;
        if (tempdraw.rect.y >= height) tempdraw.rect.y = height - 1;

        // 确保宽度有效
        if (tempdraw.rect.width <= 0) tempdraw.rect.width = 1;
        if (tempdraw.rect.x + tempdraw.rect.width > width) {
            tempdraw.rect.width = width - tempdraw.rect.x;
        }

        // 确保高度有效
        if (tempdraw.rect.height <= 0) tempdraw.rect.height = 1;
        if (tempdraw.rect.y + tempdraw.rect.height > height) {
            tempdraw.rect.height = height - tempdraw.rect.y;
        }

        // 5. 确保最终矩形在图像内（最终验证）
        tempdraw.rect.x = std::max(0, std::min(tempdraw.rect.x, width - 1));
        tempdraw.rect.y = std::max(0, std::min(tempdraw.rect.y, height - 1));

        // 计算最大允许宽度和高度
        int maxAllowedWidth = width - tempdraw.rect.x;
        int maxAllowedHeight = height - tempdraw.rect.y;

        tempdraw.rect.width = std::max(1, std::min(tempdraw.rect.width, maxAllowedWidth));
        tempdraw.rect.height = std::max(1, std::min(tempdraw.rect.height, maxAllowedHeight));

        result2Virtual.push_back(tempdraw);
        infoIndex++;
    }

    FILE_LOG_INFO("real2VirtualResize: processed %zu defects, all %zu defects retained",
        results.size(), result2Virtual.size());
    return result2Virtual;
}
/// <summary>
/// 图像逆时针旋转90度，保证显示效果与实际效果一致
/// </summary>
/// <param name="images"></param>
/// <param name="flipV"></param>
/// <returns></returns>
int GeneralMethod::RotateImage(std::vector<cv::Mat>& images, int flipV = 0) {

    for (size_t i = 0; i < images.size(); i++)
    {
        // 逆时针旋转 90 度
        transpose(images[i], images[i]); // 先转置
        flip(images[i], images[i], 0);   // 再垂直翻转

        if (flipV == 1) {
            // 水平翻转图片
            cv::flip(images[i], images[i], 0);
        }
    }
    return 0;
}

/// <summary>
/// 获取大图中的裁剪区域640*640
/// </summary>
/// <param name="inputRect"></param>
/// <param name="imageWidth"></param>
/// <param name="imageHeight"></param>
/// <returns></returns>
cv::Rect GeneralMethod::computeCropRect(const cv::Rect& inputRect, int imageWidth, int imageHeight, int imageSize) {
    // 计算输入 Rect 的中心点
    int centerX = inputRect.x + inputRect.width / 2;
    int centerY = inputRect.y + inputRect.height / 2;

    // 固定裁剪尺寸为 640x640
    int cropWidth = imageSize;
    int cropHeight = imageSize;

    // 计算裁剪区域的左上角坐标
    int x = centerX - cropWidth / 2;
    int y = centerY - cropHeight / 2;

    // 确保裁剪区域不超出图像边界
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + cropWidth > imageWidth) x = imageWidth - cropWidth;
    if (y + cropHeight > imageHeight) y = imageHeight - cropHeight;

    return cv::Rect(x, y, cropWidth, cropHeight);
}
/*===============界面显示信息处理======================*/


/*===============测试所需函数======================*/
/// <summary>
/// 绘制检测结果，用于判断缩放图的rect区域是否一致
/// </summary>
/// <param name="image"></param>
/// <param name="results"></param>
void GeneralMethod::drawResults(cv::Mat& image, const std::vector<drawInformation>& results) {
    for (const auto& result : results) {
        // 绘制矩形框
        cv::rectangle(image, result.rect, cv::Scalar(0, 255, 0), 2); // 绿色框，线宽为2

        //// 将类型转换为字符串
        //std::string errorTypeStr;
        //switch (result.ErrorType) {
        //case MINOR: errorTypeStr = "MINOR"; break;
        //case MEDIUM: errorTypeStr = "MEDIUM"; break;
        //case SERIOUS: errorTypeStr = "SERIOUS"; break;
        //default: errorTypeStr = "ABNORMAL";
        //}

        //std::string defectTypeStr;
        //switch (result.DefectType) {
        //case TYPE_POORCOATING: defectTypeStr = "0"; break;
        //case TYPE_SCRATCH: defectTypeStr = "1"; break;
        //case TYPE_CALCULUS: defectTypeStr = "2"; break;
        //case TYPE_BUBBLE: defectTypeStr = "3"; break;
        //case TYPE_TRADEMARK: defectTypeStr = "4"; break;
        //case TYPE_WATERSTAIN: defectTypeStr = "5"; break;
        //case TYPE_SMUDGE: defectTypeStr = "6"; break;
        //default: defectTypeStr = "ABNORMAL";
        //}

        //// 拼接类型信息
        //std::string label = errorTypeStr + ", " + defectTypeStr;

        //// 设置文本位置（矩形框左下角）
        //int fontFace = cv::FONT_HERSHEY_SIMPLEX;
        //double fontScale = 0.5;
        //int thickness = 1;
        //int baseline = 0;

        //cv::Size textSize = cv::getTextSize(label, fontFace, fontScale, thickness, &baseline);
        //cv::Point textOrg(result.rect.x, result.rect.y + result.rect.height); // 左下角坐标

        //// 绘制文本背景（可选）
        //cv::rectangle(image, textOrg + cv::Point(-5, baseline - textSize.height - 5),
        //    textOrg + cv::Point(textSize.width + 5, baseline + 5), cv::Scalar(0, 255, 0), -1);

        //// 绘制文本
        //cv::putText(image, label, textOrg, fontFace, fontScale, cv::Scalar(0, 0, 0), thickness, cv::LINE_AA);
    }
}

/*===============测试所需函数======================*/
// 图像拼接和检测结果转换函数
std::vector<drawInformation> GeneralMethod::stitchImagesAndTransformRects(
    const std::vector<cv::Mat>& images, const cv::Mat& stitchedImage,
    const std::vector<std::vector<drawInformation>>& batchDrawInfo)
{

    // 2. 计算每个图像的Y偏移量
    std::vector<int> yOffsets;
    int offset = 0;
    for (const auto& img : images) {
        yOffsets.push_back(offset);
        offset += img.rows;
    }

    // 3. 转换所有检测结果到拼接图坐标系
    std::vector<drawInformation> transformedDrawInfo;


    for (size_t imgIdx = 0; imgIdx < batchDrawInfo.size(); ++imgIdx) {
        const int yOffset = yOffsets[imgIdx];




        for (const auto& drawInfo : batchDrawInfo[imgIdx]) {
            // 创建新的检测结果（复制所有数据）
            drawInformation transformed = drawInfo;

            // 只修改rect的Y坐标（X保持不变）
            transformed.rect.y += yOffset;

            // 可选：验证矩形是否在图像范围内
            if (transformed.rect.x < 0) transformed.rect.x = 0;
            if (transformed.rect.y < 0) transformed.rect.y = 0;
            if (transformed.rect.x + transformed.rect.width > stitchedImage.cols) {
                transformed.rect.width = stitchedImage.cols - transformed.rect.x;
            }
            if (transformed.rect.y + transformed.rect.height > stitchedImage.rows) {
                transformed.rect.height = stitchedImage.rows - transformed.rect.y;
            }

            transformedDrawInfo.push_back(transformed);
        }
    }

    return transformedDrawInfo;
}

// 图像拼接和检测结果转换函数
std::vector<drawInformation> GeneralMethod::stitchImagesAndTransformRects(
    const std::vector<cv::Mat>& images, const cv::Mat& stitchedImage,
    const std::vector<std::vector<drawInformation>>& batchDrawInfo, std::vector<cv::Rect>& rects)
{

    // 2. 计算每个图像的Y偏移量
    std::vector<int> yOffsets;
    int offset = 0;
    for (const auto& img : images) {
        yOffsets.push_back(offset);
        offset += img.rows;
    }

    // 3. 转换所有检测结果到拼接图坐标系
    std::vector<drawInformation> transformedDrawInfo;

    for (size_t imgIdx = 0; imgIdx < batchDrawInfo.size(); ++imgIdx) {
        const int yOffset = yOffsets[imgIdx];

        for (const auto& drawInfo : batchDrawInfo[imgIdx]) {
            // 创建新的检测结果（复制所有数据）
            drawInformation transformed = drawInfo;

            // 只修改rect的Y坐标（X保持不变）
            transformed.rect.y += (yOffset + rects[imgIdx].y);
            transformed.rect.x += rects[imgIdx].x;
            // 可选：验证矩形是否在图像范围内
            if (transformed.rect.x < 0) transformed.rect.x = 0;
            if (transformed.rect.y < 0) transformed.rect.y = 0;
            if (transformed.rect.x + transformed.rect.width > stitchedImage.cols) {
                transformed.rect.width = stitchedImage.cols - transformed.rect.x;
            }
            if (transformed.rect.y + transformed.rect.height > stitchedImage.rows) {
                transformed.rect.height = stitchedImage.rows - transformed.rect.y;
            }

            transformedDrawInfo.push_back(transformed);
        }
    }

    return transformedDrawInfo;
}


// 函数：将检测结果转换到裁剪后的ROI坐标系
std::vector<drawInformation> GeneralMethod::transformToROICoordinates(
    const std::vector<drawInformation>& largeDrawInfo,
    const cv::Rect& roiRect)
{
    std::vector<drawInformation> roiDrawInfo;
    roiDrawInfo.reserve(largeDrawInfo.size());

    for (const auto& info : largeDrawInfo) {
        // 创建新对象（复制所有数据）
        drawInformation transformed = info;

        // 将坐标转换为相对于ROI的坐标系
        transformed.rect.x -= roiRect.x;
        transformed.rect.y -= roiRect.y;

        // 确保转换后的矩形在ROI范围内
        if (transformed.rect.x < 0) {
            transformed.rect.width += transformed.rect.x; // 调整宽度
            transformed.rect.x = 0;
        }
        if (transformed.rect.y < 0) {
            transformed.rect.height += transformed.rect.y; // 调整高度
            transformed.rect.y = 0;
        }
        if (transformed.rect.x + transformed.rect.width > roiRect.width) {
            transformed.rect.width = roiRect.width - transformed.rect.x;
        }
        if (transformed.rect.y + transformed.rect.height > roiRect.height) {
            transformed.rect.height = roiRect.height - transformed.rect.y;
        }

        // 如果矩形仍然有效则保留
        if (transformed.rect.width > 0 && transformed.rect.height > 0) {
            roiDrawInfo.push_back(transformed);
        }
    }

    return roiDrawInfo;
}


// 修正的矩形旋转函数（逆时针90度）
cv::Rect GeneralMethod::rotateRectCCW90(const cv::Rect& rect, int width, int height)
{
    // 直接计算旋转后的矩形（更准确的公式）
    int new_x = rect.y;
    int new_y = width - rect.x - rect.width;
    int new_width = rect.height;
    int new_height = rect.width;

    return cv::Rect(new_x, new_y, new_width, new_height);
}

// 修正的检测结果旋转函数
std::vector<drawInformation> GeneralMethod::rotateDrawInfoCCW90(
    const std::vector<drawInformation>& drawInfos,
    int width, int height)
{
    std::vector<drawInformation> rotatedInfos;
    for (const auto& info : drawInfos) {
        drawInformation rotated = info;
        // 旋转矩形
        rotated.rect = rotateRectCCW90(rotated.rect, width, height);

        // 添加边界检查，确保不超出旋转后图像范围
        int new_img_width = height;  // 旋转后图像宽度 = 原高度
        int new_img_height = width;  // 旋转后图像高度 = 原宽度

        if (rotated.rect.x < 0) rotated.rect.x = 0;
        if (rotated.rect.y < 0) rotated.rect.y = 0;
        if (rotated.rect.x + rotated.rect.width > new_img_width) {
            rotated.rect.width = new_img_width - rotated.rect.x;
        }
        if (rotated.rect.y + rotated.rect.height > new_img_height) {
            rotated.rect.height = new_img_height - rotated.rect.y;
        }

        // 确保矩形有效
        if (rotated.rect.width > 0 && rotated.rect.height > 0) {
            rotatedInfos.push_back(rotated);
        }
    }
    return rotatedInfos;
}


cv::Mat GeneralMethod::vconcatVariableWidth(const std::vector<cv::Mat>& images,
    const cv::Scalar& bgColor = cv::Scalar(0, 0, 0))
{
    if (images.empty()) return cv::Mat();


    // 1. 计算最大宽度
    int maxWidth = 0;
    for (const auto& img : images) {
        if (img.cols > maxWidth) maxWidth = img.cols;

    }

    // 2. 创建目标图像集
    std::vector<cv::Mat> paddedImages;

    for (const auto& img : images) {
        if (img.cols == maxWidth) {
            paddedImages.push_back(img);


        }
        else {
            // 3. 填充图像
            cv::Mat padded(maxWidth, img.rows, img.type(), bgColor);
            img.copyTo(padded(cv::Rect(0, 0, img.cols, img.rows)));
            paddedImages.push_back(padded.t()); // 转置使宽高适配vconcat
        }
    }

    // 4. 纵向拼接
    cv::Mat result;
    cv::vconcat(paddedImages, result);
    return result;
}


/// <summary>
/// 大尺寸图像顺时针旋转90度 分块旋转，减少缓存失效
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <param name="blockSize"></param>
void GeneralMethod::blockRotate90Clockwise(cv::Mat& src, cv::Mat& dst, int blockSize = 1024) {
    dst.create(src.cols, src.rows, src.type());

#pragma omp parallel for collapse(2)
    for (int block_i = 0; block_i < src.rows; block_i += blockSize) {
        for (int block_j = 0; block_j < src.cols; block_j += blockSize) {
            int end_i = std::min(block_i + blockSize, src.rows);
            int end_j = std::min(block_j + blockSize, src.cols);

            for (int i = block_i; i < end_i; i++) {
                const uchar* src_ptr = src.ptr<uchar>(i);
                for (int j = block_j; j < end_j; j++) {
                    int dst_i = j;
                    int dst_j = src.rows - 1 - i;
                    uchar* dst_ptr = dst.ptr<uchar>(dst_i);
                    dst_ptr[dst_j] = src_ptr[j];
                }
            }
        }
    }
}


/// <summary>
/// 大尺寸图像逆时针旋转90度 分块旋转，减少缓存失效
/// </summary>
/// <param name="src">源图像</param>
/// <param name="dst">目标图像（输出）</param>
/// <param name="blockSize">分块大小，默认1024</param>
void GeneralMethod::blockRotate90CounterClockwise(cv::Mat& src, cv::Mat& dst, int blockSize = 1024) {
    // 创建目标图像：宽度=原高度，高度=原宽度
    dst.create(src.cols, src.rows, src.type());

    const int src_rows = src.rows;
    const int src_cols = src.cols;
    const int channels = src.channels();

#pragma omp parallel for collapse(2)
    for (int block_i = 0; block_i < src_rows; block_i += blockSize) {
        for (int block_j = 0; block_j < src_cols; block_j += blockSize) {
            int end_i = std::min(block_i + blockSize, src_rows);
            int end_j = std::min(block_j + blockSize, src_cols);

            for (int i = block_i; i < end_i; i++) {
                const uchar* src_ptr = src.ptr<uchar>(i);
                for (int j = block_j; j < end_j; j++) {
                    // 逆时针旋转90度映射的更正公式：
                    // 原图(i,j) -> 目标图(src.cols-1-j, i)
                    int dst_i = src_cols - 1 - j;
                    int dst_j = i;

                    uchar* dst_ptr = dst.ptr<uchar>(dst_i);

                    if (channels == 1) {
                        dst_ptr[dst_j] = src_ptr[j];
                    }
                    else {
                        int src_index = j * channels;
                        int dst_index = dst_j * channels;
                        for (int c = 0; c < channels; c++) {
                            dst_ptr[dst_index + c] = src_ptr[src_index + c];
                        }
                    }
                }
            }
        }
    }
}
/*===============边采边计算======================*/


/*===============高效拼接函数======================*/
/**
 * @brief 高效纵向拼接多个图像（类似vconcat的增强版）
 * @param images 输入图像vector，所有图像必须具有相同的宽度和类型
 * @param result 输出拼接结果
 * @return 拼接是否成功
 */
bool GeneralMethod::efficientVerticalConcat(
    const std::vector<cv::Mat>& images,
    cv::Mat& result
) {
    // 1. 检查输入有效性
    if (images.empty()) {
        result.release();
        return true; // 空输入返回空图像，但算成功
    }

    // 2. 验证所有图像尺寸和类型一致性
    const int ref_width = images[0].cols;
    const int ref_type = images[0].type();
    int total_height = 0;

    for (const auto& img : images) {
        if (img.empty()) {
            std::cerr << "存在空图像!" << std::endl;
            result.release();
            return false;
        }
        if (img.cols != ref_width || img.type() != ref_type) {
            std::cerr << "图像宽度或类型不匹配!" << std::endl;
            std::cerr << "期望宽度: " << ref_width << ", 类型: " << ref_type << std::endl;
            std::cerr << "实际宽度: " << img.cols << ", 类型: " << img.type() << std::endl;
            result.release();
            return false;
        }
        total_height += img.rows;
    }

    // 3. 预分配结果内存
    if (result.empty() ||
        result.rows != total_height ||
        result.cols != ref_width ||
        result.type() != ref_type) {
        result.create(total_height, ref_width, ref_type);
        if (result.empty()) {
            std::cerr << "内存分配失败!" << std::endl;
            return false;
        }
    }

    // 4. 并行填充数据（按图像并行）
    int current_y = 0;

    // 如果图像数量较多，使用并行处理
    if (images.size() > 5) {
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(images.size()); i++) {
            // 计算当前图像的y偏移量（需要同步计算）
            int y_offset = 0;
            for (int j = 0; j < i; j++) {
                y_offset += images[j].rows;
            }

            cv::Mat roi = result(cv::Rect(0, y_offset, ref_width, images[i].rows));
            images[i].copyTo(roi);
        }
    }
    else {
        // 图像数量少时直接顺序处理（避免并行开销）
        for (const auto& img : images) {
            cv::Mat roi = result(cv::Rect(0, current_y, ref_width, img.rows));
            img.copyTo(roi);
            current_y += img.rows;
        }
    }

    return true;
}

/**
 * @brief 高效纵向拼接多个图像（简化版接口）
 * @param images 输入图像vector
 * @return 拼接后的图像，失败返回空Mat
 */
cv::Mat GeneralMethod::efficientVerticalConcat(const std::vector<cv::Mat>& images) {
    cv::Mat result;
    if (efficientVerticalConcat(images, result)) {
        return result;
    }
    return cv::Mat();
}
/*===============高效拼接函数======================*/



/***************************************20250811 软件算法通用库整理 王***************************************/	

/*==========缩略图生成函数=================*/
// 定义颜色映射（根据缺陷等级）
cv::Scalar GeneralMethod::getDefectColor(DefectLevel level) {
    switch (level) {
    case SERIOUS:    return cv::Scalar(0, 0, 255);     // 红色 - 严重缺陷
    case MEDIUM:     return cv::Scalar(0, 255, 255);   // 黄色 - 中等缺陷
    case MINOR:      return cv::Scalar(0, 255, 0);   // 绿色 - 轻微缺陷
    case ABNORMAL:   return cv::Scalar(128, 0, 128);   // 紫色 - 异常
    case AREAERROR:  return cv::Scalar(128, 128, 128); // 灰色 - 区域错误
    case NORMAL:     return cv::Scalar(0, 255, 0);     // 绿色 - 正常
    default:         return cv::Scalar(255, 255, 255); // 白色 - 默认
    }
}

// 比较函数，按缺陷等级降序排列
bool compareByLevel(const drawInformation& a, const drawInformation& b) {
    return a.ErrorType > b.ErrorType;
}
/**
 * @brief 在缩略图上绘制缺陷信息
 * @param defects 输入缺陷信息vector
 * @param originalSize 输入图像的原始尺寸
 * @param thumbnailSize 缩略图的尺寸
 * @param canvasGrayValue 缩略图画布的灰度值
 * @return 拼接后的图像，失败返回空Mat
 */
cv::Mat GeneralMethod::generateDefectThumbnail(const std::vector<drawInformation>& defects,
    const cv::Size& originalSize,
    const cv::Size& thumbnailSize,
    int canvasGrayValue = 128) { // 添加画布灰度值参数，默认128
    // 创建指定灰度值的缩略图画布
    cv::Mat thumbnail(thumbnailSize, CV_8UC3, cv::Scalar(canvasGrayValue, canvasGrayValue, canvasGrayValue));

    if (defects.empty()) {
        return thumbnail;
    }

    // 计算缩放比例
    double scaleX = static_cast<double>(thumbnailSize.width) / originalSize.width;
    double scaleY = static_cast<double>(thumbnailSize.height) / originalSize.height;

    // 按缺陷等级排序（等级高的先绘制）
    std::vector<drawInformation> sortedDefects = defects;
    std::sort(sortedDefects.begin(), sortedDefects.end(), compareByLevel);

    // 创建等级掩码，用于记录每个像素的最高等级
    cv::Mat levelMask = cv::Mat::zeros(thumbnailSize, CV_8UC1);

    for (const auto& defect : sortedDefects) {
        // 计算缩放后的矩形
        cv::Rect scaledRect(
            static_cast<int>(defect.rect.x * scaleX),
            static_cast<int>(defect.rect.y * scaleY),
            static_cast<int>(defect.rect.width * scaleX),
            static_cast<int>(defect.rect.height * scaleY)
        );

        // 确保矩形在缩略图范围内
        scaledRect = scaledRect & cv::Rect(0, 0, thumbnailSize.width, thumbnailSize.height);

        if (scaledRect.area() > 0) {
            // 创建当前缺陷的掩码
            cv::Mat defectMask = cv::Mat::zeros(thumbnailSize, CV_8UC1);
            cv::rectangle(defectMask, scaledRect, cv::Scalar(255), cv::FILLED);

            // 找出需要更新的区域（当前等级高于已记录等级的区域）
            cv::Mat updateRegion;
            cv::compare(levelMask, cv::Scalar(defect.ErrorType), updateRegion, cv::CMP_LT);

            cv::Mat finalUpdateMask;
            cv::bitwise_and(defectMask, updateRegion, finalUpdateMask);

            // 更新等级掩码
            levelMask.setTo(defect.ErrorType, finalUpdateMask);

            // 在缩略图上绘制缺陷
            cv::Scalar color = getDefectColor(defect.ErrorType);
            thumbnail.setTo(color, finalUpdateMask);
        }
        else
        {
            std::cout << "ccccc" << std::endl;
        }
    }

    return thumbnail;
}
//
///**
// * @brief 在缩略图上绘制缺陷信息
// * @param defects 输入缺陷信息vector
// * @param backGroundImage 输入图像的原始尺寸
// * @param thumbnailSize 缩略图的尺寸
// * @param canvasGrayValue 缩略图画布的灰度值
// * @return 拼接后的图像，失败返回空Mat
// */
//cv::Mat GeneralMethod::generateDefectThumbnail(const std::vector<drawInformation>& defects,
//    const cv::Mat& backGroundImage,
//    const cv::Size& thumbnailSize,
//    int canvasGrayValue = 128) { // 添加画布灰度值参数，默认128
//    // 创建指定灰度值的缩略图画布
//    //cv::Mat thumbnail(thumbnailSize, CV_8UC3, cv::Scalar(canvasGrayValue, canvasGrayValue, canvasGrayValue));
//    cv::Mat thumbnail = backGroundImage;
//    if (defects.empty()) {
//        return thumbnail;
//    }
//
//    //// 计算缩放比例
//    //double scaleX = static_cast<double>(thumbnailSize.width) / originalSize.width;
//    //double scaleY = static_cast<double>(thumbnailSize.height) / originalSize.height;
//
//    // 按缺陷等级排序（等级高的先绘制）
//    std::vector<drawInformation> sortedDefects = defects;
//    std::sort(sortedDefects.begin(), sortedDefects.end(), compareByLevel);
//
//    // 创建等级掩码，用于记录每个像素的最高等级
//    cv::Mat levelMask = cv::Mat::zeros(thumbnailSize, CV_8UC1);
//
//    for (const auto& defect : sortedDefects) {
//        // 计算缩放后的矩形
//        cv::Rect scaledRect(
//            static_cast<int>(defect.rect.x),
//            static_cast<int>(defect.rect.y),
//            static_cast<int>(defect.rect.width),
//            static_cast<int>(defect.rect.height)
//        );
//
//        // 确保矩形在缩略图范围内
//        scaledRect = scaledRect & cv::Rect(0, 0, thumbnailSize.width, thumbnailSize.height);
//
//        if (scaledRect.area() > 0) {
//            // 创建当前缺陷的掩码
//            cv::Mat defectMask = cv::Mat::zeros(thumbnailSize, CV_8UC1);
//            cv::rectangle(defectMask, scaledRect, cv::Scalar(255), cv::FILLED);
//
//            // 找出需要更新的区域（当前等级高于已记录等级的区域）
//            cv::Mat updateRegion;
//            cv::compare(levelMask, cv::Scalar(defect.ErrorType), updateRegion, cv::CMP_LT);
//
//            cv::Mat finalUpdateMask;
//            cv::bitwise_and(defectMask, updateRegion, finalUpdateMask);
//
//            // 更新等级掩码
//            levelMask.setTo(defect.ErrorType, finalUpdateMask);
//
//            // 在缩略图上绘制缺陷
//            cv::Scalar color = getDefectColor(defect.ErrorType);
//            thumbnail.setTo(color, finalUpdateMask);
//        }
//    }
//
//    return thumbnail;
//}