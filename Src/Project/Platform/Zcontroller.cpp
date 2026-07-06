#include "ZController.h"
#include "CameraManager.h"
//#include "PlatForm.h"
#include "Log.hpp"

void ZController::Process()
{
	CameraManager::GetInstance()->Clear();
	//while (m_running)
	//{
	//	CameraManager::GetInstance()->StitchFrameFunc(&m_running);
	//}
}

void ZController::NotifyStart()
{
	m_running = true;
	//2026.01.30 临时取消清理缓存操作
	//CameraManager::GetInstance()->ClearBuffer();	//启动采集前清空缓存
	m_Project ->Sensor_StartVisionGrabbing();
	FILE_LOG_INFO("StartProcess!");
	//m_thread = std::thread(&ZController::Process, this);
}

void ZController::NotifyStop()
{
	m_running = false;
	FILE_LOG_INFO("Process End");
	if (m_thread.joinable())
	{
		m_thread.join();
	}
	
	m_Project->Sensor_StopVisionGrabbing();
}


void ZController::NotifyStepOnPedal()
{
	FILE_LOG_INFO("Step on the pedal");
	
	m_Project->Sensor_StepOnPedal();
}

void ZController::NotifyReleasePedal()
{
	FILE_LOG_INFO("Release the pedal");
	m_Project->Sensor_ReleasePedal();
}

void ZController::NotifyShowGlassIn()
{
	FILE_LOG_INFO("Glass In");
	m_Project->Sensor_ShowGlassIn();
}

void ZController::NotifyShowGlassOut()
{
	FILE_LOG_INFO("Glass Out");
	m_Project->Sensor_ShowGlassOut();
}
