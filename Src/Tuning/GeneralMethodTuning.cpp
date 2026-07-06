#include "GeneralMethodTuning.h"

namespace Tuning {
	std::vector<drawInformation> filterDefectsByBorderRegion(
		const std::vector<drawInformation>& defects,
		int imageWidth, int imageHeight,
		float verticalRatio, float horizontalRatio)
	{
		// 默认：委托给 DLL 原版实现
		// 现场需要调整时，在此处替换为自定义逻辑
		GeneralMethod gm;
		return gm.filterDefectsByBorderRegion(defects, imageWidth, imageHeight, verticalRatio, horizontalRatio);
	}
}
