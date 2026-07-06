// CameraFactory.cpp
#include "CameraFactory.h"
#include "LineScanCamera_HR.h"
#include "LineScanCamera_HK.h"

std::shared_ptr<LineScanCamera_Base> CameraFactory::CreateCamera(
    CameraBrand brand,
    int cameraNum,
    void* cameraInfo)
{
    switch (brand)
    {
    case CameraBrand::eHuaray:
        if (cameraInfo)
            return std::make_shared<LineScanCamera_HR>(cameraNum, *static_cast<CameraInfo*>(cameraInfo)->deviceInfo_hr);
        else
            return std::make_shared<LineScanCamera_HR>(cameraNum);

    case CameraBrand::eHikvision:
        if (cameraInfo)
            return std::make_shared<LineScanCamera_HK>(cameraNum, static_cast<CameraInfo*>(cameraInfo)->deviceInfo);
        else
            return std::make_shared<LineScanCamera_HK>(cameraNum);

    default:
        return nullptr;
    }
}

CameraInfoList CameraFactory::EnumDevices(CameraBrand brand)
{
    switch (brand)
    {
    case CameraBrand::eHuaray:
        return LineScanCamera_HR::EnumDevices();

    case CameraBrand::eHikvision:
        return LineScanCamera_HK::EnumDevices();

    default:
        return CameraInfoList();
    }
}

bool CameraFactory::InitSDK(CameraBrand brand)
{
    switch (brand)
    {
    case CameraBrand::eHuaray:
        return LineScanCamera_HR::InitSDK();

    case CameraBrand::eHikvision:
        return LineScanCamera_HK::InitSDK();

    default:
        return false;
    }
}

bool CameraFactory::FinalizeSDK(CameraBrand brand)
{
    switch (brand)
    {
    case CameraBrand::eHuaray:
        return LineScanCamera_HR::FinalizeSDK();

    case CameraBrand::eHikvision:
        return LineScanCamera_HK::FinalizeSDK();

    default:
        return false;
    }
}