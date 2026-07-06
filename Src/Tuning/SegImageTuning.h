#pragma once

// 现场可调层 - segImage 覆盖函数
// 当前为空。将来需要现场调整某个 segImage 方法时，
// 在此声明同名函数（签名与 segImage.h 中一致），在 .cpp 中实现。
#include "segImage.h"

namespace Tuning {
	// 示例（需要时取消注释）：
	// cv::Mat GetDrawImage(const cv::Mat& src, int threshold_value, int level);
	// cv::Mat drawEdgesOnImage(const cv::Mat& src, const cv::Mat& edges, cv::Rect rect, const cv::Scalar& color, int thickness);
}
