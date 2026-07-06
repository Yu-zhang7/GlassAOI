#include "TypeAutoRegister.h"
#include <algorithm>

void Type0::ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs)
{
    // 计算四个角点
    cv::Point topLeft(rect.x, rect.y);
    cv::Point topRight(rect.x + rect.width, rect.y);
    cv::Point bottomLeft(rect.x, rect.y + rect.height);
    cv::Point bottomRight(rect.x + rect.width, rect.y + rect.height);

    std::vector<cv::Point> trianglePoints = { topLeft, topRight ,bottomRight,bottomLeft };

    //确保每个点都在图像边界内
    for (auto& point : trianglePoints) {
        point.x = std::max(0, std::min(point.x, cols - 1));
        point.y = std::max(0, std::min(point.y, rows - 1));
    }
    vertexs = trianglePoints;
}

void Type1::ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs)
{
    //计算外层矩形框
    double expandRatio = 0.2;
    // 计算扩充后的矩形的宽度和高度
    int expandedWidth = static_cast<int>(rect.width * expandRatio) + rect.width;
    int expandedHeight = static_cast<int>(rect.height * expandRatio) + rect.height;
    // 计算扩充后的矩形的中心点
    int centerX = rect.x + rect.width / 2;
    int centerY = rect.y + rect.height / 2;
    // 计算扩充后的矩形的左上角和右下角坐标
    int expandedX = centerX - expandedWidth / 2;
    int expandedY = centerY - expandedHeight / 2;
    int expandedWidth2 = expandedWidth;
    int expandedHeight2 = expandedHeight;

    // 确保扩充后的矩形在图像的有效范围内
    if (expandedX < 0) {
        expandedWidth2 += expandedX;
        expandedX = 0;
    }
    if (expandedY < 0) {
        expandedHeight2 += expandedY;
        expandedY = 0;
    }
    if (expandedX + expandedWidth2 > cols) {
        expandedWidth2 = cols - expandedX;
    }
    if (expandedY + expandedHeight2 > rows) {
        expandedHeight2 = rows - expandedY;
    }
    cv::Rect expandedRect(expandedX, expandedY, expandedWidth2, expandedHeight2);
    // 计算四个角点
    cv::Point topLeft(expandedRect.x, expandedRect.y);
    cv::Point topRight(expandedRect.x + expandedRect.width, expandedRect.y);
    cv::Point bottomLeft(expandedRect.x, expandedRect.y + expandedRect.height);
    cv::Point bottomRight(expandedRect.x + expandedRect.width, expandedRect.y + expandedRect.height);

    std::vector<cv::Point> trianglePoints = { topLeft, topRight ,bottomRight,bottomLeft };

    //确保每个点都在图像边界内
    for (auto& point : trianglePoints) {
        point.x = std::max(0, std::min(point.x, cols - 1));
        point.y = std::max(0, std::min(point.y, rows - 1));
    }
    vertexs = trianglePoints;
}

void Type2::ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs)
{
    float expandRatio = 1.5;//该值应大于1
    if (expandRatio < 1) {

        expandRatio = 1.1;

    }
    //cv::Rect rect = rects[i];
    cv::Point topLeft(rect.x, rect.y);
    cv::Point topRight(rect.x + rect.width, rect.y);
    cv::Point bottomLeft(rect.x, rect.y + rect.height);
    cv::Point bottomRight(rect.x + rect.width, rect.y + rect.height);

    // 计算梯形的底边宽度（这里是矩形宽度的1.5倍）
    int baseWidth = static_cast<int>(rect.width * expandRatio);

    // 计算梯形底边的顶点
    int offsetX = (baseWidth - rect.width) / 2;
    cv::Point baseLeft(rect.x - offsetX, rect.y + rect.height);
    cv::Point baseRight(rect.x + rect.width + offsetX, rect.y + rect.height);
    std::vector<cv::Point> trianglePoints = { topLeft, topRight ,baseRight,baseLeft };

    //确保每个点都在图像边界内
    for (auto& point : trianglePoints) {
        point.x = std::max(0, std::min(point.x, cols - 1));
        point.y = std::max(0, std::min(point.y, rows - 1));
    }
    vertexs = trianglePoints;
}

void Type3::ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs)
{
    //类别4 上箭头型
   //cv::Rect rect = rects[i];
    float expandRatio = 0.2;//该值应大于1
    int triangleHeight = static_cast<int>(rect.height * expandRatio);

    if (triangleHeight > rect.height) {

        triangleHeight = rect.height - 1;

    }
    // 计算三角形的顶点
    cv::Point topVertex(rect.x + rect.width / 2, rect.y - triangleHeight);
    cv::Point leftVertex(rect.x, rect.y);
    cv::Point rightVertex(rect.x + rect.width, rect.y);

    cv::Point bottomRight(rect.x + rect.width, rect.y + rect.height);
    cv::Point bottomLeft(rect.x, rect.y + rect.height);


    std::vector<cv::Point> trianglePoints = { topVertex, rightVertex ,bottomRight,bottomLeft ,leftVertex, };
    //确保每个点都在图像边界内
    for (auto& point : trianglePoints) {
        point.x = std::max(0, std::min(point.x, cols - 1));
        point.y = std::max(0, std::min(point.y, rows - 1));
    }
    vertexs = trianglePoints;
}

void Type4::ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs)
{
    //类别5 六边形
     //cv::Rect rect = rects[i];
    float expandRatio = 0.2;
    int triangleWidth = static_cast<int>(rect.height * expandRatio);

    cv::Point leftVertex(rect.x - triangleWidth, rect.y + rect.height / 2);
    cv::Point rightVertex(rect.x + rect.width + triangleWidth, rect.y + rect.height / 2);

    cv::Point topLeft(rect.x, rect.y);
    cv::Point topRight(rect.x + rect.width, rect.y);
    cv::Point bottomLeft(rect.x, rect.y + rect.height);
    cv::Point bottomRight(rect.x + rect.width, rect.y + rect.height);

    std::vector<cv::Point> trianglePoints = { leftVertex ,topLeft,topRight,rightVertex,bottomRight,bottomLeft };


    //确保每个点都在图像边界内
    for (auto& point : trianglePoints) {
        point.x = std::max(0, std::min(point.x, cols - 1));
        point.y = std::max(0, std::min(point.y, rows - 1));
    }
    vertexs = trianglePoints;
}

void Type5::ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs)
{
    //直接绘制椭圆外接矩形
}

void Type6::ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs)
{
    //cv::Rect rect = rects[i];
    float expandRatio = 0.2;
    int triangleWidth = static_cast<int>(rect.height * expandRatio);

    cv::Point leftVertex1(rect.x - triangleWidth, rect.y + rect.height / 3);
    cv::Point leftVertex2(rect.x - triangleWidth, rect.y + 2 * rect.height / 3);
    cv::Point rightVertex1(rect.x + rect.width + triangleWidth, rect.y + rect.height / 3);
    cv::Point rightVertex2(rect.x + rect.width + triangleWidth, rect.y + 2 * rect.height / 3);

    cv::Point topLeft(rect.x, rect.y);
    cv::Point topRight(rect.x + rect.width, rect.y);
    cv::Point bottomLeft(rect.x, rect.y + rect.height);
    cv::Point bottomRight(rect.x + rect.width, rect.y + rect.height);

    std::vector<cv::Point> trianglePoints = { leftVertex2,leftVertex1 ,topLeft,topRight,rightVertex1,rightVertex2,bottomRight,bottomLeft };

    // 确保每个点都在图像边界内
    for (auto& point : trianglePoints) {
        point.x = std::max(0, std::min(point.x, cols - 1));
        point.y = std::max(0, std::min(point.y, rows - 1));
    }
    vertexs = trianglePoints;
}