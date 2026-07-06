#include "Platform.h"
#include <thread>
#include "ZController.h"	//控制类
#include "Log.hpp"


Platform::~Platform()
{

	/*****************关闭光源开关(后续亮灯由硬触发自动处理)***************************/
	bool bRst_close = Platform::Instance().SetIOOutState(0, Bit_Output_Open);
	if (!bRst_close)
	{
		FILE_LOG_ERROR(u8"控制卡关闭光源指令发送失败");
	}
	else
	{
		FILE_LOG_INFO(u8"控制卡关闭光源指令发送成功");
	}
	/***********************************************************************************/


	//std::cout << "Platform::~Platform() begin" << std::endl;
	CloseControlCard();
	//std::cout << "Platform::~Platform() over" << std::endl;
}

bool Platform::InitControlCard()
{
	// 打开控制卡
	if (IS_READ_MODE == 0)
	{
		short rtn = MCF_Open_Net(m_ConnectionNumber, &m_StationNumber, &m_StationType);
		if (rtn != 0)
		{
			FILE_LOG_ERROR(u8"控制卡打开失败");
			return false;
		}
		FILE_LOG_INFO(u8"控制卡打开成功");
	}
	// 启动监听线程
	m_listenRunning = true;
	try
	{
		m_listenThread = std::thread(&Platform::IOListenerThread, this);
		//std::cout << "IO监听线程已启动" << std::endl;
		FILE_LOG_INFO(u8"IO监听线程已启动");
	}
	catch (const std::exception& e)
	{
		//std::cerr << "监听线程启动失败: " << e.what() << std::endl;
		FILE_LOG_ERROR(u8"监听线程启动失败");
		m_listenRunning = false;
		return false;
	}


	/*****************关闭光源开关(后续亮灯由硬触发自动处理)***************************/
	bool bRst_close = Platform::Instance().SetIOOutState(0, Bit_Output_Open);
	if (!bRst_close)
	{
		FILE_LOG_ERROR(u8"控制卡关闭光源指令发送失败");
	}
	else
	{
		FILE_LOG_INFO(u8"控制卡关闭光源指令发送成功");
	}
	/***********************************************************************************/

	return true;
	//关闭光源
    //MCF_Set_Output_Bit_Net(0, Bit_Output_Close, Station_Number);
}

int Platform::CloseControlCard()
{
	// 设置标志位，通知线程退出
	m_listenRunning = false;
	// 等待线程结束
	if (m_listenThread.joinable())
	{
		m_listenThread.join();
	}
	MCF_Close_Net();
	FILE_LOG_INFO(u8"控制板已关闭");
	return 0;
}

bool Platform::GetIoInputState(std::vector<unsigned short>& Logic) {
	unsigned short rtn;
	Logic.resize(8);

	for (int i = 0; i < Logic.size(); i++) {
		unsigned short Input;
		rtn = MCF_Get_Input_Bit_Net((unsigned short)i, &Input, m_StationNumber);
		Logic[i] = Input;
		if (rtn != 0) {
			return false;
		}
	}
	return true;
}

/// <summary>
/// 输出端口写入操作，控制光源
/// </summary>
/// <param name="IO_number"></param>
/// <param name="IO_outputNumber"></param>
/// <returns></returns>
bool Platform::SetIOOutState(unsigned short IO_number, unsigned short IO_outputNumber) {

	unsigned short rtn;
	
	rtn = MCF_Set_Output_Bit_Net(IO_number, IO_outputNumber, m_StationNumber);
	
	if (rtn != 0) {
		FILE_LOG_ERROR(u8"控制卡RecvSignal: MCF_Set_Output_Bit_Net  失败");
		return false;
	}
	FILE_LOG_INFO(u8"控制卡RecvSignal: MCF_Set_Output_Bit_Net  完成");
	return true;
}

/// <summary>
/// 获取输入端口状态
/// </summary>
/// <param name="IO_number"></param>
/// <returns></returns>
unsigned short Platform::GetIOInputState(unsigned short IO_number) {
	bool state_PS = false;
	unsigned short rtn;
	unsigned short Input;
	rtn = MCF_Get_Input_Bit_Net(IO_number, &Input, m_StationNumber);
	
	return Input;
}

/// <summary>
/// 获取光电开关状态，即仅获取0号输入端口状态
/// </summary>
/// <returns></returns>
bool Platform::getIOIn_PS() {
	bool state_PS = false;
	unsigned short Input;
	unsigned short rtn;

	rtn = MCF_Get_Input_Bit_Net(0, &Input, m_StationNumber);

	if (Input == 0) {

		std::cout << "Flag: Open" << std::endl;
		state_PS = true;
	}
	else {
		std::cout << "Flag: Close" << std::endl;
	}

	return state_PS;
}

void Platform::checkIOStatus()
{

}

int Platform::mainTest()
{
	return 0;
}

/// <summary>
/// ngFlag 1：正常，0：NG
/// </summary>
/// <param name="ngFlag"></param>
/// <returns></returns>
int Platform::ReturnNgOrOK_1(int ngFlag)
{
	if (ngFlag == 1) {//ok
		//FileLogPrintf(u8"PlatForm write info(OK):6.");
		//unsigned short Bit_Output_Number = 6;
		//bool bRst = SetIOOutState(Bit_Output_Number, Bit_Output_Close);
		//if (!bRst)
		//{
		//	FileLogPrintf(u8"控制卡发送指令失败:ReturnNgOrOK(ok/0).");
		//	return 1;
		//}
		//else {
		//	FileLogPrintf(u8"success:ReturnNgOrOK(ok/0).");
		//}
		//Sleep(500);
		//bRst = SetIOOutState(Bit_Output_Number, Bit_Output_Open);
		//if (!bRst)
		//{
		//	FileLogPrintf(u8"控制卡发送指令失败:ReturnNgOrOK(ok/1).");
		//	return 1;
		//}
		//else 
		//{
		//	FileLogPrintf(u8"sucess:ReturnNgOrOK(ok/1).");
		//}

		FILE_LOG_INFO(u8"PlatForm write info(NG Open):7.");
		unsigned short Bit_Output_Number = 7;
		bool bRst = SetIOOutState(Bit_Output_Number, Bit_Output_Open);
		if (!bRst)
		{
			FILE_LOG_ERROR(u8"控制卡发送指令失败:[1]ReturnNgOrOK(ng/0).");
			return 1;
		}
		else
		{
			FILE_LOG_INFO(u8"sucess:[1]ReturnNgOrOK(ok/1).");
		}
	
	}
	else {//ng
		//FileLogPrintf(u8"PlatForm write info(NG):7.");
		//unsigned short Bit_Output_Number = 7;
		//bool bRst = SetIOOutState(Bit_Output_Number, Bit_Output_Close);
		//if (!bRst)
		//{
		//	FILE_LOG_ERROR(u8"控制卡发送指令失败:ReturnNgOrOK(ng/0).");
		//	return 1;
		//}
		//else {
		//	FILE_LOG_INFO(u8"success:ReturnNgOrOK(ok/0).");
		//}
		//Sleep(500);
		//bRst = SetIOOutState(Bit_Output_Number, Bit_Output_Open);
		//if (!bRst)
		//{
		//	FILE_LOG_ERROR(u8"控制卡发送指令失败:ReturnNgOrOK(ng/0).");
		//	return 1;
		//}
		//else
		//{
		//	FileLogPrintf(u8"sucess:ReturnNgOrOK(ok/1).");
		//}


		FILE_LOG_INFO(u8"PlatForm write info(NG Close):7.");
		unsigned short Bit_Output_Number = 7;
		bool bRst = SetIOOutState(Bit_Output_Number, Bit_Output_Close);
		if (!bRst)
		{
			FILE_LOG_ERROR(u8"控制卡发送指令失败:[1]ReturnNgOrOK(ng/0).");
			return 1;
		}
		else {
			FILE_LOG_INFO(u8"success:[1]ReturnNgOrOK(ok/0).");
		}
	}

	return 0;
}

/// <summary>
/// ngFlag 1：正常，0：NG
/// </summary>
/// <param name="ngFlag"></param>
/// <returns></returns>
int Platform::ReturnNgOrOK_0(int ngFlag)
{
	if (ngFlag == 1) {
		//ok
		//FileLogPrintf(u8"PlatForm write info(OK):6.");
		//unsigned short Bit_Output_Number = 6;
		//bool bRst = SetIOOutState(Bit_Output_Number, Bit_Output_Close);
		//if (!bRst)
		//{
		//	FileLogPrintf(u8"控制卡发送指令失败:ReturnNgOrOK(ok/0).");
		//	return 1;
		//}
		//else {
		//	FileLogPrintf(u8"success:ReturnNgOrOK(ok/0).");
		//}
		//Sleep(500);
		//bRst = SetIOOutState(Bit_Output_Number, Bit_Output_Open);
		//if (!bRst)
		//{
		//	FileLogPrintf(u8"控制卡发送指令失败:ReturnNgOrOK(ok/1).");
		//	return 1;
		//}
		//else 
		//{
		//	FileLogPrintf(u8"sucess:ReturnNgOrOK(ok/1).");
		//}

		FILE_LOG_INFO(u8"PlatForm write info(NG Open):6.");
		unsigned short Bit_Output_Number = 6;
		bool bRst = SetIOOutState(Bit_Output_Number, Bit_Output_Open);
		if (!bRst)
		{
			FILE_LOG_ERROR(u8"控制卡发送指令失败:[0]ReturnNgOrOK(ng/0).");
			return 1;
		}
		else
		{
			FILE_LOG_INFO(u8"sucess:[0]ReturnNgOrOK(ok/1).");
		}

	}
	else {
		//ng
		//FileLogPrintf(u8"PlatForm write info(NG):7.");
		//unsigned short Bit_Output_Number = 7;
		//bool bRst = SetIOOutState(Bit_Output_Number, Bit_Output_Close);
		//if (!bRst)
		//{
		//	FILE_LOG_ERROR(u8"控制卡发送指令失败:ReturnNgOrOK(ng/0).");
		//	return 1;
		//}
		//else {
		//	FILE_LOG_INFO(u8"success:ReturnNgOrOK(ok/0).");
		//}
		//Sleep(500);
		//bRst = SetIOOutState(Bit_Output_Number, Bit_Output_Open);
		//if (!bRst)
		//{
		//	FILE_LOG_ERROR(u8"控制卡发送指令失败:ReturnNgOrOK(ng/0).");
		//	return 1;
		//}
		//else
		//{
		//	FileLogPrintf(u8"sucess:ReturnNgOrOK(ok/1).");
		//}

		FILE_LOG_INFO(u8"PlatForm write info(NG Close):6.");
		unsigned short Bit_Output_Number = 6;
		bool bRst = SetIOOutState(Bit_Output_Number, Bit_Output_Close);
		if (!bRst)
		{
			FILE_LOG_ERROR(u8"控制卡发送指令失败:[0]ReturnNgOrOK(ng/0).");
			return 1;
		}
		else {
			FILE_LOG_INFO(u8"success:[0]ReturnNgOrOK(ok/0).");
		}
	}

	return 0;
}

/// <summary>
/// 返回软件系统状态给控制卡。
/// 0 = 待机（黄灯亮）
/// 1 = 运行中/检测正常（绿灯亮）
/// 2 = 检测到NG（红灯亮）
/// </summary>
int Platform::SetStackLight(int status)
{
	// 定义：每个状态 [红, 黄, 绿] 的输出值（true=开，false=关）
	static const bool statusLampMap[4][3] = {
		{false, true,  false}, // status 0: 黄灯亮
		{false, false, true }, // status 1: 绿灯亮
		{true,  false, false},  // status 2: 红灯亮
		{false, false, false}	// status 3: 全灭
	};

	static const char* statusNames[] = { "WAITING", "OK", "NG" };
	static const unsigned short lampAddresses[3] = { 2, 3, 4 }; // 红=2, 黄=3, 绿=4

	if (status < 0 || status > 3) {
		FILE_LOG_ERROR(u8"无效的运行状态码: %d", status);
		return -1; // 或抛异常，根据设计
	}

	FILE_LOG_INFO(u8"PlatForm write running status. %s", statusNames[status]);

	for (int i = 0; i < 3; ++i) {
		unsigned short addr = lampAddresses[i];
		bool targetState = statusLampMap[status][i];
		bool isOpen = targetState; // true 表示打开（亮）

		bool bRst = SetIOOutState(addr, isOpen ? Bit_Output_Close : Bit_Output_Open);//如果状态相反，则对调Bit_Output_Open和Bit_Output_Close
		const char* lampName = (i == 0) ? u8"红灯" : (i == 1 ? u8"黄灯" : u8"绿灯");
		const char* action = isOpen ? u8"打开" : u8"关闭";

		if (!bRst) {
			FILE_LOG_ERROR(u8"控制卡发送指令失败: [%d] %s%s失败.", status, action, lampName);
			return 1;
		}
		else {
			FILE_LOG_INFO(u8"success: [%d] %s%s成功.", status, action, lampName);
		}
	}

	return 0;
}

// 监听线程函数
void Platform::IOListenerThread()
{
	//REGISTER_THREAD_NAME("Platform_IOListenerThread"); //用于分析线程崩溃时的调用栈
	FILE_LOG_INFO(u8"Platform::IOListenerThread begin!");
	int flag = 0;
	bool glassIn_second = false;
	bool glassOut_first = true;

	while (m_listenRunning)
	{
		// 使用锁保护IO操作
		std::lock_guard<std::mutex> lock(m_ioMutex);

		unsigned short Input;
		if (IS_READ_MODE == 0)
		{
			MCF_Get_Input_Bit_Net(0, &Input, m_StationNumber);
		}
		else
		{
			Input = READ_MODE_PORCESS_STATUS; //模拟模式下，输入始终为1
			if (READ_MODE_PORCESS_STATUS != -1)
			{
				READ_MODE_PORCESS_STATUS = -1;
			}
		}

		if (!m_isWorking)
		{
			//FILE_LOG_WARN("[Process]StartProcess Error: Machine stopped. Detection not running.");
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		if (Input == 0&& flag == 0 && isGlassStopGrabbing) {
			FILE_LOG_INFO(u8"控制卡RecvSignal: GetInput = 0, 收到控制卡板进信号，");
			glassOut_first = false;
			ZController::GetInstance()->NotifyShowGlassIn();
			//FILE_LOG_INFO(u8"控制卡RecvSignal: GetInput = 0，准备开始采集。Sleep=%f", (DelayStartTime * 1000) );
			//std::this_thread::sleep_for(std::chrono::milliseconds(int(1000 * DelayStartTime)));//20260129：按现场意见添加。延时采集开始时间
			////Sleep(1000 * DelayStartTime);//20260129：按现场意见添加。延时采集开始时间
			//FILE_LOG_INFO(u8"控制卡RecvSignal: GetInput = 0，开始采集");
			ZController::GetInstance()->NotifyStart();//开始采集	//2026.02.05 延迟开始采集放在MainProcess::StartProcess中

			glassIn_second = false;
			////////unsigned short Bit_Output_Number = 0;
			////////bool bRst = SetIOOutState(Bit_Output_Number, Bit_Output_Close);
			////////if (!bRst)
			////////{
			////////	FILE_LOG_ERROR(u8"控制卡发送指令失败");
			////////}
			////////else
			////////{
			////////	FILE_LOG_INFO(u8"控制卡发送指令成功");
			////////}
			flag = 1;
		}
		else if (Input == 0 && flag == 0 && !isGlassStopGrabbing)	//第一块未结束，第二块来了,只做标记，等第一块结束后，再开始第二块的采集
		{
			ZController::GetInstance()->NotifyShowGlassIn();
			FILE_LOG_INFO(u8"控制卡RecvSignal: GetInput = 0, 当前玻璃采集进行中，但是，已收到下一块玻璃板进信号，");
			glassIn_second = true;	//第二块板进
			
		}
		else if (Input == 1 && flag == 1) {
			glassOut_first = true;	//第一块板出
			FILE_LOG_INFO(u8"控制卡RecvSignal: GetInput = 1,收到控制卡板出信号，");
			ZController::GetInstance()->NotifyShowGlassOut();
			ZController::GetInstance()->NotifyStop();	//2026.02.05 延迟停止采集放在MainProcess::StopProcess中
			flag = 0;
		}
		else
		{
			//FILE_LOG_ERROR("ListenThread: Error!");
		}
		if (glassIn_second && glassOut_first)	//第一块结束后，第二块已在第一块结束前到达，则 在第二块结束后，直接开始第二块的采集
		{
			glassOut_first = false;
			glassIn_second = false;
			std::this_thread::sleep_for(std::chrono::milliseconds(300));	//等待300ms，确保第一块完全结束
			
			ZController::GetInstance()->NotifyStart();//开始采集
			flag = 1;
		}
		// 休眠一段时间，避免CPU占用过高
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	FILE_LOG_INFO(u8"Platform::IOListenerThread end!");
}

