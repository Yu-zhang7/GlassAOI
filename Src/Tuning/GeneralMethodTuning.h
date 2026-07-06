#pragma once

// 现场可调层 - GeneralMethod 覆盖函数
// DLL 中 GeneralMethod 类的方法保持冻结，需要现场调整时改本文件 .cpp
#include "GeneralMethod.h"

namespace Tuning {
	std::vector<drawInformation> filterDefectsByBorderRegion(
		const std::vector<drawInformation>& defects,
		int imageWidth, int imageHeight,
		float verticalRatio, float horizontalRatio);
}
