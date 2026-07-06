#ifndef GLOBAL_RECIPE_H
#define GLOBAL_RECIPE_H

#include <QCoreApplication>
#include <QTranslator>
#include <QImage>
#include <QString>
#include <QMap>
#include <QVector>


typedef enum DefectLevel
{

	NORMAL,																				//0 正常
	MINOR,																				//1 轻微缺陷
	MEDIUM,																				//2 中等缺陷
	SERIOUS,																			//3 严重缺陷
	ABNORMAL,																			//4 异常
	AREAERROR,																			//5 区域错误，非玻璃区域判断
	DEFECT_LEVEL_COUNT																	//n 
};

typedef enum DefectType
{

	TYPE_POORCOATING,																	//0 镀膜不良
	TYPE_SCRATCH,																		//1 划伤
	TYPE_CALCULUS,																		//2 结石
	TYPE_BUBBLE,																		//3 气泡
	TYPE_TRADEMARK,																		//4 商标
	TYPE_WATERSTAIN,																	//5 水渍
	TYPE_SMUDGE,																		//6 脏污
	TYPE_SCREENPRINTING,																//7 丝印
	TYPE_CHIPPED_EDGE,                                                                   //8 崩边
	TYPE_PITTING,                                                                        //9 麻点
	TYPE_GLASS_CULLET,                                                                   //10 玻渣
	TYPE_WAVINESS,                                                                       //11 云朵
	TYPE_OTHER,                                                                          //12 其他
	DEFECT_TYPE_COUNT																	//n 缺陷类型数量。后续添加新缺陷时，保持该行在最后一行。
};

//瑕疵类型是否启用
inline QMap<DefectType, bool>           m_defectTypeUsability;                         //所有缺陷类型是否启用

//瑕疵类型是否显示
inline QMap<DefectType, bool>           m_defectTypeVisibility;                         //所有缺陷类型是否显示

/* 瑕疵图例是否显示参数 */
//缺陷等级是否显示
inline bool								DISPLAY_NORMAL				= true;				//0 正常
inline bool								DISPLAY_MINOR				= false;			//1 轻微缺陷
inline bool								DISPLAY_MEDIUM				= true;				//2 中等缺陷
inline bool								DISPLAY_SERIOUS				= true;				//3 严重缺陷
inline bool								DISPLAY_ABNORMAL			= true;				//4 异常
inline bool								DISPLAY_AREAERROR			= true;				//5 区域错误，非玻璃区域判断


inline QString DefectLevelToString(DefectLevel& level)
{
	switch (level)
	{
	    case DefectLevel::NORMAL:               return QCoreApplication::translate("Global", "NORMAL");
	    case DefectLevel::MINOR:                return QCoreApplication::translate("Global", "MINOR");
	    case DefectLevel::MEDIUM:               return QCoreApplication::translate("Global", "MEDIUM");
	    case DefectLevel::SERIOUS:              return QCoreApplication::translate("Global", "SERIOUS");
	    case DefectLevel::ABNORMAL:             return QCoreApplication::translate("Global", "ABNORMAL");
	    case DefectLevel::AREAERROR:            return QCoreApplication::translate("Global", "AREAERROR");
	    default:                                return QCoreApplication::translate("Global", "ERROR");
	}
}


inline QString DefectTypeToString(DefectType type) {
	switch (type) {
        case DefectType::TYPE_POORCOATING:      return QCoreApplication::translate("Global", "Poorcoating");
        case DefectType::TYPE_SCRATCH:          return QCoreApplication::translate("Global", "Scratch");
        case DefectType::TYPE_CALCULUS:         return QCoreApplication::translate("Global", "Calculus");
        case DefectType::TYPE_BUBBLE:           return QCoreApplication::translate("Global", "Bubble");
        case DefectType::TYPE_TRADEMARK:        return QCoreApplication::translate("Global", "Trademark");
        case DefectType::TYPE_WATERSTAIN:       return QCoreApplication::translate("Global", "WaterStain");
        case DefectType::TYPE_SMUDGE:           return QCoreApplication::translate("Global", "Contamination");
        case DefectType::TYPE_SCREENPRINTING:   return QCoreApplication::translate("Global", "ScreenPrinting");
        case DefectType::TYPE_CHIPPED_EDGE:     return QCoreApplication::translate("Global", "ChippedEdge");			        //8 崩边
        case DefectType::TYPE_PITTING:          return QCoreApplication::translate("Global", "Pitting");						//9 麻点
        case DefectType::TYPE_GLASS_CULLET:     return QCoreApplication::translate("Global", "GlassCullet");			        //10 玻渣
        case DefectType::TYPE_WAVINESS:         return QCoreApplication::translate("Global", "Waviness");					    //11 云朵
        case DefectType::TYPE_OTHER:            return QCoreApplication::translate("Global", "Other");							//12 其他                                                                   
        default:                                return QCoreApplication::translate("Global", "Unknown");
	}
}

// 处理errnum系列列
inline QVector<DefectType> defectTypes = {
	DefectType::TYPE_POORCOATING,
	DefectType::TYPE_SCRATCH,
	DefectType::TYPE_CALCULUS,
	DefectType::TYPE_BUBBLE,
	DefectType::TYPE_TRADEMARK,
	DefectType::TYPE_WATERSTAIN,
	DefectType::TYPE_SMUDGE,
	DefectType::TYPE_SCREENPRINTING,
	DefectType::TYPE_CHIPPED_EDGE,
	DefectType::TYPE_PITTING,
	DefectType::TYPE_GLASS_CULLET,
	DefectType::TYPE_WAVINESS,
	DefectType::TYPE_OTHER
};


inline QString GetDefectIconPath(int defectId)
{
	DefectType defectType = static_cast<DefectType>(defectId);
	if (defectType == DefectType::TYPE_POORCOATING)			return ":/Recipe/Recipe/0.png";
	if (defectType == DefectType::TYPE_SCRATCH)				return ":/Recipe/Recipe/1.png";
	if (defectType == DefectType::TYPE_CALCULUS)			return ":/Recipe/Recipe/2.png";
	if (defectType == DefectType::TYPE_BUBBLE)				return ":/Recipe/Recipe/3.png";;
	if (defectType == DefectType::TYPE_TRADEMARK)			return ":/Recipe/Recipe/4.png";;
	if (defectType == DefectType::TYPE_WATERSTAIN)			return ":/Recipe/Recipe/5.png";
	if (defectType == DefectType::TYPE_SMUDGE)				return ":/Recipe/Recipe/6.png";
	if (defectType == DefectType::TYPE_SCREENPRINTING)		return ":/Recipe/Recipe/7.png";
	if (defectType == DefectType::TYPE_CHIPPED_EDGE)		return ":/Recipe/Recipe/8.png";;
	if (defectType == DefectType::TYPE_PITTING)				return ":/Recipe/Recipe/9.png";;
	if (defectType == DefectType::TYPE_GLASS_CULLET)		return ":/Recipe/Recipe/10.png";
	if (defectType == DefectType::TYPE_WAVINESS)			return ":/Recipe/Recipe/11.png";
	if (defectType == DefectType::TYPE_OTHER)				return ":/Recipe/Recipe/12.png";
	// 默认图标
	return ":/icons/info.png";
}
inline QImage DefectTypeToImage(DefectType type)
{
	switch (type)
	{
	case TYPE_POORCOATING:								//0 镀膜不良
		return QImage(":/Recipe/Recipe/0_S.png");
	case TYPE_SCRATCH:									//1 划伤
		return QImage(":/Recipe/Recipe/1_S.png");
	case TYPE_CALCULUS:
		return QImage(":/Recipe/Recipe/2_S.png");
	case TYPE_BUBBLE:
		return QImage(":/Recipe/Recipe/3_S.png");
	case TYPE_TRADEMARK:
		return QImage(":/Recipe/Recipe/4_S.png");
	case TYPE_WATERSTAIN:
		return QImage(":/Recipe/Recipe/5_S.png");
	case TYPE_SMUDGE:
		return QImage(":/Recipe/Recipe/6_S.png");
	case TYPE_SCREENPRINTING:
		return QImage(":/Recipe/Recipe/7_S.png");
	case TYPE_CHIPPED_EDGE:
		return QImage(":/Recipe/Recipe/8_S.png");
	case TYPE_PITTING:
		return QImage(":/Recipe/Recipe/9_S.png");
	case TYPE_GLASS_CULLET:
		return QImage(":/Recipe/Recipe/10_S.png");
	case TYPE_WAVINESS:
		return QImage(":/Recipe/Recipe/11_S.png");
	case TYPE_OTHER:
		return QImage(":/Recipe/Recipe/12_S.png");
	default:
		return QImage();
	}
}

inline QColor DefectLevelToColor(DefectLevel level)
{
	/* 获取缺陷的颜色 */
	QColor color = QColor(0, 255, 0);//绿色;
	switch (level)
	{
	case DefectLevel::NORMAL:
	{
		color = QColor(0, 255, 0);//绿色
		break;
	}

	case DefectLevel::MINOR:
	{
		color = QColor(0, 255, 0);//蓝色
		break;
	}

	case DefectLevel::MEDIUM:
	{
		color = QColor(255, 255, 0);//黄色
		break;
	}

	case DefectLevel::SERIOUS:
	{
		color = QColor(255, 0, 0);//红色
		break;
	}

	default:
		color = QColor(0, 255, 0);//绿色
		break;
	}
	return color;
}

#endif // GLOBAL_RECIPE_H