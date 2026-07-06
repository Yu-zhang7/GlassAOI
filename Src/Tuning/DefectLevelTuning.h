#pragma once

// 现场可调层 - defectLevel 覆盖函数
// DLL 中 defectLevel 类的方法保持冻结，需要现场调整时改本文件 .cpp
#include "defectLevel.h"

namespace Tuning {
	std::vector<drawInformation> GetDefectLevel(
		std::vector<drawInformation>& data,
		PrescriptionParameter& levelRankInfo);
}
