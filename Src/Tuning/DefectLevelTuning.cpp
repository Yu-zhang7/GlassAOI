#include "DefectLevelTuning.h"

namespace Tuning {
	std::vector<drawInformation> GetDefectLevel(
		std::vector<drawInformation>& data,
		PrescriptionParameter& levelRankInfo)
	{
		// 默认：委托给 DLL 原版实现
		// 现场需要调整时，在此处替换为自定义逻辑
		defectLevel dl;
		return dl.GetDefectLevel(data, levelRankInfo);
	}
}
