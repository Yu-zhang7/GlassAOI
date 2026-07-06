#include "Compute.h"
#include "DefectLevelTuning.h"

Compute::Compute()
{
    //std::string model1Name = R"(E:\code\C++_Utils\TenosorRT_Test\TenosorRT_Test\models\detect\model.engine)";
    //std::string model2Name = R"(E:\code\C++_Utils\TenosorRT_Test\TenosorRT_Test\models\detect\model2.engine)";

    //model1Name = ModelPath_cu;
    //model2Name = ModelPath_jing;
    //deploy::InferOption option;
    //option.enableSwapRB();
    //model = std::make_unique<deploy::DetectModel>(model1Name, option);
    //model2 = std::make_unique<deploy::DetectModel>(model2Name, option);

    //�°��㷨
    //todo  �޸�·����¼��config

    std::string modelOnnx = ModelPath_cu;
    std::string modelEngine = ModelPath_jing;
    m_detector.init(modelOnnx.c_str(), modelEngine.c_str());

}

Compute::~Compute()
{

}

/*=========�������躯��============*/
// ���Ʋ���������ο��ʵ��
cv::Mat drawGlassRegions(const cv::Mat& originalImage,
    const std::vector<int>& glassIndices,
    int tileWidth, int tileHeight,
    const cv::Scalar& color = cv::Scalar(0, 255, 0),
    int thickness = 3) {
    if (originalImage.empty()) {
        std::cerr << "Original image is empty!" << std::endl;
        return cv::Mat();
    }

    // ����ԭͼ�ĸ�������ͨ����
    cv::Mat resultImage;
    if (originalImage.channels() == 1) {
        cv::cvtColor(originalImage, resultImage, cv::COLOR_GRAY2BGR);
    }
    else {
        resultImage = originalImage.clone();
    }

    // ����ÿ���ж��ٸ�ͼ��
    int tilesPerRow = (originalImage.cols + tileWidth - 1) / tileWidth;

    // Ϊÿ������������ƾ��ο�
    for (int idx : glassIndices) {
        // ����ͼ���ڴ�ͼ�е�λ��
        int row = idx / tilesPerRow;
        int col = idx % tilesPerRow;

        // ������ο������
        int x = col * tileWidth;
        int y = row * tileHeight;

        // ȷ�����겻����ͼ��߽�
        int actualWidth = std::min(tileWidth, originalImage.cols - x);
        int actualHeight = std::min(tileHeight, originalImage.rows - y);

        // ���ƾ��ο�
        cv::rectangle(resultImage,
            cv::Rect(x, y, actualWidth, actualHeight),
            color, thickness);

        // ��ѡ���ھ��ο���������������
        std::string text = std::to_string(idx);
        cv::putText(resultImage, text,
            cv::Point(x + 10, y + 30),
            cv::FONT_HERSHEY_SIMPLEX, 1.0, color, 2);
    }

    return resultImage;
}
/*=========�������躯��============*/

int divideImageShallow(cv::Mat& largeImage, int imgWidth, int imgHeight,
    std::vector<cv::Mat>& tiles,
    std::vector<std::vector<int>>& imgIndexs) {
    if (largeImage.empty()) {
        std::cerr << "Input image is empty!" << std::endl;
        return -1;
    }

    // ����ָ��Ŀ���������ȡ����
    int numTilesX = (largeImage.cols + imgWidth - 1) / imgWidth;
    int numTilesY = (largeImage.rows + imgHeight - 1) / imgHeight;

    // Ԥ�����������
    tiles.reserve(numTilesX * numTilesY);
    imgIndexs.resize(numTilesY);

    for (int y = 0; y < numTilesY; ++y) {
        int y_start = y * imgHeight;
        int actual_height = std::min(imgHeight, largeImage.rows - y_start);

        for (int x = 0; x < numTilesX; ++x) {
            int x_start = x * imgWidth;
            int actual_width = std::min(imgWidth, largeImage.cols - x_start);

            // ����ROI��ǳ������
            cv::Mat tile(largeImage, cv::Rect(x_start, y_start, actual_width, actual_height));

            // ֻ�е��鲻����ʱ����Ҫ����
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
/*=============================��batch�汾+��ɸ�Ż�===============================================*/
 //����һ��ͼ��
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
//        if (flag == 0) {//�ּ�� �����ؼ����,�����ش���ͼƬ����
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
//                        if (results[n].scores[m] > threshold) {//�жϼ�����ǲ��Ƕ�������ֵ
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
//        else {//��ȷ��⣬���������Լ����������ʱ��ʹ��
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
//
//void process_batch_images(const std::vector<cv::Mat>& imageAll, const std::vector<int>& indices, deploy::DetectModel& model, std::vector<int>& errIndexs, std::vector<deploy::DetectRes>& Informations, double threshold, int flag = 0) {
//
//    errIndexs.clear();
//    Informations.clear();
//    const int batch_size = model.batch_size();
//
//    for (int i = 0; i < indices.size(); i += batch_size) {
//        std::vector<cv::Mat> images;
//        std::vector<deploy::Image> img_batch;
//        std::vector<int> img_Index_batch;
//
//        // ����������������
//        for (int j = i; j < i + batch_size && j < indices.size(); ++j) {
//            int img_index = indices[j];
//            if (img_index >= 0 && img_index < imageAll.size()) {
//                cv::Mat image;
//                cv::cvtColor(imageAll[img_index], image, cv::COLOR_GRAY2RGB);
//
//                images.push_back(image);
//                img_batch.emplace_back(image.data, image.cols, image.rows);
//                img_Index_batch.push_back(img_index);
//            }
//        }
//        std::vector<deploy::DetectRes> results;
//        if (img_batch.empty()) continue;
//        if (&model == nullptr || model.batch_size() <= 0) continue;
//        try
//        {
//            //auto results = model.predict(img_batch);
//            results = model.predict(img_batch);
//        }
//        catch (const std::exception& e)
//        {
//            FILE_LOG_ERROR("[Compute] model.predict threw: %s", e.what());
//            return;
//        }
//
//
//        if (flag == 0) { // �ּ�⣺ֻ������ȱ�ݵ�ͼ������
//            for (int n = 0; n < results.size(); n++) {
//                if (results[n].num > 0) {
//                    bool has_defect = false;
//                    for (int m = 0; m < results[n].num; m++) {
//                        if (results[n].scores[m] > threshold) {
//                            has_defect = true;
//                            break;
//                        }
//                    }
//                    if (has_defect) {
//                        errIndexs.push_back(img_Index_batch[n]);
//                    }
//                }
//            }
//        }
//        else { // ��ȷ��⣺���������ͼ����
//            for (int n = 0; n < results.size(); n++) {
//                if (results[n].num > 0) {
//                    bool has_defect = false;
//                    for (int m = 0; m < results[n].num; m++) {
//                        if (results[n].scores[m] > threshold) {
//                            has_defect = true;
//                            break;
//                        }
//                    }
//                    if (has_defect) {
//                        errIndexs.push_back(img_Index_batch[n]);
//                        Informations.push_back(results[n]);
//                    }
//                }
//            }
//        }
//    }
//}
//
//// ����һ��ͼ��
//void process_batch_imagesByIndex(const std::vector<cv::Mat>& imageAll, std::vector<int>& IndexsCu, deploy::DetectModel& model, std::vector<int>& errIndexs, std::vector<deploy::DetectRes>& Informations, double threshold) {
//
//    errIndexs.clear();
//    Informations.clear();
//    //double t = (double)cv::getTickCount();
//    const int batch_size = model.batch_size();
//    for (int i = 0; i < IndexsCu.size(); i += batch_size) {
//        std::vector<cv::Mat>        images;
//        std::vector<deploy::Image> img_batch;
//        std::vector<int>    img_Index_batch;
//
//        for (int j = i; j < i + batch_size && j < IndexsCu.size(); ++j) {
//            cv::Mat image;
//            cv::cvtColor(imageAll[IndexsCu[j]], image, cv::COLOR_GRAY2RGB);
//
//            images.push_back(image);
//            img_batch.emplace_back(image.data, image.cols, image.rows);
//            img_Index_batch.push_back(IndexsCu[j]);
//        }
//
//        auto results = model.predict(img_batch);
//
//        for (int n = 0; n < results.size(); n++) {
//            if (results[n].num > 0) {
//
//                int isflag = 0;//�ж��Ƿ������м������������ֵ
//                for (int m = 0; m < results[n].num; m++) {
//
//                    if (results[n].scores[m] > threshold) {//�жϼ�����ǲ��Ƕ�������ֵ
//                        isflag = 1;
//                        break;
//                    }
//
//                }
//                if (isflag == 0) {
//                    continue;
//                }
//                errIndexs.push_back(img_Index_batch[n]);
//                Informations.push_back(results[n]);
//            }
//
//        }
//    }
//
//    //t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
//    //double milliseconds = t * 1000; // ��ת����
//    //std::cout << "process_single_image Sum Time:" << milliseconds << std::endl;
//
//}

/*===========================���������Ƿ����ȱ�ݴ�ɸ=====================================*/

////// <summary>
/////  //1.�з���ɨ�裬�ж��Ƿ���ȱ��
///// </summary>

// �ṹ��洢ÿ��ͳ������
struct RowStats {
    double range_val; // ����
    //double var_val;   // ����
    //double max_grad;  // ����ݶ�
};

// ������ԭʼһ�µ����м������
std::vector<RowStats> calculateImageStatsHorizontal(const cv::Mat& tile) {
    std::vector<RowStats> stats(tile.rows);

    for (int y = 0; y < tile.rows; ++y) {
        const uchar* p = tile.ptr<uchar>(y);
        auto [min_it, max_it] = std::minmax_element(p, p + tile.cols);
        stats[y].range_val = *max_it - *min_it;
    }

    return stats;
}
// ��ֵ�ж��Ż���
// ����ԭʼ�ж��߼�
bool isDefectiveTileHorizontal(const cv::Mat& tile, double threshold) {
    auto stats = calculateImageStatsHorizontal(tile);
    return std::any_of(stats.begin(), stats.end(),
        [threshold](const RowStats& rs) {
            return rs.range_val > threshold;
        });
}

/// <summary>
/// �з���ɨ�裬�ж��Ƿ���ȱ��
/// </summary>
struct ColStats {
    double range_val; // ����
};

// ���򼫲���㣨������
std::vector<ColStats> calculateImageStatsVertical(const cv::Mat& tile) {
    std::vector<ColStats> stats(tile.cols);

    // ����ת�þ�������߷���Ч��
    cv::Mat transposed = tile.t();

    for (int x = 0; x < tile.cols; ++x) {
        const uchar* p = transposed.ptr<uchar>(x);
        auto [min_it, max_it] = std::minmax_element(p, p + tile.rows);
        stats[x].range_val = *max_it - *min_it;
    }

    return stats;
}
// ����ȱ���жϣ�������
bool isDefectiveTileVertical(const cv::Mat& tile, double threshold) {
    auto stats = calculateImageStatsVertical(tile);
    return std::any_of(stats.begin(), stats.end(),
        [threshold](const ColStats& cs) {
            return cs.range_val > threshold;
        });
}
/*===========================���������Ƿ����ȱ�ݴ�ɸ=====================================*/

/*=============================��batch�汾+��ɸ�Ż�===============================================*/


/*=============================��batch�汾+��ɸ�Ż�+��������ȷ��===============================================*/

/*
    ��ʱ����ת
*/
int Compute::RotateImage(std::vector<cv::Mat>& images, int flipV = 0) {

    for (size_t i = 0; i < images.size(); i++)
    {
        // ��ʱ����ת 90 ��
        transpose(images[i], images[i]); // ��ת��
        flip(images[i], images[i], 0);   // �ٴ�ֱ��ת

        if (flipV == 1) {
            // ˮƽ��תͼƬ
            cv::flip(images[i], images[i], 0);
        }
    }
    return 0;
}

// �����ľ�����ת��������ʱ��90�ȣ�
cv::Rect rotateRectCCW90(const cv::Rect& rect, int width, int height)
{
    // ֱ�Ӽ�����ת��ľ��Σ���׼ȷ�Ĺ�ʽ��
    int new_x = rect.y;
    int new_y = width - rect.x - rect.width;
    int new_width = rect.height;
    int new_height = rect.width;

    return cv::Rect(new_x, new_y, new_width, new_height);
}

// �����ļ������ת����
std::vector<drawInformation> Compute::rotateDrawInfoCCW90(
    const std::vector<drawInformation>& drawInfos,
    int width, int height)
{
    std::vector<drawInformation> rotatedInfos;
    for (const auto& info : drawInfos) {
        drawInformation rotated = info;
        // ��ת����
        rotated.rect = rotateRectCCW90(rotated.rect, width, height);

        // ���ӱ߽��飬ȷ����������ת��ͼ��Χ
        int new_img_width = height;  // ��ת��ͼ����� = ԭ�߶�
        int new_img_height = width;  // ��ת��ͼ��߶� = ԭ����

        if (rotated.rect.x < 0) rotated.rect.x = 0;
        if (rotated.rect.y < 0) rotated.rect.y = 0;
        if (rotated.rect.x + rotated.rect.width > new_img_width) {
            rotated.rect.width = new_img_width - rotated.rect.x;
        }
        if (rotated.rect.y + rotated.rect.height > new_img_height) {
            rotated.rect.height = new_img_height - rotated.rect.y;
        }

        // ȷ��������Ч
        if (rotated.rect.width > 0 && rotated.rect.height > 0) {
            rotatedInfos.push_back(rotated);
        }
    }
    return rotatedInfos;
}

/*
    ˳ʱ����ת
*/
// ˳ʱ����ת90�ȵľ�����ת����
cv::Rect rotateRectCW90(const cv::Rect& rect, int width, int height)
{
    // ˳ʱ��90����ת��ʽ
    int new_x = height - rect.y - rect.height;
    int new_y = rect.x;
    int new_width = rect.height;
    int new_height = rect.width;

    return cv::Rect(new_x, new_y, new_width, new_height);
}

// ˳ʱ����ת90�ȵļ������ת����
std::vector<drawInformation> Compute::rotateDrawInfoCW90(
    const std::vector<drawInformation>& drawInfos,
    int width, int height)
{
    std::vector<drawInformation> rotatedInfos;
    for (const auto& info : drawInfos) {
        drawInformation rotated = info;
        // ��ת����
        rotated.rect = rotateRectCW90(rotated.rect, width, height);

        // ���ӱ߽��飬ȷ����������ת��ͼ��Χ
        int new_img_width = height;  // ��ת��ͼ����� = ԭ�߶�
        int new_img_height = width;  // ��ת��ͼ��߶� = ԭ����

        if (rotated.rect.x < 0) rotated.rect.x = 0;
        if (rotated.rect.y < 0) rotated.rect.y = 0;
        if (rotated.rect.x + rotated.rect.width > new_img_width) {
            rotated.rect.width = new_img_width - rotated.rect.x;
        }
        if (rotated.rect.y + rotated.rect.height > new_img_height) {
            rotated.rect.height = new_img_height - rotated.rect.y;
        }

        // ȷ��������Ч
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
        // �����¶��󣨸����������ݣ�
        drawInformation transformed = info;

        // ������ת��Ϊ�����ROI������ϵ
        transformed.rect.x -= roiRect.x;
        transformed.rect.y -= roiRect.y;

        // ȷ��ת����ľ�����ROI��Χ��
        if (transformed.rect.x < 0) {
            //transformed.rect.width += transformed.rect.x; // ��������
            //transformed.rect.x = 0;

            continue;
        }
        if (transformed.rect.y < 0) {
            //transformed.rect.height += transformed.rect.y; // �����߶�
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

        // ���������Ȼ��Ч����
        if (transformed.rect.width > 0 && transformed.rect.height > 0) {
            roiDrawInfo.push_back(transformed);
        }
    }

    return roiDrawInfo;
}




//int Compute::getGlassRoi(cv::Mat& inputGaryImg, cv::Rect& GlassRect, int threshold) {
//
//    // ��ֵ����
//    cv::Mat binaryImage;
//    cv::threshold(inputGaryImg, binaryImage, threshold, 255, cv::THRESH_BINARY);
//
//    // �ҵ����а�ɫ���������
//    std::vector<std::vector<cv::Point>> contours;
//    std::vector<cv::Vec4i> hierarchy;
//    cv::findContours(binaryImage, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
//
//    // �ҵ����İ�ɫ����
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
//    // ��������ɫ����������չָ�����صľ���
//    GlassRect = cv::boundingRect(contours[maxIndex]);
//
//    return 0;
//}


int Compute::getGlassRoi(cv::Mat& inputGaryImg, cv::Rect& GlassRect, int threshold) {
    // ����1: �������ͼ���Ƿ���Ч
    if (inputGaryImg.empty()) {
        std::cout << "Input image is empty" << std::endl;
        return -1;
    }

    // ����2: ���ͼ���Ƿ�Ϊ��ͨ���Ҷ�ͼ
    if (inputGaryImg.channels() != 1) {
        std::cout << "Input image must be single channel grayscale image" << std::endl;
        return -1;
    }

    // ����3: ��֤��ֵ��Χ
    if (threshold < 0 || threshold > 255) {
        std::cout << "Threshold value " << threshold << " is out of range [0, 255]" << std::endl;
        return -1;
    }

    // ��ֵ����
    cv::Mat binaryImage;
    try {
        cv::threshold(inputGaryImg, binaryImage, threshold, 255, cv::THRESH_BINARY);
    }
    catch (const cv::Exception& e) {
        std::cout << "Threshold operation failed: " << e.what() << std::endl;
        return -1;
    }

    // �ҵ����а�ɫ���������
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    try {
        cv::findContours(binaryImage, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    }
    catch (const cv::Exception& e) {
        std::cout << "Find contours failed: " << e.what() << std::endl;
        return -1;
    }

    // ����4: ����Ƿ��ҵ�����
    if (contours.empty()) {
        //cv::imwrite("binary_image.png", binaryImage);
        //cv::imwrite("inputGaryImg.png", inputGaryImg);
        std::cout << "No contours found" << std::endl;
        return -1;
    }

    // �ҵ����İ�ɫ����
    int maxArea = 0;
    int maxIndex = -1;

    try {
        for (size_t i = 0; i < contours.size(); i++) {
            // ����5: ��������Ƿ���Ч
            if (contours[i].empty()) {
                continue; // ����������
            }

            double area = cv::contourArea(contours[i]);
            // ����6: �����������Ƿ���Ч
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

    // ����7: ���ѡ�е������Ƿ���Ч
    if (maxIndex < 0 || maxIndex >= static_cast<int>(contours.size()) || contours[maxIndex].empty()) {
        std::cout << "Invalid maxIndex or empty contour selected" << std::endl;
        return -1;
    }

    try {
        // ��������ɫ�������Ӿ���
        GlassRect = cv::boundingRect(contours[maxIndex]);

        // ����8: ������ɵľ����Ƿ���Ч
        if (GlassRect.width <= 0 || GlassRect.height <= 0) {
            std::cout << "Invalid bounding rectangle generated" << std::endl;
            return -1;
        }

        // ����9: �������Ƿ���ͼ��Χ��
        if (GlassRect.x < 0 || GlassRect.y < 0 ||
            GlassRect.x + GlassRect.width > inputGaryImg.cols ||
            GlassRect.y + GlassRect.height > inputGaryImg.rows) {
            std::cout << "Generated rectangle is outside image boundaries" << std::endl;
            // �������ѡ���������ζ�����ֱ�ӷ��ش���
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
    // �������
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

    // ����ͼ���󣬶�����0.1���²���
    cv::Size smallSize(
        static_cast<int>(inputGaryImg.cols * scale),
        static_cast<int>(inputGaryImg.rows * scale)
    );

    // ȷ����С�ߴ�
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

    // ���²���ͼ���Ͻ�����ֵ����
    cv::Mat binaryImage;
    try {
        cv::threshold(smallImg, binaryImage, threshold, 255, cv::THRESH_BINARY);
    }
    catch (const cv::Exception& e) {
        std::cout << "Threshold operation failed: " << e.what() << std::endl;
        return -1;
    }

    // �ҵ����а�ɫ���������
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    try {
        cv::findContours(binaryImage, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    }
    catch (const cv::Exception& e) {
        std::cout << "Find contours failed: " << e.what() << std::endl;
        return -1;
    }

    // ����Ƿ��ҵ�����
    if (contours.empty()) {
        std::cout << "No contours found" << std::endl;
        return -1;
    }

    // �ҵ����İ�ɫ����
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

    // ���ѡ�е������Ƿ���Ч
    if (maxIndex < 0 || maxIndex >= static_cast<int>(contours.size()) || contours[maxIndex].empty()) {
        std::cout << "Invalid maxIndex or empty contour selected" << std::endl;
        return -1;
    }

    try {
        // ��������ɫ�������Ӿ��Σ����²���ͼ���ϣ�
        cv::Rect smallRect = cv::boundingRect(contours[maxIndex]);

        // ������ӳ���ԭͼ�ߴ�
        double invScale = 1.0 / scale;
        GlassRect.x = static_cast<int>(smallRect.x * invScale);
        GlassRect.y = static_cast<int>(smallRect.y * invScale);
        GlassRect.width = static_cast<int>(smallRect.width * invScale);
        GlassRect.height = static_cast<int>(smallRect.height * invScale);

        // ������ɵľ����Ƿ���Ч
        if (GlassRect.width <= 0 || GlassRect.height <= 0) {
            std::cout << "Invalid bounding rectangle generated" << std::endl;
            return -1;
        }

        // ȷ��������ͼ��Χ��
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
// �ϲ�ȱ�ݼ������ԭʼ��ͼ�ĺ���
std::vector<drawInformation> Compute::mergeDefectsToLargeImage(
    const std::vector<std::vector<drawInformation>>& batchDrawInfo,
    int rowHeight = 256) {

    std::vector<drawInformation> mergedDefects;

    // ����ÿ�����εļ����
    for (int batchIndex = 0; batchIndex < batchDrawInfo.size(); ++batchIndex) {
        const auto& batch = batchDrawInfo[batchIndex];
        int yOffset = batchIndex * rowHeight;  // ���㵱ǰ�����ڴ�ͼ�е�Yƫ����

        // ������ǰ�����е�ÿ��ȱ��
        for (const auto& defect : batch) {
            drawInformation mergedDefect = defect;

            // �����������굽ԭʼ��ͼ��λ��
            mergedDefect.rect.y += yOffset;
            mergedDefect.realRect.y += yOffset;  // ����realRectҲ��Ҫͬ���ĵ���

            mergedDefects.push_back(mergedDefect);
        }
    }

    return mergedDefects;
}

// ����İ汾��ʹ��ֱ�����ط��ʣ�����ͼ���ǵ�ͨ���Ҷ�ͼ��
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
                // ������ǵ�ͨ��8λ����������ת��������ѡ�������Ա�֤���ܺͰ�ȫ��
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
/// ���ݺ���Ҷ�ֵ�仯����ͼ���ɸ
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
        std::vector<int> localIndices;  // �̱߳��ش洢

        for (int j = range.start; j < range.end; ++j) {
            int i = glassIndices[j];  // ��ȡ���������ԭʼ����

            int gridY = i / imgIndexs[0].size();
            int gridX = i % imgIndexs[0].size();
            cv::Rect roi(gridX * tileWidth, gridY * tileHeight, mats[i].cols, mats[i].rows);

            if (isDefectiveTileHorizontal(mats[i], range_val)) { // ����ָ����ֵ��Ϊ���ж�ȱ������
                localIndices.push_back(i);  // ��¼ԭʼ����
            }
        }

        // �������Ӽ�⵽��������������������
        if (!localIndices.empty()) {
            std::lock_guard<std::mutex> lock(mtx);
            detectIndexs.insert(detectIndexs.end(), localIndices.begin(), localIndices.end());
        }
        });

    // �����ռ�������ǳ������
    std::vector<cv::Mat> detectRects;
    for (int idx : detectIndexs) {
        detectRects.emplace_back(mats[idx]);  // ǳ����
    }

    return { detectRects, detectIndexs };
}

/// <summary>
/// ��������Ҷ�ֵ�仯����ͼ���ɸ
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
//std::vector<drawInformation> Compute::ComputeProcess( cv::Mat& largeImg, 
//    PrescriptionParameter& parameter,
//    const double& glassThreshold_val, const double& range_val,
//    int tileHeight = 256,int tileWidth = 256)
//
//{
//    double threshold_1, threshold_2;
//    threshold_1 = parameter.thresholdDetect_1;
//    threshold_2 = parameter.thresholdDetect_2;
//    std::vector<double> thresholdsCls = parameter.thresholdsCls;
//
//
//    std::vector<cv::Mat>            mats;
//    std::vector<std::vector<int>>   imgIndexs;
//    std::vector<drawInformation>    saveInformations;
//
//    if (largeImg.channels() == 3) {
//        cv::cvtColor(largeImg, largeImg, cv::COLOR_BGR2GRAY);
//    }
//    try
//    {
//        divideImageShallow(largeImg, tileWidth, tileHeight, mats, imgIndexs);
//    }
//    catch (const std::exception& e)
//    {
//        FILE_LOG_ERROR("[ComputeProcess] ComputerThread Error: divideImageShallow failed!");
//        return saveInformations;
//    }
//    std::vector<int>  glassIndices;
//    try
//    {
//        glassIndices = findGlassTilesFast(mats, glassThreshold_val);
//    }
//    catch (const std::exception&)
//    {
//        FILE_LOG_ERROR("[ComputeProcess] ComputerThread Error: findGlassTilesFast failed!");
//
//        return saveInformations;
//    }
//
//    // �Ҷ�ֵ�仯��ɸ - ֻ������������
//    auto [detectRects, detectIndexs] = detectDefectiveGlassTilesVertical(
//        mats, glassIndices, imgIndexs, tileWidth, tileHeight, range_val);
//
//    //cv::Mat resultImage = drawGlassRegions(largeImg, detectIndexs, tileWidth, tileHeight, cv::Scalar(0, 255, 0), 3);
//
//
//    //1.�ּ��batch
//    std::vector<int> indexs_Cu;//��ԭʼͼ�е�����
//    std::vector<int> indexs_Cu_;//�ڴ�ɸ�б�detectRects�е�����
//    std::vector<deploy::DetectRes> Informations_Cu;
//
//	//auto model_Clone = (*model).clone(); 
//    process_batch_images(mats, detectIndexs, (*model), indexs_Cu_, Informations_Cu, threshold_1, 1);
//
//    //����ɸ���ת��Ϊԭʼͼ����е�����
//    //if (indexs_Cu_.size() == 0) {
//    //    return resultImage;
//    //}
//    //for (int i = 0; i < indexs_Cu_.size(); i++) {
//    //    indexs_Cu.push_back(detectIndexs[indexs_Cu_[i]]);
//    //}
//
//    //2.�����batch
//    std::vector<int> indexs_Jing;
//    std::vector<deploy::DetectRes> Informations_Jing;
//    //process_batch_imagesByIndex(mats, indexs_Cu, (*model2), indexs_Jing, Informations_Jing, threshold_2);
//
//
//    for (int i = 0; i < indexs_Cu_.size(); i++) {
//        deploy::DetectRes result = Informations_Cu[i];
//        if (result.num > 0) {
//            //std::cout << "mats[indexs_Cu[i]]:" << indexs_Cu_[i] << std::endl;
//            cv::Mat img = mats[indexs_Cu_[i]].clone();
//            for (int j = 0; j < result.num; j++) {
//                if (result.scores[j] < threshold_2) {
//
//                    continue;
//                }
//
//                DefectType TypeValue;
//                std::string typeName = "";
//                if (result.classes[j] == DefectType::TYPE_POORCOATING) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_POORCOATING;
//                }
//                else if (result.classes[j] == DefectType::TYPE_SCRATCH) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_SCRATCH;
//                    
//                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[1])) {
//                        TypeValue = DefectType::TYPE_SMUDGE;
//                    }
//
//                    //FILE_LOG_DEBUG("[ComputeProcess] ComputerThread Error: Scratch Detect! %f ,", result.scores[j], static_cast<float>(parameter.thresholdsCls[1]));
//                }
//                else if (result.classes[j] == DefectType::TYPE_CALCULUS) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_CALCULUS;
//
//                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[2])) {
//                        TypeValue = DefectType::TYPE_SMUDGE;
//                    }
//                    //FILE_LOG_DEBUG("[ComputeProcess] ComputerThread Error: Scratch Detect! %f ,", result.scores[j], static_cast<float>(parameter.thresholdsCls[2]));
//                }
//                else if (result.classes[j] == DefectType::TYPE_BUBBLE) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_BUBBLE;
//                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[3])) {
//                        TypeValue = DefectType::TYPE_SMUDGE;
//                    }
//                    //FILE_LOG_DEBUG("[ComputeProcess] ComputerThread Error: Scratch Detect! %f ,", result.scores[j], static_cast<float>(parameter.thresholdsCls[3]));
//                }
//                else if (result.classes[j] == DefectType::TYPE_TRADEMARK) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_TRADEMARK;
//                    //TypeValue = DefectType::TYPE_SMUDGE;//20251123 Ϊ����ڵ�����Ϊ�̱꣬�������̱��Ϊ����
//                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[4])) {
//                        TypeValue = DefectType::TYPE_SMUDGE;
//                    }
//                    //FILE_LOG_DEBUG("[ComputeProcess] ComputerThread Error: Scratch Detect! %f ,", result.scores[j], static_cast<float>(parameter.thresholdsCls[4]));
//                }
//                else if (result.classes[j] == DefectType::TYPE_WATERSTAIN) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_WATERSTAIN;
//                    if (result.scores[j] < static_cast<float>(parameter.thresholdsCls[5])) {
//                        TypeValue = DefectType::TYPE_SMUDGE;
//                    }
//                    //FILE_LOG_DEBUG("[ComputeProcess] ComputerThread Error: Scratch Detect! %f ,", result.scores[j], static_cast<float>(parameter.thresholdsCls[5]));
//                }
//                else if (result.classes[j] == DefectType::TYPE_SMUDGE) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_SMUDGE;
//
//                }
//                else if (result.classes[j] == DefectType::TYPE_SCREENPRINTING) {//20251122 �������
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_SCREENPRINTING;
//
//                }
//                else {
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_SMUDGE;
//                }
//                cv::Mat RoiSegImg;
//                float left = result.boxes[j].left;
//                float top = result.boxes[j].top;
//                float right = result.boxes[j].right;
//                float bottom = result.boxes[j].bottom;
//
//                int width = static_cast<int>(right - left);
//                int height = static_cast<int>(bottom - top);
//
//                cv::Rect rect(
//                    std::max(0, static_cast<int>(std::floor(left))),
//                    std::max(0, static_cast<int>(std::floor(top))),
//                    std::max(0, static_cast<int>(std::ceil(right - left))),
//                    std::max(0, static_cast<int>(std::ceil(bottom - top)))
//                );
//
//                // clamp x/y within image
//                if (rect.x >= img.cols) continue;
//                if (rect.y >= img.rows) continue;
//
//                if (rect.x < 0) rect.x = 0;
//                if (rect.y < 0) rect.y = 0;
//
//                if (rect.x + rect.width > img.cols) rect.width = img.cols - rect.x;
//                if (rect.y + rect.height > img.rows) rect.height = img.rows - rect.y;
//
//                // �����Ƿ���С
//                if (rect.width <= 0 || rect.height <= 0) continue;
//
//                std::vector<float> errorInfo;
//                ConnectedComponentItem analysisInfo;
//                DefectLevel levelType = levelUtils.defectLevelProcessByEdge(img, rect, TypeValue, analysisInfo, errorInfo);
//                if (levelType == ABNORMAL) {//��ȱ�ݹ�С С����С��ֵ
//                    continue;
//                }
//                if (levelType == AREAERROR) {//δ�ָ�ɹ�����£��������⴦��
//
//
//                    levelType = levelUtils.defectLevelProcessSingle(img, rect, TypeValue, analysisInfo, errorInfo);
//
//                }
//                else {
//
//                    //�����ж�
//                    //�����ۺ����ݽ����ж�
//                    if (TypeValue == DefectType::TYPE_SMUDGE || TypeValue == DefectType::TYPE_BUBBLE) {
//                        if (analysisInfo.numComponent == 2 || analysisInfo.numComponent == 3) {//��һ����
//
//                            levelUtils.secondDetect(img, rect, analysisInfo, TypeValue, levelType, errorInfo);
//                        }
//
//                    }
//                }
//
//                levelUtils.DoubleChangeErrorType(img, rect, analysisInfo, TypeValue, levelType, errorInfo, result.scores[j]);
//
//                drawInformation saveInformation;
//                saveInformation.rect.x = indexs_Cu_[i] % imgIndexs[0].size() * img.cols + rect.x;
//                saveInformation.rect.y = indexs_Cu_[i] / imgIndexs[0].size() * img.rows + rect.y;
//                saveInformation.rect.width = rect.width;
//                saveInformation.rect.height = rect.height;
//                saveInformation.ErrorType = levelType;
//                saveInformation.DefectType = TypeValue;
//                saveInformation.Errorinfo = errorInfo;
//                saveInformation.confidence = result.scores[j];
//                saveInformations.push_back(saveInformation);
//            }
//        }
//
//    }
//
//    //���ݲ����Եȼ���������
//    std::vector<drawInformation>  resultInformations = levelUtils.GetDefectLevel(saveInformations,parameter);
//
//  
//    return resultInformations;
//}



cv::Mat Compute::ComputeProcessDraw(cv::Mat& largeImg,
    PrescriptionParameter& parameter,
    const double& glassThreshold_val, const double& range_val,
    int tileHeight = 256, int tileWidth = 256)
{
    cv::Mat resultImage;        //�޷������㷨ʱ����ʱ������
	return resultImage;				   
}
/*=============================��batch�汾+��ɸ�Ż�+��������ȷ��===============================================*/


/*================================20251201  ��batch�汾+��ɸ�Ż�+��������ȷ��+��ͨ��ͼ��===================================================*/

/// <summary>
/// // �ϲ���������������ȥ��
/// ��Ҫ���ڶԲ�ͬ��Դ��ɸ��Ľ�����кϲ�
/// </summary>
/// <param name="indices1"></param>
/// <param name="indices2"></param>
/// <returns></returns>
std::vector<int> mergeAndDeduplicateIndices(const std::vector<int>& indices1,
    const std::vector<int>& indices2) {
    std::set<int> uniqueIndices; // ʹ��set�Զ�ȥ�غ�����

    // �����һ������������Ԫ��
    uniqueIndices.insert(indices1.begin(), indices1.end());

    // ����ڶ�������������Ԫ��
    uniqueIndices.insert(indices2.begin(), indices2.end());

    // ת����vector
    std::vector<int> result(uniqueIndices.begin(), uniqueIndices.end());

    return result;
}

/// <summary>
/// ��ͨ���ںϷ���
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
//void process_batch_imagesChannel(const std::vector<cv::Mat>& imageChannel0, const std::vector<cv::Mat>& imageChannel1, const std::vector<cv::Mat>& imageChannel2,
//    const std::vector<int>& indices, deploy::DetectModel& model,
//    std::vector<int>& errIndexs, std::vector<deploy::DetectRes>& Informations, double threshold, int flag = 0) {
//
//    errIndexs.clear();
//    Informations.clear();
//    const int batch_size = model.batch_size();
//
//    for (int i = 0; i < indices.size(); i += batch_size) {
//        std::vector<cv::Mat> images;
//        std::vector<deploy::Image> img_batch;
//        std::vector<int> img_Index_batch;
//
//        // ����������������
//        for (int j = i; j < i + batch_size && j < indices.size(); ++j) {
//            int img_index = indices[j];
//            if (img_index >= 0 && img_index < imageChannel0.size()) {
//
//                //cv::cvtColor(imageAll[img_index], image, cv::COLOR_GRAY2RGB);
//                //�Ҷ�ͼתRGB�Ķ�ͨ���ϲ�
//                cv::Mat image;
//                std::vector<cv::Mat> channels;
//                //͸ ͸ ��
//                channels.push_back(imageChannel0[img_index]);
//                channels.push_back(imageChannel0[img_index]);
//                channels.push_back(imageChannel2[img_index]);
//                cv::merge(channels, image);
//
//                images.push_back(image);
//                img_batch.emplace_back(image.data, image.cols, image.rows);
//                img_Index_batch.push_back(img_index);
//            }
//        }
//
//        auto results = model.predict(img_batch);
//
//        if (flag == 0) { // �ּ�⣺ֻ������ȱ�ݵ�ͼ������
//            for (int n = 0; n < results.size(); n++) {
//                if (results[n].num > 0) {
//                    bool has_defect = false;
//                    for (int m = 0; m < results[n].num; m++) {
//                        if (results[n].scores[m] > threshold) {
//                            has_defect = true;
//                            break;
//                        }
//                    }
//                    if (has_defect) {
//                        errIndexs.push_back(img_Index_batch[n]);
//                    }
//                }
//            }
//        }
//        else { // ��ȷ��⣺���������ͼ����
//            for (int n = 0; n < results.size(); n++) {
//                if (results[n].num > 0) {
//                    bool has_defect = false;
//                    for (int m = 0; m < results[n].num; m++) {
//                        if (results[n].scores[m] > threshold) {
//                            has_defect = true;
//                            break;
//                        }
//                    }
//                    if (has_defect) {
//                        errIndexs.push_back(img_Index_batch[n]);
//                        Informations.push_back(results[n]);
//                    }
//                }
//            }
//        }
//    }
//}
//

void Compute::process_batch_m_imagesChannel(
    const std::vector<cv::Mat>& imageChannel0,
    const std::vector<cv::Mat>& imageChannel1,
    const std::vector<cv::Mat>& imageChannel2,
    const std::vector<int>& indices,
    std::vector<int>& errIndexs,
    std::vector<std::vector<drawInformation>>& Informations,
    double threshold,
    int flag = 0) {

    errIndexs.clear();
    Informations.clear();

    const int batch_size = MAX_BATCH_SIZE;

    for (int i = 0; i < indices.size(); i += batch_size) {
        std::vector<cv::Mat> images;
        std::vector<int> batch_img_indices;  // �洢ʵ�ʵ�ͼ������

        // ����������������
        for (int j = i; j < i + batch_size && j < indices.size(); ++j) {
            int img_index = indices[j];
            if (img_index >= 0 && img_index < imageChannel0.size()) {
                // ������ͨ��ͼ��͸-͸-��
                cv::Mat image;
                std::vector<cv::Mat> channels;
                channels.push_back(imageChannel0[img_index]);
                channels.push_back(imageChannel0[img_index]);
                channels.push_back(imageChannel1[img_index]);
                cv::merge(channels, image);

                images.push_back(image);
                batch_img_indices.push_back(img_index);  // ����ʵ��ͼ������
            }
        }

        std::vector<std::vector<Object>> results;
        m_detector.detect(images, results);

        if (flag == 0) {
            // �ּ��ģʽ��ֻ������ȱ�ݵ�ͼ������
            for (int n = 0; n < results.size(); n++) {
                bool has_defect = false;
                for (int m = 0; m < results[n].size(); m++) {
                    if (results[n][m].confidence > threshold) {
                        has_defect = true;
                        break;
                    }
                }
                if (has_defect) {
                    errIndexs.push_back(batch_img_indices[n]);
                }
            }
        }
        else {
            // ��ȷ���ģʽ�����������ͼ����
            for (int n = 0; n < results.size(); n++) {
                std::vector<drawInformation> img_draw_infos;
                bool has_defect = false;

                for (int m = 0; m < results[n].size(); m++) {
                    Object& obj = results[n][m];
                    if (obj.confidence > threshold) {
                        has_defect = true;

                        // ת��Object��drawInformation
                        drawInformation draw_info;
                        draw_info.DefectId = m;
                        draw_info.confidence = obj.confidence;

                        // ע�⣺ȷ��obj.box�������ʽ
                        int x = static_cast<int>(obj.box.x);
                        int y = static_cast<int>(obj.box.y);
                        int width = static_cast<int>(obj.box.width);
                        int height = static_cast<int>(obj.box.height);

                        draw_info.rect = cv::Rect(x, y, width, height);
                        draw_info.realRect = cv::Rect(x, y, width, height); // ����ʵ������޸�
                        draw_info.DefectType = static_cast<DefectType>(obj.label);
                        draw_info.ErrorType = NORMAL;
                        draw_info.camera_id = 0;

                        img_draw_infos.push_back(draw_info);
                    }
                }

                if (has_defect) {
                    errIndexs.push_back(batch_img_indices[n]);
                    Informations.push_back(img_draw_infos);
                }
            }
        }
    }
}

/// <summary>
/// �Լ���дtensorRT����������
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

    //FILE_LOG_INFO("confidence=%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f", 
    //    parameter.thresholdsCls);

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

    //ȷ���������������
    std::vector<int>  glassIndices = findGlassTilesFast(mats2, glassThreshold_val);


    // �Ҷ�ֵ�仯��ɸ - ֻ������������
    // ͸���Ҷȴ�ɸ
    auto [detectRects0, detectIndexs0] = detectDefectiveGlassTilesVertical(
        mats0, glassIndices, imgIndexs, tileWidth, tileHeight, range_val);
    //�����Ҷȴ�ɸ
    auto [detectRects, detectIndexs] = detectDefectiveGlassTilesVertical(
        mats2, glassIndices, imgIndexs, tileWidth, tileHeight, range_val);

    //�ϲ����Դ��ɸ���
    std::vector<int> mergeIndexs = mergeAndDeduplicateIndices(detectIndexs0, detectIndexs);

    //1.�ּ��batch
    std::vector<int> indexs_Cu;//��ԭʼͼ�е�����
    std::vector<int> indexs_Cu_;//�ڴ�ɸ�б�detectRects�е�����
    std::vector<std::vector<drawInformation>> Informations_Cu;
    process_batch_m_imagesChannel(mats0, mats1, mats2, mergeIndexs, indexs_Cu_, Informations_Cu, threshold_1, 1);

    //2.�����batch

    for (int i = 0; i < indexs_Cu_.size(); i++) {
        std::vector<drawInformation> result = Informations_Cu[i];
        if (result.size() > 0) {
            cv::Mat img = mats0[indexs_Cu_[i]].clone();//͸���ָ����ȼ�
            //cv::Mat img = mats2[indexs_Cu_[i]].clone();//�����ָ����ȼ�
            for (int j = 0; j < result.size(); j++) {
                if (result[j].confidence < threshold_2) {

                    continue;
                }

                DefectType TypeValue;
                std::string typeName = "";
                if (result[j].DefectType == DefectType::TYPE_POORCOATING) {//0

                    typeName = " ";
                    TypeValue = DefectType::TYPE_POORCOATING;
                    if (result[j].confidence < static_cast<float>(parameter.thresholdsCls[0])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }
                }
                else if (result[j].DefectType == DefectType::TYPE_SCRATCH) {//1

                    typeName = " ";
                    TypeValue = DefectType::TYPE_SCRATCH;
                    if (result[j].confidence < static_cast<float>(parameter.thresholdsCls[1])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }
                }
                else if (result[j].DefectType == DefectType::TYPE_CALCULUS) {//2

                    typeName = " ";
                    TypeValue = DefectType::TYPE_CALCULUS;
                    if (result[j].confidence < static_cast<float>(parameter.thresholdsCls[2])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }
                }
                else if (result[j].DefectType == DefectType::TYPE_BUBBLE) {//3

                    typeName = " ";
                    TypeValue = DefectType::TYPE_BUBBLE;
                    if (result[j].confidence < static_cast<float>(parameter.thresholdsCls[3])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result[j].DefectType == DefectType::TYPE_TRADEMARK) {//4

                    typeName = " ";
                    TypeValue = DefectType::TYPE_TRADEMARK;
                    if (result[j].confidence < static_cast<float>(parameter.thresholdsCls[4])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result[j].DefectType == DefectType::TYPE_WATERSTAIN) {//5

                    typeName = " ";
                    TypeValue = DefectType::TYPE_WATERSTAIN;
                    if (result[j].confidence < static_cast<float>(parameter.thresholdsCls[5])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result[j].DefectType == DefectType::TYPE_SMUDGE) {//6

                    typeName = " ";
                    TypeValue = DefectType::TYPE_SMUDGE;

                }
                else if (result[j].DefectType == DefectType::TYPE_SCREENPRINTING) {//7

                    typeName = " ";
                    TypeValue = DefectType::TYPE_SCREENPRINTING;
                    if (result[j].confidence < static_cast<float>(parameter.thresholdsCls[7])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result[j].DefectType == DefectType::TYPE_CHIPPED_EDGE) {//8

                    typeName = " ";
                    TypeValue = DefectType::TYPE_CHIPPED_EDGE;
                    if (result[j].confidence < static_cast<float>(parameter.thresholdsCls[8])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result[j].DefectType == DefectType::TYPE_PITTING) {//9

                    typeName = " ";
                    TypeValue = DefectType::TYPE_PITTING;
                    if (result[j].confidence < static_cast<float>(parameter.thresholdsCls[9])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result[j].DefectType == DefectType::TYPE_GLASS_CULLET) {//10

                    typeName = " ";
                    TypeValue = DefectType::TYPE_GLASS_CULLET;
                    if (result[j].confidence < static_cast<float>(parameter.thresholdsCls[10])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result[j].DefectType == DefectType::TYPE_WAVINESS) {//11

                    typeName = " ";
                    TypeValue = DefectType::TYPE_WAVINESS;
                    if (result[j].confidence < static_cast<float>(parameter.thresholdsCls[11])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else if (result[j].DefectType == DefectType::TYPE_OTHER) {//12

                    typeName = " ";
                    TypeValue = DefectType::TYPE_OTHER;
                    if (result[j].confidence < static_cast<float>(parameter.thresholdsCls[12])) {
                        TypeValue = DefectType::TYPE_SMUDGE;
                    }

                }
                else {
                    typeName = " ";
                    TypeValue = DefectType::TYPE_SMUDGE;
                }

                cv::Mat RoiSegImg;
   
                cv::Rect rect= result[j].rect;

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
                ConnectedComponentItem analysisInfo;
                DefectLevel levelType = levelUtils.defectLevelProcessByEdge(img, rect, TypeValue, analysisInfo, errorInfo);
                if (levelType == ABNORMAL) {//��ȱ�ݹ�С С����С��ֵ
                    continue;
                }
                if (levelType == AREAERROR) {//δ�ָ�ɹ�����£��������⴦��
                    levelType = levelUtils.defectLevelProcessSingle(img, rect, TypeValue, analysisInfo, errorInfo);
                }
                else { 

                }
                
                //�����жϣ��˴�Ϊ��������жϣ�ͨ�ð����迪��
                //cv::Mat imgDouble = mats2[indexs_Cu_[i]].clone();//���������ж�,��Ϊ��Ĥ�������������ϱ���Ϊ��ɫ
                //levelUtils.DoubleChangeErrorType(imgDouble, rect, analysisInfo, TypeValue, levelType, errorInfo, result[j].confidence);

                drawInformation saveInformation;
                saveInformation.rect.x = indexs_Cu_[i] % imgIndexs[0].size() * img.cols + rect.x;
                saveInformation.rect.y = indexs_Cu_[i] / imgIndexs[0].size() * img.rows + rect.y;
                saveInformation.rect.width = rect.width;
                saveInformation.rect.height = rect.height;
                saveInformation.ErrorType = levelType;
                saveInformation.DefectType = TypeValue;
                saveInformation.Errorinfo = errorInfo;
                saveInformation.confidence = result[j].confidence;
                saveInformations.push_back(saveInformation);
            }
        }

    }

    //���ݲ����Եȼ���������
    std::vector<drawInformation>  resultInformations = Tuning::GetDefectLevel(saveInformations, parameter);

    return resultInformations;
}
/*================================��batch�汾+��ɸ�Ż�+��������ȷ��+��ͨ��ͼ��===================================================*/

/*================================����������㣬�����ֳ�ʵ�������д===================================================*/
/*
    �Ϸ��ֳ������罺����Ե���ڽ�ӡ�����±�Ե������Ҫ�����ļ�ⷽʽ

    ʵ�ֹ��ܣ�������ͼ�ܱ�һ�������ڵĽ��е�����⣬Ŀǰ����Ϊ256*2�ľ���
*/

/// <summary>
/// ��ȡԭͼ�����п�����Ե��һ���������������ߵ����л�����������
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

    // ���㵱ǰ�����ţ�����imgIndexs�洢������������
    for (int y = 0; y < numRows; ++y) {
        for (int x = 0; x < numCols; ++x) {
            int tileIndex = y * numCols + x;

            // �ж��Ƿ��ڱ߽總�����������borderWidthTiles�л��������borderWidthTiles��
            // ��������ߵ�borderWidthTiles�л����ұߵ�borderWidthTiles��
            if (y < borderWidthTiles || y >= numRows - borderWidthTiles ||
                x < borderWidthTiles || x >= numCols - borderWidthTiles) {
                borderIndices.push_back(tileIndex);
            }
        }
    }

    return borderIndices;
}

/// <summary>
/// ����ֳ�ʹ�õľֲ�������
/// </summary>
/// <param name="largeImgs"></param>
/// <param name="parameter"></param>
/// <param name="glassThreshold_val"></param>
/// <param name="range_val"></param>
/// <param name="tileHeight"></param>
/// <param name="tileWidth"></param>
/// <returns></returns>
//std::vector<drawInformation> Compute::ComputeProcessPart(std::vector<cv::Mat>& largeImgs,
//    PrescriptionParameter& parameter,
//    const double& glassThreshold_val, const double& range_val,
//    int tileHeight = 256, int tileWidth = 256) {
//
//    double threshold_1, threshold_2;
//    threshold_1 = parameter.thresholdDetect_1;
//    threshold_2 = parameter.thresholdDetect_2;
//    std::vector<double> thresholdsCls = parameter.thresholdsCls;
//
//
//    std::vector<cv::Mat>            mats0, mats1, mats2;
//    std::vector<std::vector<int>>   imgIndexs;
//    std::vector<drawInformation>    saveInformations;
//
//    if (largeImgs[0].channels() == 3) {
//        cv::cvtColor(largeImgs[0], largeImgs[0], cv::COLOR_BGR2GRAY);
//    }
//    if (largeImgs[1].channels() == 3) {
//        cv::cvtColor(largeImgs[1], largeImgs[1], cv::COLOR_BGR2GRAY);
//    }
//    if (largeImgs[2].channels() == 3) {
//        cv::cvtColor(largeImgs[2], largeImgs[2], cv::COLOR_BGR2GRAY);
//    }
//    divideImageShallow(largeImgs[0], tileWidth, tileHeight, mats0, imgIndexs);
//    divideImageShallow(largeImgs[1], tileWidth, tileHeight, mats1, imgIndexs);
//    divideImageShallow(largeImgs[2], tileWidth, tileHeight, mats2, imgIndexs);
//
//
//    //����ʵ�����󣬻�ȡָ���Ĵ�������
//    //std::vector<int>  glassIndices = findGlassTilesFast(mats2, glassThreshold_val);
//    std::vector<int>  glassIndices = getBorderTilesIndices(imgIndexs, tileWidth, tileHeight,2);
//
//    // �Ҷ�ֵ�仯��ɸ - ֻ������������
//    // ͸���Ҷȴ�ɸ
//    auto [detectRects0, detectIndexs0] = detectDefectiveGlassTilesVertical(
//        mats0, glassIndices, imgIndexs, tileWidth, tileHeight, range_val);
//    //�����Ҷȴ�ɸ
//    auto [detectRects, detectIndexs] = detectDefectiveGlassTilesVertical(
//        mats2, glassIndices, imgIndexs, tileWidth, tileHeight, range_val);
//
//    //�ϲ����Դ��ɸ���
//    std::vector<int> mergeIndexs = mergeAndDeduplicateIndices(detectIndexs0, detectIndexs);
//
//    //1.�ּ��batch
//    std::vector<int> indexs_Cu;//��ԭʼͼ�е�����
//    std::vector<int> indexs_Cu_;//�ڴ�ɸ�б�detectRects�е�����
//    std::vector<deploy::DetectRes> Informations_Cu;
//    process_batch_imagesChannel(mats0, mats1, mats2, mergeIndexs, (*model2), indexs_Cu_, Informations_Cu, threshold_1, 1);
//
//    //2.�����batch
//    std::vector<int> indexs_Jing;
//    std::vector<deploy::DetectRes> Informations_Jing;
//
//    for (int i = 0; i < indexs_Cu_.size(); i++) {
//        deploy::DetectRes result = Informations_Cu[i];
//        if (result.num > 0) {
//            cv::Mat img = mats2[indexs_Cu_[i]].clone();
//            for (int j = 0; j < result.num; j++) {
//                if (result.scores[j] < threshold_2) {
//
//                    continue;
//                }
//
//                DefectType TypeValue;
//                std::string typeName = "";
//                if (result.classes[j] == DefectType::TYPE_POORCOATING) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_POORCOATING;
//                }
//                else if (result.classes[j] == DefectType::TYPE_SCRATCH) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_SCRATCH;
//                }
//                else if (result.classes[j] == DefectType::TYPE_CALCULUS) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_CALCULUS;
//                }
//                else if (result.classes[j] == DefectType::TYPE_BUBBLE) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_BUBBLE;
//
//                }
//                else if (result.classes[j] == DefectType::TYPE_TRADEMARK) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_TRADEMARK;
//
//                }
//                else if (result.classes[j] == DefectType::TYPE_WATERSTAIN) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_WATERSTAIN;
//
//                }
//                else if (result.classes[j] == DefectType::TYPE_SMUDGE) {
//
//                    typeName = " ";
//                    TypeValue = DefectType::TYPE_SMUDGE;
//
//                }
//                cv::Mat RoiSegImg;
//                cv::Rect rect = cv::Rect(result.boxes[j].left, result.boxes[j].top,
//                    result.boxes[j].right - result.boxes[j].left,
//                    result.boxes[j].bottom - result.boxes[j].top)
//                    & cv::Rect(0, 0, img.cols, img.rows);
//
//                // ��ѡ���������Ƿ���Ч
//                if (rect.width <= 0 || rect.height <= 0) {
//                    // ������Ч���ε����
//                    continue; // ���������������
//                }
//                std::vector<float> errorInfo;
//                ConnectedComponentItem analysisInfo;
//                DefectLevel levelType = levelUtils.defectLevelProcessByEdge(img, rect, TypeValue, analysisInfo, errorInfo);
//                if (levelType == ABNORMAL) {//��ȱ�ݹ�С С����С��ֵ
//                    continue;
//                }
//                if (levelType == AREAERROR) {//δ�ָ�ɹ�����£��������⴦��
//
//
//                    levelType = levelUtils.defectLevelProcessSingle(img, rect, TypeValue, analysisInfo, errorInfo);
//
//                }
//                else {
//
//                    //�����ж�
//                    //�����ۺ����ݽ����ж�
//                    if (TypeValue == DefectType::TYPE_SMUDGE || TypeValue == DefectType::TYPE_BUBBLE) {
//                        if (analysisInfo.numComponent == 2 || analysisInfo.numComponent == 3) {//��һ����
//
//                            levelUtils.secondDetect(img, rect, analysisInfo, TypeValue, levelType, errorInfo);
//                        }
//
//                    }
//                }
//
//                levelUtils.DoubleChangeErrorType(img, rect, analysisInfo, TypeValue, levelType, errorInfo, result.scores[j]);
//
//                drawInformation saveInformation;
//                saveInformation.rect.x = indexs_Cu_[i] % imgIndexs[0].size() * img.cols + rect.x;
//                saveInformation.rect.y = indexs_Cu_[i] / imgIndexs[0].size() * img.rows + rect.y;
//                saveInformation.rect.width = rect.width;
//                saveInformation.rect.height = rect.height;
//                saveInformation.ErrorType = levelType;
//                saveInformation.DefectType = TypeValue;
//                saveInformation.Errorinfo = errorInfo;
//                saveInformation.confidence = result.scores[j];
//                saveInformations.push_back(saveInformation);
//            }
//        }
//    }
//
//    //���ݲ����Եȼ���������
//    std::vector<drawInformation>  resultInformations = levelUtils.GetDefectLevel(saveInformations, parameter);
//
//    return resultInformations;
//}

/*================================����������㣬�����ֳ�ʵ�������д===================================================*/

