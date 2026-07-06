#include "json/Result2Json.h"


Json::Value result2Json::detectInfo(std::vector<drawInformation> RectData) {
    //子节点  
    Json::Value DetectInfo;
    // 检查 RectData 是否包含元素
    if (!RectData.empty()) {
        // 将点数组添加到 JSON 对象中
        Json::Value pointsJson(Json::arrayValue);
        Json::Value ErrorInfo(Json::arrayValue);
        for (const auto& rect : RectData) {
            DetectInfo["1camera_id"] = Json::Value(rect.camera_id);
            DetectInfo["2DefectType"] = Json::Value(rect.DefectType);
            DetectInfo["3ErrorType"] = Json::Value(rect.ErrorType);
            Json::Value rectJson(Json::arrayValue);
            rectJson.append(rect.rect.x);
            rectJson.append(rect.rect.y);
            rectJson.append(rect.rect.width);
            rectJson.append(rect.rect.height);
            pointsJson.append(rectJson);
            DetectInfo["4Rects"] = pointsJson;
            //Json::Value erroeInfo;
            Json::Value erroeInfo(Json::arrayValue);
            erroeInfo.append(rect.Errorinfo[0]);
            erroeInfo.append(rect.Errorinfo[1]);
            erroeInfo.append(rect.Errorinfo[2]);
            ErrorInfo.append(erroeInfo);
            DetectInfo["5ErrorInfo"] = ErrorInfo;
        }
    }

    return DetectInfo;
}

void result2Json::readFileJson(std::string readIn, int& glassLevel, std::vector<drawInformation>& InfoData)
{
    Json::Reader reader;
    Json::Value root;

    std::ifstream in(readIn, std::ios::binary);

    if (!in.is_open())
    {
        std::cout << "Error opening file\n";
        return;
    }

    if (reader.parse(in, root))
    {
        // 提取 1Level
        glassLevel = root["1Level"].asInt();
        //std::cout << "1Level: " << glassLevel << std::endl;

        // 检查 "2DetectInfos" 键是否存在，并且它是一个对象
        if (!root.isMember("2DetectInfos") || !root["2DetectInfos"].isObject()) {
            std::cerr << "\"2DetectInfos\" key is missing or not an object!" << std::endl;
        }

        // 遍历 "2DetectInfos" 对象
        const Json::Value& detectInfos = root["2DetectInfos"];
        for (const auto& key : detectInfos.getMemberNames()) {
            const Json::Value& detectInfo = detectInfos[key];

            int camera_id = detectInfo["1camera_id"].asInt();

            DefectType defectTypeNum;
            if (detectInfo.isMember("2DefectType") && detectInfo["2DefectType"].isInt()) {
                int defectType = detectInfo["2DefectType"].asInt();
                if (defectType >= TYPE_POORCOATING && defectType <= TYPE_SMUDGE) {
                    defectTypeNum = static_cast<DefectType>(defectType);
                }
            }


            DefectLevel errorTypeNum;
            if (detectInfo.isMember("3ErrorType") && detectInfo["3ErrorType"].isInt()) {
                int errorType = detectInfo["3ErrorType"].asInt();
                if (errorType >= TYPE_POORCOATING && errorType <= TYPE_SMUDGE) {
                    errorTypeNum = static_cast<DefectLevel>(errorType);
                }
            }

            // 提取 "4Rects" 字段
            std::vector<cv::Rect> rects;
            if (detectInfo.isMember("4Rects") && detectInfo["4Rects"].isArray()) {
                const Json::Value& rectsJson = detectInfo["4Rects"];
                for (unsigned i = 0; i < rectsJson.size(); ++i) {
                    const Json::Value& rectArray = rectsJson[i];
                    if (rectArray.isArray() && rectArray.size() == 4) {
                        // Correctly index the elements of rectArray
                        int x = rectArray[i].asInt();
                        int y = rectArray[i + 1].asInt();
                        int width = rectArray[i + 2].asInt();
                        int height = rectArray[i + 3].asInt();
                        rects.push_back(cv::Rect(x, y, width, height));
                    }
                    else {
                        std::cerr << "Invalid rectangle data at index " << i << "!" << std::endl;
                    }
                }
            }
            else {
                std::cerr << "\"4Rects\" field is missing or not an array!" << std::endl;
            }

            // 提取 "5ErrorInfo" 字段
            std::vector<std::vector<float>> TotalErrorinfos;
            const Json::Value& errorInfo = detectInfo["5ErrorInfo"];
            for (auto&& errorArray : errorInfo) {
                std::vector<float> Errorinfos;
                for (auto&& value : errorArray) {
                    float val = static_cast<float>(value.asDouble());
                    Errorinfos.push_back(val);
                }
                TotalErrorinfos.push_back(Errorinfos);
            }

            // 确保两个容器的大小一致
            if (rects.size() != TotalErrorinfos.size()) {
                std::cerr << "Mismatched number of rectangles and error info entries!" << std::endl;
            }
            else {
                for (size_t i = 0; i < rects.size(); ++i) {
                    drawInformation DrawInformation;
                    DrawInformation.camera_id = camera_id;
                    DrawInformation.DefectType = defectTypeNum;
                    DrawInformation.ErrorType = errorTypeNum;
                    DrawInformation.rect = rects[i];
                    DrawInformation.Errorinfo = TotalErrorinfos[i];
                    InfoData.push_back(DrawInformation);
                }
            }

            
        }
    }
    else
    {
        std::cout << "parse error\n" << std::endl;
    }

    in.close();
}

void result2Json::writeFileJson(std::string PathName, int glassLevel, std::vector<drawInformation> writeInfo)
{
    //根节点  
    Json::Value root;
    std::vector<drawInformation> RectData0;
    std::vector<drawInformation> RectData1;
    std::vector<drawInformation> RectData2;
    std::vector<drawInformation> RectData3;
    std::vector<drawInformation> RectData4;
    std::vector<drawInformation> RectData5;
    std::vector<drawInformation> RectData6;

    for (int i = 0; i < writeInfo.size(); i++) {
        if (writeInfo[i].DefectType == 0) {
            drawInformation temp;
            temp.camera_id = writeInfo[i].camera_id;
            temp.DefectType = writeInfo[i].DefectType;
            temp.rect = writeInfo[i].rect;
            temp.ErrorType = writeInfo[i].ErrorType;
            temp.Errorinfo = writeInfo[i].Errorinfo;
            RectData0.push_back(temp);
        }
        if (writeInfo[i].DefectType == 1) {
            drawInformation temp;
            temp.camera_id = writeInfo[i].camera_id;
            temp.DefectType = writeInfo[i].DefectType;
            temp.rect = writeInfo[i].rect;
            temp.ErrorType = writeInfo[i].ErrorType;
            temp.Errorinfo = writeInfo[i].Errorinfo;
            RectData1.push_back(temp);
        }
        if (writeInfo[i].DefectType == 2) {
            drawInformation temp;
            temp.camera_id = writeInfo[i].camera_id;
            temp.DefectType = writeInfo[i].DefectType;
            temp.rect = writeInfo[i].rect;
            temp.ErrorType = writeInfo[i].ErrorType;
            temp.Errorinfo = writeInfo[i].Errorinfo;
            RectData2.push_back(temp);
        }
        if (writeInfo[i].DefectType == 3) {
            drawInformation temp;
            temp.camera_id = writeInfo[i].camera_id;
            temp.DefectType = writeInfo[i].DefectType;
            temp.rect = writeInfo[i].rect;
            temp.ErrorType = writeInfo[i].ErrorType;
            temp.Errorinfo = writeInfo[i].Errorinfo;
            RectData3.push_back(temp);
        }
        if (writeInfo[i].DefectType == 4) {
            drawInformation temp;
            temp.camera_id = writeInfo[i].camera_id;
            temp.DefectType = writeInfo[i].DefectType;
            temp.rect = writeInfo[i].rect;
            temp.ErrorType = writeInfo[i].ErrorType;
            temp.Errorinfo = writeInfo[i].Errorinfo;
            RectData4.push_back(temp);
        }
        if (writeInfo[i].DefectType == 5) {
            drawInformation temp;
            temp.camera_id = writeInfo[i].camera_id;
            temp.DefectType = writeInfo[i].DefectType;
            temp.rect = writeInfo[i].rect;
            temp.ErrorType = writeInfo[i].ErrorType;
            temp.Errorinfo = writeInfo[i].Errorinfo;
            RectData5.push_back(temp);
        }
        if (writeInfo[i].DefectType == 6) {
            drawInformation temp;
            temp.camera_id = writeInfo[i].camera_id;
            temp.DefectType = writeInfo[i].DefectType;
            temp.rect = writeInfo[i].rect;
            temp.ErrorType = writeInfo[i].ErrorType;
            temp.Errorinfo = writeInfo[i].Errorinfo;
            RectData6.push_back(temp);
        }
    }

    //根节点属性  
    //root["1PhotoId"] = Json::Value(PhotoId);
    root["1Level"] = Json::Value(glassLevel);

    //子节点  
    Json::Value DetectInfo = detectInfo(RectData0);
    Json::Value DetectInfo1 = detectInfo(RectData1);
    Json::Value DetectInfo2 = detectInfo(RectData2);
    Json::Value DetectInfo3 = detectInfo(RectData3);
    Json::Value DetectInfo4 = detectInfo(RectData4);
    Json::Value DetectInfo5 = detectInfo(RectData5);
    Json::Value DetectInfo6 = detectInfo(RectData6);

    // 创建一个对象来存储检测信息
    Json::Value detectInfosObject;
    if (!DetectInfo.empty()) {
        detectInfosObject["0"] = DetectInfo;
    }
    if (!DetectInfo1.empty()) {
        detectInfosObject["1"] = DetectInfo1;
    }
    if (!DetectInfo2.empty()) {
        detectInfosObject["2"] = DetectInfo2;
    }
    if (!DetectInfo3.empty()) {
        detectInfosObject["3"] = DetectInfo3;
    }
    if (!DetectInfo4.empty()) {
        detectInfosObject["4"] = DetectInfo4;
    }
    if (!DetectInfo5.empty()) {
        detectInfosObject["5"] = DetectInfo5;
    }
    if (!DetectInfo6.empty()) {
        detectInfosObject["6"] = DetectInfo6;
    }
    /*detectInfosObject["1"] = DetectInfo1;
    detectInfosObject["2"] = DetectInfo2;
    detectInfosObject["3"] = DetectInfo3;
    detectInfosObject["4"] = DetectInfo4;
    detectInfosObject["5"] = DetectInfo5;
    detectInfosObject["6"] = DetectInfo6;*/

    // 子节点对象挂到根节点上
    root["2DetectInfos"] = detectInfosObject;


    //缩进输出  
    Json::StyledWriter sw;
    //std::cout << "StyledWriter:" << std::endl;
    //std::cout << sw.write(root) << std::endl;

    //输出到文件  
    std::ofstream os;
    os.open(PathName + ".json", std::ios::out | std::ios::app);
    if (!os.is_open())
        std::cout << "error：can not find or create the file which named \" demo.json\"." << std::endl;
    os << sw.write(root);
    os.close();

}
