#ifndef RESULT2JSON
#define RESULT2JSON

#include "json.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <fstream>  

#include "GeneralMethod.h"

class result2Json {
public:
    Json::Value detectInfo(std::vector<drawInformation> RectData);
    void readFileJson(std::string readIn, int& glassLevel, std::vector<drawInformation>& InfoData);
    void writeFileJson(std::string PathName, int glassLevel, std::vector<drawInformation> writeInfo);
};

#endif // !RESULT2JSON

