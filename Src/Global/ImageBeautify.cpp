#include "ImageBeautify.h"

ImageBeautify::ImageBeautify()
{
}

ImageBeautify::~ImageBeautify()
{
}


/// <summary>
/// 获取玻璃区域同时返回边缘点，用于剔除离边缘点较近的缺陷区域
/// </summary>
/// <param name="inputGaryImg"></param>
/// <param name="GlassRect"></param>
/// <param name="glassImage"></param>
/// <param name="threshold"></param>
/// <param name="edgePoints"></param>
/// <returns></returns>
int ImageBeautify::getGlassRoi(cv::Mat& inputGaryImg,  cv::Rect& GlassRect, cv::Mat& glassImage,int threshold,std::vector<cv::Point>& edgePoints) {

    // 阈值处理
    cv::Mat binaryImage;
    cv::threshold(inputGaryImg, binaryImage, threshold, 255, cv::THRESH_BINARY);

    // 找到所有白色区域的轮廓
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    //cv::findContours(binaryImage, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    cv::findContours(binaryImage, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

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
    GlassRect = cv::boundingRect(contours[maxIndex]);

    edgePoints = contours[maxIndex];

    // 转换为裁剪图像中的相对坐标
    edgePoints.clear();
    edgePoints.reserve(edgePoints.size());
    for (const auto& pt : contours[maxIndex]) {
        edgePoints.emplace_back(pt.x - GlassRect.x, pt.y - GlassRect.y);
    }

    //获取rect内的图像
    glassImage = inputGaryImg(GlassRect);

    {
        //cv::Mat drawEdge = glassImage.clone();
        //cv::cvtColor(drawEdge, drawEdge, cv::COLOR_GRAY2BGR);
        //// 设置绘制参数
        //cv::Scalar color = cv::Scalar(0, 0, 255); // 红色 (BGR格式)
        //int radius = 3;                           // 圆点半径
        //int thickness = -1;                       // 实心圆

        //// 遍历所有点并绘制到图像上
        //for (const auto& pt : edgePoints) {
        //    cv::circle(drawEdge, pt, radius, color, thickness);
        //}
        //std::cout << "drawEdge" << std::endl;
    }

    return 0;
}

int ImageBeautify::getGlassRoi(cv::Mat& inputGaryImg, cv::Rect& GlassRect, cv::Mat& glassImage, int threshold) {

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
    GlassRect = cv::boundingRect(contours[maxIndex]);

    //获取rect内的图像
    glassImage = inputGaryImg(GlassRect);

    return 0;
}

/// <summary>
/// 仅获取rect区域
/// </summary>
/// <param name="inputGaryImg"></param>
/// <param name="GlassRect"></param>
/// <param name="threshold"></param>
/// <returns></returns>
int ImageBeautify::getGlassRoi(cv::Mat& inputGaryImg, cv::Rect& GlassRect, int threshold) {

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
    GlassRect = cv::boundingRect(contours[maxIndex]);

    return 0;
}

/// <summary>
/// 对美化后的效果图赋值原有缺陷图像，测试单次结果约0.6ms
/// </summary>
/// <param name="background">输入美化后的大图</param>
/// <param name="patch">识别的roi内小图</param>
/// <param name="roi">小图位于大图中的rect区域</param>
/// <param name="colorFlag">颜色标志物  默认为绿色  0：绿色  1 ：黄色  2：红色</param>
/// <param name="colorVal">单次颜色深度 0-255</param>
/// <param name="alpha">透明度  0 - 1  越接近1约接近透明</param>
void ImageBeautify::MergeImage(cv::Mat& background, cv::Mat patch, cv::Rect roi, int colorFlag, int colorVal,double alpha) {

    // 确保背景图像是三通道图像
    if (background.channels() != 3) {
        std::cerr << "Background image must be a 3-channel image!" << std::endl;
        return;
    }



    // 确保ROI区域在背景图像范围内
    if (roi.x < 0 || roi.y < 0 || roi.x + roi.width > background.cols || roi.y + roi.height > background.rows) {
        std::cerr << "ROI is out of bounds!" << std::endl;
        return;
    }

    // 提取背景图像中的ROI区域
    cv::Mat roiBackground = background(roi);

    // 计算背景图像ROI区域的灰度值平均值
    cv::Scalar meanBackground = cv::mean(roiBackground);

    // 计算补丁图像的灰度值平均值
    cv::Scalar meanPatch = cv::mean(patch);

    // 计算灰度值平均值的差值
    double diff = meanBackground[0] - meanPatch[0];

    // 创建一个临时图像来存储调整后的补丁图像
    cv::Mat adjustedPatch = patch.clone();

    ////// 调整补丁图像的灰度值
    //adjustedPatch.convertTo(adjustedPatch, CV_32F);
    //adjustedPatch += diff;
    //adjustedPatch.convertTo(adjustedPatch, background.type());
    // 确保背景图像是三通道图像
    if (adjustedPatch.channels() != 3) {
        cv::cvtColor(adjustedPatch, adjustedPatch, cv::COLOR_GRAY2BGR);
    }
    //// 根据标志位确定颜色
    //cv::Vec3b color = cv::Vec3b(0, colorVal, 0);
    //switch (colorFlag) {
    //case 0: // 绿色
    //    color = cv::Vec3b(0, colorVal, 0);
    //    break;
    //case 1: // 黄色
    //    color = cv::Vec3b(0, colorVal, colorVal);
    //    break;
    //case 2: // 红色
    //    color = cv::Vec3b(0, 0, colorVal);
    //    break;
    //default:
    //    std::cerr << "Invalid color flag!" << std::endl;
    //    return;
    //}

    //// 将颜色应用到调整后的补丁图像上，使其具有半透明效果
    //cv::Mat coloredPatch = cv::Mat::zeros(adjustedPatch.size(), CV_8UC3);
    //coloredPatch.setTo(color);
    //cv::addWeighted(adjustedPatch, alpha, coloredPatch, 1.0 - alpha, 0.0, adjustedPatch);

    // 将调整后的补丁图像放置到背景图像的ROI区域
    //adjustedPatch.copyTo(roiBackground);
    adjustedPatch.copyTo(background(roi));



    //// 定义扩展区域的大小
    //int extendSize = 10; // 可以根据需要调整这个值
    //int extendedWidth = std::min(roi.width + 2 * extendSize, background.cols - roi.x);
    //int extendedHeight = std::min(roi.height + 2 * extendSize, background.rows - roi.y);

    //// 计算扩展后的ROI区域
    //cv::Rect extendedRoi(
    //    std::max(roi.x - extendSize, 0),
    //    std::max(roi.y - extendSize, 0),
    //    extendedWidth,
    //    extendedHeight
    //);

    //// 对扩展后的区域应用双边滤波
    //cv::Mat extendedRegion = background(extendedRoi);
    //cv::Mat filteredRegion;
    //cv::bilateralFilter(extendedRegion, filteredRegion, 5, 20, 20);

    //// 将滤波后的区域放回原图
    //filteredRegion.copyTo(background(extendedRoi));
}


void ImageBeautify::MeanImage(std::vector<std::vector<cv::Mat>> matVector, cv::Mat& meanImage, int threshold)
{
    //获取每个相机玻璃区域的长度
    std::vector<cv::Mat> glassImage;
    std::vector<cv::Rect> glassRect;

    for (int i = 0; i < matVector.size(); i++) {
        cv::Mat glassImage_;
        cv::Rect glassRect_;
        getGlassRoi(matVector[i][2], glassRect_, glassImage_, threshold);
        if (glassImage_.rows == 0 || glassImage_.cols == 0) {

            continue;//该区域无玻璃，跳过该区域

        }
        glassImage.push_back(glassImage_);
        glassRect.push_back(glassRect_);

    }

    //计算图像的最大高度，进行补充
    int maxHeight = 0;
    for (int i = 0; i < glassImage.size(); i++) {
        if (glassRect[i].height > maxHeight) {
            maxHeight = glassRect[i].height;
        }
    }
    // 对于每个图像，如果其高度小于最大高度，则使用最后一行进行补充
    for (int i = 0; i < glassImage.size(); i++) {
        cv::Mat& img = glassImage[i];
        if (img.rows < maxHeight) {
            cv::Mat lastRow = img.row(img.rows - 1);
            int newHeight = maxHeight - img.rows;
            // 创建一个新的图像，宽度与单行数据相同，高度为newHeight
            cv::Mat newImage(newHeight, lastRow.size().width, lastRow.type());

            // 将单行数据复制到新图像的每一行
            for (int i = 0; i < newHeight; ++i) {
                lastRow.copyTo(newImage.row(i));
            }

            cv::Mat newImg;
            cv::vconcat(img, newImage, img);


        }
    }

    cv::Mat largetImage;
    largetImage = glassImage[0];
    for (int i = 1; i < glassImage.size(); i++) {

        cv::hconcat(largetImage, glassImage[i], largetImage);

    }

    cv::Mat largetImageClone = largetImage.clone();

    // 获取图像的行数和列数
    int rows = largetImage.rows;
    int cols = largetImage.cols;

    // 存储每一行的平均值
    std::vector<double> rowMeans(rows);

    // 计算每一行的平均值并替换该行的所有像素值
    for (int i = 0; i < rows; ++i) {
        cv::Mat row = largetImage.row(i);
        cv::Scalar mean = cv::mean(row);
        rowMeans[i] = mean[0];
        row.setTo(mean[0]);
    }

    meanImage= largetImage.clone();
    largetImage.release();

}

/// <summary>
/// 输入多相机多通道的vector，获得合成图像以及背景图像尺寸
/// </summary>
/// <param name="matVector">多相机多通道的图像</param>
/// <param name="lightChannel">用于处理的光场 当前是2  对应亮场</param>
/// <param name="threshold">用于获取玻璃区域的阈值 当前测试非玻璃区域阈值在10以下</param>
/// <param name="mergeImage">多光场合成图像，为进行补充，用于进行算法计算</param>
/// <param name="sizeImage">生成背景图的尺寸 Point x：宽 Point y：高度</param>
/// <param name="grayVal"></param>
void ImageBeautify::getInfoImage(std::vector<std::vector<cv::Mat>> matVector,std::vector<int> lightChannel, int threshold, std::vector<cv::Mat>& mergeImage,cv::Point& sizeImage,int& grayVal) {
    mergeImage.resize(lightChannel.size());
    //获取每个相机玻璃区域的长度
    std::vector<cv::Mat> glassImage_0;
    std::vector<cv::Mat> glassImage_1;
    std::vector<cv::Mat> glassImage_2;
    std::vector<cv::Rect> glassRect;

    for (int i = 0; i < matVector.size(); i++) {
        cv::Mat glassImage_;
        cv::Rect glassRect_;
        getGlassRoi(matVector[i][lightChannel[2]], glassRect_, glassImage_, threshold);
        if (glassImage_.rows == 0 || glassImage_.cols == 0) {

            continue;//该区域无玻璃，跳过该区域

        }

        glassImage_0.push_back(matVector[i][lightChannel[0]](glassRect_));
        glassImage_1.push_back(matVector[i][lightChannel[1]](glassRect_));
        glassImage_2.push_back(glassImage_);
        glassRect.push_back(glassRect_);

    }
    //计算图像的最大高度，进行补充
    int maxHeight = 0;
    for (int i = 0; i < glassImage_2.size(); i++) {
        if (glassRect[i].height > maxHeight) {
            maxHeight = glassRect[i].height;
        }
    }

    // 对于每个图像，如果其高度小于最大高度，则使用0值进行补充
    for (int i = 0; i < glassImage_2.size(); i++) {
        cv::Mat& img0 = glassImage_0[i];
        cv::Mat& img1 = glassImage_1[i];
        cv::Mat& img2 = glassImage_2[i];
        if (img2.rows < maxHeight) {
            int newHeight = maxHeight - img2.rows;
            // 创建一个新的图像，宽度与原图像相同，高度为newHeight，并使用0值填充
            cv::Mat newImage = cv::Mat::zeros(newHeight, img2.cols, img2.type());

            cv::vconcat(img0, newImage, img0);
            cv::vconcat(img1, newImage, img1);
            cv::vconcat(img2, newImage, img2);
        }
    }
    //cv::Mat largetImage;
    mergeImage[0] = glassImage_0[0];
    mergeImage[1] = glassImage_1[0];
    mergeImage[2] = glassImage_2[0];
    for (int i = 1; i < glassImage_2.size(); i++) {

        cv::hconcat(mergeImage[0], glassImage_0[i], mergeImage[0]);
        cv::hconcat(mergeImage[1], glassImage_1[i], mergeImage[1]);
        cv::hconcat(mergeImage[2], glassImage_2[i], mergeImage[2]);

    }

    sizeImage.x = mergeImage[2].cols;
    sizeImage.y = mergeImage[2].rows;

    // 获取图像的高度和宽度
    int height = mergeImage[2].rows;
    int width = mergeImage[2].cols;

    // 计算中心行的索引
    int centerRow = height / 2;

    // 提取中心行
    cv::Mat centerRowPixels = mergeImage[2].row(centerRow);

    // 计算中心行的灰度平均值
    grayVal = static_cast<int>(cv::mean(centerRowPixels)[0]);


}


//void ImageBeautify::mainTest()
//{
//	cv::Mat image = cv::imread("img/1/1.png",0);
//	cv::Mat imageRoi = cv::imread("img/1/5_3.png",0);
//
//    cv::Mat drawImage;
//    cv::cvtColor(image,drawImage,cv::COLOR_GRAY2BGR);
//    cv::Rect roi(700,1000,imageRoi.cols,imageRoi.rows);
//
//    for (int i = 0; i < 100; i++) {
//        auto start = std::chrono::high_resolution_clock::now();
//        MergeImage(drawImage, imageRoi, roi, 1, 50, 1);
//        auto end = std::chrono::high_resolution_clock::now();
//        std::chrono::duration<double> elapsed = end - start;
//
//        std::cout << "Elapsed time: " << elapsed.count() << " seconds" << std::endl;
//
//    }
//
//}

/// <summary>
/// 图例背景颜色更换
/// </summary>
/// <param name="legendImage">输入图例图像RGBA</param>
/// <param name="colorFlag">背景修改颜色 0：绿色 1：黄色  2：红色 其他：绿色</param>
/// <returns></returns>
cv::Mat ImageBeautify::getColorLegend(cv::Mat legendImage,int colorFlag) {

    // 确保输入图像是4通道的RGBA图像
    if (legendImage.channels() != 4) {     
        std::cerr << "Input image must be a 4-channel RGBA image!" << std::endl;
        return legendImage;
    }

    // 创建一个副本以避免修改原始图像
    cv::Mat resultImage = legendImage.clone();

    // 定义颜色
    cv::Vec4b greenColor(0, 255, 0, 255);   // 绿色
    cv::Vec4b yellowColor(0, 255, 255, 255); // 黄色
    cv::Vec4b redColor(0, 0, 255, 255);    // 红色

    // 遍历图像的每个像素
    for (int i = 0; i < resultImage.rows; ++i) {
        for (int j = 0; j < resultImage.cols; ++j) {
            cv::Vec4b& pixel = resultImage.at<cv::Vec4b>(i, j);

            // 检查像素是否透明
            if (pixel[3] == 0) {
                // 根据colorFlag填充颜色
                switch (colorFlag) {
                case 0:
                    pixel = greenColor;
                    break;
                case 1:
                    pixel = yellowColor;
                    break;
                case 2:
                    pixel = redColor;
                    break;
                default:
                    pixel = greenColor;
                    break;
                }
            }
        }
    }

    return resultImage;

}

////生成对应图例样式
//void ImageBeautify::mainTest()
//{
//    // 读取RGBA图像
//    cv::Mat rgbaImage = cv::imread("E:\\code\\C++\\OpenCVDemo\\Project1\\img\\瑕疵图标/脏污.png", cv::IMREAD_UNCHANGED);
//
//    cv::Mat result = getColorLegend(rgbaImage,2);
//
//    std::cout << "Image size: "<< std::endl;
//}


/// <summary>
/// // 获取ROI区域的图像
/// </summary>
/// <param name="image"></param>
/// <param name="startCol">起始列</param>
/// <param name="endCol">终止列</param>
/// <param name="startRow">起始行</param>
/// <param name="endRow">终止行</param>
/// <returns></returns>
cv::Mat ImageBeautify::getChannelRoiImg(cv::Mat& image, int startCol, int endCol, int startRow, int endRow) {
    // 检查列范围是否有效
    // 检查输入的区域是否有效
    if (startCol >= endCol || startRow >= endRow || startCol < 0 || startRow < 0 ) {
        std::cout << "Invalid region specified" << std::endl;
        return image;
    }
    return image(cv::Rect(startCol, startRow, endCol - startCol, endRow - startRow));
}


//std::vector<cv::Mat> ImageBeautify::divImg3Channel(const cv::Mat& src, std::vector<std::vector<float>> start_end,int cameraCol,int roiChannel,int threshold) {
//    //1、图像多通道拆分
//    //高度改变
//    int height = src.rows;
//    int width = src.cols;
//    std::vector<cv::Mat> result;
//    // 确保高度是3的倍数
//    if (height % 3 != 0) {
//        std::cerr << "Height of the image is not a multiple of 3." << std::endl;
//        return result;
//    }
//    // 创建三个目标图像
//    cv::Mat img1 = cv::Mat(height / 3, width, src.type());
//    cv::Mat img2 = cv::Mat(height / 3, width, src.type());
//    cv::Mat img3 = cv::Mat(height / 3, width, src.type());
//    // 拆分图像
//    for (int i = 0; i < height; ++i) {
//        int row = i % 3;
//        int dstRow = i / 3;
//
//        if (row == 0) {
//            src.row(i).copyTo(img1.row(dstRow));
//        }
//        else if (row == 1) {
//            src.row(i).copyTo(img2.row(dstRow));
//        }
//        else if (row == 2) {
//            src.row(i).copyTo(img3.row(dstRow));
//        }
//    }
//    int cameraNum = img1.cols / cameraCol;
//    height = img1.rows;
//
//    // 预计算所有ROI参数
//    std::vector<cv::Rect> rois;
//    rois.reserve(cameraNum);
//    int totalWidth = 0;
//    for (int i = 0; i < cameraNum; ++i) {
//        int start = i * cameraCol + static_cast<int>(start_end[i][0]);
//        int end = i * cameraCol + static_cast<int>(start_end[i][1]);
//        rois.emplace_back(start, 0, end - start, height);
//        totalWidth += rois.back().width;
//    }
//
//    // 预分配目标矩阵
//    cv::Mat channel_1(height, totalWidth, img1.type());
//    cv::Mat channel_2(height, totalWidth, img2.type());
//    cv::Mat channel_3(height, totalWidth, img3.type());
//
//    // 并行处理三个通道
//    auto process_channel = [&](cv::Mat& target, const cv::Mat& source) {
//        int currentCol = 0;
//        for (const auto& roi : rois) {
//            source(roi).copyTo(target(cv::Rect(currentCol, 0, roi.width, height)));
//            currentCol += roi.width;
//        }
//        };
//
//    process_channel(channel_1, img1);
//    process_channel(channel_2, img2);
//    process_channel(channel_3, img3);
//
//    img1.release();
//    img2.release();
//    img3.release();
//
//    cv::Mat glassImage_;
//    cv::Rect glassRect_;
//    getGlassRoi(channel_3, glassRect_, glassImage_, threshold);
//
//    channel_2 = channel_2(glassRect_);
//    channel_1 = channel_1(glassRect_);
//
//
//    result.push_back(channel_1);
//    result.push_back(channel_2);
//    result.push_back(glassImage_);
//
//    return result;//仅按照通道顺序拆分图像
//}


std::vector<cv::Mat> ImageBeautify::divImg3Channel(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol, int roiChannel, int threshold) {
    //1、图像多通道拆分
    //高度改变
    int height = src.rows;
    int width = src.cols;
    std::vector<cv::Mat> result;
    // 确保高度是3的倍数
    if (height % 3 != 0) {
        std::cerr << "Height of the image is not a multiple of 3." << std::endl;
        return result;
    }
    // 创建三个目标图像
    cv::Mat img1 = cv::Mat(height / 3, width, src.type());
    cv::Mat img2 = cv::Mat(height / 3, width, src.type());
    cv::Mat img3 = cv::Mat(height / 3, width, src.type());
    // 拆分图像
    for (int i = 0; i < height; ++i) {
        int row = i % 3;
        int dstRow = i / 3;

        if (row == 0) {
            src.row(i).copyTo(img1.row(dstRow));
        }
        else if (row == 1) {
            src.row(i).copyTo(img2.row(dstRow));
        }
        else if (row == 2) {
            src.row(i).copyTo(img3.row(dstRow));
        }
    }
    int cameraNum = img1.cols / cameraCol;
    std::cout<<"cameraNum:"<<cameraNum<<",single width:"<<cameraCol << std::endl;
    height = img1.rows;
    cv::Mat channel_1, channel_2, channel_3;
    //2、图像起止点裁剪
    for (int numberCam = 0; numberCam < cameraNum; numberCam++) {
        cv::Mat img = img1;
        cv::Mat imgCut = getChannelRoiImg(img, numberCam * cameraCol + static_cast<int>(start_end[numberCam+1][0]), numberCam * cameraCol + static_cast<int>(start_end[numberCam+1][1]), 0, height);
        if (numberCam == 0) {
            channel_1 = imgCut.clone(); // 第一次赋值
        }
        else {
            cv::hconcat(channel_1, imgCut, channel_1); // 横向拼接
        }

        cv::Mat img_ = img2;
        cv::Mat imgCut_ = getChannelRoiImg(img_, numberCam * cameraCol + static_cast<int>(start_end[numberCam+1][0]), numberCam * cameraCol + static_cast<int>(start_end[numberCam+1][1]), 0, height);
        if (numberCam == 0) {
            channel_2 = imgCut_.clone(); // 第一次赋值
        }
        else {
            cv::hconcat(channel_2, imgCut_, channel_2); // 横向拼接
        }

        cv::Mat img__ = img3;
        cv::Mat imgCut__ = getChannelRoiImg(img__, numberCam * cameraCol + static_cast<int>(start_end[numberCam+1][0]), numberCam * cameraCol + static_cast<int>(start_end[numberCam+1][1]), 0, height);
        if (numberCam == 0) {
            channel_3 = imgCut__.clone(); // 第一次赋值
        }
        else {
            cv::hconcat(channel_3, imgCut__, channel_3); // 横向拼接
        }
    }

    img1.release();
    img2.release();
    img3.release();

    cv::Mat glassImage_;
    cv::Rect glassRect_;
    std::vector<cv::Point> edgePoints;
    //getGlassRoi(channel_3, glassRect_, glassImage_, threshold);
    getGlassRoi(channel_3, glassRect_, glassImage_, threshold, edgePoints);
    channel_2 = channel_2(glassRect_);
    channel_1 = channel_1(glassRect_);


    result.push_back(channel_1);
    result.push_back(channel_2);
    result.push_back(glassImage_);

    return result;//仅按照通道顺序拆分图像
}

/// <summary>
/// 带边缘点集的三通道点拆分
/// </summary>
/// <param name="src"></param>
/// <param name="start_end"></param>
/// <param name="cameraCol"></param>
/// <param name="roiChannel"></param>
/// <param name="threshold"></param>
/// <param name="edgePoints"></param>
/// <returns></returns>
std::vector<cv::Mat> ImageBeautify::divImg3Channel(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol, int roiChannel, int threshold,std::vector<cv::Point>& edgePoints) {
    //1、图像多通道拆分
    //高度改变
    int height = src.rows;
    int width = src.cols;
    std::vector<cv::Mat> result;
    // 确保高度是3的倍数
    if (height % 3 != 0) {
        std::cerr << "Height of the image is not a multiple of 3." << std::endl;
        return result;
    }
    // 创建三个目标图像
    cv::Mat img1 = cv::Mat(height / 3, width, src.type());
    cv::Mat img2 = cv::Mat(height / 3, width, src.type());
    cv::Mat img3 = cv::Mat(height / 3, width, src.type());
    // 拆分图像
    for (int i = 0; i < height; ++i) {
        int row = i % 3;
        int dstRow = i / 3;

        if (row == 0) {
            src.row(i).copyTo(img1.row(dstRow));
        }
        else if (row == 1) {
            src.row(i).copyTo(img2.row(dstRow));
        }
        else if (row == 2) {
            src.row(i).copyTo(img3.row(dstRow));
        }
    }
    int cameraNum = img1.cols / cameraCol;
    std::cout << "cameraNum:" << cameraNum << ",single width:" << cameraCol << std::endl;
    height = img1.rows;
    cv::Mat channel_1, channel_2, channel_3;
    std::vector<cv::Mat> channel1, channel2, channel3;
    //2、图像起止点裁剪
    for (int numberCam = 0; numberCam < cameraNum; numberCam++) {
        cv::Mat img = img1;
        img = getChannelRoiImg(img, numberCam * cameraCol + static_cast<int>(start_end[numberCam + 1][0]), numberCam * cameraCol + static_cast<int>(start_end[numberCam + 1][1]), 0, height);
        //if (numberCam == 0) {
        //    channel_1 = img.clone(); // 第一次赋值
        //}
        //else {
        //    cv::hconcat(channel_1, img, channel_1); // 横向拼接
        //}
        channel1.push_back(img);

        cv::Mat img_ = img2;
        img_ = getChannelRoiImg(img_, numberCam * cameraCol + static_cast<int>(start_end[numberCam + 1][0]), numberCam * cameraCol + static_cast<int>(start_end[numberCam + 1][1]), 0, height);
        //if (numberCam == 0) {
        //    channel_2 = img_.clone(); // 第一次赋值
        //}
        //else {
        //    cv::hconcat(channel_2, img_, channel_2); // 横向拼接
        //}
        channel2.push_back(img_);

        cv::Mat img__ = img3;
        img__ = getChannelRoiImg(img__, numberCam * cameraCol + static_cast<int>(start_end[numberCam + 1][0]), numberCam * cameraCol + static_cast<int>(start_end[numberCam + 1][1]), 0, height);
        //if (numberCam == 0) {
        //    channel_3 = img__.clone(); // 第一次赋值
        //}
        //else {
        //    cv::hconcat(channel_3, img__, channel_3); // 横向拼接
        //}
        channel3.push_back(img__);
    }
    cv::hconcat(channel1, channel_1);
    cv::hconcat(channel2, channel_2);
    cv::hconcat(channel3, channel_3);
    channel1.clear();
    channel2.clear();
    channel3.clear();
    img1.release();
    img2.release();
    img3.release();

    //cv::Mat channel_1, channel_2, channel_3;
    //std::vector<cv::Mat> result;
    //channel_1 = cv::imread("I:/项目文件/玻璃缺陷检测/图像数据/丝印玻璃/丝印玻璃/GP_82456_9_34/82456_0_F0.png", 0);
    //channel_2 = cv::imread("I:/项目文件/玻璃缺陷检测/图像数据/丝印玻璃/丝印玻璃/GP_82456_9_34/82456_0_F1.png", 0);
    //channel_3 = cv::imread("I:/项目文件/玻璃缺陷检测/图像数据/丝印玻璃/丝印玻璃/GP_82456_9_34/82456_0_F2.png", 0);


    cv::Mat glassImage_;
    cv::Rect glassRect_;
    std::vector<cv::Point> edgePoints_;
    //getGlassRoi(channel_3, glassRect_, glassImage_, threshold);
    int flag = getGlassRoi(channel_3, glassRect_, glassImage_, threshold, edgePoints_);
    if (flag == -1) {//未找到玻璃区域
        std::cout << "No Glass Area." << std::endl;
        return result;
    }
    channel_2 = channel_2(glassRect_);
    channel_1 = channel_1(glassRect_);


    result.push_back(channel_1);
    result.push_back(channel_2);
    result.push_back(glassImage_);
    edgePoints = edgePoints_;

    return result;//仅按照通道顺序拆分图像
}

std::vector<cv::Mat> ImageBeautify::divImg3ChannelEntire(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol, int threshold, cv::Rect& rect) {
    //1、图像多通道拆分
    //高度改变
    int height = src.rows;
    int width = src.cols;
    std::vector<cv::Mat> result;
    // 确保高度是3的倍数
    if (height % 3 != 0) {
        std::cerr << "Height of the image is not a multiple of 3." << std::endl;
        return result;
    }
    // 创建三个目标图像
    cv::Mat img1 = cv::Mat(height / 3, width, src.type());
    cv::Mat img2 = cv::Mat(height / 3, width, src.type());
    cv::Mat img3 = cv::Mat(height / 3, width, src.type());
    // 拆分图像
    for (int i = 0; i < height; ++i) {
        int row = i % 3;
        int dstRow = i / 3;

        if (row == 0) {
            src.row(i).copyTo(img1.row(dstRow));
        }
        else if (row == 1) {
            src.row(i).copyTo(img2.row(dstRow));
        }
        else if (row == 2) {
            src.row(i).copyTo(img3.row(dstRow));
        }
    }
    int cameraNum = img1.cols / cameraCol;
    std::cout << "cameraNum:" << cameraNum << ",single width:" << cameraCol << std::endl;
    height = img1.rows;
    cv::Mat channel_1, channel_2, channel_3;
    std::vector<cv::Mat> channel1, channel2, channel3;
    //2、图像起止点裁剪
    for (int numberCam = 0; numberCam < cameraNum; numberCam++) {
        cv::Mat img = img1;
        img = getChannelRoiImg(img, numberCam * cameraCol + static_cast<int>(start_end[numberCam + 1][0]), numberCam * cameraCol + static_cast<int>(start_end[numberCam + 1][1]), 0, height);
        channel1.push_back(img);

        cv::Mat img_ = img2;
        img_ = getChannelRoiImg(img_, numberCam * cameraCol + static_cast<int>(start_end[numberCam + 1][0]), numberCam * cameraCol + static_cast<int>(start_end[numberCam + 1][1]), 0, height);
        channel2.push_back(img_);

        cv::Mat img__ = img3;
        img__ = getChannelRoiImg(img__, numberCam * cameraCol + static_cast<int>(start_end[numberCam + 1][0]), numberCam * cameraCol + static_cast<int>(start_end[numberCam + 1][1]), 0, height);
        channel3.push_back(img__);
    }
    cv::hconcat(channel1, channel_1);
    cv::hconcat(channel2, channel_2);
    cv::hconcat(channel3, channel_3);
    channel1.clear();
    channel2.clear();
    channel3.clear();
    img1.release();
    img2.release();
    img3.release();

    cv::Rect glassRect_;
    std::vector<cv::Point> edgePoints_;
    int flag = getGlassRoi(channel_3, glassRect_, threshold);
    if (flag == -1) {//未找到玻璃区域
        std::cout << "No Glass Area." << std::endl;
        return result;
    }

    result.push_back(channel_1);
    result.push_back(channel_2);
    result.push_back(channel_3);
    rect = glassRect_;

    return result;//仅按照通道顺序拆分图像
}

/// <summary>
/// 三通道图像拆分，返回三光源图像
/// </summary>
/// <param name="src"></param>
/// <param name="start_end"></param>
/// <param name="cameraCol"></param>
/// <returns></returns>
std::vector<cv::Mat> ImageBeautify::divImgChannel3(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol) {
    //1、图像多通道拆分
    //高度改变
    int height = src.rows;
    int width = src.cols;
    int cameraNum = src.cols / cameraCol;

    std::vector<cv::Mat> result;
    // 确保高度是3的倍数
    if (height % 3 != 0) {
        std::cerr << "Height of the image is not a multiple of 3." << std::endl;
        return result;
    }

    // 计算裁剪后的总宽度
    int total_cropped_width = 0;
    std::vector<int> cameraHightValue;//记录每个相机对应的像素数
    for (int cam = 1; cam <= cameraNum; cam++) {
        total_cropped_width += static_cast<int>(start_end[cam][1] - start_end[cam][0]);
        cameraHightValue.push_back(static_cast<int>(start_end[cam][1] - start_end[cam][0]));
    }

    // 创建目标图像（三通道，高度为 height/3，宽度为裁剪后的总宽度）
    cv::Mat img1 = cv::Mat::zeros(src.rows / 3, total_cropped_width, src.type());
    cv::Mat img2 = cv::Mat::zeros(src.rows / 3, total_cropped_width, src.type());
    cv::Mat img3 = cv::Mat::zeros(src.rows / 3, total_cropped_width, src.type());

    // 获取指针
    const uchar* src_data = src.data;
    uchar* dst1_data = img1.data;
    uchar* dst2_data = img2.data;
    uchar* dst3_data = img3.data;

    size_t src_step = src.step;
    size_t dst_step = img1.step;

    // 遍历每个相机
    int current_dst_x = 0; // 当前目标图像的写入位置

    for (int cam = 1; cam <= cameraNum; cam++) {
        int start_px = static_cast<int>(start_end[cam][0]);
        int end_px = static_cast<int>(start_end[cam][1]);
        int cropped_width = end_px - start_px;

        // 计算当前相机在拼接图像中的水平偏移量
        int camera_offset = (cam - 1) * cameraCol; // Camera1 偏移 0，Camera2 偏移 4096，...

        // 遍历每一行
#pragma omp parallel for
        for (int y = 0; y < src.rows / 3; y++) {
            // 源图像的行指针（考虑相机偏移 + 裁剪范围）
            const uchar* src_row0 = src_data + (y * 3) * src_step + (camera_offset + start_px) * src.elemSize();
            const uchar* src_row1 = src_data + (y * 3 + 1) * src_step + (camera_offset + start_px) * src.elemSize();
            const uchar* src_row2 = src_data + (y * 3 + 2) * src_step + (camera_offset + start_px) * src.elemSize();

            // 目标图像的行指针
            uchar* dst1_row = dst1_data + y * dst_step + current_dst_x * src.elemSize();
            uchar* dst2_row = dst2_data + y * dst_step + current_dst_x * src.elemSize();
            uchar* dst3_row = dst3_data + y * dst_step + current_dst_x * src.elemSize();

            // 拷贝数据
            memcpy(dst1_row, src_row0, cropped_width * src.elemSize());
            memcpy(dst2_row, src_row1, cropped_width * src.elemSize());
            memcpy(dst3_row, src_row2, cropped_width * src.elemSize());
        }

        current_dst_x += cropped_width; // 更新目标写入位置
    }

    result.push_back(img1);
    result.push_back(img2);
    result.push_back(img3);


    return result;//仅按照通道顺序拆分图像
}

/// <summary>
/// 双通道图像拆分，返回两光源图像
/// </summary>
/// <param name="src"></param>
/// <param name="start_end"></param>
/// <param name="cameraCol"></param>
/// <returns></returns>
std::vector<cv::Mat> ImageBeautify::divImgChannel2(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol) {
    //1、图像多通道拆分
    //高度改变
    int height = src.rows;
    int width = src.cols;
    int cameraNum = src.cols / cameraCol;

    std::vector<cv::Mat> result;
    // 确保高度是2的倍数
   /* if (height % 2 != 0) {
        std::cerr << "Height of the image is not a multiple of 2." << std::endl;
        return result;
    }*/

    // 计算裁剪后的总宽度
    int total_cropped_width = 0;
    std::vector<int> cameraHightValue;//记录每个相机对应的像素数
    for (int cam = 1; cam <= cameraNum; cam++) {
        total_cropped_width += static_cast<int>(start_end[cam][1] - start_end[cam][0]);
        cameraHightValue.push_back(static_cast<int>(start_end[cam][1] - start_end[cam][0]));
    }

    // 创建目标图像（双通道，高度为 height/2，宽度为裁剪后的总宽度）
    cv::Mat img1 = cv::Mat::zeros(src.rows / 2, total_cropped_width, src.type());
    cv::Mat img2 = cv::Mat::zeros(src.rows / 2, total_cropped_width, src.type());

    // 获取指针
    const uchar* src_data = src.data;
    uchar* dst1_data = img1.data;
    uchar* dst2_data = img2.data;

    size_t src_step = src.step;
    size_t dst_step = img1.step;

    // 遍历每个相机
    int current_dst_x = 0; // 当前目标图像的写入位置

    for (int cam = 1; cam <= cameraNum; cam++) {
        int start_px = static_cast<int>(start_end[cam][0]);
        int end_px = static_cast<int>(start_end[cam][1]);
        int cropped_width = end_px - start_px;

        // 计算当前相机在拼接图像中的水平偏移量
        int camera_offset = (cam - 1) * cameraCol; // Camera1 偏移 0，Camera2 偏移 4096，...

        // 遍历每一行
#pragma omp parallel for
        for (int y = 0; y < src.rows / 2; y++) {
            // 源图像的行指针（考虑相机偏移 + 裁剪范围）
            const uchar* src_row0 = src_data + (y * 2) * src_step + (camera_offset + start_px) * src.elemSize();
            const uchar* src_row1 = src_data + (y * 2 + 1) * src_step + (camera_offset + start_px) * src.elemSize();

            // 目标图像的行指针
            uchar* dst1_row = dst1_data + y * dst_step + current_dst_x * src.elemSize();
            uchar* dst2_row = dst2_data + y * dst_step + current_dst_x * src.elemSize();

            // 拷贝数据
            memcpy(dst1_row, src_row0, cropped_width * src.elemSize());
            memcpy(dst2_row, src_row1, cropped_width * src.elemSize());
        }

        current_dst_x += cropped_width; // 更新目标写入位置
    }

    result.push_back(img1);
    result.push_back(img2);

    return result;//仅按照通道顺序拆分图像
}

/// <summary>
/// 单光源图像处理，返回单个拼接图像
/// </summary>
/// <param name="src"></param>
/// <param name="start_end"></param>
/// <param name="cameraCol"></param>
/// <returns></returns>
std::vector<cv::Mat> ImageBeautify::divImgChannel1(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol) {
    // 1、图像处理（单光源）
    int height = src.rows;
    int width = src.cols;
    int cameraNum = src.cols / cameraCol;

    std::vector<cv::Mat> result;

    // 计算裁剪后的总宽度
    int total_cropped_width = 0;
    std::vector<int> cameraWidthValues; // 记录每个相机对应的有效像素宽度
    for (int cam = 1; cam <= cameraNum; cam++) {
        total_cropped_width += static_cast<int>(start_end[cam][1] - start_end[cam][0]);
        cameraWidthValues.push_back(static_cast<int>(start_end[cam][1] - start_end[cam][0]));
    }

    // 创建目标图像（高度不变，宽度为裁剪后的总宽度）
    cv::Mat dst = cv::Mat::zeros(height, total_cropped_width, src.type());

    // 获取指针
    const uchar* src_data = src.data;
    uchar* dst_data = dst.data;

    size_t src_step = src.step;
    size_t dst_step = dst.step;

    // 遍历每个相机
    int current_dst_x = 0; // 当前目标图像的写入位置

    for (int cam = 1; cam <= cameraNum; cam++) {
        int start_px = static_cast<int>(start_end[cam][0]);
        int end_px = static_cast<int>(start_end[cam][1]);
        int cropped_width = end_px - start_px;

        // 计算当前相机在拼接图像中的水平偏移量
        int camera_offset = (cam - 1) * cameraCol; // Camera1 偏移 0，Camera2 偏移 cameraCol，...

        // 遍历每一行
#pragma omp parallel for
        for (int y = 0; y < height; y++) {
            // 源图像的行指针（考虑相机偏移 + 裁剪范围）
            const uchar* src_row = src_data + y * src_step + (camera_offset + start_px) * src.elemSize();

            // 目标图像的行指针
            uchar* dst_row = dst_data + y * dst_step + current_dst_x * src.elemSize();

            // 拷贝数据
            memcpy(dst_row, src_row, cropped_width * src.elemSize());
        }

        current_dst_x += cropped_width; // 更新目标写入位置
    }

    result.push_back(dst);
    return result; // 返回单个拼接后的图像
}
/*===========图像拆分 OpenCV================*/
///// <summary>
/// 三通道图像拆分，返回三光源图像
/// Opencv版本
/// </summary>
std::vector<cv::Mat> ImageBeautify::divImgChannel3OpenCV(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol) {
    std::vector<cv::Mat> result;
    try {
        if (src.empty()) {
            FILE_LOG_DEBUG("[ImageBeautify] divImgChannel3: input src empty.");
            return result;
        }
        if (cameraCol <= 0) {
            FILE_LOG_DEBUG("[ImageBeautify] divImgChannel3: invalid cameraCol=%d.", cameraCol);
            return result;
        }

        int height = src.rows;
        int width = src.cols;

        if (height % 3 != 0) {
            FILE_LOG_DEBUG("[ImageBeautify] divImgChannel3: height %d not multiple of 3.", height);
            return result;
        }

        // 拆分三通道（按行周期3分块）
        cv::Mat img1(height / 3, width, src.type());
        cv::Mat img2(height / 3, width, src.type());
        cv::Mat img3(height / 3, width, src.type());
        for (int i = 0; i < height; ++i) {
            int dstRow = i / 3;
            int mod = i % 3;
            if (mod == 0) src.row(i).copyTo(img1.row(dstRow));
            else if (mod == 1) src.row(i).copyTo(img2.row(dstRow));
            else src.row(i).copyTo(img3.row(dstRow));
        }

        // 计算 cameraNum
        if (cameraCol == 0) {
            FILE_LOG_DEBUG("[ImageBeautify] divImgChannel3: cameraCol==0");
            return result;
        }
        int cameraNum = img1.cols / cameraCol;
        if (cameraNum <= 0) {
            FILE_LOG_DEBUG("[ImageBeautify] divImgChannel3: cameraNum <= 0 (cols=%d cameraCol=%d).", img1.cols, cameraCol);
            return result;
        }

        // 校验 start_end 长度（代码中其它位置使用 start_end[cam] where cam in [1..cameraNum]）
        if (static_cast<int>(start_end.size()) <= cameraNum) {
            FILE_LOG_DEBUG("[ImageBeautify] divImgChannel3: start_end size=%d too small for cameraNum=%d.", (int)start_end.size(), cameraNum);
            return result;
        }

        // 裁剪每个相机区域并收集 tiles（使用 clone 保证数据安全）
        std::vector<cv::Mat> channel1_tiles, channel2_tiles, channel3_tiles;
        for (int numberCam = 0; numberCam < cameraNum; ++numberCam) {
            int camIndex = numberCam + 1; // 保持原有索引习惯
            int start_px = numberCam * cameraCol + static_cast<int>(start_end[camIndex][0]);
            int end_px = numberCam * cameraCol + static_cast<int>(start_end[camIndex][1]);

            // 边界修正
            if (start_px < 0) start_px = 0;
            if (end_px > img1.cols) end_px = img1.cols;
            if (end_px <= start_px) {
                FILE_LOG_DEBUG("[ImageBeautify] divImgChannel3: invalid start/end for cam=%d start=%d end=%d", camIndex, start_px, end_px);
                return result;
            }
            int w = end_px - start_px;
            cv::Rect roi(start_px, 0, w, img1.rows);

            channel1_tiles.push_back(img1(roi).clone());
            channel2_tiles.push_back(img2(roi).clone());
            channel3_tiles.push_back(img3(roi).clone());
        }

        // 横向拼接每个通道的 tiles
        cv::Mat channel_1, channel_2, channel_3;
        if (!channel1_tiles.empty()) cv::hconcat(channel1_tiles, channel_1);
        if (!channel2_tiles.empty()) cv::hconcat(channel2_tiles, channel_2);
        if (!channel3_tiles.empty()) cv::hconcat(channel3_tiles, channel_3);

        // 释放中间资源
        img1.release(); img2.release(); img3.release();
        channel1_tiles.clear(); channel2_tiles.clear(); channel3_tiles.clear();

        result.push_back(std::move(channel_1));
        result.push_back(std::move(channel_2));
        result.push_back(std::move(channel_3));
        return result;
    }
    catch (const cv::Exception& e) {
        FILE_LOG_ERROR("[ImageBeautify] divImgChannel3: OpenCV exception: %s", e.what());
        return std::vector<cv::Mat>();
    }
    catch (const std::exception& e) {
        FILE_LOG_ERROR("[ImageBeautify] divImgChannel3: std::exception: %s", e.what());
        return std::vector<cv::Mat>();
    }
    catch (...) {
        FILE_LOG_ERROR("[ImageBeautify] divImgChannel3: unknown exception");
        return std::vector<cv::Mat>();
    }
}

/// <summary>
/// 两通道拆分
/// Opencv版本
/// </summary>
/// <param name="src"></param>
/// <param name="start_end"></param>
/// <param name="cameraCol"></param>
/// <returns></returns>
std::vector<cv::Mat> ImageBeautify::divImgChannel2OpenCV(const cv::Mat& src, std::vector<std::vector<float>> start_end, int cameraCol) {
    std::vector<cv::Mat> result;
    try {
        if (src.empty()) {
            FILE_LOG_DEBUG("[ImageBeautify] divImgChannel2: input src empty.");
            return result;
        }
        if (cameraCol <= 0) {
            FILE_LOG_DEBUG("[ImageBeautify] divImgChannel2: invalid cameraCol=%d.", cameraCol);
            return result;
        }

        int height = src.rows;
        int width = src.cols;

        if (height % 2 != 0) {
            FILE_LOG_DEBUG("[ImageBeautify] divImgChannel2: height %d not multiple of 2.", height);
            return result;
        }

        // 拆分两通道（按行周期2分块）
        cv::Mat img1(height / 2, width, src.type());
        cv::Mat img2(height / 2, width, src.type());
        for (int i = 0; i < height; ++i) {
            int dstRow = i / 2;
            int mod = i % 2;
            if (mod == 0) src.row(i).copyTo(img1.row(dstRow));
            else src.row(i).copyTo(img2.row(dstRow));
        }

        int cameraNum = img1.cols / cameraCol;
        if (cameraNum <= 0) {
            FILE_LOG_DEBUG("[ImageBeautify] divImgChannel2: cameraNum <= 0 (cols=%d cameraCol=%d).", img1.cols, cameraCol);
            return result;
        }

        if (static_cast<int>(start_end.size()) <= cameraNum) {
            FILE_LOG_DEBUG("[ImageBeautify] divImgChannel2: start_end size=%d too small for cameraNum=%d.", (int)start_end.size(), cameraNum);
            return result;
        }

        std::vector<cv::Mat> channel1_tiles, channel2_tiles;
        for (int numberCam = 0; numberCam < cameraNum; ++numberCam) {
            int camIndex = numberCam + 1;
            int start_px = numberCam * cameraCol + static_cast<int>(start_end[camIndex][0]);
            int end_px = numberCam * cameraCol + static_cast<int>(start_end[camIndex][1]);

            if (start_px < 0) start_px = 0;
            if (end_px > img1.cols) end_px = img1.cols;
            if (end_px <= start_px) {
                FILE_LOG_DEBUG("[ImageBeautify] divImgChannel2: invalid start/end for cam=%d start=%d end=%d", camIndex, start_px, end_px);
                return result;
            }
            int w = end_px - start_px;
            cv::Rect roi(start_px, 0, w, img1.rows);

            channel1_tiles.push_back(img1(roi).clone());
            channel2_tiles.push_back(img2(roi).clone());
        }

        cv::Mat channel_1, channel_2;
        if (!channel1_tiles.empty()) cv::hconcat(channel1_tiles, channel_1);
        if (!channel2_tiles.empty()) cv::hconcat(channel2_tiles, channel_2);

        img1.release(); img2.release();
        channel1_tiles.clear(); channel2_tiles.clear();

        result.push_back(std::move(channel_1));
        result.push_back(std::move(channel_2));
        return result;
    }
    catch (const cv::Exception& e) {
        FILE_LOG_ERROR("[ImageBeautify] divImgChannel2: OpenCV exception: %s", e.what());
        return std::vector<cv::Mat>();
    }
    catch (const std::exception& e) {
        FILE_LOG_ERROR("[ImageBeautify] divImgChannel2: std::exception: %s", e.what());
        return std::vector<cv::Mat>();
    }
    catch (...) {
        FILE_LOG_ERROR("[ImageBeautify] divImgChannel2: unknown exception");
        return std::vector<cv::Mat>();
    }
}

/*===========图像拆分 OpenCV================*/

//
void ImageBeautify::mainTest()
{
    // 读取RGBA图像
    cv::Mat rgbaImage = cv::imread("E:\\项目文件\\玻璃缺陷检测\\数据\\20250312/2025-03-12_16-09-32.png", 0);
    std::vector<std::vector<float>> start_end = {
    {0, 4056},     // Camera1
    {82, 3730},    // Camera2
    {436, 3665},   // Camera3
    {396, 3722},   // Camera4
    {315, 4096},   // Camera5
    {467, 3755},   // Camera6
    {423, 3682}    // Camera7
    };
    std::vector<cv::Mat> divImg = divImg3Channel(rgbaImage, start_end, 4096, 1, 30);

    std::cout << "Image size: " << std::endl;
}
