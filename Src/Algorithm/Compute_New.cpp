#include "Compute.h"

Compute::Compute()
{
    std::string model1Name = R"(E:\code\C++_Utils\TenosorRT_Test\TenosorRT_Test\models\detect\model.engine)";
    std::string model2Name = R"(E:\code\C++_Utils\TenosorRT_Test\TenosorRT_Test\models\detect\model2.engine)";

    model1Name = ModelPath_cu;
    model2Name = ModelPath_jing;
    deploy::InferOption option;
    option.enableSwapRB();
    model = std::make_unique<deploy::DetectModel>(model1Name, option);
    model2 = std::make_unique<deploy::DetectModel>(model2Name, option);
}

Compute::~Compute()
{

}

/*=========测试所需函数============*/
// 绘制玻璃区域矩形框的实现
cv::Mat drawGlassRegions(const cv::Mat& originalImage,
    const std::vector<int>& glassIndices,
    int tileWidth, int tileHeight,
    const cv::Scalar& color = cv::Scalar(0, 255, 0),
    int thickness = 3) {
    if (originalImage.empty()) {
        std::cerr << "Original image is empty!" << std::endl;
        return cv::Mat();
    }

    // 创建原图的副本（三通道）
    cv::Mat resultImage;
    if (originalImage.channels() == 1) {
        cv::cvtColor(originalImage, resultImage, cv::COLOR_GRAY2BGR);
    }
    else {
        resultImage = originalImage.clone();
    }

    // 计算每行有多少个图块
    int tilesPerRow = (originalImage.cols + tileWidth - 1) / tileWidth;

    // 为每个玻璃区域绘制矩形框
    for (int idx : glassIndices) {
        // 计算图块在大图中的位置
        int row = idx / tilesPerRow;
        int col = idx % tilesPerRow;

        // 计算矩形框的坐标
        int x = col * tileWidth;
        int y = row * tileHeight;

        // 确保坐标不超出图像边界
        int actualWidth = std::min(tileWidth, originalImage.cols - x);
        int actualHeight = std::min(tileHeight, originalImage.rows - y);

        // 绘制矩形框
        cv::rectangle(resultImage,
            cv::Rect(x, y, actualWidth, actualHeight),
            color, thickness);

        // 可选：在矩形框上添加索引文字
        std::string text = std::to_string(idx);
        cv::putText(resultImage, text,
            cv::Point(x + 10, y + 30),
            cv::FONT_HERSHEY_SIMPLEX, 1.0, color, 2);
    }

    return resultImage;
}
/*=========测试所需函数============*/

int divideImageShallow(cv::Mat& largeImage, int imgWidth, int imgHeight,
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
/*=============================多batch版本+粗筛优化===============================================*/
 //处理一批图像
//void process_batch_images(const std::vector<cv::Mat>& imageAll, deploy::DetectModel& model, std::vector<int>& errIndexs, std::vector<deploy::DetectRes>& Informations, double threshold, int flag = 0) {
//
//    errIndexs.clear();
//    Informations.clear();
//    double t = (double)cv::getTickCount();
//    const int batch_size = model.batch_size();
//    for (int i = 0; i < imageAll.size(); i += batch_size) {
//        std::vector<cv::Mat>        images;
//        std::vector<deploy::Image> img_batch;
//        std::vector<int>    img_Index_batch;
//
//        for (int j = i; j < i + batch_size && j < imageAll.size(); ++j) {
//            cv::Mat image;
//            cv::cvtColor(imageAll[j], image, cv::COLOR_GRAY2RGB);
//
//            images.push_back(image);
//            img_batch.emplace_back(image.data, image.cols, image.rows);
//            img_Index_batch.push_back(j);
//        }
//
//        auto results = model.predict(img_batch);
//
//        if (flag == 0) {//粗检测 不返回检测结果,仅返回待检图片索引
//
//            for (int n = 0; n < results.size(); n++) {
//                if (results[n].num > 0) {
//                    cv::Mat drawRect;
//                    cv::cvtColor(imageAll[img_Index_batch[n]], drawRect, cv::COLOR_GRAY2RGB);
//                    visualize(drawRect, results[n]);
//
//                    int isflag = 0;
//                    for (int m = 0; m < results[n].num; m++) {
//
//                        if (results[n].scores[m] > threshold) {//判断检测结果是不是都大于阈值
//                            isflag = 1;
//                            break;
//                        }
//
//                    }
//                    if (isflag == 0) {
//                        continue;
//                    }
//                    errIndexs.push_back(img_Index_batch[n]);
//
//                }
//            }
//        }
//        else {//精确检测，返回索引以及检测结果，暂时不使用
//
//            errIndexs.clear();
//            Informations.clear();
//            for (int n = 0; n < results.size(); n++) {
//                if (results[n].num > 0) {
//                    errIndexs.push_back(img_Index_batch[n]);
//                    Informations.push_back(results[n]);
//                }
//            }
//
//        }
//    }
//
//}

void process_batch_images(const std::vector<cv::Mat>& imageAll, const std::vector<int>& indices, deploy::DetectModel& model, std::vector<int>& errIndexs, std::vector<deploy::DetectRes>& Informations, double threshold, int flag = 0) {

    errIndexs.clear();
    Informations.clear();
    const int batch_size = model.batch_size();

    for (int i = 0; i < indices.size(); i += batch_size) {
        std::vector<cv::Mat> images;
        std::vector<deploy::Image> img_batch;
        std::vector<int> img_Index_batch;

        // 根据索引构建批次
        for (int j = i; j < i + batch_size && j < indices.size(); ++j) {
            int img_index = indices[j];
            if (img_index >= 0 && img_index < imageAll.size()) {
                cv::Mat image;
                cv::cvtColor(imageAll[img_index], image, cv::COLOR_GRAY2RGB);

                images.push_back(image);
                img_batch.emplace_back(image.data, image.cols, image.rows);
                img_Index_batch.push_back(img_index);
            }
        }
        std::vector<deploy::DetectRes> results;
        if (img_batch.empty()) continue;
        if (&model == nullptr || model.batch_size() <= 0) continue;
        try
        {
            //auto results = model.predict(img_batch);
            results = model.predict(img_batch);
        }
        catch (const std::exception& e)
        {
            FILE_LOG_ERROR("[Compute] model.predict threw: %s", e.what());
            return;
        }


        if (flag == 0) { // 粗检测：只返回有缺陷的图像索引
            for (int n = 0; n < results.size(); n++) {
                if (results[n].num > 0) {
                    bool has_defect = false;
                    for (int m = 0; m < results[n].num; m++) {
                        if (results[n].scores[m] > threshold) {
                            has_defect = true;
                            break;
                        }
                    }
                    if (has_defect) {
                        errIndexs.push_back(img_Index_batch[n]);
                    }
                }
            }
        }
        else { // 精确检测：返回索引和检测结果
            for (int n = 0; n < results.size(); n++) {
                if (results[n].num > 0) {
                    bool has_defect = false;
                    for (int m = 0; m < results[n].num; m++) {
                        if (results[n].scores[m] > threshold) {
                            has_defect = true;
                            break;
                        }
                    }
                    if (has_defect) {
                        errIndexs.push_back(img_Index_batch[n]);
                        Informations.push_back(results[n]);
                    }
                }
            }
        }
    }
}

// 处理一批图像
void process_batch_imagesByIndex(const std::vector<cv::Mat>& imageAll, std::vector<int>& IndexsCu, deploy::DetectModel& model, std::vector<int>& errIndexs, std::vector<deploy::DetectRes>& Informations, double threshold) {

    errIndexs.clear();
    Informations.clear();
    //double t = (double)cv::getTickCount();
    const int batch_size = model.batch_size();
    for (int i = 0; i < IndexsCu.size(); i += batch_size) {
        std::vector<cv::Mat>        images;
        std::vector<deploy::Image> img_batch;
        std::vector<int>    img_Index_batch;

        for (int j = i; j < i + batch_size && j < IndexsCu.size(); ++j) {
            cv::Mat image;
            cv::cvtColor(imageAll[IndexsCu[j]], image, cv::COLOR_GRAY2RGB);

            images.push_back(image);
            img_batch.emplace_back(image.data, image.cols, image.rows);
            img_Index_batch.push_back(IndexsCu[j]);
        }

        auto results = model.predict(img_batch);

        for (int n = 0; n < results.size(); n++) {
            if (results[n].num > 0) {

                int isflag = 0;//判断是否是所有检测结果都大于阈值
                for (int m = 0; m < results[n].num; m++) {

                    if (results[n].scores[m] > threshold) {//判断检测结果是不是都大于阈值
                        isflag = 1;
                        break;
                    }

                }
                if (isflag == 0) {
                    continue;
                }
                errIndexs.push_back(img_Index_batch[n]);
                Informations.push_back(results[n]);
            }

        }
    }

    //t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
    //double milliseconds = t * 1000; // 秒转毫秒
    //std::cout << "process_single_image Sum Time:" << milliseconds << std::endl;

}

/*===========================玻璃区域是否存在缺陷粗筛=====================================*/

////// <summary>
/////  //1.行方向扫描，判断是否有缺陷
///// </summary>

// 结构体存储每行统计数据
struct RowStats {
    double range_val; // 极差
    //double var_val;   // 方差
    //double max_grad;  // 最大梯度
};

// 保持与原始一致的逐行极差计算
std::vector<RowStats> calculateImageStatsHorizontal(const cv::Mat& tile) {
    std::vector<RowStats> stats(tile.rows);

    for (int y = 0; y < tile.rows; ++y) {
        const uchar* p = tile.ptr<uchar>(y);
        auto [min_it, max_it] = std::minmax_element(p, p + tile.cols);
        stats[y].range_val = *max_it - *min_it;
    }

    return stats;
}
// 阈值判断优化版
// 保持原始判断逻辑
bool isDefectiveTileHorizontal(const cv::Mat& tile, double threshold) {
    auto stats = calculateImageStatsHorizontal(tile);
    return std::any_of(stats.begin(), stats.end(),
        [threshold](const RowStats& rs) {
            return rs.range_val > threshold;
        });
}

/// <summary>
/// 列方向扫描，判断是否有缺陷
/// </summary>
struct ColStats {
    double range_val; // 极差
};

// 纵向极差计算（新增）
std::vector<ColStats> calculateImageStatsVertical(const cv::Mat& tile) {
    std::vector<ColStats> stats(tile.cols);

    // 创建转置矩阵以提高访问效率
    cv::Mat transposed = tile.t();

    for (int x = 0; x < tile.cols; ++x) {
        const uchar* p = transposed.ptr<uchar>(x);
        auto [min_it, max_it] = std::minmax_element(p, p + tile.rows);
        stats[x].range_val = *max_it - *min_it;
    }

    return stats;
}
// 纵向缺陷判断（新增）
bool isDefectiveTileVertical(const cv::Mat& tile, double threshold) {
    auto stats = calculateImageStatsVertical(tile);
    return std::any_of(stats.begin(), stats.end(),
        [threshold](const ColStats& cs) {
            return cs.range_val > threshold;
        });
}
/*===========================玻璃区域是否存在缺陷粗筛=====================================*/

/*=============================多batch版本+粗筛优化===============================================*/


/*=============================多batch版本+粗筛优化+玻璃区域确定===============================================*/

/*
    逆时针旋转
*/
int Compute::RotateImage(std::vector<cv::Mat>& images, int flipV = 0) {

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

// 修正的矩形旋转函数（逆时针90度）
cv::Rect rotateRectCCW90(const cv::Rect& rect, int width, int height)
{
    // 直接计算旋转后的矩形（更准确的公式）
    int new_x = rect.y;
    int new_y = width - rect.x - rect.width;
    int new_width = rect.height;
    int new_height = rect.width;

    return cv::Rect(new_x, new_y, new_width, new_height);
}

// 修正的检测结果旋转函数
std::vector<drawInformation> Compute::rotateDrawInfoCCW90(
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

/*
    顺时针旋转
*/
// 顺时针旋转90度的矩形旋转函数
cv::Rect rotateRectCW90(const cv::Rect& rect, int width, int height)
{
    // 顺时针90度旋转公式
    int new_x = height - rect.y - rect.height;
    int new_y = rect.x;
    int new_width = rect.height;
    int new_height = rect.width;

    return cv::Rect(new_x, new_y, new_width, new_height);
}

// 顺时针旋转90度的检测结果旋转函数
std::vector<drawInformation> Compute::rotateDrawInfoCW90(
    const std::vector<drawInformation>& drawInfos,
    int width, int height)
{
    std::vector<drawInformation> rotatedInfos;
    for (const auto& info : drawInfos) {
        drawInformation rotated = info;
        // 旋转矩形
        rotated.rect = rotateRectCW90(rotated.rect, width, height);

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


std::vector<drawInformation> Compute::transformToROICoordinates(
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
            //transformed.rect.width += transformed.rect.x; // 调整宽度
            //transformed.rect.x = 0;

            continue;
        }
        if (transformed.rect.y < 0) {
            //transformed.rect.height += transformed.rect.y; // 调整高度
            //transformed.rect.y = 0;
            continue;
        }
        if (transformed.rect.x + transformed.rect.width > roiRect.width) {
            //transformed.rect.width = roiRect.width - transformed.rect.x;
            continue;
        }
        if (transformed.rect.y + transformed.rect.height > roiRect.height) {
            //transformed.rect.height = roiRect.height - transformed.rect.y;
            continue;
        }

        // 如果矩形仍然有效则保留
        if (transformed.rect.width > 0 && transformed.rect.height > 0) {
            roiDrawInfo.push_back(transformed);
        }
    }

    return roiDrawInfo;
}




//int Compute::getGlassRoi(cv::Mat& inputGaryImg, cv::Rect& GlassRect, int threshold) {
//
//    // 阈值处理
//    cv::Mat binaryImage;
//    cv::threshold(inputGaryImg, binaryImage, threshold, 255, cv::THRESH_BINARY);
//
//    // 找到所有白色区域的轮廓
//    std::vector<std::vector<cv::Point>> contours;
//    std::vector<cv::Vec4i> hierarchy;
//    cv::findContours(binaryImage, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
//
//    // 找到最大的白色区域
//    int maxArea = 0;
//    int maxIndex = -1;
//    for (size_t i = 0; i < contours.size(); i++) {
//        int area = cv::contourArea(contours[i]);
//        if (area > maxArea) {
//            maxArea = area;
//            maxIndex = i;
//        }
//    }
//
//    if (maxIndex == -1) {
//        std::cout << "No white region found" << std::endl;
//        return -1;
//    }
//
//    // 计算最大白色区域向外扩展指定像素的矩形
//    GlassRect = cv::boundingRect(contours[maxIndex]);
//
//    return 0;
//}


int Compute::getGlassRoi(cv::Mat& inputGaryImg, cv::Rect& GlassRect, int threshold) {
    // 防护1: 检查输入图像是否有效
    if (inputGaryImg.empty()) {
        std::cout << "Input image is empty" << std::endl;
        return -1;
    }

    // 防护2: 检查图像是否为单通道灰度图
    if (inputGaryImg.channels() != 1) {
        std::cout << "Input image must be single channel grayscale image" << std::endl;
        return -1;
    }

    // 防护3: 验证阈值范围
    if (threshold < 0 || threshold > 255) {
        std::cout << "Threshold value " << threshold << " is out of range [0, 255]" << std::endl;
        return -1;
    }

    // 阈值处理
    cv::Mat binaryImage;
    try {
        cv::threshold(inputGaryImg, binaryImage, threshold, 255, cv::THRESH_BINARY);
    }
    catch (const cv::Exception& e) {
        std::cout << "Threshold operation failed: " << e.what() << std::endl;
        return -1;
    }

    // 找到所有白色区域的轮廓
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    try {
        cv::findContours(binaryImage, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    }
    catch (const cv::Exception& e) {
        std::cout << "Find contours failed: " << e.what() << std::endl;
        return -1;
    }

    // 防护4: 检查是否找到轮廓
    if (contours.empty()) {
        //cv::imwrite("binary_image.png", binaryImage);
        //cv::imwrite("inputGaryImg.png", inputGaryImg);
        std::cout << "No contours found" << std::endl;
        return -1;
    }

    // 找到最大的白色区域
    int maxArea = 0;
    int maxIndex = -1;

    try {
        for (size_t i = 0; i < contours.size(); i++) {
            // 防护5: 检查轮廓是否有效
            if (contours[i].empty()) {
                continue; // 跳过空轮廓
            }

            double area = cv::contourArea(contours[i]);
            // 防护6: 检查面积计算是否有效
            if (area < 0) {
                std::cout << "Invalid contour area calculated for contour " << i << std::endl;
                continue;
            }

            if (area > maxArea) {
                maxArea = static_cast<int>(area);
                maxIndex = static_cast<int>(i);
            }
        }
    }
    catch (const cv::Exception& e) {
        std::cout << "Contour processing failed: " << e.what() << std::endl;
        return -1;
    }

    if (maxIndex == -1) {
        std::cout << "No valid white region found" << std::endl;
        return -1;
    }

    // 防护7: 检查选中的轮廓是否有效
    if (maxIndex < 0 || maxIndex >= static_cast<int>(contours.size()) || contours[maxIndex].empty()) {
        std::cout << "Invalid maxIndex or empty contour selected" << std::endl;
        return -1;
    }

    try {
        // 计算最大白色区域的外接矩形
        GlassRect = cv::boundingRect(contours[maxIndex]);

        // 防护8: 检查生成的矩形是否有效
        if (GlassRect.width <= 0 || GlassRect.height <= 0) {
            std::cout << "Invalid bounding rectangle generated" << std::endl;
            return -1;
        }

        // 防护9: 检查矩形是否在图像范围内
        if (GlassRect.x < 0 || GlassRect.y < 0 ||
            GlassRect.x + GlassRect.width > inputGaryImg.cols ||
            GlassRect.y + GlassRect.height > inputGaryImg.rows) {
            std::cout << "Generated rectangle is outside image boundaries" << std::endl;
            // 这里可以选择修正矩形而不是直接返回错误
            GlassRect = GlassRect & cv::Rect(0, 0, inputGaryImg.cols, inputGaryImg.rows);
        }

    }
    catch (const cv::Exception& e) {
        std::cout << "Bounding rectangle calculation failed: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}


int Compute::getGlassRoiSample(cv::Mat& inputGaryImg, cv::Rect& GlassRect, int threshold, double scale = 0.1) {
    // 基本检查
    if (inputGaryImg.empty()) {
        std::cout << "Input image is empty" << std::endl;
        return -1;
    }

    if (inputGaryImg.channels() != 1) {
        std::cout << "Input image must be single channel grayscale image" << std::endl;
        return -1;
    }

    if (threshold < 0 || threshold > 255) {
        std::cout << "Threshold value " << threshold << " is out of range [0, 255]" << std::endl;
        return -1;
    }

    // 无论图像多大，都进行0.1倍下采样
    cv::Size smallSize(
        static_cast<int>(inputGaryImg.cols * scale),
        static_cast<int>(inputGaryImg.rows * scale)
    );

    // 确保最小尺寸
    smallSize.width = std::max(smallSize.width, 10);
    smallSize.height = std::max(smallSize.height, 10);

    cv::Mat smallImg;
    try {
        cv::resize(inputGaryImg, smallImg, smallSize, 0, 0, cv::INTER_AREA);
    }
    catch (const cv::Exception& e) {
        std::cout << "Image resize failed: " << e.what() << std::endl;
        return -1;
    }

    // 在下采样图像上进行阈值处理
    cv::Mat binaryImage;
    try {
        cv::threshold(smallImg, binaryImage, threshold, 255, cv::THRESH_BINARY);
    }
    catch (const cv::Exception& e) {
        std::cout << "Threshold operation failed: " << e.what() << std::endl;
        return -1;
    }

    // 找到所有白色区域的轮廓
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    try {
        cv::findContours(binaryImage, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    }
    catch (const cv::Exception& e) {
        std::cout << "Find contours failed: " << e.what() << std::endl;
        return -1;
    }

    // 检查是否找到轮廓
    if (contours.empty()) {
        std::cout << "No contours found" << std::endl;
        return -1;
    }

    // 找到最大的白色区域
    int maxArea = 0;
    int maxIndex = -1;

    try {
        for (size_t i = 0; i < contours.size(); i++) {
            if (contours[i].empty()) {
                continue;
            }

            double area = cv::contourArea(contours[i]);
            if (area < 0) {
                std::cout << "Invalid contour area calculated for contour " << i << std::endl;
                continue;
            }

            if (area > maxArea) {
                maxArea = static_cast<int>(area);
                maxIndex = static_cast<int>(i);
            }
        }
    }
    catch (const cv::Exception& e) {
        std::cout << "Contour processing failed: " << e.what() << std::endl;
        return -1;
    }

    if (maxIndex == -1) {
        std::cout << "No valid white region found" << std::endl;
        return -1;
    }

    // 检查选中的轮廓是否有效
    if (maxIndex < 0 || maxIndex >= static_cast<int>(contours.size()) || contours[maxIndex].empty()) {
        std::cout << "Invalid maxIndex or empty contour selected" << std::endl;
        return -1;
    }

    try {
        // 计算最大白色区域的外接矩形（在下采样图像上）
        cv::Rect smallRect = cv::boundingRect(contours[maxIndex]);

        // 将矩形映射回原图尺寸
        double invScale = 1.0 / scale;
        GlassRect.x = static_cast<int>(smallRect.x * invScale);
        GlassRect.y = static_cast<int>(smallRect.y * invScale);
        GlassRect.width = static_cast<int>(smallRect.width * invScale);
        GlassRect.height = static_cast<int>(smallRect.height * invScale);

        // 检查生成的矩形是否有效
        if (GlassRect.width <= 0 || GlassRect.height <= 0) {
            std::cout << "Invalid bounding rectangle generated" << std::endl;
            return -1;
        }

        // 确保矩形在图像范围内
        if (GlassRect.x < 0 || GlassRect.y < 0 ||
            GlassRect.x + GlassRect.width > inputGaryImg.cols ||
            GlassRect.y + GlassRect.height > inputGaryImg.rows) {
            std::cout << "Generated rectangle is outside image boundaries, adjusting..." << std::endl;
            GlassRect = GlassRect & cv::Rect(0, 0, inputGaryImg.cols, inputGaryImg.rows);
        }

    }
    catch (const cv::Exception& e) {
        std::cout << "Bounding rectangle calculation failed: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
// 合并缺陷检测结果到原始大图的函数
std::vector<drawInformation> Compute::mergeDefectsToLargeImage(
    const std::vector<std::vector<drawInformation>>& batchDrawInfo,
    int rowHeight = 256) {

    std::vector<drawInformation> mergedDefects;

    // 遍历每个批次的检测结果
    for (int batchIndex = 0; batchIndex < batchDrawInfo.size(); ++batchIndex) {
        const auto& batch = batchDrawInfo[batchIndex];
        int yOffset = batchIndex * rowHeight;  // 计算当前批次在大图中的Y偏移量

        // 处理当前批次中的每个缺陷
        for (const auto& defect : batch) {
            drawInformation mergedDefect = defect;

            // 调整矩形坐标到原始大图的位置
            mergedDefect.rect.y += yOffset;
            mergedDefect.realRect.y += yOffset;  // 假设realRect也需要同样的调整

            mergedDefects.push_back(mergedDefect);
        }
    }

    return mergedDefects;
}

// 更快的版本，使用直接像素访问（假设图像是单通道灰度图）
std::vector<int> Compute::findGlassTilesFast(const std::vector<cv::Mat>& tiles, double threshold) {
    std::vector<int> glassIndices;
    glassIndices.reserve(tiles.size() / 2);
    int thresh = static_cast<int>(threshold);

    cv::parallel_for_(cv::Range(0, static_cast<int>(tiles.size())), [&](const cv::Range& range) {
        std::vector<int> localIndices;

        for (int i = range.start; i < range.end; i++) {
            const cv::Mat& tile = tiles[i];
            if (tile.empty()) continue;
            if (tile.channels() != 1 || tile.depth() != CV_8U) {
                // 如果不是单通道8位，跳过或先转换（这里选择跳过以保证性能和安全）
                continue;
            }

            bool hasGlass = false;
            for (int r = 0; r < tile.rows && !hasGlass; r++) {
                const uchar* row = tile.ptr<uchar>(r);
                for (int c = 0; c < tile.cols; c++) {
                    if (row[c] > thresh) {
                        hasGlass = true;
                        break;
                    }
                }
            }

            if (hasGlass) {
                localIndices.push_back(i);
            }
        }

        if (!localIndices.empty()) {
            cv::AutoLock lock(mutex);
            glassIndices.insert(glassIndices.end(), localIndices.begin(), localIndices.end());
        }
        });

    std::sort(glassIndices.begin(), glassIndices.end());
    return glassIndices;
}

/// <summary>
/// 根据横向灰度值变化进行图像粗筛
/// </summary>
/// <param name="mats"></param>
/// <param name="glassIndices"></param>
/// <param name="imgIndexs"></param>
/// <param name="tileWidth"></param>
/// <param name="tileHeight"></param>
/// <param name="range_val"></param>
/// <returns></returns>
std::pair<std::vector<cv::Mat>, std::vector<int>> Compute::detectDefectiveGlassTilesHorizontal(
    const std::vector<cv::Mat>& mats,
    const std::vector<int>& glassIndices,
    const std::vector<std::vector<int>>& imgIndexs,
    int tileWidth,
    int tileHeight,
    double range_val) {

    std::mutex mtx;
    std::vector<int> detectIndexs;

    cv::parallel_for_(cv::Range(0, static_cast<int>(glassIndices.size())), [&](const cv::Range& range) {
        std::vector<int> localIndices;  // 线程本地存储

        for (int j = range.start; j < range.end; ++j) {
            int i = glassIndices[j];  // 获取玻璃区域的原始索引

            int gridY = i / imgIndexs[0].size();
            int gridX = i % imgIndexs[0].size();
            cv::Rect roi(gridX * tileWidth, gridY * tileHeight, mats[i].cols, mats[i].rows);

            if (isDefectiveTileHorizontal(mats[i], range_val)) { // 大于指定阈值则为待判断缺陷区域
                localIndices.push_back(i);  // 记录原始索引
            }
        }

        // 批量添加检测到的索引（减少锁竞争）
        if (!localIndices.empty()) {
            std::lock_guard<std::mutex> lock(mtx);
            detectIndexs.insert(detectIndexs.end(), localIndices.begin(), localIndices.end());
        }
        });

    // 最终收集（按需浅拷贝）
    std::vector<cv::Mat> detectRects;
    for (int idx : detectIndexs) {
        detectRects.emplace_back(mats[idx]);  // 浅拷贝
    }

    return { detectRects, detectIndexs };
}

/// <summary>
/// 根据纵向灰度值变化进行图像粗筛
/// </summary>
/// <param name="mats"></param>
/// <param name="glassIndices"></param>
/// <param name="imgIndexs"></param>
/// <param name="tileWidth"></param>
/// <param name="tileHeight"></param>
/// <param name="range_val"></param>
/// <returns></returns>
std::pair<std::vector<cv::Mat>, std::vector<int>> Compute::detectDefectiveGlassTilesVertical(
    const std::vector<cv::Mat>& mats,
    const std::vector<int>& glassIndices,
    const std::vector<std::vector<int>>& imgIndexs,
    int tileWidth,
    int tileHeight,
    double range_val) {

    std::mutex mtx;
    std::vector<int> detectIndexs;

    cv::parallel_for_(cv::Range(0, static_cast<int>(glassIndices.size())), [&](const cv::Range& range) {
        std::vector<int> localIndices;

        for (int j = range.start; j < range.end; ++j) {
            int i = glassIndices[j];

            int gridY = i / imgIndexs[0].size();
            int gridX = i % imgIndexs[0].size();
            cv::Rect roi(gridX * tileWidth, gridY * tileHeight, mats[i].cols, mats[i].rows);

            if (isDefectiveTileVertical(mats[i], range_val)) {
                localIndices.push_back(i);
            }
        }

        if (!localIndices.empty()) {
            std::lock_guard<std::mutex> lock(mtx);
            detectIndexs.insert(detectIndexs.end(), localIndices.begin(), localIndices.end());
        }
        });

    std::vector<cv::Mat> detectRects;
    for (int idx : detectIndexs) {
        detectRects.emplace_back(mats[idx]);
    }

    return { detectRects, detectIndexs };
}
std::vector<drawInformation> Compute::ComputeProcess( cv::Mat& largeImg, 
    PrescriptionParameter& parameter,
    const double& glassThreshold_val, const double& range_val,
    int tileHeight = 256,int tileWidth = 256)

{
    double threshold_1, threshold_2;
    threshold_1 = parameter.thresholdDetect_1;
    threshold_2 = parameter.thresholdDetect_2;
    std::vector<double> thresholdsCls = parameter.thresholdsCls;


    std::vector<cv::Mat>            mats;
    std::vector<std::vector<int>>   imgIndexs;
    std::vector<drawInformation>    saveInformations;

    if (largeImg.channels() == 3) {
        cv::cvtColor(largeImg, largeImg, cv::COLOR_BGR2GRAY);
    }
    try
    {
        divideImageShallow(largeImg, tileWidth, tileHeight, mats, imgIndexs);
    }
    catch (const std::exception& e)
    {
        FILE_LOG_ERROR("[ComputeProcess] ComputerThread Error: divideImageShallow failed!");
        return saveInformations;
    }
    std::vector<int>  glassIndices;
    try
    {
        glassIndices = findGlassTilesFast(mats, glassThreshold_val);
    }
    catch (const std::exception&)
    {
        FILE_LOG_ERROR("[ComputeProcess] ComputerThread Error: findGlassTilesFast failed!");

        return saveInformations;
    }

    // 灰度值变化粗筛 - 只遍历玻璃区域
    auto [detectRects, detectIndexs] = detectDefectiveGlassTilesVertical(
        mats, glassIndices, imgIndexs, tileWidth, tileHeight, range_val);

    //cv::Mat resultImage = drawGlassRegions(largeImg, detectIndexs, tileWidth, tileHeight, cv::Scalar(0, 255, 0), 3);


    //1.粗检测batch
    std::vector<int> indexs_Cu;//在原始图中的索引
    std::vector<int> indexs_Cu_;//在粗筛列表detectRects中的索引
    std::vector<deploy::DetectRes> Informations_Cu;

	//auto model_Clone = (*model).clone(); 
    process_batch_images(mats, detectIndexs, (*model), indexs_Cu_, Informations_Cu, threshold_1, 1);

    //将粗筛结果转换为原始图拆分中的索引
    //if (indexs_Cu_.size() == 0) {
    //    return resultImage;
    //}
    //for (int i = 0; i < indexs_Cu_.size(); i++) {
    //    indexs_Cu.push_back(detectIndexs[indexs_Cu_[i]]);
    //}

    //2.精检测batch
    std::vector<int> indexs_Jing;
    std::vector<deploy::DetectRes> Informations_Jing;
    //process_batch_imagesByIndex(mats, indexs_Cu, (*model2), indexs_Jing, Informations_Jing, threshold_2);


    for (int i = 0; i < indexs_Cu_.size(); i++) {
        deploy::DetectRes result = Informations_Cu[i];
        if (result.num > 0) {
            //std::cout << "mats[indexs_Cu[i]]:" << indexs_Cu_[i] << std::endl;
            cv::Mat img = mats[indexs_Cu_[i]].clone();
            for (int j = 0; j < result.num; j++) {
                if (result.scores[j] < threshold_2) {

                    continue;
                }

                DefectType TypeValue;
                std::string typeName = "";
                if (result.classes[j] == DefectType::TYPE_POORCOATING) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_POORCOATING;
                }
                else if (result.classes[j] == DefectType::TYPE_SCRATCH) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_SCRATCH;
                    
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[1])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                    //FILE_LOG_DEBUG("[ComputeProcess] ComputerThread Error: Scratch Detect! %f ,", result.scores[j], static_cast<float>(parameter.thresholdsCls[1]));
                }
                else if (result.classes[j] == DefectType::TYPE_CALCULUS) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_CALCULUS;

                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[2])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }
                    //FILE_LOG_DEBUG("[ComputeProcess] ComputerThread Error: Scratch Detect! %f ,", result.scores[j], static_cast<float>(parameter.thresholdsCls[2]));
                }
                else if (result.classes[j] == DefectType::TYPE_BUBBLE) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_BUBBLE;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[3])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }
                    //FILE_LOG_DEBUG("[ComputeProcess] ComputerThread Error: Scratch Detect! %f ,", result.scores[j], static_cast<float>(parameter.thresholdsCls[3]));
                }
                else if (result.classes[j] == DefectType::TYPE_TRADEMARK) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_TRADEMARK;
                    //TypeValue = DefectType::TYPE_SMUDGE;//20251123 为解决黑点误判为商标，把所有商标改为脏污
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[4])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }
                    //FILE_LOG_DEBUG("[ComputeProcess] ComputerThread Error: Scratch Detect! %f ,", result.scores[j], static_cast<float>(parameter.thresholdsCls[4]));
                }
                else if (result.classes[j] == DefectType::TYPE_WATERSTAIN) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_WATERSTAIN;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[5])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }
                    //FILE_LOG_DEBUG("[ComputeProcess] ComputerThread Error: Scratch Detect! %f ,", result.scores[j], static_cast<float>(parameter.thresholdsCls[5]));
                }
                else if (result.classes[j] == DefectType::TYPE_SMUDGE) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_SMUDGE;

                }
                else if (result.classes[j] == DefectType::TYPE_SCREENPRINTING) {//20251122 新增类别

                    typeName = " ";
                    TypeValue = DefectType::TYPE_SCREENPRINTING;

                }
                else {
                    typeName = " ";
                    TypeValue = DefectType::TYPE_SMUDGE;
                }
                cv::Mat RoiSegImg;
                float left = result.boxes[j].left;
                float top = result.boxes[j].top;
                float right = result.boxes[j].right;
                float bottom = result.boxes[j].bottom;

                int width = static_cast<int>(right - left);
                int height = static_cast<int>(bottom - top);

                cv::Rect rect(
                    std::max(0, static_cast<int>(std::floor(left))),
                    std::max(0, static_cast<int>(std::floor(top))),
                    std::max(0, static_cast<int>(std::ceil(right - left))),
                    std::max(0, static_cast<int>(std::ceil(bottom - top)))
                );

                // clamp x/y within image
                if (rect.x >= img.cols) continue;
                if (rect.y >= img.rows) continue;

                if (rect.x < 0) rect.x = 0;
                if (rect.y < 0) rect.y = 0;

                if (rect.x + rect.width > img.cols) rect.width = img.cols - rect.x;
                if (rect.y + rect.height > img.rows) rect.height = img.rows - rect.y;

                // 跳过非法大小
                if (rect.width <= 0 || rect.height <= 0) continue;

                std::vector<float> errorInfo;
                ConnectedComponentItem analysisInfo;
                DefectLevel levelType = levelUtils.defectLevelProcessByEdge(img, rect, TypeValue, analysisInfo, errorInfo);
                if (levelType == ABNORMAL) {//该缺陷过小 小于最小阈值
                    continue;
                }
                if (levelType == AREAERROR) {//未分割成功情况下，进行特殊处理


                    levelType = levelUtils.defectLevelProcessSingle(img, rect, TypeValue, analysisInfo, errorInfo);

                }
                else {

                    //辅助判断
                    //对脏污和气泡进行判断
                    if (TypeValue == DefectType::TYPE_SMUDGE || TypeValue == DefectType::TYPE_BUBBLE) {
                        if (analysisInfo.numComponent == 2 || analysisInfo.numComponent == 3) {//单一区域

                            levelUtils.secondDetect(img, rect, analysisInfo, TypeValue, levelType, errorInfo);
                        }

                    }
                }

                levelUtils.DoubleChangeErrorType(img, rect, analysisInfo, TypeValue, levelType, errorInfo, result.scores[j]);

                drawInformation saveInformation;
                saveInformation.rect.x = indexs_Cu_[i] % imgIndexs[0].size() * img.cols + rect.x;
                saveInformation.rect.y = indexs_Cu_[i] / imgIndexs[0].size() * img.rows + rect.y;
                saveInformation.rect.width = rect.width;
                saveInformation.rect.height = rect.height;
                saveInformation.ErrorType = levelType;
                saveInformation.DefectType = TypeValue;
                saveInformation.Errorinfo = errorInfo;
                saveInformation.confidence = result.scores[j];
                saveInformations.push_back(saveInformation);
            }
        }

    }

    //根据参数对等级进行修正
    std::vector<drawInformation>  resultInformations = levelUtils.GetDefectLevel(saveInformations,parameter);

  
    return resultInformations;
}



cv::Mat Compute::ComputeProcessDraw(cv::Mat& largeImg,
    PrescriptionParameter& parameter,
    const double& glassThreshold_val, const double& range_val,
    int tileHeight = 256, int tileWidth = 256)
{
	////////////double threshold_1, threshold_2;
 ////////////   threshold_1 = parameter.thresholdDetect_1;
 ////////////   threshold_2 = parameter.thresholdDetect_2;
 ////////////   std::vector<double> thresholdsCls = parameter.thresholdsCls;

 ////////////   cv::Mat drawImage;

 ////////////   std::vector<cv::Mat>            mats;
 ////////////   std::vector<std::vector<int>>   imgIndexs;
 ////////////   std::vector<drawInformation>    saveInformations;

 ////////////   if (largeImg.channels() == 3) {
 ////////////       cv::cvtColor(largeImg, largeImg, cv::COLOR_BGR2GRAY);
 ////////////   }

 ////////////   divideImageShallow(largeImg, tileWidth, tileHeight, mats, imgIndexs);

 ////////////   std::vector<int>  glassIndices = findGlassTilesFast(mats, glassThreshold_val);


 ////////////   // 灰度值变化粗筛 - 只遍历玻璃区域
 ////////////   auto [detectRects, detectIndexs] = detectDefectiveGlassTilesVertical(
 ////////////       mats, glassIndices, imgIndexs, tileWidth, tileHeight, range_val);

 ////////////   cv::Mat resultImage = drawGlassRegions(largeImg, detectIndexs, tileWidth, tileHeight, cv::Scalar(0, 255, 0), 3);


 ////////////   //1.粗检测batch
 ////////////   std::vector<int> indexs_Cu;//在原始图中的索引
 ////////////   std::vector<int> indexs_Cu_;//在粗筛列表detectRects中的索引
 ////////////   std::vector<deploy::DetectRes> Informations_Cu;
 ////////////   process_batch_images(mats, detectIndexs,(*model), indexs_Cu_, Informations_Cu, threshold_1,1);

 ////////////   //将粗筛结果转换为原始图拆分中的索引
 ////////////   //if (indexs_Cu_.size() == 0) {
 ////////////   //    return resultImage;
 ////////////   //}
 ////////////   //for (int i = 0; i < indexs_Cu_.size(); i++) {
 ////////////   //    indexs_Cu.push_back(detectIndexs[indexs_Cu_[i]]);
 ////////////   //}

 ////////////   //2.精检测batch
 ////////////   std::vector<int> indexs_Jing;
 ////////////   std::vector<deploy::DetectRes> Informations_Jing;
 ////////////   //process_batch_imagesByIndex(mats, indexs_Cu, (*model2), indexs_Jing, Informations_Jing, threshold_2);


 ////////////   //process_imageCls(mats, *modelCls,indexs_Cu_, Informations_Cu, thresholdsCls);
 ////////////   for (int i = 0; i < indexs_Cu_.size(); i++) {
 ////////////       deploy::DetectRes result = Informations_Cu[i];
 ////////////       if (result.num > 0) {
 ////////////           //std::cout<<"mats[indexs_Cu[i]]:"<< indexs_Cu_[i]<<std::endl;
 ////////////           cv::Mat img = mats[indexs_Cu_[i]].clone();
 ////////////           for (int j = 0; j < result.num; j++) {
 ////////////               if (result.scores[j] < threshold_2) {

 ////////////                   continue;
 ////////////               }

 ////////////               DefectType TypeValue;
 ////////////               std::string typeName = "";
 ////////////               if (result.classes[j] == DefectType::TYPE_POORCOATING) {

 ////////////                   typeName = " ";
 ////////////                   TypeValue = DefectType::TYPE_POORCOATING;
 ////////////               }
 ////////////               else if (result.classes[j] == DefectType::TYPE_SCRATCH) {

 ////////////                   typeName = " ";
 ////////////                   TypeValue = DefectType::TYPE_SCRATCH;
 ////////////               }
 ////////////               else if (result.classes[j] == DefectType::TYPE_CALCULUS) {

 ////////////                   typeName = " ";
 ////////////                   TypeValue = DefectType::TYPE_CALCULUS;
 ////////////               }
 ////////////               else if (result.classes[j] == DefectType::TYPE_BUBBLE) {

 ////////////                   typeName = " ";
 ////////////                   TypeValue = DefectType::TYPE_BUBBLE;

 ////////////               }
 ////////////               else if (result.classes[j] == DefectType::TYPE_TRADEMARK) {

 ////////////                   typeName = " ";
 ////////////                   TypeValue = DefectType::TYPE_TRADEMARK;

 ////////////               }
 ////////////               else if (result.classes[j] == DefectType::TYPE_WATERSTAIN) {

 ////////////                   typeName = " ";
 ////////////                   TypeValue = DefectType::TYPE_WATERSTAIN;

 ////////////               }
 ////////////               else if (result.classes[j] == DefectType::TYPE_SMUDGE) {

 ////////////                   typeName = " ";
 ////////////                   TypeValue = DefectType::TYPE_SMUDGE;

 ////////////               }
 ////////////               cv::Mat RoiSegImg;
 ////////////               float left = result.boxes[j].left;
 ////////////               float top = result.boxes[j].top;
 ////////////               float right = result.boxes[j].right;
 ////////////               float bottom = result.boxes[j].bottom;

 ////////////               int width = static_cast<int>(right - left);
 ////////////               int height = static_cast<int>(bottom - top);

 ////////////               cv::Rect rect(static_cast<int>(left), static_cast<int>(top), width, height);

 ////////////               if (rect.x < 0) {
 ////////////                   rect.x = 0;
 ////////////               }
 ////////////               if (rect.y < 0) {
 ////////////                   rect.y = 0;
 ////////////               }
 ////////////               if (rect.x >= img.cols) {
 ////////////                   rect.x = img.cols - 1;
 ////////////               }
 ////////////               if (rect.y >= img.rows) {
 ////////////                   rect.y = img.rows - 1;
 ////////////               }
 ////////////               if (rect.x + rect.width >= img.cols) {
 ////////////                   rect.width = img.cols - rect.x - 1;
 ////////////               }
 ////////////               if (rect.y + rect.height >= img.rows) {
 ////////////                   rect.height = img.rows - rect.y - 1;
 ////////////               }
 ////////////               std::vector<float> errorInfo;
 ////////////               ConnectedComponentItem analysisInfo;
	////////////																																												  
 ////////////               DefectLevel levelType = levelUtils.defectLevelProcessByEdge(img, rect, TypeValue, analysisInfo, errorInfo);
 ////////////               if (levelType == ABNORMAL) {//该缺陷过小 小于最小阈值
 ////////////                   continue;
 ////////////               }
 ////////////               if (levelType == AREAERROR) {//未分割成功情况下，进行特殊处理

 ////////////                   levelType = levelUtils.defectLevelProcessSingle(img, rect, TypeValue, analysisInfo, errorInfo);
	////////////					

 ////////////               }
 ////////////               else {
	////////////																									 
	////////////																										   

 ////////////                   //辅助判断
 ////////////                   //对脏污和气泡进行判断
 ////////////                   if (TypeValue == DefectType::TYPE_SMUDGE || TypeValue == DefectType::TYPE_BUBBLE) {
 ////////////                       if (analysisInfo.numComponent == 2 || analysisInfo.numComponent == 3) {//单一区域

 ////////////                           levelUtils.secondDetect(img, rect, analysisInfo, TypeValue, levelType, errorInfo);
 ////////////                       }

 ////////////                   }
 ////////////               }

 ////////////               levelUtils.DoubleChangeErrorType(img, rect, analysisInfo, TypeValue, levelType, errorInfo, result.scores[j]);
	////////////				 

 ////////////               drawInformation saveInformation;
 ////////////               saveInformation.rect.x = indexs_Cu_[i] % imgIndexs[0].size() * img.cols + rect.x;
 ////////////               saveInformation.rect.y = indexs_Cu_[i] / imgIndexs[0].size() * img.rows + rect.y;
 ////////////               saveInformation.rect.width = rect.width;
 ////////////               saveInformation.rect.height = rect.height;
 ////////////               saveInformation.ErrorType = levelType;
 ////////////               saveInformation.DefectType = TypeValue;
 ////////////               saveInformation.Errorinfo = errorInfo;
 ////////////               saveInformation.confidence = result.scores[j];
 ////////////               saveInformations.push_back(saveInformation);
 ////////////           }
 ////////////       }

 ////////////   }


 ////////////   for (int i = 0; i < saveInformations.size(); i++) {

 ////////////       cv::rectangle(resultImage, saveInformations[i].rect, cv::Scalar(0, 0, 255), 2);



 ////////////       // 标注类别 (0-7)
 ////////////       //std::string label = std::to_string(static_cast<int>(saveInformations[i].DefectType));
 ////////////       // 标注类别 (0-7)
 ////////////       std::string label;
 ////////////       if (saveInformations[i].DefectType == TYPE_POORCOATING) {
 ////////////           label = "0";
 ////////////       }
 ////////////       else if (saveInformations[i].DefectType == TYPE_SCRATCH) {
 ////////////           label = "1";
 ////////////       }
 ////////////       else if (saveInformations[i].DefectType == TYPE_CALCULUS) {
 ////////////           label = "2";
 ////////////       }
 ////////////       else if (saveInformations[i].DefectType == TYPE_BUBBLE) {
 ////////////           label = "3";
 ////////////       }
 ////////////       else if (saveInformations[i].DefectType == TYPE_TRADEMARK) {
 ////////////           label = "4";
 ////////////       }
 ////////////       else if (saveInformations[i].DefectType == TYPE_WATERSTAIN) {
 ////////////           label = "5";
 ////////////       }
 ////////////       else if (saveInformations[i].DefectType == TYPE_SMUDGE) {
 ////////////           label = "6";
 ////////////       }
 ////////////       else if (saveInformations[i].DefectType == TYPE_SCREENPRINTING) {
 ////////////           label = "7";
 ////////////       }
 ////////////       else {
 ////////////           label = "8";
 ////////////       }

 ////////////       // 计算文本位置（矩形框左上角上方）
 ////////////       cv::Point textOrg(
 ////////////           saveInformations[i].rect.x,
 ////////////           saveInformations[i].rect.y - 5
 ////////////       );

 ////////////       // 如果矩形框太靠上，将文本放在框内
 ////////////       if (saveInformations[i].rect.y < 20) {
 ////////////           textOrg.y = saveInformations[i].rect.y + 15;
 ////////////       }

 ////////////       // 绘制类别文本
 ////////////       cv::putText(
 ////////////           resultImage,
 ////////////           label,
 ////////////           textOrg,
 ////////////           cv::FONT_HERSHEY_SIMPLEX,
 ////////////           0.5,
 ////////////           cv::Scalar(0, 0, 255),
 ////////////           1
 ////////////       );
 ////////////   }
    cv::Mat resultImage;        //无法运行算法时的临时方法。
	return resultImage;				   
}
/*=============================多batch版本+粗筛优化+玻璃区域确定===============================================*/


/*================================20251201  多batch版本+粗筛优化+玻璃区域确定+三通道图像===================================================*/

/// <summary>
/// // 合并两个索引向量并去重
/// 主要用于对不同光源粗筛后的结果进行合并
/// </summary>
/// <param name="indices1"></param>
/// <param name="indices2"></param>
/// <returns></returns>
std::vector<int> mergeAndDeduplicateIndices(const std::vector<int>& indices1,
    const std::vector<int>& indices2) {
    std::set<int> uniqueIndices; // 使用set自动去重和排序

    // 插入第一个向量的所有元素
    uniqueIndices.insert(indices1.begin(), indices1.end());

    // 插入第二个向量的所有元素
    uniqueIndices.insert(indices2.begin(), indices2.end());

    // 转换回vector
    std::vector<int> result(uniqueIndices.begin(), uniqueIndices.end());

    return result;
}

/// <summary>
/// 多通道融合方案
/// </summary>
/// <param name="imageChannel0"></param>
/// <param name="imageChannel1"></param>
/// <param name="imageChannel2"></param>
/// <param name="indices"></param>
/// <param name="model"></param>
/// <param name="errIndexs"></param>
/// <param name="Informations"></param>
/// <param name="threshold"></param>
/// <param name="flag"></param>
void process_batch_imagesChannel(const std::vector<cv::Mat>& imageChannel0, const std::vector<cv::Mat>& imageChannel1, const std::vector<cv::Mat>& imageChannel2,
    const std::vector<int>& indices, deploy::DetectModel& model,
    std::vector<int>& errIndexs, std::vector<deploy::DetectRes>& Informations, double threshold, int flag = 0) {

    errIndexs.clear();
    Informations.clear();
    const int batch_size = model.batch_size();

    for (int i = 0; i < indices.size(); i += batch_size) {
        std::vector<cv::Mat> images;
        std::vector<deploy::Image> img_batch;
        std::vector<int> img_Index_batch;

        // 根据索引构建批次
        for (int j = i; j < i + batch_size && j < indices.size(); ++j) {
            int img_index = indices[j];
            if (img_index >= 0 && img_index < imageChannel0.size()) {

                //cv::cvtColor(imageAll[img_index], image, cv::COLOR_GRAY2RGB);
                //灰度图转RGB改多通道合并
                cv::Mat image;
                std::vector<cv::Mat> channels;
                //俄罗斯 :透 亮 暗
                channels.push_back(imageChannel0[img_index]);
                channels.push_back(imageChannel1[img_index]);
                channels.push_back(imageChannel2[img_index]);
                cv::merge(channels, image);

                images.push_back(image);
                img_batch.emplace_back(image.data, image.cols, image.rows);
                img_Index_batch.push_back(img_index);
            }
        }

        auto results = model.predict(img_batch);

        if (flag == 0) { // 粗检测：只返回有缺陷的图像索引
            for (int n = 0; n < results.size(); n++) {
                if (results[n].num > 0) {
                    bool has_defect = false;
                    for (int m = 0; m < results[n].num; m++) {
                        if (results[n].scores[m] > threshold) {
                            has_defect = true;
                            break;
                        }
                    }
                    if (has_defect) {
                        errIndexs.push_back(img_Index_batch[n]);
                    }
                }
            }
        }
        else { // 精确检测：返回索引和检测结果
            for (int n = 0; n < results.size(); n++) {
                if (results[n].num > 0) {
                    bool has_defect = false;
                    for (int m = 0; m < results[n].num; m++) {
                        if (results[n].scores[m] > threshold) {
                            has_defect = true;
                            break;
                        }
                    }
                    if (has_defect) {
                        errIndexs.push_back(img_Index_batch[n]);
                        Informations.push_back(results[n]);
                    }
                }
            }
        }
    }
}

/// <summary>
/// 多通道融合计算
/// </summary>
/// <param name="largeImgs"></param>
/// <param name="parameter"></param>
/// <param name="glassThreshold_val"></param>
/// <param name="range_val"></param>
/// <param name="tileHeight"></param>
/// <param name="tileWidth"></param>
/// <returns></returns>
std::vector<drawInformation> Compute::ComputeProcessMultiChannel(std::vector<cv::Mat>& largeImgs,
    PrescriptionParameter& parameter,
    const double& glassThreshold_val, const double& range_val,
    int tileHeight = 256, int tileWidth = 256)

{

    double threshold_1, threshold_2;
    threshold_1 = parameter.thresholdDetect_1;
    threshold_2 = parameter.thresholdDetect_2;
    std::vector<double> thresholdsCls = parameter.thresholdsCls;

    std::vector<cv::Mat>            mats0, mats1, mats2;
    std::vector<std::vector<int>>   imgIndexs;
    std::vector<drawInformation>    saveInformations;

    if (largeImgs[0].channels() == 3) {
        cv::cvtColor(largeImgs[0], largeImgs[0], cv::COLOR_BGR2GRAY);
    }
    if (largeImgs[1].channels() == 3) {
        cv::cvtColor(largeImgs[1], largeImgs[1], cv::COLOR_BGR2GRAY);
    }
    if (largeImgs[2].channels() == 3) {
        cv::cvtColor(largeImgs[2], largeImgs[2], cv::COLOR_BGR2GRAY);
    }
    divideImageShallow(largeImgs[0], tileWidth, tileHeight, mats0, imgIndexs);
    divideImageShallow(largeImgs[1], tileWidth, tileHeight, mats1, imgIndexs);
    divideImageShallow(largeImgs[2], tileWidth, tileHeight, mats2, imgIndexs);

    //确定玻璃区域的索引
    std::vector<int>  glassIndices = findGlassTilesFast(mats2, glassThreshold_val);

    // 灰度值变化粗筛 - 只遍历玻璃区域
    // 透场灰度粗筛
    auto [detectRects0, detectIndexs0] = detectDefectiveGlassTilesVertical(
        mats0, glassIndices, imgIndexs, tileWidth, tileHeight, range_val);
    //亮场灰度粗筛
    auto [detectRects, detectIndexs] = detectDefectiveGlassTilesVertical(
        mats2, glassIndices, imgIndexs, tileWidth, tileHeight, range_val);

    //合并多光源粗筛结果
    std::vector<int> mergeIndexs = mergeAndDeduplicateIndices(detectIndexs0, detectIndexs);

    //1.粗检测batch
    std::vector<int> indexs_Cu;//在原始图中的索引
    std::vector<int> indexs_Cu_;//在粗筛列表detectRects中的索引
    std::vector<deploy::DetectRes> Informations_Cu;
    process_batch_imagesChannel(mats0, mats1, mats2, mergeIndexs, (*model), indexs_Cu_, Informations_Cu, threshold_1, 1);

    //2.精检测batch
    std::vector<int> indexs_Jing;
    std::vector<deploy::DetectRes> Informations_Jing;

    for (int i = 0; i < indexs_Cu_.size(); i++) {
        deploy::DetectRes result = Informations_Cu[i];
        if (result.num > 0) {
            //FILE_LOG_DEBUG("[Algotithm] ComputeProcessMultiChannel:6 for  begin 1 !");
            cv::Mat img = mats0[indexs_Cu_[i]].clone();//透场分割，计算等级
            //cv::Mat img = mats2[indexs_Cu_[i]].clone();//亮场分割，计算等级
            for (int j = 0; j < result.num; j++) {
                if (result.scores[j] < threshold_2) {

                    continue;
                }
                DefectType TypeValue;
                std::string typeName = "";
                if (result.classes[j] == DefectType::TYPE_POORCOATING) {//0

                    typeName = " ";
                    TypeValue = DefectType::TYPE_POORCOATING;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[0])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }
                }
                else if (result.classes[j] == DefectType::TYPE_SCRATCH) {//1

                    typeName = " ";
                    TypeValue = DefectType::TYPE_SCRATCH;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[1])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }
                }
                else if (result.classes[j] == DefectType::TYPE_CALCULUS) {//2

                    typeName = " ";
                    TypeValue = DefectType::TYPE_CALCULUS;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[2])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }
                }
                else if (result.classes[j] == DefectType::TYPE_BUBBLE) {//3

                    typeName = " ";
                    TypeValue = DefectType::TYPE_BUBBLE;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[3])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result.classes[j] == DefectType::TYPE_TRADEMARK) {//4

                    typeName = " ";
                    TypeValue = DefectType::TYPE_TRADEMARK;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[4])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result.classes[j] == DefectType::TYPE_WATERSTAIN) {//5

                    typeName = " ";
                    TypeValue = DefectType::TYPE_WATERSTAIN;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[5])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result.classes[j] == DefectType::TYPE_SMUDGE) {//6

                    typeName = " ";
                    TypeValue = DefectType::TYPE_SMUDGE;

                }
                else if (result.classes[j] == DefectType::TYPE_SCREENPRINTING) {//7

                    typeName = " ";
                    TypeValue = DefectType::TYPE_SCREENPRINTING;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[7])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result.classes[j] == DefectType::TYPE_CHIPPED_EDGE) {//8

                    typeName = " ";
                    TypeValue = DefectType::TYPE_CHIPPED_EDGE;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[8])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result.classes[j] == DefectType::TYPE_PITTING) {//9

                    typeName = " ";
                    TypeValue = DefectType::TYPE_PITTING;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[9])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result.classes[j] == DefectType::TYPE_GLASS_CULLET) {//10

                    typeName = " ";
                    TypeValue = DefectType::TYPE_GLASS_CULLET;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[10])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result.classes[j] == DefectType::TYPE_WAVINESS) {//11

                    typeName = " ";
                    TypeValue = DefectType::TYPE_WAVINESS;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[11])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result.classes[j] == DefectType::TYPE_OTHER) {//12

                    typeName = " ";
                    TypeValue = DefectType::TYPE_OTHER;
                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[12])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else {
                    typeName = " ";
                    TypeValue = DefectType::TYPE_SMUDGE;
                }
                cv::Mat RoiSegImg;
                float left = result.boxes[j].left;
                float top = result.boxes[j].top;
                float right = result.boxes[j].right;
                float bottom = result.boxes[j].bottom;

                int width = static_cast<int>(right - left);
                int height = static_cast<int>(bottom - top);

                cv::Rect rect(static_cast<int>(left), static_cast<int>(top), width, height);

                if (rect.x < 0) {
                    rect.x = 0;
                }
                if (rect.y < 0) {
                    rect.y = 0;
                }
                if (rect.x >= img.cols) {
                    rect.x = img.cols - 1;
                }
                if (rect.y >= img.rows) {
                    rect.y = img.rows - 1;
                }
                if (rect.x + rect.width >= img.cols) {
                    rect.width = img.cols - rect.x - 1;
                }
                if (rect.y + rect.height >= img.rows) {
                    rect.height = img.rows - rect.y - 1;
                }
                std::vector<float> errorInfo;
              
                drawInformation saveInformation;
                saveInformation.rect.x = indexs_Cu_[i] % imgIndexs[0].size() * img.cols + rect.x;
                saveInformation.rect.y = indexs_Cu_[i] / imgIndexs[0].size() * img.rows + rect.y;
                saveInformation.rect.width = rect.width;
                saveInformation.rect.height = rect.height;
                saveInformation.ErrorType = levelType;
                saveInformation.DefectType = TypeValue;
                saveInformation.Errorinfo = errorInfo;
                saveInformation.confidence = result.scores[j];
                saveInformations.push_back(saveInformation);

            }
        }

    }
    //根据参数对等级进行修正
    //std::vector<drawInformation>  resultInformations = levelUtils.GetDefectLevel(saveInformations, parameter);

   
    return resultInformations;
}

int Compute::DefectLevelCompute(cv::Mat& image0, cv::Mat& image1, cv::Mat& image2, cv::Rect rect, std::vector<float>& errorInfo, DefectType& TypeValue, DefectLevel& levelType) {

    ConnectedComponentItem analysisInfo;
    levelType = levelUtils.defectLevelProcessByEdge(image0, rect, TypeValue, analysisInfo, errorInfo);

    if (levelType == AREAERROR) {//未分割成功情况下，进行特殊处理


        levelType = levelUtils.defectLevelProcessSingle(img, rect, TypeValue, analysisInfo, errorInfo);
    }
    else {

        //辅助判断
        //对脏污和气泡进行判断
        if (TypeValue == DefectType::TYPE_SMUDGE || TypeValue == DefectType::TYPE_BUBBLE) {
            if (analysisInfo.numComponent == 2 || analysisInfo.numComponent == 3) {//单一区域

                levelUtils.secondDetect(img, rect, analysisInfo, TypeValue, levelType, errorInfo);
            }

        }
    }
    cv::Mat imgDouble = mats2[indexs_Cu_[i]].clone();//亮场二次判断,因为镀膜不良仅在亮场上表现为白色
    levelUtils.DoubleChangeErrorType(imgDouble, rect, analysisInfo, TypeValue, levelType, errorInfo, result.scores[j]);

}

/*================================多batch版本+粗筛优化+玻璃区域确定+三通道图像===================================================*/

/*================================特殊区域计算，根据现场实际情况编写===================================================*/
/*
    合肥现场存在溢胶，边缘存在胶印，导致边缘区域需要单独的检测方式

    实现功能：对整张图周边一定区域内的进行单独检测，目前建议为256*2的距离
*/

/// <summary>
/// 获取原图索引中靠近边缘的一周索引，即四条边的两行或者两列索引
/// </summary>
/// <param name="imgIndexs"></param>
/// <param name="imgWidth"></param>
/// <param name="imgHeight"></param>
/// <param name="borderWidthTiles"></param>
/// <returns></returns>
std::vector<int> getBorderTilesIndices(const std::vector<std::vector<int>>& imgIndexs,
    int imgWidth, int imgHeight,
    int borderWidthTiles = 2) {
    std::vector<int> borderIndices;

    if (imgIndexs.empty()) return borderIndices;

    int numRows = imgIndexs.size();
    if (numRows == 0) return borderIndices;

    int numCols = imgIndexs[0].size();

    // 计算当前索引号（假设imgIndexs存储的是列索引）
    for (int y = 0; y < numRows; ++y) {
        for (int x = 0; x < numCols; ++x) {
            int tileIndex = y * numCols + x;

            // 判断是否在边界附近：最上面的borderWidthTiles行或最下面的borderWidthTiles行
            // 或者最左边的borderWidthTiles列或最右边的borderWidthTiles列
            if (y < borderWidthTiles || y >= numRows - borderWidthTiles ||
                x < borderWidthTiles || x >= numCols - borderWidthTiles) {
                borderIndices.push_back(tileIndex);
            }
        }
    }

    return borderIndices;
}

/// <summary>
/// 针对现场使用的局部区域检测
/// </summary>
/// <param name="largeImgs"></param>
/// <param name="parameter"></param>
/// <param name="glassThreshold_val"></param>
/// <param name="range_val"></param>
/// <param name="tileHeight"></param>
/// <param name="tileWidth"></param>
/// <returns></returns>
std::vector<drawInformation> Compute::ComputeProcessPart(std::vector<cv::Mat>& largeImgs,
    PrescriptionParameter& parameter,
    const double& glassThreshold_val, const double& range_val,
    int tileHeight = 256, int tileWidth = 256) {

    double threshold_1, threshold_2;
    threshold_1 = parameter.thresholdDetect_1;
    threshold_2 = parameter.thresholdDetect_2;
    std::vector<double> thresholdsCls = parameter.thresholdsCls;


    std::vector<cv::Mat>            mats0, mats1, mats2;
    std::vector<std::vector<int>>   imgIndexs;
    std::vector<drawInformation>    saveInformations;

    if (largeImgs[0].channels() == 3) {
        cv::cvtColor(largeImgs[0], largeImgs[0], cv::COLOR_BGR2GRAY);
    }
    if (largeImgs[1].channels() == 3) {
        cv::cvtColor(largeImgs[1], largeImgs[1], cv::COLOR_BGR2GRAY);
    }
    if (largeImgs[2].channels() == 3) {
        cv::cvtColor(largeImgs[2], largeImgs[2], cv::COLOR_BGR2GRAY);
    }
    divideImageShallow(largeImgs[0], tileWidth, tileHeight, mats0, imgIndexs);
    divideImageShallow(largeImgs[1], tileWidth, tileHeight, mats1, imgIndexs);
    divideImageShallow(largeImgs[2], tileWidth, tileHeight, mats2, imgIndexs);


    //根据实际需求，获取指定的待检区域
    //std::vector<int>  glassIndices = findGlassTilesFast(mats2, glassThreshold_val);
    std::vector<int>  glassIndices = getBorderTilesIndices(imgIndexs, tileWidth, tileHeight,2);

    // 灰度值变化粗筛 - 只遍历玻璃区域
    // 透场灰度粗筛
    auto [detectRects0, detectIndexs0] = detectDefectiveGlassTilesVertical(
        mats0, glassIndices, imgIndexs, tileWidth, tileHeight, range_val);
    //亮场灰度粗筛
    auto [detectRects, detectIndexs] = detectDefectiveGlassTilesVertical(
        mats2, glassIndices, imgIndexs, tileWidth, tileHeight, range_val);

    //合并多光源粗筛结果
    std::vector<int> mergeIndexs = mergeAndDeduplicateIndices(detectIndexs0, detectIndexs);

    //1.粗检测batch
    std::vector<int> indexs_Cu;//在原始图中的索引
    std::vector<int> indexs_Cu_;//在粗筛列表detectRects中的索引
    std::vector<deploy::DetectRes> Informations_Cu;
    process_batch_imagesChannel(mats0, mats1, mats2, mergeIndexs, (*model2), indexs_Cu_, Informations_Cu, threshold_1, 1);

    //2.精检测batch
    std::vector<int> indexs_Jing;
    std::vector<deploy::DetectRes> Informations_Jing;

    for (int i = 0; i < indexs_Cu_.size(); i++) {
        deploy::DetectRes result = Informations_Cu[i];
        if (result.num > 0) {
            cv::Mat img = mats2[indexs_Cu_[i]].clone();
            for (int j = 0; j < result.num; j++) {
                if (result.scores[j] < threshold_2) {

                    continue;
                }

                DefectType TypeValue;
                std::string typeName = "";
                if (result.classes[j] == DefectType::TYPE_POORCOATING) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_POORCOATING;
                }
                else if (result.classes[j] == DefectType::TYPE_SCRATCH) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_SCRATCH;
                }
                else if (result.classes[j] == DefectType::TYPE_CALCULUS) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_CALCULUS;
                }
                else if (result.classes[j] == DefectType::TYPE_BUBBLE) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_BUBBLE;

                }
                else if (result.classes[j] == DefectType::TYPE_TRADEMARK) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_TRADEMARK;

                }
                else if (result.classes[j] == DefectType::TYPE_WATERSTAIN) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_WATERSTAIN;

                }
                else if (result.classes[j] == DefectType::TYPE_SMUDGE) {

                    typeName = " ";
                    TypeValue = DefectType::TYPE_SMUDGE;

                }
                cv::Mat RoiSegImg;
                cv::Rect rect = cv::Rect(result.boxes[j].left, result.boxes[j].top,
                    result.boxes[j].right - result.boxes[j].left,
                    result.boxes[j].bottom - result.boxes[j].top)
                    & cv::Rect(0, 0, img.cols, img.rows);

                // 可选：检查矩形是否有效
                if (rect.width <= 0 || rect.height <= 0) {
                    // 处理无效矩形的情况
                    continue; // 或者跳过这个检测框
                }
                std::vector<float> errorInfo;
                ConnectedComponentItem analysisInfo;
                DefectLevel levelType = levelUtils.defectLevelProcessByEdge(img, rect, TypeValue, analysisInfo, errorInfo);
                if (levelType == ABNORMAL) {//该缺陷过小 小于最小阈值
                    continue;
                }
                if (levelType == AREAERROR) {//未分割成功情况下，进行特殊处理


                    levelType = levelUtils.defectLevelProcessSingle(img, rect, TypeValue, analysisInfo, errorInfo);

                }
                else {

                    //辅助判断
                    //对脏污和气泡进行判断
                    if (TypeValue == DefectType::TYPE_SMUDGE || TypeValue == DefectType::TYPE_BUBBLE) {
                        if (analysisInfo.numComponent == 2 || analysisInfo.numComponent == 3) {//单一区域

                            levelUtils.secondDetect(img, rect, analysisInfo, TypeValue, levelType, errorInfo);
                        }

                    }
                }

                levelUtils.DoubleChangeErrorType(img, rect, analysisInfo, TypeValue, levelType, errorInfo, result.scores[j]);

                drawInformation saveInformation;
                saveInformation.rect.x = indexs_Cu_[i] % imgIndexs[0].size() * img.cols + rect.x;
                saveInformation.rect.y = indexs_Cu_[i] / imgIndexs[0].size() * img.rows + rect.y;
                saveInformation.rect.width = rect.width;
                saveInformation.rect.height = rect.height;
                saveInformation.ErrorType = levelType;
                saveInformation.DefectType = TypeValue;
                saveInformation.Errorinfo = errorInfo;
                saveInformation.confidence = result.scores[j];
                saveInformations.push_back(saveInformation);
            }
        }
    }

    //根据参数对等级进行修正
    std::vector<drawInformation>  resultInformations = levelUtils.GetDefectLevel(saveInformations, parameter);

    return resultInformations;
}

/*================================特殊区域计算，根据现场实际情况编写===================================================*/

