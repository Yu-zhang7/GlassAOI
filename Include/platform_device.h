/*
* 版权所有 © [2024] [湖南科洛德科技有限公司]. 保留所有权利.
*
* 本软件由 [湖南科洛德科技有限公司] 所开发并拥有版权。未经明确书面许可，任何个人或团体不得以任何形式或通过任何手段复制、修改、发布、传播、展示、执行、或以其他方式使用本软件的任何部分。
*
* 本软件受到国际版权法和相关条约的保护。任何未经授权的使用都将面临民事和刑事诉讼的严惩。
*/

#ifndef PLATFORM_DEVICE_H
#define PLATFORM_DEVICE_H
#include <iostream>
#include <string>
#include <functional>
#include <vector>

#ifndef XM_SIGPLATFORM_COMPILE_DLL
#define KY_SIGPLATFORM_EXPORT_SYMBOL __declspec(dllimport)
#else
#define KY_SIGPLATFORM_EXPORT_SYMBOL __declspec(dllexport)
#endif


//mac地址结构定义
#define SIG_MAC_ADDR_SIZE 6
typedef struct _SIG_MAC_ADDR
{
	unsigned char mac_addr[SIG_MAC_ADDR_SIZE];
}SIG_MAC_ADDR;

//ip地址结构定义
#define SIG_MAC_IPV4_SIZE 4
typedef struct _SIG_IPV4_ADDR
{
	unsigned char ip_addr[SIG_MAC_IPV4_SIZE];
}SIG_IPV4_ADDR;

//通讯返回值定义
typedef enum _P_RetValType
{
	P_RET_SUCCESS = 0,
	P_PARAM_ERROR = 1,
	P_TIME_OUT = 2,
	P_PARSE_ERROR = 3,
	P_DEVICE_RET_ERROR = 4,
	P_CONNECTION_ERROR = 5,
	P_TASK_TIMEOUT = 6,
	P_TASK_OP_PAST = 7,
	P_TASK_OP_FULL = 8,
	P_TASK_OP_TOO_LONG = 9
}PlatformRetValType;

#pragma pack(push) //保存对齐状态
#pragma pack(1)
typedef struct _TaskInfo
{
	unsigned int taskSearchID;	//任务搜索ID -- 任务编码值
	unsigned char taskType;		// 任务类型：0x01，固定喷打
	unsigned short portAddr;		//任务端口地址，取值0x01~0x80对应喷打端口0~7.
	unsigned char opCode;		//任务操作码 0x01喷打
	unsigned int opData0;		//喷打点数
	unsigned int opData1;		//喷打点间距
	unsigned int opData2;		//喷打点持续时间
	unsigned int opData3;		//undef
	unsigned int opData4;		//undef
	unsigned int opData5;		//undef
}M_TaskInfo;

typedef struct _CutMsg
{
	unsigned char CutId;			//光电通道号
	unsigned char CodeId;			//编码通道号
	unsigned short CutNum;			//光电计数
	unsigned int LockCodeCount;	//光电锁存编码值
	unsigned int CodeCount;		//编码脉冲计数
	float ZynqTemp;			//温度
}M_CutMsg;
#pragma pack(pop)


//code脉宽结构体定义
#define CODE_PULSE_WIDTH_ITEM_VAL_SIZE 1000
typedef struct _CodePulseWidthItem
{
	unsigned int channel_id;
	unsigned int pulse_width[CODE_PULSE_WIDTH_ITEM_VAL_SIZE];
}CodePulseWidthItem;


///////////////////////////////////////////////////////////////////
/*多IO控制结构体定义 */
#define CONFIG_NUM_PER_IO 8
typedef struct _SPWConfig
{
	int start_tick;
	int high_ns;
	int low_ns;
	int pulse_size;
	int enable;
	_SPWConfig()
	{
		enable = 0;
		pulse_size = 0;
		start_tick = 0;
		low_ns = 0;
		high_ns = 0;
	}
}SPWConfig;

typedef struct _SIOConfig
{
	int enable;
	int io_id;
	SPWConfig pw_conf[CONFIG_NUM_PER_IO];
	_SIOConfig()
	{
		enable = 0;
		io_id = 0;
	}
}SIOConfig;

class MIoConfig
{
public:
	MIoConfig() { period_ns = 5000; };
	std::vector<SIOConfig> m_io_configs;

	int period_ns;
};

KY_SIGPLATFORM_EXPORT_SYMBOL bool readMIOConfigFromPath(MIoConfig & conf, const char * filepath);
KY_SIGPLATFORM_EXPORT_SYMBOL bool writeMIOConfigToPath(MIoConfig & conf, const char * filepath);

KY_SIGPLATFORM_EXPORT_SYMBOL bool serializeMIOConfigToPtr(MIoConfig & conf, char *ptr_dst, unsigned int &copy_len);
KY_SIGPLATFORM_EXPORT_SYMBOL bool deserializeMIOConfigFromPtr(MIoConfig & conf, char *ptr_src, unsigned int &copy_len);
///////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////
/*多场控制结构体定义 */
#define SINGLE_IO_WIDTH_NUM 4
#define IO_GROUP_SIZE 16
#define IO_AMOUNT_NUM 20
#define ILLUM_AMOUNT_NUM 16
#define SYNC_EXPOSURE_AMOUNT 8

typedef struct _SingleIOSta
{
	unsigned char out_width_sta[SINGLE_IO_WIDTH_NUM];
	unsigned int out_width_val[SINGLE_IO_WIDTH_NUM];
}SingleIOStatus;

typedef struct _AllIOSta
{
	SingleIOStatus all_io_status[IO_AMOUNT_NUM];
}AllIOStatus;

typedef struct _AllSyncExpoWidth
{
	unsigned int sync_width[SYNC_EXPOSURE_AMOUNT];
	unsigned int exposure_width[SYNC_EXPOSURE_AMOUNT];
}AllSyncExpoWidth;

typedef struct _SingleIOSetting
{
	unsigned char valid;
	unsigned char use_width_idx;
}SingleIOSetting;


typedef struct _SingleIllumSetting
{
	SingleIOSetting io_seq[IO_AMOUNT_NUM];
	unsigned int sync_sel;
	unsigned int exposure_sel;
}SingleIllumSetting;


typedef struct _IllumProp
{
	unsigned int illum_size;
	unsigned int period_width;
	unsigned int sync_sel_mode;
	unsigned int sync_out_mode;
	unsigned int light_delay;
	unsigned int sync_delay;
	unsigned int code_k_reg;
	unsigned int code_min_period_limit;
}IllumProp;

typedef struct _xmIllumChannelSetting
{
	IllumProp illum_prop;
	AllIOStatus io_status;
	SingleIllumSetting illum_setting[ILLUM_AMOUNT_NUM];
	AllSyncExpoWidth sync_expo_width;
}xmIllumChannelSetting;

#define PLATFORM_CHANNEL_SIZE 2
typedef struct _xmIllumPlatformSetting
{
	xmIllumChannelSetting m_channel[PLATFORM_CHANNEL_SIZE];
}xmIllumPlatformSetting;

KY_SIGPLATFORM_EXPORT_SYMBOL bool write_illumm_channel_setting_to_file(std::ofstream &outfile, xmIllumChannelSetting &illum_ch_setting);
KY_SIGPLATFORM_EXPORT_SYMBOL bool read_illumm_channel_setting_from_file(std::ifstream &infile, xmIllumChannelSetting &illum_ch_setting);

KY_SIGPLATFORM_EXPORT_SYMBOL bool write_illum_setting_to_path(const char * filepath, xmIllumPlatformSetting &setting);
KY_SIGPLATFORM_EXPORT_SYMBOL bool read_illum_setting_from_path(const char * filepath, xmIllumPlatformSetting &setting);

KY_SIGPLATFORM_EXPORT_SYMBOL bool write_illum_channel_setting_to_path(const char * filepath, xmIllumChannelSetting &setting);
KY_SIGPLATFORM_EXPORT_SYMBOL bool read_illum_channel_setting_from_path(const char * filepath, xmIllumChannelSetting &setting);

KY_SIGPLATFORM_EXPORT_SYMBOL bool serialize_illum_setting(xmIllumChannelSetting &illum_ch_setting, char * ptr_dst, unsigned int buff_size, unsigned int &out_len);
KY_SIGPLATFORM_EXPORT_SYMBOL bool deserialize_illum_setting(xmIllumChannelSetting &illum_ch_setting, char * ptr_src);
///////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
/*信号平台的类定义*/
class platform_dev_pack;
typedef std::function<void(unsigned int board_status, unsigned char* pdata)>  platform_board_callback;
class platform_device 
{
public:
	KY_SIGPLATFORM_EXPORT_SYMBOL platform_device();
	KY_SIGPLATFORM_EXPORT_SYMBOL ~platform_device();

	//初始化设备
	KY_SIGPLATFORM_EXPORT_SYMBOL int init_device(); 

	//释放设备
	KY_SIGPLATFORM_EXPORT_SYMBOL int release_device(); 

	//打开设备
	KY_SIGPLATFORM_EXPORT_SYMBOL int open_device();    

	//关闭设备
	KY_SIGPLATFORM_EXPORT_SYMBOL int close_device();   

	//获取连接状态
	KY_SIGPLATFORM_EXPORT_SYMBOL bool get_open_stat();

	//获取连接IP地址
	KY_SIGPLATFORM_EXPORT_SYMBOL std::string get_ps_server_ip();

	//设置连接IP地址
	KY_SIGPLATFORM_EXPORT_SYMBOL void set_ps_server_ip(const char *ip_addr);

	//下发数据
	KY_SIGPLATFORM_EXPORT_SYMBOL bool send_data_to_general_client(char * send_data, unsigned int send_len, char *rev_data, unsigned int buff_len, unsigned int &rev_len);

	//设置光源配置
	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType set_illum_setting(unsigned short channel_id, xmIllumChannelSetting &channel_setting);

	//获取光源配置
	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType get_illum_setting(unsigned short channel_id, xmIllumChannelSetting &channel_setting);

	//设置MIO配置
	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType set_miopw_setting(MIoConfig &miopw_conf );
	
	//获取MIO配置
	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType get_miopw_setting( MIoConfig &miopw_conf);

	//清除MIO配置
	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType clear_miopw();

	//获取当前配置
	KY_SIGPLATFORM_EXPORT_SYMBOL bool get_cur_code(unsigned int &cur_code);

	//设置回调函数
	KY_SIGPLATFORM_EXPORT_SYMBOL void set_boardinfo_callback(platform_board_callback pcall_back);

	//通过地址获取寄存器的值
	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType get_reg_val(unsigned short addr_s, unsigned short addr_e, unsigned int * pRetVal);

	//保存所有的参数
	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType save_all_param();

	//通过地址修改寄存器的值
	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType mod_reg_val(unsigned short addr_s, unsigned short addr_e, const unsigned int * pSetVal);

	//下发任务
	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType write_task(const M_TaskInfo & taskInfo);

	//清除所有任务
	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType clear_all_task();

	//开启或关闭编码信号脉宽功能
	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType start_code_pulse_func(bool start);
	
	//获取CodePulseItem
	KY_SIGPLATFORM_EXPORT_SYMBOL int get_code_pulse_item(CodePulseWidthItem &item);

	//内部函数
	KY_SIGPLATFORM_EXPORT_SYMBOL void run_boardinfo(unsigned int board_status, unsigned char* pdata);

	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType get_dev_net_ip(SIG_IPV4_ADDR &ip_addr);

	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType get_dev_net_mac(SIG_MAC_ADDR &mac_addr);

	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType mode_dev_net_ip(const SIG_IPV4_ADDR ip_addr);

	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType mode_dev_net_mac(const SIG_MAC_ADDR mac_addr);

	KY_SIGPLATFORM_EXPORT_SYMBOL PlatformRetValType clear_task(int idx);

private:
	int normal_judge();
	
private:
	platform_dev_pack * m_ptr_pack;
	std::string m_platform_server_ip;
	platform_board_callback m_boardinfo_callback;

	bool m_connect_stat;
	bool m_init_flag;
};

#endif