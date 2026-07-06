#include "showVirtualInfo.h"

showVirtualInfo::showVirtualInfo()
{
}

showVirtualInfo::~showVirtualInfo()
{
}

/*========================异形玻璃模拟图生成================================*/

cv::Mat smoothBinaryEdgesAdvanced(const cv::Mat& inputBinary) {
    // 运行时检查，避免 CV_Assert 在 release 下抛异常或终止
    if (inputBinary.empty() || inputBinary.type() != CV_8UC1) {
        // 返回一个空图或拷贝作为安全退路
        return inputBinary.empty() ? cv::Mat() : inputBinary.clone();
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::Mat binary_inv;
    try {
        cv::bitwise_not(inputBinary, binary_inv);
        cv::findContours(binary_inv, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    }
    catch (const cv::Exception& e) {
        // 发生异常时返回原图的安全拷贝
        std::cerr << "[smoothBinaryEdgesAdvanced] OpenCV exception: " << e.what() << std::endl;
        return inputBinary.clone();
    }

    cv::Mat contourMask = cv::Mat::zeros(inputBinary.size(), CV_8UC1);

    for (const auto& contour : contours) {
        if (contour.empty()) continue;
        double epsilon = 0.005 * cv::arcLength(contour, true);
        std::vector<cv::Point> approx;
        try {
            cv::approxPolyDP(contour, approx, epsilon, true);
        }
        catch (...) {
            continue;
        }

        if (approx.empty()) continue;
        std::vector<std::vector<cv::Point>> drawContours = { approx };
        cv::drawContours(contourMask, drawContours, -1, cv::Scalar(255), cv::FILLED);
    }

    // 反转回与 inputBinary 相同的颜色关系（保持输入的白/黑语义）
    cv::bitwise_not(contourMask, contourMask);
    return contourMask;
}

cv::Mat showVirtualInfo::generateVirtualImage(const cv::Mat& legendImage, cv::Size virtualSize, int glassColor, int BackgroundColor, int threshold)
{
    // 输入验证
    if (legendImage.empty()) {
        std::cerr << "[generateVirtualImage] legendImage is empty" << std::endl;
        return cv::Mat();
    }
    if (virtualSize.width <= 0 || virtualSize.height <= 0) {
        std::cerr << "[generateVirtualImage] invalid virtualSize, must be > 0" << std::endl;
        return cv::Mat();
    }
    if (threshold < 0) threshold = 0;
    if (threshold > 255) threshold = 255;

    cv::Mat binaryImage;
    try {
        if (legendImage.channels() == 3) {
            cv::Mat gray;
            cv::cvtColor(legendImage, gray, cv::COLOR_BGR2GRAY);
            cv::threshold(gray, binaryImage, threshold, 255, cv::THRESH_BINARY);
        }
        else {
            cv::threshold(legendImage, binaryImage, threshold, 255, cv::THRESH_BINARY);
        }
    }
    catch (const cv::Exception& e) {
        std::cerr << "[generateVirtualImage] threshold/cvt exception: " << e.what() << std::endl;
        return cv::Mat();
    }

    cv::Mat small;
    try {
        // 注意 resize 的参数顺序：dsize, fx, fy, interpolation
        cv::resize(binaryImage, small, virtualSize, 0, 0, cv::INTER_AREA);
    }
    catch (const cv::Exception& e) {
        std::cerr << "[generateVirtualImage] resize exception: " << e.what() << std::endl;
        return cv::Mat();
    }

    // 安全计算 scale（避免除 0）
    float scale = 0.f;
    if (legendImage.rows > 0) scale = static_cast<float>(small.rows) / static_cast<float>(legendImage.rows);

    const int border = 10;
    cv::Mat padded;
    try {
        // 使用 0 填充（黑色），保持原逻辑注释说明一致性（若需白色改为 255）
        cv::copyMakeBorder(small, padded, border, border, border, border,
            cv::BORDER_CONSTANT, cv::Scalar(0));
    }
    catch (const cv::Exception& e) {
        std::cerr << "[generateVirtualImage] copyMakeBorder exception: " << e.what() << std::endl;
        return cv::Mat();
    }

    cv::Mat binaryInv;
    try {
        cv::threshold(padded, binaryInv, threshold, 255, cv::THRESH_BINARY_INV);
    }
    catch (const cv::Exception& e) {
        std::cerr << "[generateVirtualImage] threshold on padded failed: " << e.what() << std::endl;
        return cv::Mat();
    }

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 15));
    cv::Mat opened;
    try {
        cv::morphologyEx(binaryInv, opened, cv::MORPH_OPEN, kernel);
    }
    catch (const cv::Exception& e) {
        std::cerr << "[generateVirtualImage] morphologyEx failed: " << e.what() << std::endl;
        opened = binaryInv.clone();
    }

    cv::Mat smoothEdge = smoothBinaryEdgesAdvanced(opened);

    // 检查 smoothEdge 尺寸是否满足裁剪
    if (smoothEdge.empty() ||
        smoothEdge.rows < border + virtualSize.height ||
        smoothEdge.cols < border + virtualSize.width) {
        // 在异常情况下，退回到 opened（保证尺寸）
        if (!opened.empty() &&
            opened.rows >= border + virtualSize.height &&
            opened.cols >= border + virtualSize.width) {
            smoothEdge = opened;
        }
        else {
            std::cerr << "[generateVirtualImage] smoothEdge/opened too small for cropping" << std::endl;
            return cv::Mat();
        }
    }

    cv::Mat cropped;
    try {
        cropped = smoothEdge(cv::Rect(border, border, virtualSize.width, virtualSize.height)).clone();
    }
    catch (const cv::Exception& e) {
        std::cerr << "[generateVirtualImage] crop exception: " << e.what() << std::endl;
        return cv::Mat();
    }

    cv::Mat result;
    try {
        cropped.convertTo(result, CV_8U);
        // 0 -> glassColor, 255 -> BackgroundColor （根据原逻辑）
        cv::Mat maskGlass = (result == 0);
        cv::Mat maskBg = (result == 255);
        if (!maskGlass.empty()) result.setTo(glassColor, maskGlass);
        if (!maskBg.empty()) result.setTo(BackgroundColor, maskBg);
    }
    catch (const cv::Exception& e) {
        std::cerr << "[generateVirtualImage] color fill exception: " << e.what() << std::endl;
        return cv::Mat();
    }

    return result;
}

std::vector<drawInformation> filterDefectsInGlassRegion(
    const std::vector<drawInformation>& defects,
    const cv::Rect& glassRect) {

    std::vector<drawInformation> filteredDefects;

    if (glassRect.width <= 0 || glassRect.height <= 0) {
        std::cerr << "[filterDefectsInGlassRegion] invalid glassRect" << std::endl;
        return filteredDefects;
    }

    for (const auto& defect : defects) {
        // 直接检查缺陷是否完全在玻璃区域内
        if ((defect.rect & glassRect) == defect.rect) {
            filteredDefects.push_back(defect);
        }
    }

    return filteredDefects;
}

std::vector<drawInformation> filterDefectsInWhiteRegions(
    const std::vector<drawInformation>& defects,
    const cv::Mat& binaryImage) {

    std::vector<drawInformation> filteredDefects;

    if (binaryImage.empty()) {
        std::cerr << "[filterDefectsInWhiteRegions] binaryImage is empty" << std::endl;
        return filteredDefects;
    }
    if (binaryImage.channels() != 1 || binaryImage.depth() != CV_8U) {
        std::cerr << "[filterDefectsInWhiteRegions] binaryImage must be CV_8UC1" << std::endl;
        return filteredDefects;
    }

    for (const auto& defect : defects) {
        cv::Rect rect = defect.rect;

        // 与图像边界求交集，保证安全
        rect = rect & cv::Rect(0, 0, binaryImage.cols, binaryImage.rows);

        if (rect.width <= 0 || rect.height <= 0) {
            continue;
        }

        bool isInWhiteRegion = false;

        // 方法3：检查四个角点（保留），但先确保角点在图像范围内
        std::vector<cv::Point> corners = {
            cv::Point(rect.x, rect.y),
            cv::Point(rect.x + rect.width - 1, rect.y),
            cv::Point(rect.x, rect.y + rect.height - 1),
            cv::Point(rect.x + rect.width - 1, rect.y + rect.height - 1)
        };

        int whiteCorners = 0;
        for (const auto& corner : corners) {
            if (corner.x >= 0 && corner.x < binaryImage.cols && corner.y >= 0 && corner.y < binaryImage.rows) {
                uchar cornerValue = binaryImage.at<uchar>(corner.y, corner.x);
                if (cornerValue == 255) whiteCorners++;
            }
        }
        if (whiteCorners == 4) isInWhiteRegion = true;

        // 方法2（可选，更稳健）：如果四角法不足，可以按需启用像素比例判断（注释掉以节省开销）
        /*
        if (!isInWhiteRegion) {
            cv::Mat roi = binaryImage(rect);
            int whitePixels = cv::countNonZero(roi);
            double whiteRatio = static_cast<double>(whitePixels) / (rect.width * rect.height);
            if (whiteRatio > 0.5) isInWhiteRegion = true;
        }
        */

        if (isInWhiteRegion) filteredDefects.push_back(defect);
    }

    return filteredDefects;
}



// 分块处理的OpenMP版本
cv::Rect findGlassRegionByBoundaryScan_OpenMP_Chunked(const cv::Mat& binaryImage, int chunk_size = 1000) {
    int rows = binaryImage.rows;
    int cols = binaryImage.cols;

    int top = rows, bottom = -1, left = cols, right = -1;

#pragma omp parallel
    {
        int local_top = rows;
        int local_bottom = -1;
        int local_left = cols;
        int local_right = -1;

        // 分块处理，避免缓存问题
        int num_chunks = (rows + chunk_size - 1) / chunk_size;

#pragma omp for nowait
        for (int chunk = 0; chunk < num_chunks; chunk++) {
            int start_row = chunk * chunk_size;
            int end_row = std::min((chunk + 1) * chunk_size, rows);

            for (int i = start_row; i < end_row; i++) {
                const uchar* rowPtr = binaryImage.ptr<uchar>(i);

                for (int j = 0; j < cols; j++) {
                    if (rowPtr[j] > 0) {
                        if (i < local_top) local_top = i;
                        if (i > local_bottom) local_bottom = i;
                        if (j < local_left) local_left = j;
                        if (j > local_right) local_right = j;
                    }
                }
            }
        }

        // 合并结果
#pragma omp critical
        {
            if (local_top < top) top = local_top;
            if (local_bottom > bottom) bottom = local_bottom;
            if (local_left < left) left = local_left;
            if (local_right > right) right = local_right;
        }
    }

    if (top > bottom || left > right) {
        std::cout << "未找到有效玻璃区域" << std::endl;
        return cv::Rect();
    }

    //std::cout << "检测到玻璃区域: [" << top << ", " << bottom << ", " << left << ", " << right << "]" << std::endl;
    return cv::Rect(left, top, right - left + 1, bottom - top + 1);
}
bool showVirtualInfo::GetGlassAreaAndResult(
    const cv::Mat& largeImage, int threshold, int showGrayValue, std::vector<drawInformation>& drawInfos, int minAreaThreshold,
    cv::Rect& glassRect, cv::Mat& glassMaskImage, std::vector<drawInformation>& drawInfosFilter) {

    // 输入保护
    if (largeImage.empty()) {
        std::cerr << "[GetGlassAreaAndResult] largeImage is empty" << std::endl;
        return false;
    }
    if (threshold < 0) threshold = 0;
    if (threshold > 255) threshold = 255;

    cv::Mat binaryImage;
    try {
        if (largeImage.channels() == 3) {
            cv::Mat gray;
            cv::cvtColor(largeImage, gray, cv::COLOR_BGR2GRAY);
            cv::threshold(gray, binaryImage, threshold, 255, cv::THRESH_BINARY);
        }
        else {
            cv::threshold(largeImage, binaryImage, threshold, 255, cv::THRESH_BINARY);
        }
    }
    catch (const cv::Exception& e) {
        std::cerr << "[GetGlassAreaAndResult] threshold/cvt exception: " << e.what() << std::endl;
        return false;
    }

    // 使用优化的边界扫描方法替代 findNonZero + boundingRect
    try {
        glassRect = findGlassRegionByBoundaryScan_OpenMP_Chunked(binaryImage, 2000);

        ////为现场采集崩边数据，向图像四周扩充100个像素
		//glassRect.x = std::max(0, glassRect.x - 100);
		//glassRect.y = std::max(0, glassRect.y - 100);
		//glassRect.width = std::min(binaryImage.cols - glassRect.x, glassRect.width + 200);
		//glassRect.height = std::min(binaryImage.rows - glassRect.y, glassRect.height + 200);

    }
    catch (const cv::Exception& e) {
        std::cerr << "[GetGlassAreaAndResult] boundary scan exception: " << e.what() << std::endl;
        return false;
    }

    // 检查是否找到有效区域
    if (glassRect.width <= 0 || glassRect.height <= 0) {
        std::cout << "[GetGlassAreaAndResult] No white region found in binary image" << std::endl;
        return false;
    }

    // 修正矩形，确保在图像边界内
    glassRect &= cv::Rect(0, 0, binaryImage.cols, binaryImage.rows);
    if (glassRect.width <= 0 || glassRect.height <= 0) {
        std::cerr << "[GetGlassAreaAndResult] invalid glassRect after clamp" << std::endl;
        return false;
    }

    int area = glassRect.width * glassRect.height;
    if (area < minAreaThreshold) {
        std::cout << "[GetGlassAreaAndResult] Glass area too small: " << area << " < " << minAreaThreshold << std::endl;
        return false;
    }

    // 过滤检测结果到玻璃区域内
    try {
        //drawInfosFilter = filterDefectsInWhiteRegions(drawInfos, binaryImage);//可能导致检测结果不正确
        drawInfosFilter = filterDefectsInGlassRegion(drawInfos, glassRect);//仅保存rect以内的检测结果
        
    }
    catch (...) {
        std::cerr << "[GetGlassAreaAndResult] filterDefectsInWhiteRegions threw" << std::endl;
        drawInfosFilter.clear();
    }

    // 获取玻璃区域二值化图（安全浅拷贝）
    try {
        if (glassRect.x >= 0 && glassRect.y >= 0 &&
            glassRect.x + glassRect.width <= binaryImage.cols &&
            glassRect.y + glassRect.height <= binaryImage.rows) {
            glassMaskImage = binaryImage(glassRect).clone();
        }
        else {
            // 做修正后再尝试
            cv::Rect safeRect = glassRect & cv::Rect(0, 0, binaryImage.cols, binaryImage.rows);
            if (safeRect.width > 0 && safeRect.height > 0) {
                glassMaskImage = binaryImage(safeRect).clone();
                glassRect = safeRect;
            }
            else {
                std::cerr << "[GetGlassAreaAndResult] cannot extract glassMaskImage, safeRect invalid" << std::endl;
                return false;
            }
        }
    }
    catch (const cv::Exception& e) {
        std::cerr << "[GetGlassAreaAndResult] extracting glassMaskImage failed: " << e.what() << std::endl;
        return false;
    }

    return true;
}

/*========================异形玻璃模拟图生成================================*/



/*========================均匀度玻璃模拟图生成================================*/
// 下采样图像：将n*n区域的平均灰度值作为新像素
cv::Mat showVirtualInfo::downsampleImage(const cv::Mat& inputImage, int regionSize) {
    if (inputImage.empty() || regionSize <= 0) {
        return cv::Mat();
    }

    cv::Mat gray;
    try {
        if (inputImage.channels() == 3 || inputImage.channels() == 4) {
            cv::cvtColor(inputImage, gray, cv::COLOR_BGR2GRAY);
        }
        else {
            // 仍然确保单通道 8U
            gray = inputImage.clone();
        }

        // 强制转换到 8-bit 单通道（如果原来不是）
        if (gray.type() != CV_8UC1) {
            gray.convertTo(gray, CV_8U);
        }

        // 计算目标尺寸：使用 ceil 以包含右/下余数像素，避免丢列/行
        int newWidth = (gray.cols + regionSize - 1) / regionSize;
        int newHeight = (gray.rows + regionSize - 1) / regionSize;

        if (newWidth <= 0 || newHeight <= 0) return cv::Mat();

        cv::Mat downsampled;
        // INTER_AREA 对应区域平均，是推荐的降采样方法
        cv::resize(gray, downsampled, cv::Size(newWidth, newHeight), 0, 0, cv::INTER_AREA);

        // 确保类型为 CV_8UC1
        if (downsampled.type() != CV_8UC1) {
            downsampled.convertTo(downsampled, CV_8U);
        }

        return downsampled;
    }
    catch (const cv::Exception&) {
        return cv::Mat();
    }
}

// 提取玻璃区域：根据灰度阈值生成二值化掩码
cv::Mat showVirtualInfo::extractGlassRegions(const cv::Mat& image, int glassThreshold) {
    if (image.empty()) return cv::Mat();

    try {
        cv::Mat img8;
        if (image.type() != CV_8UC1) {
            // 如果是浮点或其他类型，先转换到 8U 单通道
            if (image.channels() == 3 || image.channels() == 4) {
                cv::cvtColor(image, img8, cv::COLOR_BGR2GRAY);
            }
            else {
                image.convertTo(img8, CV_8U);
            }
        }
        else {
            img8 = image;
        }

        cv::Mat binaryMask;
        cv::threshold(img8, binaryMask, std::clamp(glassThreshold, 0, 255), 255, cv::THRESH_BINARY);

        // 动态选择 kernel 大小，不能大于图像尺寸的一半
        int k = 5;
        k = std::min(k, std::max(1, std::min(binaryMask.cols / 2, binaryMask.rows / 2)));
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(k, k));

        if (kernel.empty() == false && binaryMask.cols >= k && binaryMask.rows >= k) {
            cv::morphologyEx(binaryMask, binaryMask, cv::MORPH_CLOSE, kernel);
            cv::morphologyEx(binaryMask, binaryMask, cv::MORPH_OPEN, kernel);
        }

        return binaryMask;
    }
    catch (const cv::Exception&) {
        return cv::Mat();
    }
}

// 查找玻璃区域轮廓
// 查找轮廓：clone 输入以避免修改 caller 的 Mat，并使用基于图像尺寸的最小面积过滤
std::vector<std::vector<cv::Point>> showVirtualInfo::findGlassContours(const cv::Mat& binaryMask) {
    std::vector<std::vector<cv::Point>> filteredContours;
    if (binaryMask.empty()) return filteredContours;

    // 确保为单通道 8U
    if (binaryMask.type() != CV_8UC1) {
        // 不抛异常，返回空
        return filteredContours;
    }

    try {
        // clone 一份，findContours 会修改图像
        cv::Mat maskCopy = binaryMask.clone();

        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(maskCopy, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // 根据图像大小动态设置最小面积阈值，避免硬编码 100 在极小下采样时过滤掉所有轮廓
        double imageArea = static_cast<double>(maskCopy.cols) * maskCopy.rows;
        double minArea = std::max(4.0, imageArea * 0.0005); // 经验值，可调整

        for (const auto& c : contours) {
            double area = cv::contourArea(c);
            if (area >= minArea) filteredContours.push_back(c);
        }
    }
    catch (const cv::Exception&) {
        // 返回空
        return filteredContours;
    }

    return filteredContours;
}

// 为玻璃区域创建伪彩图
cv::Mat showVirtualInfo::createPseudoColorForRegions(const cv::Mat& downsampledImage,
    const std::vector<std::vector<cv::Point>>& contours) {
    if (downsampledImage.empty() || contours.empty()) return cv::Mat();

    try {
        // 确保输入为单通道 8U
        cv::Mat ds;
        if (downsampledImage.type() != CV_8UC1) {
            downsampledImage.convertTo(ds, CV_8U);
        }
        else {
            ds = downsampledImage;
        }

        // 创建二值掩码并绘制轮廓
        cv::Mat mask = cv::Mat::zeros(ds.size(), CV_8UC1);
        for (const auto& contour : contours) {
            if (!contour.empty()) {
                // 防止异常轮廓坐标越界：可选地对点做裁剪，但通常由 findContours 保证
                cv::drawContours(mask, std::vector<std::vector<cv::Point>>{contour}, -1, cv::Scalar(255), cv::FILLED);
            }
        }

        // 对下采样图做伪彩色（输入要求 8U 单通道或 3通道）
        cv::Mat pseudoColor;
        cv::applyColorMap(ds, pseudoColor, cv::COLORMAP_JET);

        // 结果为三通道，背景为黑色
        cv::Mat result = cv::Mat::zeros(pseudoColor.size(), pseudoColor.type());

        // 使用二值掩码复制伪彩图区域
        pseudoColor.copyTo(result, mask);

        return result;
    }
    catch (const cv::Exception&) {
        return cv::Mat();
    }
}

// 主函数：生成玻璃伪彩图
cv::Mat showVirtualInfo::generateGlassPseudoColor(const cv::Mat& inputImage, int glassThreshold, int regionSize) {
    // 输入验证
    if (inputImage.empty()) {
        //std::cerr << "[generateGlassPseudoColor] Input image is empty" << std::endl;
        return cv::Mat();
    }

    if (glassThreshold < 0 || glassThreshold > 255) {
        //std::cerr << "[generateGlassPseudoColor] Invalid glass threshold: " << glassThreshold << std::endl;
        return cv::Mat();
    }

    if (regionSize <= 0) {
        //std::cerr << "[generateGlassPseudoColor] Invalid region size: " << regionSize << std::endl;
        return cv::Mat();
    }

    try {
        //std::cout << "[generateGlassPseudoColor] Starting processing..." << std::endl;
        //std::cout << "[generateGlassPseudoColor] Input image size: " << inputImage.cols << "x" << inputImage.rows << std::endl;
        //std::cout << "[generateGlassPseudoColor] Parameters - threshold: " << glassThreshold << ", region size: " << regionSize << std::endl;

        // 步骤1: 下采样图像
        //std::cout << "[generateGlassPseudoColor] Step 1: Downsampling image..." << std::endl;
        cv::Mat downsampled = downsampleImage(inputImage, regionSize);
        if (downsampled.empty()) {
            //std::cerr << "[generateGlassPseudoColor] Downsampling failed" << std::endl;
            return cv::Mat();
        }
        //std::cout << "[generateGlassPseudoColor] Downsampled size: " << downsampled.cols << "x" << downsampled.rows << std::endl;

        // 步骤2: 提取玻璃区域
        //std::cout << "[generateGlassPseudoColor] Step 2: Extracting glass regions..." << std::endl;
        cv::Mat glassMask = extractGlassRegions(downsampled, glassThreshold);
        if (glassMask.empty()) {
            //std::cerr << "[generateGlassPseudoColor] Glass region extraction failed" << std::endl;
            return cv::Mat();
        }

        // 步骤3: 查找轮廓
        //std::cout << "[generateGlassPseudoColor] Step 3: Finding contours..." << std::endl;
        std::vector<std::vector<cv::Point>> contours = findGlassContours(glassMask);
        //std::cout << "[generateGlassPseudoColor] Found " << contours.size() << " glass regions" << std::endl;

        if (contours.empty()) {
            //std::cerr << "[generateGlassPseudoColor] No glass regions found" << std::endl;
            return cv::Mat();
        }

        // 步骤4: 生成伪彩图
        //std::cout << "[generateGlassPseudoColor] Step 4: Creating pseudo color map..." << std::endl;
        cv::Mat result = createPseudoColorForRegions(downsampled, contours);
        if (result.empty()) {
            //std::cerr << "[generateGlassPseudoColor] Pseudo color creation failed" << std::endl;
            return cv::Mat();
        }

        //std::cout << "[generateGlassPseudoColor] Processing completed successfully" << std::endl;
        return result;
    }
    catch (const cv::Exception& e) {
        //std::cerr << "[generateGlassPseudoColor] OpenCV exception: " << e.what() << std::endl;
        return cv::Mat();
    }
    catch (const std::exception& e) {
        //std::cerr << "[generateGlassPseudoColor] Standard exception: " << e.what() << std::endl;
        return cv::Mat();
    }
    catch (...) {
        //std::cerr << "[generateGlassPseudoColor] Unknown exception" << std::endl;
        return cv::Mat();
    }
}


/// <summary>
/// 20260203 亮场确定区域透场绘制伪彩图
/// </summary>
/// <param name="inputImage">透场图</param>
/// <param name="inputImage2">亮场图</param>
/// <param name="glassThreshold"></param>
/// <param name="regionSize"></param>
/// <returns></returns>
cv::Mat showVirtualInfo::generateGlassPseudoColorChannel2(const cv::Mat& inputImage, const cv::Mat& inputImage2, int glassThreshold, int regionSize) {
    // 输入验证
    if (inputImage.empty()) {
        //std::cerr << "[generateGlassPseudoColor] Input image is empty" << std::endl;
        return cv::Mat();
    }

    if (glassThreshold < 0 || glassThreshold > 255) {
        //std::cerr << "[generateGlassPseudoColor] Invalid glass threshold: " << glassThreshold << std::endl;
        return cv::Mat();
    }

    if (regionSize <= 0) {
        //std::cerr << "[generateGlassPseudoColor] Invalid region size: " << regionSize << std::endl;
        return cv::Mat();
    }

    try {
        //std::cout << "[generateGlassPseudoColor] Starting processing..." << std::endl;
        //std::cout << "[generateGlassPseudoColor] Input image size: " << inputImage.cols << "x" << inputImage.rows << std::endl;
        //std::cout << "[generateGlassPseudoColor] Parameters - threshold: " << glassThreshold << ", region size: " << regionSize << std::endl;

        // 步骤1: 下采样图像
        //std::cout << "[generateGlassPseudoColor] Step 1: Downsampling image..." << std::endl;
        cv::Mat downsampled1 = downsampleImage(inputImage, regionSize);
        cv::Mat downsampled2 = downsampleImage(inputImage2, regionSize);
        if (downsampled1.empty()|| downsampled2.empty()) {
            //std::cerr << "[generateGlassPseudoColor] Downsampling failed" << std::endl;
            return cv::Mat();
        }
        //std::cout << "[generateGlassPseudoColor] Downsampled size: " << downsampled.cols << "x" << downsampled.rows << std::endl;

        // 步骤2: 提取玻璃区域
        //std::cout << "[generateGlassPseudoColor] Step 2: Extracting glass regions..." << std::endl;
        cv::Mat glassMask = extractGlassRegions(downsampled2, glassThreshold);
        if (glassMask.empty()) {
            //std::cerr << "[generateGlassPseudoColor] Glass region extraction failed" << std::endl;
            return cv::Mat();
        }

        // 步骤3: 查找轮廓
        //std::cout << "[generateGlassPseudoColor] Step 3: Finding contours..." << std::endl;
        std::vector<std::vector<cv::Point>> contours = findGlassContours(glassMask);
        //std::cout << "[generateGlassPseudoColor] Found " << contours.size() << " glass regions" << std::endl;

        if (contours.empty()) {
            //std::cerr << "[generateGlassPseudoColor] No glass regions found" << std::endl;
            return cv::Mat();
        }

        // 步骤4: 生成伪彩图
        //std::cout << "[generateGlassPseudoColor] Step 4: Creating pseudo color map..." << std::endl;
        cv::Mat result = createPseudoColorForRegions(downsampled1, contours);
        if (result.empty()) {
            //std::cerr << "[generateGlassPseudoColor] Pseudo color creation failed" << std::endl;
            return cv::Mat();
        }

        //std::cout << "[generateGlassPseudoColor] Processing completed successfully" << std::endl;
        return result;
    }
    catch (const cv::Exception& e) {
        //std::cerr << "[generateGlassPseudoColor] OpenCV exception: " << e.what() << std::endl;
        return cv::Mat();
    }
    catch (const std::exception& e) {
        //std::cerr << "[generateGlassPseudoColor] Standard exception: " << e.what() << std::endl;
        return cv::Mat();
    }
    catch (...) {
        //std::cerr << "[generateGlassPseudoColor] Unknown exception" << std::endl;
        return cv::Mat();
    }
}
/*========================均匀度玻璃模拟图生成================================*/



/**
 * @brief 多帧拼接并生成虚拟图，代替原有的generateVirtualImage
 * 
 * 此项目为单张使用
 * @param images 输入图像集，所有图像尺寸相同
 * @param thresholdValue 灰度阈值，用于二值化图像
 * @param downSampleSize 下采样尺寸（宽×高）
 * @return 生成的虚拟图
 */
cv::Mat showVirtualInfo::createVirtualMap(const std::vector<cv::Mat>& images,  int glassColor, int BackgroundColor,
    int thresholdValue = 20)
 {

    // 检查输入是否为空
    if (images.empty()) {
        //std::cerr << "错误: 输入图像集为空!" << std::endl;
        return cv::Mat();
    }

    // 检查所有图像是否尺寸相同
    cv::Size firstSize = images[0].size();
    for (size_t i = 1; i < images.size(); i++) {
        if (images[i].empty())
        {
            return cv::Mat();
        }
        if (images[i].size() != firstSize) {
            //std::cerr << "错误: 图像尺寸不一致! 图像" << i
            //    << "的尺寸为" << images[i].size()
            //    << ", 但第一张图像的尺寸为" << firstSize << std::endl;
            return cv::Mat();
        }
        if (images[i].rows == 0 || images[i].cols == 0) {
            return cv::Mat();
        }
    }
    cv::Mat virtualMap;
    if (images.size() > 1) {
   
        cv::vconcat(images, virtualMap);
    }
    else if(images.size() == 1){
        virtualMap = images[0];
    }
    else {
        return cv::Mat();
    }


    cv::Mat binary;
    cv::threshold(virtualMap, binary, thresholdValue, 255, cv::THRESH_BINARY);

    int kernelSize = 3;
    // 创建核（结构元素）
    cv::Mat  kernel = cv::getStructuringElement(cv::MORPH_RECT,
        cv::Size(kernelSize, kernelSize));

    cv::Mat virtualMapFilter;
    cv::morphologyEx(binary, virtualMapFilter, cv::MORPH_CLOSE, kernel,
        cv::Point(-1, -1), 1);

    cv::Mat result;
    try {
        virtualMapFilter.convertTo(result, CV_8U);
        cv::Mat maskGlass = (result == 0);
        cv::Mat maskBg = (result == 255);
        if (!maskGlass.empty()) result.setTo(glassColor, maskGlass);
        if (!maskBg.empty()) result.setTo(BackgroundColor, maskBg);
    }
    catch (const cv::Exception& e) {
        std::cerr << "[generateVirtualImage] color fill exception: " << e.what() << std::endl;
        return cv::Mat();
    }

    return result;
}