#include "glassData2Json.h"


glassData2Json::glassData2Json()
{
}

glassData2Json::~glassData2Json()
{
}


/*====================Json Write============================*/
void mergeDrawInformation(const std::vector<drawInformation>& drawInfos, std::map<DefectType, std::vector<drawInformation>>& mergedInfos) {
    for (const auto& info : drawInfos) {
        mergedInfos[info.DefectType].push_back(info);
    }
}

void addDefectTypeToJson(const std::vector<drawInformation>& drawInfos, Json::Value& type, const std::string& typeKey, Json::Value& root) {
    Json::Value errorRectArray(Json::arrayValue);
    Json::Value errorInfoArray(Json::arrayValue);
    Json::Value errorRealRectArray(Json::arrayValue);

    for (const auto& info : drawInfos) {
        type["camera_id"].append(info.camera_id);
        type["DefectType"].append(info.DefectType);
        type["ErrorType"].append(info.ErrorType);
        type["confidence"].append(info.confidence);
        type["DefectId"].append(info.DefectId);

        Json::Value errorRect_temp(Json::arrayValue);
        errorRect_temp.append(info.rect.x);
        errorRect_temp.append(info.rect.y);
        errorRect_temp.append(info.rect.width);
        errorRect_temp.append(info.rect.height);
        errorRectArray.append(errorRect_temp);

        Json::Value errorRealRect_temp(Json::arrayValue);
        errorRealRect_temp.append(info.realRect.x);
        errorRealRect_temp.append(info.realRect.y);
        errorRealRect_temp.append(info.realRect.width);
        errorRealRect_temp.append(info.realRect.height);
        errorRealRectArray.append(errorRealRect_temp);

        Json::Value errorInfo_temp(Json::arrayValue);
        if (info.Errorinfo.size() == 3) {

            errorInfo_temp.append(info.Errorinfo[0]);
            errorInfo_temp.append(info.Errorinfo[1]);
            errorInfo_temp.append(info.Errorinfo[2]);
            errorInfoArray.append(errorInfo_temp);

        }

    }

    type["Rects"] = errorRectArray;
    type["RealRects"] = errorRealRectArray;
    type["ErrorInfo"] = errorInfoArray;
    root["DetectInfos"][typeKey] = type;
}

int glassData2Json::saveData2Json(std::vector<drawInformation> drawInfos, int Level, float glassPixelWidth, float glassPixelHeight, std::string savePath)
{
    //合并drawInfos中相同类型的drawInformation
    std::map<DefectType, std::vector<drawInformation>> mergedInfos;
    mergeDrawInformation(drawInfos, mergedInfos);
    /*===========创建Json结构=============*/
    Json::Value root;

    root["Level"] = Json::Value(Level);
    root["GlassPixelWidth"] = Json::Value(glassPixelWidth);
    root["GlassPixelHeight"] = Json::Value(glassPixelHeight);
    /*=================Type_0====================*/
    Json::Value type_0;
    type_0["camera_id"] = Json::Value();
    type_0["DefectType"] = Json::Value();
    type_0["ErrorType"] = Json::Value();
    type_0["Rects"] = Json::Value();
    type_0["RealRects"] = Json::Value();
    type_0["ErrorInfo"] = Json::Value();
    type_0["confidence"] = Json::Value();
    type_0["DefectId"] = Json::Value();
    addDefectTypeToJson(mergedInfos[TYPE_POORCOATING], type_0, "Type_0", root);
    /*=================Type_0====================*/


    /*=================Type_1====================*/
    Json::Value type_1;
    type_1["camera_id"] = Json::Value();
    type_1["DefectType"] = Json::Value();
    type_1["ErrorType"] = Json::Value();
    type_1["Rects"] = Json::Value();
    type_1["RealRects"] = Json::Value();
    type_1["ErrorInfo"] = Json::Value();
    type_1["confidence"] = Json::Value();
    type_1["DefectId"] = Json::Value();
    addDefectTypeToJson(mergedInfos[TYPE_SCRATCH], type_1, "Type_1", root);
    /*=================Type_1====================*/

    /*=================Type_2====================*/
    Json::Value type_2;
    type_2["camera_id"] = Json::Value();
    type_2["DefectType"] = Json::Value();
    type_2["ErrorType"] = Json::Value();
    type_2["Rects"] = Json::Value();
    type_2["RealRects"] = Json::Value();
    type_2["ErrorInfo"] = Json::Value();
    type_2["confidence"] = Json::Value();
    type_2["DefectId"] = Json::Value();
    addDefectTypeToJson(mergedInfos[TYPE_CALCULUS], type_2, "Type_2", root);
    /*=================Type_2====================*/

    /*=================Type_3====================*/
    Json::Value type_3;
    type_3["camera_id"] = Json::Value();
    type_3["DefectType"] = Json::Value();
    type_3["ErrorType"] = Json::Value();
    type_3["Rects"] = Json::Value();
    type_3["RealRects"] = Json::Value();
    type_3["ErrorInfo"] = Json::Value();
    type_3["confidence"] = Json::Value();
    type_3["DefectId"] = Json::Value();
    addDefectTypeToJson(mergedInfos[TYPE_BUBBLE], type_3, "Type_3", root);
    /*=================Type_3====================*/

    /*=================Type_4====================*/
    Json::Value type_4;
    type_4["camera_id"] = Json::Value();
    type_4["DefectType"] = Json::Value();
    type_4["ErrorType"] = Json::Value();
    type_4["Rects"] = Json::Value();
    type_4["RealRects"] = Json::Value();
    type_4["ErrorInfo"] = Json::Value();
    type_4["confidence"] = Json::Value();
    type_4["DefectId"] = Json::Value();
    addDefectTypeToJson(mergedInfos[TYPE_TRADEMARK], type_4, "Type_4", root);
    /*=================Type_4====================*/

    /*=================Type_5====================*/
    Json::Value type_5;
    type_5["camera_id"] = Json::Value();
    type_5["DefectType"] = Json::Value();
    type_5["ErrorType"] = Json::Value();
    type_5["Rects"] = Json::Value();
    type_5["RealRects"] = Json::Value();
    type_5["ErrorInfo"] = Json::Value();
    type_5["confidence"] = Json::Value();
    type_5["DefectId"] = Json::Value();
    addDefectTypeToJson(mergedInfos[TYPE_WATERSTAIN], type_5, "Type_5", root);
    /*=================Type_5====================*/

    /*=================Type_6====================*/
    Json::Value type_6;
    type_6["camera_id"] = Json::Value();
    type_6["DefectType"] = Json::Value();
    type_6["ErrorType"] = Json::Value();
    type_6["Rects"] = Json::Value();
    type_6["RealRects"] = Json::Value();
    type_6["ErrorInfo"] = Json::Value();
    type_6["confidence"] = Json::Value();
    type_6["DefectId"] = Json::Value();
    addDefectTypeToJson(mergedInfos[TYPE_SMUDGE], type_6, "Type_6", root);
    /*=================Type_6====================*/

    /*=================Type_7====================*/
    Json::Value type_7;
    type_7["camera_id"] = Json::Value();
    type_7["DefectType"] = Json::Value();
    type_7["ErrorType"] = Json::Value();
    type_7["Rects"] = Json::Value();
    type_7["RealRects"] = Json::Value();
    type_7["ErrorInfo"] = Json::Value();
    type_7["confidence"] = Json::Value();
    type_7["DefectId"] = Json::Value();
    addDefectTypeToJson(mergedInfos[TYPE_SCREENPRINTING], type_7, "Type_7", root);
    /*=================Type_7====================*/

        /*=================Type_3====================*/
    Json::Value type_8;
    type_8["camera_id"] = Json::Value();
    type_8["DefectType"] = Json::Value();
    type_8["ErrorType"] = Json::Value();
    type_8["Rects"] = Json::Value();
    type_8["RealRects"] = Json::Value();
    type_8["ErrorInfo"] = Json::Value();
    type_8["confidence"] = Json::Value();
    type_8["DefectId"] = Json::Value();
    addDefectTypeToJson(mergedInfos[TYPE_CHIPPED_EDGE], type_8, "Type_8", root);
    /*=================Type_3====================*/

    /*=================Type_4====================*/
    Json::Value type_9;
    type_9["camera_id"] = Json::Value();
    type_9["DefectType"] = Json::Value();
    type_9["ErrorType"] = Json::Value();
    type_9["Rects"] = Json::Value();
    type_9["RealRects"] = Json::Value();
    type_9["ErrorInfo"] = Json::Value();
    type_9["confidence"] = Json::Value();
    type_9["DefectId"] = Json::Value();
    addDefectTypeToJson(mergedInfos[TYPE_PITTING], type_9, "Type_9", root);
    /*=================Type_4====================*/

    /*=================Type_5====================*/
    Json::Value type_10;
    type_10["camera_id"] = Json::Value();
    type_10["DefectType"] = Json::Value();
    type_10["ErrorType"] = Json::Value();
    type_10["Rects"] = Json::Value();
    type_10["RealRects"] = Json::Value();
    type_10["ErrorInfo"] = Json::Value();
    type_10["confidence"] = Json::Value();
    type_10["DefectId"] = Json::Value();
    addDefectTypeToJson(mergedInfos[TYPE_GLASS_CULLET], type_10, "Type_10", root);
    /*=================Type_5====================*/

    /*=================Type_6====================*/
    Json::Value type_11;
    type_11["camera_id"] = Json::Value();
    type_11["DefectType"] = Json::Value();
    type_11["ErrorType"] = Json::Value();
    type_11["Rects"] = Json::Value();
    type_11["RealRects"] = Json::Value();
    type_11["ErrorInfo"] = Json::Value();
    type_11["confidence"] = Json::Value();
    type_11["DefectId"] = Json::Value();
    addDefectTypeToJson(mergedInfos[TYPE_WAVINESS], type_11, "Type_11", root);
    /*=================Type_6====================*/

    /*=================Type_7====================*/
    Json::Value type_12;
    type_12["camera_id"] = Json::Value();
    type_12["DefectType"] = Json::Value();
    type_12["ErrorType"] = Json::Value();
    type_12["Rects"] = Json::Value();
    type_12["RealRects"] = Json::Value();
    type_12["ErrorInfo"] = Json::Value();
    type_12["confidence"] = Json::Value();
    type_12["DefectId"] = Json::Value();
    addDefectTypeToJson(mergedInfos[TYPE_OTHER], type_12, "Type_12", root);
    /*=================Type_7====================*/

    /*===========创建Json结构=============*/

    Json::StyledWriter sw;
    //std::cout << sw.write(root) << std::endl << std::endl;

    std::ofstream os;
    os.open(savePath, std::ios::out);
    if (!os.is_open())
    {
        std::cout << "error:can't find the file" << std::endl;
    }
    os << sw.write(root);
    os.close();

    return 0;
}

/*====================Json Write============================*/



/*====================Json Read============================*/
std::vector<drawInformation> glassData2Json::parseJsonToDrawInfos(const std::string& filePath, float& glassPixelWidth, float& glassPixelHeight)
{
    std::cout << filePath << std::endl;
    if (std::filesystem::exists(filePath))
    {
        std::cout<< "file exists" << std::endl;
    }

    std::ifstream file(filePath, std::ios::binary);
    bool temp = file.is_open();
    std::vector<drawInformation> drawInfos;
    if (!temp)
    {
        return drawInfos;
    }

    Json::Value root;
    file >> root;

    if (root.isMember("DetectInfos")) 
    {
        const Json::Value& detectInfos = root["DetectInfos"]; 
        for (Json::Value::const_iterator it = detectInfos.begin(); it != detectInfos.end(); ++it) {
            const Json::Value& type = *it;
            const Json::Value& cameraIds = type["camera_id"];
            const Json::Value& defectTypes = type["DefectType"];
            const Json::Value& errorTypes = type["ErrorType"];
            const Json::Value& rects = type["Rects"];
            const Json::Value& realRects = type["RealRects"];
            const Json::Value& errorInfos = type["ErrorInfo"];
            const Json::Value& confidence = type["confidence"];
            const Json::Value& DefectId = type["DefectId"];

            for (Json::ArrayIndex i = 0; i < cameraIds.size(); ++i) {
                drawInformation info;
                info.camera_id = cameraIds[i].asInt();
                int defecttype = defectTypes[i].asInt();
                if (defecttype == 0) {

                    info.DefectType = TYPE_POORCOATING;
                }
                else if (defecttype == 1)
                {
                    info.DefectType = TYPE_SCRATCH;
                }
                else if (defecttype == 2)
                {
                    info.DefectType = TYPE_CALCULUS;
                }
                else if (defecttype == 3)
                {
                    info.DefectType = TYPE_BUBBLE;
                }
                else if (defecttype == 4)
                {
                    info.DefectType = TYPE_TRADEMARK;
                }
                else if (defecttype == 5)
                {
                    info.DefectType = TYPE_WATERSTAIN;
                }
                else if (defecttype == 6)
                {
                    info.DefectType = TYPE_SMUDGE;
                }
                else if (defecttype == 7)
                {
                    info.DefectType = TYPE_SCREENPRINTING;
                }
                else if (defecttype == 8)
                {
                    info.DefectType = TYPE_CHIPPED_EDGE;
                }
                else if (defecttype == 9)
                {
                    info.DefectType = TYPE_PITTING;
                }
                else if (defecttype == 10)
                {
                    info.DefectType = TYPE_GLASS_CULLET;
                }
                else if (defecttype == 11)
                {
                    info.DefectType = TYPE_WAVINESS;
                }
                else if (defecttype == 12)
                {
                    info.DefectType = TYPE_OTHER;
                }

                int type = std::atoi((errorTypes[i].asString()).c_str());
                if (type == 0) {

                    info.ErrorType = NORMAL;
                }
                else if (type == 1)
                {
                    info.ErrorType = MINOR;
                }
                else if (type == 2)
                {
                    info.ErrorType = MEDIUM;
                }
                else if (type == 3)
                {
                    info.ErrorType = SERIOUS;
                }
                else if (type == 4)
                {
                    info.ErrorType = ABNORMAL;
                }
                else
                {
                    info.ErrorType = AREAERROR;
                }
                const Json::Value& rect = rects[i];
                const Json::Value& realRect = realRects[i];
                info.rect.x = rect[0].asInt();
                info.rect.y = rect[1].asInt();
                info.rect.width = rect[2].asInt();
                info.rect.height = rect[3].asInt();
                info.realRect.x = realRect[0].asInt();
                info.realRect.y = realRect[1].asInt();
                info.realRect.width = realRect[2].asInt();
                info.realRect.height = realRect[3].asInt();
                info.confidence = confidence[i].asFloat();
                info.DefectId = DefectId[i].asInt();
                const Json::Value& errorInfo = errorInfos[i];
                for (Json::ArrayIndex j = 0; j < errorInfo.size(); ++j) {
                    info.Errorinfo.push_back(std::stof(errorInfo[j].asString()));
                }

                drawInfos.push_back(info);
            }
        }
    }
    if (root.isMember("GlassPixelWidth"))
    {
        glassPixelWidth = root["GlassPixelWidth"].asFloat();
    }
    else
    {
        glassPixelWidth = 0.0f;
    }
    if (root.isMember("GlassPixelHeight"))
    {
        glassPixelHeight = root["GlassPixelHeight"].asFloat();
    }
    else
    {
        glassPixelHeight = 0.0f;
    }
    return drawInfos;
}
/*====================Json Read============================*/

void glassData2Json::mainTest()
{
    /*================写入测试=======================*/
    std::vector<drawInformation> drawInfos;
    int Level = 0;
    std::string savePath = "./result/savePath.json";

    for (int i = 0; i < 10; i++) {
        drawInformation temp;
        temp.DefectType = TYPE_POORCOATING;
        temp.ErrorType = ABNORMAL;
        temp.Errorinfo = { 0.1,0.2,0.3 };
        temp.camera_id = i;
        temp.rect = cv::Rect(i, i, i, i);

        drawInfos.push_back(temp);

    }

    for (int i = 0; i < 5; i++) {
        drawInformation temp;
        temp.DefectType = TYPE_SCRATCH;
        temp.ErrorType = MINOR;
        temp.Errorinfo = { 0.1,0.2,0.3 };
        temp.camera_id = i;
        temp.rect = cv::Rect(i, i, i, i);

        drawInfos.push_back(temp);

    }

    for (int i = 0; i < 1; i++) {
        drawInformation temp;
        temp.DefectType = TYPE_CALCULUS;
        temp.ErrorType = MEDIUM;
        temp.Errorinfo = { 0.1,0.2,0.3 };
        temp.camera_id = i;
        temp.rect = cv::Rect(i, i, i, i);

        drawInfos.push_back(temp);

    }

    for (int i = 0; i < 15; i++) {
        drawInformation temp;
        temp.DefectType = TYPE_BUBBLE;
        temp.ErrorType = SERIOUS;
        temp.Errorinfo = { 0.1,0.2,0.3 };
        temp.camera_id = i;
        temp.rect = cv::Rect(i, i, i, i);

        drawInfos.push_back(temp);

    }

    for (int i = 0; i < 3; i++) {
        drawInformation temp;
        temp.DefectType = TYPE_TRADEMARK;
        temp.ErrorType = ABNORMAL;
        temp.Errorinfo = { 0.1,0.2,0.3 };
        temp.camera_id = i;
        temp.rect = cv::Rect(i, i, i, i);

        drawInfos.push_back(temp);

    }
    float GlassWidth = 1000;
    float GlassHeight = 1000;
    saveData2Json(drawInfos, Level, GlassWidth, GlassHeight, savePath);
    /*================写入测试=======================*/


    /*================读入测试=======================*/
    //std::string filePath = "./result/savePath.json";
    //std::vector<drawInformation> drawInfos = parseJsonToDrawInfos(filePath);

    //// 打印解析后的 drawInfos 以验证
    //for (const auto& info : drawInfos) {
    //    std::cout << "Camera ID: " << info.camera_id << ", Defect Type: " << static_cast<int>(info.DefectType)
    //        << ", Error Type: " << info.ErrorType << ", Rect: (" << info.rect.x << ", " << info.rect.y << ", "
    //        << info.rect.width << ", " << info.rect.height << "), Error Info: ";
    //    for (const auto& errorInfo : info.Errorinfo) {
    //        std::cout << errorInfo << " ";
    //    }
    //    std::cout << std::endl;
    //}

    //int Level = 0;
    //std::string savePath = "./result/savePath2.json";
    //saveData2Json(drawInfos, Level, savePath);
    /*================读入测试=======================*/

}

