#ifndef ZCONTROLLER_H
#define ZCONTROLLER_H

#include <thread>
#include "MainProcess.h"
//#include "PlatForm.h"

class Plantform;

class ZController
{
public:
	static ZController* GetInstance()
	{
		static ZController controller;
		return &controller;
	}

	~ZController()
	{
		//m_Project = nullptr;
	}

	void Process();

	void NotifyStart();

	void NotifyStop();

	void NotifyStepOnPedal();

	void NotifyReleasePedal();

	void NotifyShowGlassIn();

	void NotifyShowGlassOut();

	void SetWidget(MainProcess* widget)
	{
		m_Project = widget;
	}

private:
	ZController() :
		m_Project(nullptr),
		m_running(false)
	{

	}

private:
	MainProcess*	m_Project;
	std::thread		m_thread;
	bool			m_running;
};

#endif
