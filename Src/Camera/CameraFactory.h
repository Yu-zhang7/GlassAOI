// CameraFactory.h
#ifndef CAMERAFACTORY_H
#define CAMERAFACTORY_H

#include "LineScanCamera_Base.h"
#include <memory>

enum class CameraBrand
{
    eHikvision,
    eHuaray
};

class CameraFactory
{
public:
    static std::shared_ptr<LineScanCamera_Base> CreateCamera(
        CameraBrand brand,
        int cameraNum,
        void* cameraInfo = nullptr);

    static CameraInfoList EnumDevices(CameraBrand brand);
    static bool InitSDK(CameraBrand brand);
    static bool FinalizeSDK(CameraBrand brand);
};

#endif