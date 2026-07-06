#pragma once

#include <stdio.h>
#include <string>
#include "GeneralMethod.h"
#include "./include/json.h"
class glassData2Json
{
public:
	glassData2Json();
	~glassData2Json();
	int saveData2Json(std::vector<drawInformation> drawInfos, int Level, float GlassWidth, float GlassHeight, std::string savePath);
	std::vector<drawInformation> parseJsonToDrawInfos(const std::string& filePath, float& glassPixelWidth, float& glassPixelHeight);

	void mainTest();

private:



};

