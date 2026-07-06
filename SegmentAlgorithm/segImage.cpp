#include "segImage.h"

segImage::segImage()
{
}

segImage::~segImage()
{
    //std::cout << "segImage::~segImage()" << std::endl;
}

#include <opencv2/opencv.hpp>

#include <opencv2/opencv.hpp>


/*=============边缘提取方案======================*/
/**
     * @brief 创建对比图像（原图、边缘图、结果图并排显示）
     * @param original 原图
     * @param edges 边缘图
     * @param result 结果图
     * @return 拼接后的对比图像
     */
static cv::Mat createComparisonImage(const cv::Mat& original, const cv::Mat& edges, const cv::Mat& result) {
    int width = original.cols;
    int height = original.rows;
    int separator = 5; // 分隔线宽度

    // 创建大画布
    cv::Mat comparison(height, width * 3 + separator * 2, CV_8UC3, cv::Scalar(40, 40, 40));

    // 放置原图
    cv::Mat roi1 = comparison(cv::Rect(0, 0, width, height));
    if (original.channels() == 1) {
        cv::cvtColor(original, roi1, cv::COLOR_GRAY2BGR);
    }
    else {
        original.copyTo(roi1);
    }

    // 放置边缘图（转换为彩色）
    cv::Mat roi2 = comparison(cv::Rect(width + separator, 0, width, height));
    if (edges.channels() == 1) {
        cv::cvtColor(edges, roi2, cv::COLOR_GRAY2BGR);
    }
    else {
        edges.copyTo(roi2);
    }

    // 放置结果图
    cv::Mat roi3 = comparison(cv::Rect(width * 2 + separator * 2, 0, width, height));
    result.copyTo(roi3);

    // 添加分隔线
    cv::line(comparison, cv::Point(width, 0), cv::Point(width, height),
        cv::Scalar(255, 255, 255), separator);
    cv::line(comparison, cv::Point(width * 2 + separator, 0),
        cv::Point(width * 2 + separator, height), cv::Scalar(255, 255, 255), separator);

    // 添加标题
    cv::putText(comparison, "Original", cv::Point(10, 30),
        cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
    cv::putText(comparison, "Edges", cv::Point(width + separator + 10, 30),
        cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
    cv::putText(comparison, "Result", cv::Point(width * 2 + separator * 2 + 10, 30),
        cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);

    return comparison;
}


/**
 * @brief 使用Sobel算子进行边缘检测
 * @param src 输入图像
 * @param scale 缩放因子
 * @param delta 偏移量
 * @return 边缘检测结果
 */
cv::Mat edgeDetectionSobel(const cv::Mat& src,
    double scale = 1,
    double delta = 0) {
    cv::Mat gray, grad_x, grad_y;
    cv::Mat abs_grad_x, abs_grad_y, edges;

    // 转换为灰度图
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    }
    else {
        gray = src.clone();
    }

    // 计算x和y方向的梯度
    cv::Sobel(gray, grad_x, CV_16S, 1, 0, 3, scale, delta, cv::BORDER_DEFAULT);
    cv::Sobel(gray, grad_y, CV_16S, 0, 1, 3, scale, delta, cv::BORDER_DEFAULT);

    // 转换为绝对值
    cv::convertScaleAbs(grad_x, abs_grad_x);
    cv::convertScaleAbs(grad_y, abs_grad_y);

    // 合并梯度
    cv::addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, edges);

    return edges;
}
/**
     * @brief 在原图上绘制边缘检测结果
     * @param src 原始图像
     * @param edges 边缘检测结果
     * @param color 边缘颜色 (BGR格式)
     * @param thickness 线条粗细
     * @return 带有边缘标注的结果图像
     */
//cv::Mat segImage::drawEdgesOnImage(const cv::Mat& src, const cv::Mat& edges, cv::Rect rect,
//    const cv::Scalar& color = cv::Scalar(0, 255, 0),
//    int thickness = 2) {
//    cv::Mat result = src.clone();
//
//    // 如果原图是灰度图，转换为彩色图以便绘制彩色边缘
//    if (result.channels() == 1) {
//        cv::cvtColor(result, result, cv::COLOR_GRAY2BGR);
//    }
//
//    // 创建边缘掩码（阈值化处理）
//    cv::Mat edge_mask;
//    cv::threshold(edges, edge_mask, 30, 255, cv::THRESH_BINARY);
//
//    // 查找边缘轮廓
//    std::vector<std::vector<cv::Point>> contours;
//    cv::findContours(edge_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
//
//    // 绘制边缘轮廓
//    for (const auto& contour : contours) {
//        // 过滤太小的轮廓
//        double area = cv::contourArea(contour);
//        if (area < 3) continue;
//
//        // 绘制轮廓
//        cv::drawContours(result, std::vector<std::vector<cv::Point>>{contour}, -1, color, thickness);
//    }
//
//    return result;
//}
cv::Mat segImage::drawEdgesOnImage(const cv::Mat& src, const cv::Mat& edges, cv::Rect rect,
    const cv::Scalar& color = cv::Scalar(0, 255, 0),
    int thickness = 2) {
    cv::Mat result = src.clone();

    // 如果原图是灰度图，转换为彩色图以便绘制彩色边缘
    if (result.channels() == 1) {
        cv::cvtColor(result, result, cv::COLOR_GRAY2BGR);
    }

    // 创建边缘掩码（阈值化处理）
    cv::Mat edge_mask;
    cv::threshold(edges, edge_mask, 30, 255, cv::THRESH_BINARY);

    // 查找边缘轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edge_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 绘制边缘轮廓
    for (const auto& contour : contours) {
        // 过滤太小的轮廓
        double area = cv::contourArea(contour);
        if (area < 3) continue;

        // 检查轮廓是否在指定矩形区域内
        bool contour_in_rect = false;
        for (const auto& point : contour) {
            if (rect.contains(point)) {
                contour_in_rect = true;
                break;
            }
        }

        // 只在轮廓至少有一个点在矩形区域内时才绘制
        if (contour_in_rect) {
            cv::drawContours(result, std::vector<std::vector<cv::Point>>{contour}, -1, color, thickness);
        }
    }

    return result;
}
// 检测黑色填充区域（灰度值小于20的区域）
cv::Mat segImage::detectBlackPadding(const cv::Mat& src, int threshold_value = 20) {
    cv::Mat gray;
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    }
    else {
        gray = src.clone();
    }

    // 创建黑色区域掩码
    cv::Mat black_mask;
    cv::threshold(gray, black_mask, threshold_value, 255, cv::THRESH_BINARY_INV);

    return black_mask;
}

/**
 * @brief 统计所有轮廓的面积和长度之和
 * @param edges 边缘检测结果（二值图像）
 * @param min_area 最小面积阈值，过滤太小的轮廓
 * @return 包含总面积和总长度的pair
 */
static std::pair<double, double> calculateTotalAreaAndLength(const cv::Mat& edges, double min_area = 10.0) {
    // 创建边缘掩码（阈值化处理）
    cv::Mat edge_mask;
    cv::threshold(edges, edge_mask, 30, 255, cv::THRESH_BINARY);

    // 查找边缘轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edge_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double total_area = 0.0;
    double total_length = 0.0;

    // 计算所有轮廓的面积和长度之和
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area < min_area) continue;

        total_area += area;
        total_length += cv::arcLength(contour, true);
    }

    return std::make_pair(total_area, total_length);
}


std::pair<double, double> segImage::GetCalculateResult(const cv::Mat& src,cv::Mat& maskImage,int threshold_value = 20) {

    //1.分割图像
    // 检测黑色填充区域
    cv::Mat black_mask = detectBlackPadding(src, threshold_value);
    // 八方向Sobel边缘检测
    cv::Mat edges_merged = EightDirectionSobel::edgeDetection8Direction(src, 0, 30); // 合并结果
    cv::Mat connected;
    // 创建矩形结构元素
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT,
        cv::Size(3, 3));

    // 闭操作：先膨胀后腐蚀，用于连接断点
    cv::morphologyEx(edges_merged, connected, cv::MORPH_CLOSE, kernel);

    // 从black_mask中剔除膨胀后的边缘
    cv::Mat edges_dilated;
    cv::dilate(black_mask, edges_dilated, kernel);
    connected.setTo(0, edges_dilated);
    maskImage = connected.clone();
    //计算分割结果
    //auto [total_area, total_length] = calculateTotalAreaAndLength(connected);
	double total_area = 0.0;
	double total_length = 0.0;
    return std::make_pair(total_area, total_length);
}

cv::Mat segImage::GetDrawImage(const cv::Mat& src, int threshold_value = 20,int level = 0) {
    //1.分割图像
  // 检测黑色填充区域
    cv::Mat black_mask = detectBlackPadding(src, threshold_value);
    // 八方向Sobel边缘检测
    cv::Mat edges_merged = EightDirectionSobel::edgeDetection8Direction(src, 0, 30); // 合并结果
    cv::Mat connected;
    // 创建矩形结构元素
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));

    // 闭操作：先膨胀后腐蚀，用于连接断点
    cv::morphologyEx(edges_merged, connected, cv::MORPH_CLOSE, kernel);

    // 从black_mask中剔除膨胀后的边缘
    cv::Mat kernel_ = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 15));//剔除边缘使用
    cv::Mat edges_dilated;
    cv::dilate(black_mask, edges_dilated, kernel);
    connected.setTo(0, edges_dilated);

    return connected;
}

/*=============边缘提取方案======================*/



/*==============批处理测试=======================*/
int segImage::processAllImages(const std::string& inputDir, const std::string& outputDir) {
    // 检查输入目录是否存在
    if (!fs::exists(inputDir)) {
        std::cerr << "输入目录不存在: " << inputDir << std::endl;
        return -1;
    }

    // 创建输出目录（如果不存在）
    if (!fs::exists(outputDir)) {
        fs::create_directories(outputDir);
        std::cout << "创建输出目录: " << outputDir << std::endl;
    }

    // 支持的图像格式
    std::vector<std::string> imageExtensions = { ".png", ".jpg", ".jpeg", ".bmp", ".tiff", ".tif" };

    // 遍历目录中的所有文件
    int processedCount = 0;
    for (const auto& entry : fs::directory_iterator(inputDir)) {
        if (entry.is_regular_file()) {
            std::string filePath = entry.path().string();
            std::string extension = entry.path().extension().string();

            // 转换为小写以便比较
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            // 检查是否为图像文件
            if (std::find(imageExtensions.begin(), imageExtensions.end(), extension) != imageExtensions.end()) {
                std::cout << "处理图像: " << filePath << std::endl;

                // 读取图像
                cv::Mat image = cv::imread(filePath, cv::IMREAD_COLOR);
                if (image.empty()) {
                    std::cerr << "无法加载图像: " << filePath << std::endl;
                    continue;
                }
                cv::Mat mask;
                auto [total_area, total_length] = GetCalculateResult(image, mask);
                // 在原图上绘制边缘
                cv::Mat connected = GetDrawImage(image,20,2);

                // 定义颜色映射
                static const std::vector<cv::Scalar> color_map = {
                    cv::Scalar(0, 255, 0),    // 绿色 - level 0
                    cv::Scalar(0, 255, 255),  // 黄色 - level 1
                    cv::Scalar(0, 0, 255)     // 红色 - level 2
                };
                int level = 1;
                // 确保level在有效范围内
                int color_index = std::clamp(level, 0, static_cast<int>(color_map.size() - 1));
                cv::Scalar color = color_map[color_index];
                cv::Rect rect;
                // 在原图上绘制边缘
                cv::Mat result_with_edges = drawEdgesOnImage(image, connected, rect, color, 1);

                std::cout << "总面积: " << total_area << " 像素" << std::endl;
                std::cout << "总长度: " << total_length << " 像素" << std::endl;
                // 生成输出文件名
                std::string filename = entry.path().stem().string() + "_result.png";
                std::string outputPath = (fs::path(outputDir) / filename).string();
                // 创建对比图像（左右布局）
                cv::Mat comparison = createComparisonImage(image, result_with_edges, result_with_edges);

                // 可视化结果并保存

                cv::imwrite(outputPath, comparison);
                processedCount++;
            }
        }
    }

    std::cout << "处理完成! 共处理 " << processedCount << " 张图像" << std::endl;
    return processedCount;
}

// 你的其他函数实现保持不变，但需要修改visualizeDefects函数以支持保存
void segImage::visualizeDefects(const cv::Mat& image, const cv::Mat& defect_mask,
    const cv::Mat& black_mask, const std::string& outputPath) {
    // 创建结果图像（绘制轮廓）
    cv::Mat result = image.clone();

    // 查找轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(defect_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 绘制轮廓
    cv::drawContours(result, contours, -1, cv::Scalar(0, 255, 0), 2);

    // 创建拼接图像（包含分隔线）
    int separatorWidth = 2; // 分隔区域宽度
    cv::Mat combined(image.rows, image.cols + result.cols + separatorWidth, image.type(), cv::Scalar(0, 0, 0));

    // 将原图复制到左侧
    image.copyTo(combined(cv::Rect(0, 0, image.cols, image.rows)));

    // 在分隔区域绘制黑色线（可选，因为背景已经是黑色）
    cv::rectangle(combined, cv::Rect(image.cols, 0, separatorWidth, image.rows),
        cv::Scalar(0, 0, 0), cv::FILLED);

    // 添加白色边框线来突出分隔（可选）
    cv::line(combined, cv::Point(image.cols, 0), cv::Point(image.cols, image.rows),
        cv::Scalar(255, 255, 255), 1);
    cv::line(combined, cv::Point(image.cols + separatorWidth, 0),
        cv::Point(image.cols + separatorWidth, image.rows), cv::Scalar(255, 255, 255), 1);

    // 将结果图复制到右侧
    result.copyTo(combined(cv::Rect(image.cols + separatorWidth, 0, result.cols, result.rows)));

    // 保存拼接后的图像
    cv::imwrite(outputPath, combined);
    std::cout << "保存结果: " << outputPath << std::endl;
}

/*==============批处理测试=======================*/

int segImage::mainTest() {
    //// 读取图像（替换为您的图像路径）
    //std::string imagePath = R"(G:\项目文档\AOI检测测试\分类模型\玻璃数据分类训练\训练1\dataset\train\0\297.png)";
    //cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);
    //if (image.empty()) {
    //    std::cerr << "无法加载图像!" << std::endl;
    //    return -1;
    //}

    //// 检测黑色填充区域
    //cv::Mat black_mask = detectBlackPadding(image, 20);

    //// 基于单行灰度分析的缺陷检测
    //auto result = detectDefectsByRowAnalysis(image, black_mask, 0.3, 25);
    //cv::Mat defect_mask = result.first;



    /*==============批处理测试=======================*/

    std::string inputDir = R"(G:\项目文档\AOI检测测试\分类模型\玻璃数据分类训练\训练1\dataset\train\6)";
    std::string outputDir = "./result/segImg/6_6";

    processAllImages(inputDir, outputDir);

    return 0;
}
