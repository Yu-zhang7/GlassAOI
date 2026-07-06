#if !defined(_GMC_4D_PCI_H_1C474833_BDCF_826DF5_INCLUDED_)
#define _GMC_4D_PCI_H_1C474833_BDCF_826DF5_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

#if _WIN32_WINNT < _WIN32_WINNT_WIN2K
#error MXMPAC need _WIN32_WINNT >= _WIN32_WINNT_WIN2K.
#endif

#include <wtypes.h>

#ifdef __cplusplus
extern "C" {
#endif
/********************************************************************************************************************************************************************
                                                       1 控制卡打开函数
********************************************************************************************************************************************************************/
//1.0 网络并联模式设置函数(打开卡前设置)               宏定义1.0
short WINAPI MCF_Set_Switch_State_Net                 (unsigned short Mode = 0); 
//    网卡设置函数(打开卡前设置)                       0:自动搜索网卡  [1,]:用户指定网卡 
short WINAPI MCF_Set_Card_Number_Net                  (unsigned short Card_Number = 0); 
short WINAPI MCF_Get_Card_Number_Net                  (unsigned short *Card_Number); 
//1.1 初始化函数                                       [1,100]                          [0,99]                          宏定义1.1                  
short WINAPI MCF_Open_Net                             (unsigned short Connection_Number,unsigned short *Station_Number, unsigned short *Station_Type);  
short WINAPI MCF_Get_Open_Net                         (unsigned short *Connection_Number,unsigned short *Station_Number, unsigned short *Station_Type);    
short WINAPI MCF_Close_Net                            ();   
//1.2 链接超时紧急停止函数                             [0,60000]
short WINAPI MCF_Set_Link_TimeOut_Net                 (unsigned long Time_1MS,unsigned long TimeOut_Output,unsigned short StationNumber = 0); 
//    链接超时紧急停止触发使能函数        
short WINAPI MCF_Set_Trigger_Output_Bit_Net           (unsigned short Bit_Output_Number,unsigned short Bit_Output_Enable,unsigned short StationNumber = 0);
//1.3 链接监测函数
short WINAPI MCF_Get_Link_State_Net                   (unsigned short StationNumber = 0);  

/**************************************************************************************************************************************
                                                      2 通用输入输出函数
********************************************************************************************************************************************************************/
//2.1 通用IO全部输出函数                               [OUT31,OUT0]                     [0,99]        
short WINAPI MCF_Set_Output_Net                       (unsigned long  All_Output_Logic, unsigned short StationNumber = 0);                                                    
short WINAPI MCF_Get_Output_Net                       (unsigned long *All_Output_Logic, unsigned short StationNumber = 0);       
//2.2 通用IO按位输出函数                               宏定义2.3.1                      宏定义2.3.2                      [0,99]  
short WINAPI MCF_Set_Output_Bit_Net                   (unsigned short Bit_Output_Number,unsigned short Bit_Output_Logic, unsigned short StationNumber = 0);                                                           
short WINAPI MCF_Get_Output_Bit_Net                   (unsigned short Bit_Output_Number,unsigned short *Bit_Output_Logic,unsigned short StationNumber = 0);  
//2.3 通用IO输出复用1：按位输出保持时间函数            宏定义2.3.1                      宏定义2.3.2                      [0,65535]                       [0,99]  
short WINAPI MCF_Set_Output_Time_Bit_Net              (unsigned short Bit_Output_Number,unsigned short Bit_Output_Logic, unsigned short Output_Time_1MS, unsigned short StationNumber = 0);
//    通用IO输出复用2：按位置输出保持时间函数          宏定义2.3.1                           [0,1000]                          [-2^31,(2^31-1)]            [0,99]
short WINAPI MCF_Set_Compare_Output_Bit_Net           (unsigned short Compare_Output_Number, unsigned short Compare_Output_1MS,unsigned long Compare_dDist,unsigned short StationNumber = 0);
//2.4 通用IO全部输入函数                               [Input31,Input0]                 [Input48,Input32]               [0,99]  
short WINAPI MCF_Get_Input_Net                        (unsigned long *All_Input_Logic1, unsigned long *All_Input_Logic2,unsigned short StationNumber = 0); 
//2.5 通用IO按位输入函数                               宏定义2.4.1                      宏定义2.4.2                     [0,99]  
short WINAPI MCF_Get_Input_Bit_Net                    (unsigned short Bit_Input_Number, unsigned short *Bit_Input_Logic,unsigned short StationNumber = 0);  

//2.6 通用IO按位输入下升沿高速捕获清除函数             [Bit_Input_0,Bit_Input_3]        [0,99] 
short WINAPI MCF_Clear_Input_Fall_Bit_Net             (unsigned short Bit_Input_Number, unsigned short StationNumber = 0); 
//2.7 通用IO按位输入下升沿高速捕获读取函数             [Bit_Input_0,Bit_Input_3]        宏定义2.7                      [0,99] 
short WINAPI MCF_Get_Input_Fall_Bit_Net               (unsigned short Bit_Input_Number, unsigned short *Bit_Input_Fall,unsigned short StationNumber = 0); 
//2.9 通用IO按位输入下升沿高速计数读取函数             [Bit_Input_0,Bit_Input_3]        [0,(2^32-1)]                    10个最新编码器锁存数据 &Array[10]     [0,99] 
short WINAPI MCF_Get_Input_Fall_Count_Bit_Net         (unsigned short Bit_Input_Number, unsigned long *Input_Count_Fall,unsigned long *Lock_Data_Buffer,unsigned short StationNumber = 0); 

//2.10 通用IO按位输入数据锁存保持(最近10个下升沿数据)打开函数(必须在MCF_Open_Net前面提前调用)                                    
short WINAPI MCF_Open_Input_Lock_Bit_Net              (unsigned short Lock_Mode = 0,unsigned short StationNumber = 0); 
//2.11 通用IO按位输入滤波函数                          [Bit_Input_0,Bit_Input_3]        [1,100]MS                     [0,99] 
short WINAPI MCF_Set_Input_Filter_Time_Bit_Net        (unsigned short Bit_Input_Number, unsigned long Filter_Time_1MS,unsigned short StationNumber = 0); 
/********************************************************************************************************************************************************************
                                                      3 轴专用输入输出函数
********************************************************************************************************************************************************************/
//3.1 伺服使能设置函数                                 宏定义0.0           宏定义3.1                   [0,99] 
short WINAPI MCF_Set_Servo_Enable_Net                 (unsigned short Axis,unsigned short  Servo_Logic,unsigned short StationNumber = 0); 
short WINAPI MCF_Get_Servo_Enable_Net                 (unsigned short Axis,unsigned short *Servo_Logic,unsigned short StationNumber = 0);
//3.2 伺服报警复位设置函数                             宏定义0.0           宏定义3.2                   [0,99] 
short WINAPI MCF_Set_Servo_Alarm_Reset_Net            (unsigned short Axis,unsigned short  Alarm_Logic,unsigned short StationNumber = 0);    
short WINAPI MCF_Get_Servo_Alarm_Reset_Net            (unsigned short Axis,unsigned short *Alarm_Logic,unsigned short StationNumber = 0); 
//3.3 伺服报警输入获取函数                             宏定义0.0           宏定义3.3                            [0,99] 
short WINAPI MCF_Get_Servo_Alarm_Net                  (unsigned short Axis,unsigned short *Servo_Alarm_State,   unsigned short StationNumber = 0);
//3.4 伺服定位完成输入获取函数                         宏定义0.0           宏定义3.4                            [0,99]
short WINAPI MCF_Get_Servo_INP_Net                    (unsigned short Axis,unsigned short *Servo_INP_State,     unsigned short StationNumber = 0);
//3.5 编码器Z相输入获取函数                            宏定义0.0           宏定义3.5                            [0,99]
short WINAPI MCF_Get_Z_Net                            (unsigned short Axis,unsigned short *Z_State,   unsigned short StationNumber = 0);
//3.6 原点输入获取函数                                 宏定义0.0           宏定义3.6                            [0,99] 
short WINAPI MCF_Get_Home_Net                         (unsigned short Axis,unsigned short *Home_State,          unsigned short StationNumber = 0);
//3.7 正限位输入获取函数                               宏定义0.0           宏定义3.7                            [0,99] 
short WINAPI MCF_Get_Positive_Limit_Net               (unsigned short Axis,unsigned short *Positive_Limit_State,unsigned short StationNumber = 0); 
//3.8 负限位输入获取函数                               宏定义0.0           宏定义3.8                            [0,99] 
short WINAPI MCF_Get_Negative_Limit_Net               (unsigned short Axis,unsigned short *Negative_Limit_State,unsigned short StationNumber = 0);    
/********************************************************************************************************************************************************************
                                                      4 轴设置函数
********************************************************************************************************************************************************************/
//4.1 脉冲通道输出设置函数                             宏定义0.0           宏定义4.1                 [0,99] 
short WINAPI MCF_Set_Pulse_Mode_Net                   (unsigned short Axis,unsigned long  Pulse_Mode,unsigned short StationNumber = 0);                                                       
short WINAPI MCF_Get_Pulse_Mode_Net                   (unsigned short Axis,unsigned long *Pulse_Mode,unsigned short StationNumber = 0);    
//4.2 位置设置函数                                     宏定义0.0           [-2^31,(2^31-1)] [0,99] 
short WINAPI MCF_Set_Position_Net                     (unsigned short Axis,long  Position,  unsigned short StationNumber = 0);                                                         
short WINAPI MCF_Get_Position_Net                     (unsigned short Axis,long *Position,  unsigned short StationNumber = 0);   
//4.3 编码器设置函数                                  宏定义0.0           [-2^31,(2^31-1)] [0,99]  
short WINAPI MCF_Set_Encoder_Net                      (unsigned short Axis,long  Encoder,   unsigned short StationNumber = 0);                                                            
short WINAPI MCF_Get_Encoder_Net                      (unsigned short Axis,long *Encoder,   unsigned short StationNumber = 0);  
//    通过Z相清除AB编码器值
short WINAPI MCF_Z_Clear_Encoder_Net                  (unsigned short Axis,unsigned short Enable,unsigned short StationNumber = 0);
//    通过Z相后固定距离输出IO                         宏定义0.0            [0,255]              [0,65535]            [0,255]
short WINAPI MCF_Z_Output_Bit_Net                     (unsigned short Axis,unsigned short Number,unsigned long dDist,unsigned short Time_1MS,unsigned short StationNumber = 0);
//4.4 速度获取                                        宏定义0.0           [-2^15,(2^15-1)]    [-2^15,(2^15-1)]   [0,99] 
short WINAPI MCF_Get_Vel_Net                          (unsigned short Axis,double *Command_Vel,double *Encode_Vel,unsigned short StationNumber = 0);        
/********************************************************************************************************************************************************************
                                                      5 轴硬件触发停止运动函数
********************************************************************************************************************************************************************/
//5.1 通用IO输入复用：做为紧急停止函数                 宏定义2.4.1                      宏定义5.1                [0,99] 
short WINAPI MCF_Set_EMG_Bit_Net                      (unsigned short EMG_Input_Number, unsigned short  EMG_Mode,unsigned short StationNumber = 0); 
short WINAPI MCF_Set_EMG_Output_Net                   (unsigned short EMG_Input_Number, unsigned short  EMG_Mode,unsigned long EMG_Output,unsigned short StationNumber = 0);     
short WINAPI MCF_Set_EMG_Output_Enable_Net            (unsigned short Bit_Output_Number,unsigned short Bit_Output_Enable,unsigned short StationNumber = 0);
//    通用IO输入复用：做为触发停止                    [0,3]                   宏定义0.0            [Bit_Input_0,Bit_Input_15]       宏定义5.4                   [0,99] 
short WINAPI MCF_Set_Input_Trigger_Net                (unsigned short Channel,unsigned short  Axis,unsigned short  Bit_Input_Number,unsigned long  Trigger_Mode,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Input_Trigger_Net                (unsigned short Channel,unsigned short *Axis,unsigned short *Bit_Input_Number,unsigned long *Trigger_Mode,unsigned short StationNumber = 0);
//5.2 软件限位触发运动停止函数                         宏定义0.0           [-2^31,2^31]P     >     [-2^31,2^31]P          [0,99] 
short WINAPI MCF_Set_Soft_Limit_Net                   (unsigned short Axis,long  Positive_Position,long  Negative_Position,unsigned short StationNumber = 0);                           
short WINAPI MCF_Get_Soft_Limit_Net                   (unsigned short Axis,long *Positive_Position,long *Negative_Position,unsigned short StationNumber = 0);
//5.3 软件限位触发运动停止开关函数                     宏定义0.0           宏定义5.3                        [0,99] 
short WINAPI MCF_Set_Soft_Limit_Enable_Net            (unsigned short Axis,unsigned long  Soft_Limit_Enable,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Soft_Limit_Enable_Net            (unsigned short Axis,unsigned long *Soft_Limit_Enable,unsigned short StationNumber = 0);
//5.4 伺服报警触发运动停止函数                         宏定义0.0           宏定义5.4                   [0,99] 
short WINAPI MCF_Set_Alarm_Trigger_Net                (unsigned short Axis,unsigned long  Trigger_Mode,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Alarm_Trigger_Net                (unsigned short Axis,unsigned long *Trigger_Mode,unsigned short StationNumber = 0);
//5.5 Index触发运动停止函数                            宏定义0.0           宏定义5.4                   [0,99] 
short WINAPI MCF_Set_Index_Trigger_Net                (unsigned short Axis,unsigned long  Trigger_Mode,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Index_Trigger_Net                (unsigned short Axis,unsigned long *Trigger_Mode,unsigned short StationNumber = 0);
//5.6 原点触发运动停止函数                             宏定义0.0           宏定义5.4                   [0,99] 
short WINAPI MCF_Set_Home_Trigger_Net                 (unsigned short Axis,unsigned long  Trigger_Mode,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Home_Trigger_Net                 (unsigned short Axis,unsigned long *Trigger_Mode,unsigned short StationNumber = 0);
//5.7 正限位触发运动停止函数                           宏定义0.0           宏定义5.4                   [0,99] 
short WINAPI MCF_Set_ELP_Trigger_Net                  (unsigned short Axis,unsigned long  Trigger_Mode,unsigned short StationNumber = 0);
short WINAPI MCF_Get_ELP_Trigger_Net                  (unsigned short Axis,unsigned long *Trigger_Mode,unsigned short StationNumber = 0);
//5.8 负限位触发运动停止函数                           宏定义0.0           宏定义5.4                   [0,99] 
short WINAPI MCF_Set_ELN_Trigger_Net                  (unsigned short Axis,unsigned long  Trigger_Mode,unsigned short StationNumber = 0);
short WINAPI MCF_Get_ELN_Trigger_Net                  (unsigned short Axis,unsigned long *Trigger_Mode,unsigned short StationNumber = 0);                               
//5.9 原点触发位置记录函数 	                           宏定义0.0           [-2^31,(2^31-1)]  [0,99] 		  [0,99]
short WINAPI MCF_Get_Home_Rise_Position_Net           (unsigned short Axis,long *Position,unsigned short StationNumber = 0);  
short WINAPI MCF_Get_Home_Fall_Position_Net           (unsigned short Axis,long *Position,unsigned short StationNumber = 0);   
short WINAPI MCF_Get_Home_Rise_Encoder_Net            (unsigned short Axis,long *Encoder,unsigned short StationNumber = 0);  
short WINAPI MCF_Get_Home_Fall_Encoder_Net            (unsigned short Axis,long *Encoder,unsigned short StationNumber = 0);  
//5.10 轴状态清除函数                                  宏定义0.0           [0,99] 
short WINAPI MCF_Clear_Axis_State_Net                 (unsigned short Axis,unsigned short StationNumber = 0);  
//5.11 轴状态触发停止运动查询函数                      宏定义0.0           MC_Retrun.h[0,28]      [0,99] 
short WINAPI MCF_Get_Axis_State_Net                   (unsigned short Axis,unsigned short *Reason,unsigned short StationNumber = 0); 
/********************************************************************************************************************************************************************
                                                      6 轴回原点函数
********************************************************************************************************************************************************************/
//6.1 设置回零参数                                     宏定义0.0           [1,65535]                       [1,65535]
short WINAPI MCF_Search_Home_dMaxA_Time_Net			  (unsigned short Axis,unsigned short H_dMaxA_Time = 10,unsigned short L_dMaxA_Time = 10, unsigned short StationNumber = 0);
//                                                     宏定义0.0           [1,35]                          宏定义6.1.1                宏定义6.1.2               宏定义6.1.3                (0,10M]P/S     (0,10M]P/S     [-2^31,(2^31-1)]
short WINAPI MCF_Search_Home_Set_Net                  (unsigned short Axis,unsigned short Search_Home_Mode,unsigned short Limit_Logic,unsigned short Home_Logic,unsigned short Index_Logic,double H_dMaxV,double L_dMaxV,long Offset_Positio = 1000,unsigned short Trigger_Source = 0, unsigned short StationNumber = 0);
//6.2 设置回零启动                                     宏定义0.0           [0,99] 
short WINAPI MCF_Search_Home_Start_Net                (unsigned short Axis,unsigned short StationNumber = 0);
//6.3 设置回零停止                                     宏定义0.0           [0,99] 
short WINAPI MCF_Search_Home_Stop_Net                 (unsigned short Axis,unsigned short StationNumber = 0);
//6.4 获取回零状态                                     宏定义0.0           MC_Retrun.h{0,31,32}       [0,99]    
short WINAPI MCF_Search_Home_Get_State_Net            (unsigned short Axis,unsigned short *Home_State,unsigned short StationNumber = 0);
//6.5 设置回零缓停时间                				   宏定义0.0           [0,1000] 单位：ms         [0,99]
short WINAPI MCF_Search_Home_Stop_Time_Net			  (unsigned short Axis,unsigned short Stop_Time, unsigned short StationNumber = 0);
//6.6 设置回零完成后保持位置值                         宏定义0.0            [0,99]
short WINAPI MCF_Search_Home_Keep_Position_Net        (unsigned short Axis, unsigned short StationNumber = 0);
//6.7 设置回零完成后保持编码器值                       宏定义0.0            [0,99]
short WINAPI MCF_Search_Home_Keep_Encoder_Net         (unsigned short Axis, unsigned short StationNumber = 0);
//6.8 设置回零在零点位置离开速度                       宏定义0.0            [0,99]
short WINAPI MCF_Search_Home_Leave_Vel_Net            (unsigned short Axis, double M_dMaxV, unsigned short StationNumber = 0);
/********************************************************************************************************************************************************************
                                                      7 点位运动控制函数
********************************************************************************************************************************************************************/
//7.1 速度控制函数                                     宏定义0.0           (0,10M]P/S    (0,1T]P^2/S  [0,99] 
short WINAPI MCF_JOG_Net                              (unsigned short Axis,double dMaxV, double dMaxA,unsigned short StationNumber = 0);                                                  
//7.2 单轴运动位置改变函数                             宏定义0.0           [-2^31,(2^31-1)]  宏定义0.3                    [0,99] 
short WINAPI MCF_Uniaxial_dDist_Change_Net            (unsigned short Axis,long dDist,  unsigned short Position_Mode,unsigned short StationNumber = 0);   
//7.3 单轴运动速度改变函数                             宏定义0.0           (0,10M]P/S   [0,99] 
short WINAPI MCF_Uniaxial_dMaxV_Change_Net            (unsigned short Axis,double dMaxV,unsigned short StationNumber = 0);      
short WINAPI MCF_Uniaxial_dMaxA_Change_Net            (unsigned short Axis,double dMaxA,unsigned short StationNumber = 0); 
//7.4 单轴曲线函数                                     宏定义0.0           [0,dMaxV]      (0,10M]P/S    (0,1T]P^2/S   (0,100T]P^3/S [0,dMaxV]      宏定义0.4               [0,99] 
short WINAPI MCF_Set_Axis_Profile_Net                 (unsigned short Axis,double  dV_ini,double dMaxV, double  dMaxA,double  dJerk,double  dV_end,unsigned short  Profile,unsigned short StationNumber = 0);     
short WINAPI MCF_Get_Axis_Profile_Net                 (unsigned short Axis,double *dV_ini,double *dMaxV,double *dMaxA,double *dJerk,double *dV_end,unsigned short *Profile,unsigned short StationNumber = 0);  
//7.5 单轴运动函数                                     宏定义0.0           [-2^31,(2^31-1)]  宏定义0.3                    [0,99] 
short WINAPI MCF_Uniaxial_Net                         (unsigned short Axis,long dDist,       unsigned short Position_Mode,unsigned short StationNumber = 0);   
short WINAPI MCF_Uniaxial_Time_Net                    (unsigned short Axis,long dDist,unsigned short StationNumber = 0);
//7.6 单轴停止曲线函数                                 宏定义0.0           (0,1T]P^2/S   (0,100T]P^3/S 宏定义0.4               [0,99] 
short WINAPI MCF_Set_Axis_Stop_Profile_Net            (unsigned short Axis,double  dMaxA,double  dJerk,unsigned short  Profile,unsigned short StationNumber = 0);                    
short WINAPI MCF_Get_Axis_Stop_Profile_Net            (unsigned short Axis,double *dMaxA,double *dJerk,unsigned short *Profile,unsigned short StationNumber = 0);
//7.7 轴停止函数                                       宏定义0.0           宏定义7.7                     [0,99] 
short WINAPI MCF_Axis_Stop_Net                        (unsigned short Axis,unsigned short Axis_Stop_Mode,unsigned short StationNumber = 0); 
//7.8 单轴运动改变周期函数                             宏定义0.0           [1,1000]MS           [0,99] 
short WINAPI MCF_Uniaxial_Cycle_Change_Net            (unsigned short Axis,unsigned short Cycle,unsigned short StationNumber = 0);   
/********************************************************************************************************************************************************************
                                                      8 插补运动控制函数
********************************************************************************************************************************************************************/
//8.1 坐标系曲线函数                                   宏定义0.1                 [0,dMaxV]      (0,10M]P/S    (0,1T]P^2/S   (0,100T]P^3/S [0,dMaxV]      宏定义0.4               [0,99]     
short WINAPI MCF_Set_Coordinate_Profile_Net           (unsigned short Coordinate,double  dV_ini,double  dMaxV,double  dMaxA,double  dJerk,double  dV_end,unsigned short Profile, unsigned short StationNumber = 0); 
short WINAPI MCF_Get_Coordinate_Profile_Net           (unsigned short Coordinate,double *dV_ini,double *dMaxV,double *dMaxA,double *dJerk,double *dV_end,unsigned short *Profile,unsigned short StationNumber = 0);
//8.2 圆半径插补运动函数                               宏定义0.1                 宏定义0.0                 [-2^31,(2^31-1)] [-2^31,(2^31-1)]  宏定义0.5                 宏定义0.3                    [0,99] 
short WINAPI MCF_Arc2_Radius_Net                      (unsigned short Coordinate,unsigned short *Axis_List,long *dDist_List,long Arc_Radius,  unsigned short Direction, unsigned short Position_Mode,unsigned short StationNumber = 0); 
//8.3 圆圆心插补运动函数                               宏定义0.1                 宏定义0.0                 [-2^31,(2^31-1)] [-2^31,(2^31-1)]  宏定义0.5                 宏定义0.3                    [0,99] 
short WINAPI MCF_Arc2_Centre_Net                      (unsigned short Coordinate,unsigned short *Axis_List,long *dDist_List,long *Center_List,unsigned short Direction, unsigned short Position_Mode,unsigned short StationNumber = 0); 
//8.4 直线插补运动函数                                 宏定义0.1                 宏定义0.0                 [-2^31,(2^31-1)] 宏定义0.3                   [0,99] 
short WINAPI MCF_Line2_Net                            (unsigned short Coordinate,unsigned short *Axis_List,long *dDist_List,unsigned short Position_Mode,unsigned short StationNumber = 0);
short WINAPI MCF_Line3_Net                            (unsigned short Coordinate,unsigned short *Axis_List,long *dDist_List,unsigned short Position_Mode,unsigned short StationNumber = 0);
short WINAPI MCF_Line4_Net                            (unsigned short Coordinate,unsigned short *Axis_List,long *dDist_List,unsigned short Position_Mode,unsigned short StationNumber = 0);
//8.5 坐标系停止曲线函数                               宏定义0.1                 (0,1T]P^2/S   (0,100T]P^3/S  宏定义0.4               [0,99] 
short WINAPI MCF_Set_Coordinate_Stop_Profile_Net      (unsigned short Coordinate,double  dMaxA, double  dJerk,unsigned short  Profile,unsigned short StationNumber = 0);                  
short WINAPI MCF_Get_Coordinate_Stop_Profile_Net      (unsigned short Coordinate,double *dMaxA, double *dJerk,unsigned short *Profile,unsigned short StationNumber = 0); 
//8.6 螺旋线圆半径插补运动函数                         宏定义0.1                 宏定义0.0                 [-2^31,(2^31-1)] [-2^31,(2^31-1)]  宏定义0.5                 宏定义0.3                    [0,99] 
short WINAPI MCF_Screw3_Radius_Net                    (unsigned short Coordinate,unsigned short *Axis_List,long *dDist_List,long Arc_Radius,  unsigned short Direction, unsigned short Position_Mode,unsigned short StationNumber = 0); 
//8.7 螺旋线圆圆心插补运动函数                         宏定义0.1                 宏定义0.0                 [-2^31,(2^31-1)] [-2^31,(2^31-1)]  宏定义0.5                 宏定义0.3                    [0,99] 
short WINAPI MCF_Screw3_Centre_Net                    (unsigned short Coordinate,unsigned short *Axis_List,long *dDist_List,long *Center_List,unsigned short Direction, unsigned short Position_Mode,unsigned short StationNumber = 0); 
//8.8 坐标系停止函数                                   宏定义0.1                 宏定义5.6                           [0,99] 
short WINAPI MCF_Coordinate_Stop_Net                  (unsigned short Coordinate,unsigned short Coordinate_Stop_Mode,unsigned short StationNumber = 0);                                                
/********************************************************************************************************************************************************************
                                                      9 缓冲区函数
********************************************************************************************************************************************************************/
//9.1 缓冲区停止曲线函数                               宏定义0.2                    (0,1T]P^2/S    (0,100T]P^3/S  宏定义0.4              [0,99]
short WINAPI MCF_Buffer_Set_Stop_Profile_Net          (unsigned short Buffer_Number,double  dMaxA, double  dJerk, unsigned short Profile,unsigned short StationNumber = 0);
//9.2 缓冲区停止函数                                   宏定义0.2                    宏定义9.2                       [0,99] 
short WINAPI MCF_Buffer_Stop_Net                      (unsigned short Buffer_Number,unsigned short Buffer_Stop_Mode,unsigned short StationNumber = 0);
//9.3 缓冲区在线改变速度倍率                           宏定义0.2                    (0,10]                [0,99] 
short WINAPI MCF_Buffer_Change_Velocity_Ratio_Net     (unsigned short Buffer_Number,double Velocity_Ratio,unsigned short StationNumber = 0);
//9.4 缓冲区建立开始函数                               宏定义0.2                    [0,99] 
short WINAPI MCF_Buffer_Start_Net                     (unsigned short Buffer_Number,unsigned short StationNumber = 0);
//9.5 缓冲区速度倍率                                   宏定义0.2                    宏定义9.5                                [0,99] 
short WINAPI MCF_Buffer_Set_Velocity_Ratio_Enable_Net (unsigned short Buffer_Number,unsigned short Velocity_Ratio_Enable = 0,unsigned short StationNumber = 0);
//9.6 缓冲区前瞻处理降速比                             宏定义0.2                    (0,1]                   [0,99]
short WINAPI MCF_Buffer_Set_Reduce_Ratio_Net          (unsigned short Buffer_Number,double Reduce_Ratio = 1,unsigned short StationNumber = 0);
//9.7 缓冲区曲线函数                                   宏定义0.2                    [0,dMaxV]     (0,10M]P/S    (0,1T]P^2/S  (0,100T]P^3/S [0,dMaxV]     宏定义0.4               [0,99]  
short WINAPI MCF_Buffer_Set_Profile_Net               (unsigned short Buffer_Number,double dV_ini,double dMaxV, double dMaxA,double dJerk, double dV_end,unsigned short Profile ,unsigned short StationNumber = 0);
//9.8 缓冲区单轴运动                                   宏定义0.2                    宏定义0.0           [-2^31,(2^31-1)]  宏定义0.3                    [0,99] 
short WINAPI MCF_Buffer_Uniaxial_Net                  (unsigned short Buffer_Number,unsigned short Axis,long dDist,       unsigned short Position_Mode,unsigned short StationNumber = 0);
//    缓冲区单轴运动距离同步跟随函数  
short WINAPI MCF_Buffer_Sync_Follow_Net               (unsigned short Buffer_Number,unsigned short Axis,long dDist,unsigned short StationNumber = 0);
//9.9 缓冲区直线插补运动                               宏定义0.2                    宏定义0.0                 [-2^31,(2^31-1)] 宏定义0.3                    [0,99] 
short WINAPI MCF_Buffer_Line2_Net                     (unsigned short Buffer_Number,unsigned short *Axis_List,long *dDist_List,unsigned short Position_Mode,unsigned short StationNumber = 0);
short WINAPI MCF_Buffer_Line3_Net                     (unsigned short Buffer_Number,unsigned short *Axis_List,long *dDist_List,unsigned short Position_Mode,unsigned short StationNumber = 0);
short WINAPI MCF_Buffer_Line4_Net                     (unsigned short Buffer_Number,unsigned short *Axis_List,long *dDist_List,unsigned short Position_Mode,unsigned short StationNumber = 0);
//9.10 缓冲区平面圆半径插补运动函数                    宏定义0.2                    宏定义0.0                 [-2^31,(2^31-1)] [-2^31,(2^31-1)]  宏定义0.5                宏定义0.3                    [0,99] 
short WINAPI MCF_Buffer_Arc_Radius_Net                (unsigned short Buffer_Number,unsigned short *Axis_List,long *dDist_List,long Arc_Radius,  unsigned short Direction,unsigned short Position_Mode,unsigned short StationNumber = 0);
//9.11 缓冲区平面圆圆心插补运动函数                    宏定义0.2                    宏定义0.0                 [-2^31,(2^31-1)] [-2^31,(2^31-1)]  宏定义0.5                宏定义0.3                    [0,99] 
short WINAPI MCF_Buffer_Arc_Centre_Net                (unsigned short Buffer_Number,unsigned short *Axis_List,long *dDist_List,long *Center_List,unsigned short Direction,unsigned short Position_Mode,unsigned short StationNumber = 0);
//9.12 缓冲区延时函数                                  宏定义0.2                    [0,2^31-1]           [0,99]
short WINAPI MCF_Buffer_Delay_Net                     (unsigned short Buffer_Number,unsigned long number,unsigned short StationNumber = 0);
//9.13 缓冲区IO输出函数                                宏定义0.2                    宏定义2.3.1               宏定义2.3.2          [0,99]     
short WINAPI MCF_Buffer_Set_Output_Bit_Net            (unsigned short Buffer_Number,unsigned short Bit_Number,unsigned short output,unsigned short StationNumber = 0);
//9.14 缓冲区IO等待函数                                宏定义0.2                    宏定义2.4.1               宏定义2.4.2          (0,2^15-1]              [0,99] 
short WINAPI MCF_Buffer_Wait_Input_Bit_Net            (unsigned short Buffer_Number,unsigned short Bit_Number,unsigned short Logic,unsigned short Time_Out,unsigned short StationNumber = 0);
//9.15 缓冲区建立结束                                  宏定义0.2                    [0,99] 
short WINAPI MCF_Buffer_End_Net                       (unsigned short Buffer_Number,unsigned short StationNumber = 0);
//9.16 缓冲区执行函数                                  宏定义0.2                    宏定义9.16                  [0,99]
short WINAPI MCF_Buffer_Execute_Net                   (unsigned short Buffer_Number,unsigned short Execute_Mode,unsigned short StationNumber = 0);
//9.17 缓冲区断点启动函数                              宏定义0.2                    [0,99]
short WINAPI MCF_Buffer_Execute_BreakPoint_Net        (unsigned short Buffer_Number,unsigned short StationNumber = 0);
//9.18 缓冲区状态查询函数                              宏定义0.2                    MC_Retrun.h{0,29,30}                   [0,2^15-1]
short WINAPI MCF_Buffer_Get_State_Net                 (unsigned short Buffer_Number,unsigned short *Execute_State,unsigned short *Execute_Number,unsigned short StationNumber = 0);
//9.19 缓冲区剩余可插入指令空间百分比查询              宏定义0.2                    [0,100]
short WINAPI MCF_Buffer_Get_Remainder_Space_Net        (unsigned short Buffer_Number,unsigned short *Remainder_Space_Ratio,unsigned short StationNumber = 0);
//9.20 缓冲区开始插入(建议查询到剩余有一半以上空间)    宏定义0.2                    [0,99] 
short WINAPI MCF_Buffer_Insert_Start_Net               (unsigned short Buffer_Number,unsigned short StationNumber = 0);
//9.22 缓冲区结束插入                                  宏定义0.2                    [0,99] 
short WINAPI MCF_Buffer_Insert_End_Net                 (unsigned short Buffer_Number,unsigned short StationNumber = 0);
//9.23 计算加入指令所占用的空间百分比                    宏定义0.2                  [0,100]                            [0,99] 
//     在MCF_Buffer_Start_Net或者MCF_Buffer_Insert_Start_Net后开始从0计算
short WINAPI MCF_Buffer_Count_Occupy_Space_Net        (unsigned short Buffer_Number,unsigned short *Occupy_Space_Ratio,unsigned short StationNumber = 0);

/********************************************************************************************************************************************************************
                                                      10 示波器10K采样频率数据捕捉函数
********************************************************************************************************************************************************************/
//10.1 数据捕捉打开/关闭函数(必须在MCF_Open_Net前面提前调用,而且只支持一个运动控制卡)                                    
short WINAPI MCF_Capture_Open_Net                     (unsigned short Capture_Mode = 0);
short WINAPI MCF_Capture_Close_Net                    ();
//10.2 数据捕捉检查数据更新函数                        宏定义10.2            
short WINAPI MCF_Capture_State_Net                    (unsigned short *Capture_State);
//10.3 读取采样连续的10000个位置命令数据                宏定义0.0           &Array[Capture_Frequency*Capture_Time_1MS]  
short WINAPI MCF_Capture_Read_Command_Net             (unsigned short Axis,long *Command);
//10.4 读取采样连续的10000个编码器数据                  宏定义0.0           &Array[Capture_Frequency*Capture_Time_1MS]
short WINAPI MCF_Capture_Read_Encoder_Net             (unsigned short Axis,long *Encoder);
//10.5 读取采样连续的50000个模拟量数据                  宏定义0.0           &Array[Capture_Frequency*Capture_Time_1MS]
short WINAPI MCF_Capture_Read_AD_Net                  (unsigned short Axis,long *AD);
//10.6 ADC采样滤波                                     宏定义0.0           [0,1]
short WINAPI MCF_Capture_Filter_AD_Net                (unsigned short Axis,double Filter_Coefficient = 1);
//10.7 数据捕捉频率设置                                宏定义10.7            
short WINAPI MCF_Capture_Frequency_Net                (unsigned short Capture_Frequency = 1,unsigned short StationNumber = 0);
//10.8 数据捕捉缓存时间设置                            [2,1000] 2的倍数            
short WINAPI MCF_Capture_Time_Net                     (unsigned short Capture_Time_1MS = 100,unsigned short StationNumber = 0);
/********************************************************************************************************************************************************************
                                                      11 电子齿轮控制函数
********************************************************************************************************************************************************************/
//11.1 电子齿轮设置函数                                宏定义0.0           宏定义0.0                   (0,(2^31-1)]               (0,(2^31-1)]            宏定义11.1.1                  宏定义11.1.2         [0,99]
short WINAPI MCF_Set_Gear_Net                         (unsigned short Axis,unsigned short  Follow_Axis,unsigned long  Denominator,unsigned long  Molecule,unsigned short  Follow_Source,unsigned short  Dir,unsigned short StationNumber = 0); //关闭使能再使能数据有效
short WINAPI MCF_Get_Gear_Net                         (unsigned short Axis,unsigned short *Follow_Axis,unsigned long *Denominator,unsigned long *Molecule,unsigned short *Follow_Source,unsigned short *Dir,unsigned short StationNumber = 0);
//11.2 电子齿轮开关函数                                宏定义0.0           宏定义11.2                  [0,99] 
short WINAPI MCF_Set_Gear_Enable_Net                  (unsigned short Axis,unsigned short  Gear_Enable,unsigned short StationNumber = 0); 
short WINAPI MCF_Get_Gear_Enable_Net                  (unsigned short Axis,unsigned short *Gear_Enable,unsigned short StationNumber = 0);
//11.3 电子齿轮运动距离后自动关闭                      宏定义0.0           [-2^31,(2^31-1)] [0,99] 
short WINAPI MCF_Set_Gear_Auto_Disable_Net            (unsigned short Axis,long dDist,      unsigned short StationNumber = 0); 
/********************************************************************************************************************************************************************
                                                      12 位置比较输出函数
********************************************************************************************************************************************************************/
//12.1 设置一维位置比较器                              宏定义0.0
short WINAPI MCF_Set_Compare_Config_Net               (unsigned short Axis,unsigned short  Enable,unsigned short  Compare_Source,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Compare_Config_Net               (unsigned short Axis,unsigned short *Enable,unsigned short *Compare_Source,unsigned short StationNumber = 0);
//12.2 清除一维位置所有/当前比较点/关闭任意点          宏定义0.0
short WINAPI MCF_Clear_Compare_Points_Net             (unsigned short Axis,unsigned short StationNumber = 0);
short WINAPI MCF_Clear_Compare_Current_Points_Net     (unsigned short Axis,unsigned short StationNumber = 0);
//    按照 MCF_Add_Compare_Point_Net 数据累加计算          宏定义0.0           [1,(2^31-1)}
short WINAPI MCF_Disable_Compare_Any_Points_Net       (unsigned short Axis,unsigned long  Point_Number,unsigned short StationNumber = 0);
//12.3 添加一维位置比较点                              宏定义0.0
short WINAPI MCF_Add_Compare_Point_Net                (unsigned short Axis,long  Position,unsigned short Dir, unsigned short Action,unsigned short Actpara,unsigned short StationNumber = 0);
//12.4 读取当前一维比较点位置                          宏定义0.0
short WINAPI MCF_Get_Compare_Current_Point_Net        (unsigned short Axis,long *Position,unsigned short StationNumber = 0);
//12.5 查询已经比较过的一维比较点个数(注意数据溢出)    宏定义0.0           [0,256]  
short WINAPI MCF_Get_Compare_Points_Runned_Net        (unsigned short Axis,unsigned short *Point_Number,unsigned short StationNumber = 0);
//12.6 查询可以加入的一维比较点个数                    宏定义0.0           [0,256]
short WINAPI MCF_Get_Compare_Points_Remained_Net      (unsigned short Axis,unsigned short *Point_Number,unsigned short StationNumber = 0);
//12.7 查询所有未完成一维比较点个数和位置              宏定义0.0               
short WINAPI MCF_Get_Compare_Points_Incomplete_Net    (unsigned short Axis,unsigned short *Incomplete_Number,long *Incomplete_Position,unsigned short StationNumber = 0);
/********************************************************************************************************************************************************************
                                                      13 PWM输出函数
********************************************************************************************************************************************************************/
//13.1 设置PWM输出参数                                 宏定义13.1.1            宏定义13.1.2           宏定义13.1.3                       宏定义13.1.4
short WINAPI MCF_Set_Pwm_Config_Net                   (unsigned short  Channel,unsigned short  Enable,unsigned short  Output_Port_Config,unsigned short  Output_Start_Logic,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Pwm_Config_Net                   (unsigned short  Channel,unsigned short *Enable,unsigned short *Output_Port_Config,unsigned short *Output_Start_Logic,unsigned short StationNumber = 0);
//13.2 输出PWM信号                                     宏定义13.1.1            [0,1000000]            [0,100]                  (0,(2^31-1)] 
short WINAPI MCF_Set_Pwm_Output_Net                   (unsigned short  Channel,unsigned long Frequency,unsigned long DutyCycle,unsigned long Pwm_Number,unsigned short StationNumber = 0);
//13.3 PWM完成信号                                     宏定义13.1.1            宏定义13.3.1 
short WINAPI MCF_Get_Pwm_State_Net                    (unsigned short  Channel,unsigned short *Finish,unsigned short StationNumber = 0);
/********************************************************************************************************************************************************************
                                                      14 手轮函数
********************************************************************************************************************************************************************/
//14.1 开启手轮功能                                    宏定义11.1.2 
short WINAPI MCF_Hand_Wheel_Open_Net                  (unsigned short Dir,unsigned short StationNumber = 0);
//14.2 关闭手轮功能
short WINAPI MCF_Hand_Wheel_Close_Net                 (unsigned short StationNumber = 0);
//14.3 设置硬件手轮编码器通道                          宏定义0.0                        
short WINAPI MCF_Hand_Wheel_Config_Encoder_Net        (unsigned short Axis,unsigned short StationNumber = 0);
//14.4 设置硬件手轮速率配置输入点                      宏定义2.4.1                     
short WINAPI MCF_Hand_Wheel_Config_X1_Net             (unsigned short Bit_Input_Number,unsigned short StationNumber = 0);
short WINAPI MCF_Hand_Wheel_Config_X10_Net            (unsigned short Bit_Input_Number,unsigned short StationNumber = 0);
short WINAPI MCF_Hand_Wheel_Config_X100_Net           (unsigned short Bit_Input_Number,unsigned short StationNumber = 0);
//    设置硬件手轮速率大小                                                [1,100]
short WINAPI MCF_Hand_Wheel_Speed_X1_Net              (unsigned short Speed_X = 1,  unsigned short StationNumber = 0);
short WINAPI MCF_Hand_Wheel_Speed_X10_Net             (unsigned short Speed_X = 10, unsigned short StationNumber = 0);
short WINAPI MCF_Hand_Wheel_Speed_X100_Net            (unsigned short Speed_X = 100,unsigned short StationNumber = 0);
//14.4 设置硬件手轮轴号配置输入点                      宏定义0.0           宏定义2.4.1
short WINAPI MCF_Hand_Wheel_Config_Axis_Net           (unsigned short Axis,unsigned short Bit_Input_Number,unsigned short StationNumber = 0);
//14.5 设置手轮运动平滑滤波时间                        宏定义0.0           [1,1000]MS 
short WINAPI MCF_Hand_Wheel_Config_Filter_Time_Net    (unsigned short Axis,unsigned long Filter_Time_1MS,unsigned short StationNumber = 0);
/********************************************************************************************************************************************************************
                                                      15 模拟量输入输出函数
********************************************************************************************************************************************************************/
//15.1 读取单次ADC采样                                  宏定义0.0           [-2^15,(2^15-1)]
short WINAPI MCF_Single_Read_AD_Net                   (unsigned short Channel,short *AD,unsigned short StationNumber = 0);
//15.2 读取单次DAC输出                                  宏定义0.0           [-2^15,(2^15-1)]
short WINAPI MCF_Single_Write_DA_Net                  (unsigned short Channel,short DA,unsigned short StationNumber = 0);
//15.3 设置AD双向比较器停止对应轴 
short WINAPI MCF_Set_AD_Compare_Net                   (unsigned short Channel,short AD_Compare,unsigned short Stop_Axis, unsigned short StationNumber = 0);
//15.4 设置AD触发数值                                 [0,7]                                                   
short WINAPI MCF_Set_AD_Capture_Net                   (unsigned short Channel,short AD_Capture,unsigned short StationNumber = 0);
//    0：保持不变  1：清除复位                                                    
short WINAPI MCF_Clear_AD_Capture_Net                 (unsigned short C_1,unsigned short C_2,unsigned short C_3,unsigned short C_4,
	                                                   unsigned short C_5,unsigned short C_6,unsigned short C_7,unsigned short C_8,unsigned short StationNumber = 0);
//     读取到达设置AD值捕获到的对应通道的AD值和轴X位置
short WINAPI MCF_Get_Capture_AD_1_Net                 (short *AD_5,long *Position_1,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Capture_AD_2_Net                 (short *AD_6,long *Position_1,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Capture_AD_3_Net                 (short *AD_7,long *Position_1,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Capture_AD_4_Net                 (short *AD_8,long *Position_1,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Capture_AD_5_Net                 (short *AD_1,long *Position_1,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Capture_AD_6_Net                 (short *AD_2,long *Position_1,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Capture_AD_7_Net                 (short *AD_3,long *Position_1,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Capture_AD_8_Net                 (short *AD_4,long *Position_1,unsigned short StationNumber = 0);
//15.5 设置触发位置数值                               [0,7]                                 
short WINAPI MCF_Set_Position_Capture_AD_Net          (unsigned short Channel,long Position_1,unsigned short StationNumber = 0);
short WINAPI MCF_Get_Position_Capture_AD_Net          (unsigned short Channel,short *AD,unsigned short StationNumber = 0);
short WINAPI MCF_Clear_Position_Capture_AD_Net        (unsigned short C_1,unsigned short C_2,unsigned short C_3,unsigned short C_4,
	                                                   unsigned short C_5,unsigned short C_6,unsigned short C_7,unsigned short C_8,unsigned short StationNumber = 0);
//15.6 获取AD最大值                                   [0,7]
short WINAPI MCF_Get_Limit_AD_Net                     (unsigned short Channel,short *MAX_AD,short *MIN_AD,unsigned short StationNumber = 0);
short WINAPI MCF_Clear_Limit_AD_Net                   (unsigned short C_1,unsigned short C_2,unsigned short C_3,unsigned short C_4,
	                                                   unsigned short C_5,unsigned short C_6,unsigned short C_7,unsigned short C_8,unsigned short StationNumber = 0);
/********************************************************************************************************************************************************************
                                                       16 系统函数
********************************************************************************************************************************************************************/
//16.1 模块版本号                                     [0x00000000,0xFFFFFFFF] [0,99] 
short WINAPI MCF_Get_Version_Net                      (unsigned long *Version,unsigned short StationNumber = 0);                                                  
//16.2 序列号                                         [0x00000000,0xFFFFFFFF] [0,99] 
short WINAPI MCF_Get_Serial_Number_Net                (INT64 *Serial_Number,unsigned short StationNumber = 0);     
//16.3 模块运行时间                                   [0x00000000,0xFFFFFFFF] [0,99]    单位：秒
short WINAPI MCF_Get_Run_Time_Net                     (unsigned long *Run_Time,unsigned short StationNumber = 0); 
//16.4 Flash 读写功能目前暂时大小2Kbytes,也即定义一个 unsigned int Array[256] 存放数据
short WINAPI MCF_Flash_Write_Net                      (unsigned long Pass_Word_Setup,unsigned long *Flash_Write_Data,unsigned short StationNumber = 0);
short WINAPI MCF_Flash_Read_Net                       (unsigned long Pass_Word_Check,unsigned long *Flash_Read_Data,unsigned short StationNumber = 0);
//16.5 开启网络回路,一发一收，正常控制使用(默认)   
short WINAPI MCF_LookBack_Enable_Net                  ();
//16.6 关闭网络回路，只发不收，测试老化模式下使用,或者检测各个级联模块是否工作
short WINAPI MCF_LookBack_Disable_Net                 ();
//16.7 通讯时间监测                                    &Array[12]
short WINAPI MCF_Get_Connect_Time_Count_Net           (unsigned long *Connect_Count); 
//16.8 系统定时回调函数
short WINAPI	MCF_Set_CallBack_Net                     (long CallBack,unsigned long Time_1MS);

#ifdef __cplusplus
} 
#endif

#endif // _MXMPAC_H_1C474833_BDCF_826DF5_INCLUDED_






