#pragma once

#include <iostream>
#include <vector>
#include <atomic>
#include <functional>
#include <mutex>
//--控制板SDK
#include "MCDLL_Define.h"	
#include "MCDLL_NET.h"
#include "MCDLL_Return.h"
//----

class Platform
{
public:
	// 单例访问
	static Platform& Instance()
	{
		static Platform instance;
		return instance;
	}

	// 禁止拷贝
	Platform(const Platform&) = delete;
	Platform& operator=(const Platform&) = delete;

	/* 打开控制卡 */
	bool InitControlCard();

	/* 关闭控制卡 */
	int CloseControlCard();

	bool GetIoInputState(std::vector<unsigned short>& Logic);

	bool SetIOOutState(unsigned short IO_number, unsigned short IO_outputNumber);

	unsigned short GetIOInputState(unsigned short IO_number);

	bool getIOIn_PS();

	void checkIOStatus();

	int mainTest();

	//右侧玻璃发送NG/OK信号
	int ReturnNgOrOK_1(int ngFlag);

	//左侧玻璃发送NG/OK信号
	int ReturnNgOrOK_0(int ngFlag);

	// 返回软件系统状态给控制卡。 0=待机(黄灯)，1=运行中/检测正常(绿灯)，2=检测到NG(红灯)
	int SetStackLight(int status);

private:
	Platform() = default;
	~Platform();

	// 监听线程主函数
	void IOListenerThread();

private:

	/***** 控制板硬件配置 *****/
	unsigned short m_ConnectionNumber		= 1;
	unsigned short m_StationNumber			= 0;			// 设置站点顺序
	unsigned short m_StationType			= 0;			// 设置站号类型
	/*************************/

	std::thread m_listenThread;								// 监听线程
	std::atomic<bool> m_listenRunning{ false };				// 控制线程退出
	std::mutex m_ioMutex;									// 用于保护IO操作

	bool m_isClose = false;

};
