/********************************************************************************************************************************************************************
                                                       0 全局宏定义
********************************************************************************************************************************************************************/	
//0.0 Axis
#define Axis_1                   0    //第1轴
#define Axis_2                   1    //第2轴
#define Axis_3                   2    //第3轴
#define Axis_4                   3    //第4轴
#define Axis_5                   4    //第5轴
#define Axis_6                   5    //第6轴
#define Axis_7                   6    //第7轴
#define Axis_8                   7    //第8轴
#define Axis_9                   8    //第9轴
#define Axis_10                  9    //第10轴
#define Axis_11                  10   //第11轴
#define Axis_12                  11   //第12轴
#define Axis_13                  12   //第13轴
#define Axis_14                  13   //第14轴
#define Axis_15                  14   //第15轴
#define Axis_16                  15   //第16轴
#define Axis_17                  16   //第17轴
#define Axis_18                  17   //第18轴
#define Axis_19                  18   //第19轴
#define Axis_20                  19   //第20轴
#define Axis_21                  20   //第21轴
#define Axis_22                  21   //第22轴
#define Axis_23                  22   //第23轴
#define Axis_24                  23   //第24轴
#define AXIS_25                  24
#define AXIS_26                  25
#define AXIS_27                  26
#define AXIS_28                  27
#define AXIS_29                  28
#define AXIS_30                  29
#define AXIS_31                  30
#define AXIS_32                  31
#define AXIS_33                  32
#define AXIS_34                  33
#define AXIS_35                  34
#define AXIS_36                  35
#define AXIS_37                  36
#define AXIS_38                  37
#define AXIS_39                  38
#define AXIS_40                  39
#define AXIS_41                  40
#define AXIS_42                  41
#define AXIS_43                  42
#define AXIS_44                  43
#define AXIS_45                  44
#define AXIS_46                  45
#define AXIS_47                  46
#define AXIS_48                  47

//0.1 Coordinate
#define Coordinate_0             0    //第0坐标系
#define Coordinate_1             1    //第1坐标系
//0.2 Buffer_Number
#define Buffer_Number_0          0    //第0缓冲区
//0.3 Position_Mode
#define Position_Absolute        0    //绝对位置模式
#define Position_Opposite        1    //相对位置模式
//0.4 Profile
#define Profile_T                0    //T型曲线
#define Profile_S                1    //S型曲线
//0.5 Direction
#define Clock_Wise               0    //顺弧
#define Counter_Clock_Wise       1    //逆弧
/********************************************************************************************************************************************************************
                                                       1 控制卡打开函数
********************************************************************************************************************************************************************/
//1.0
#define Mode_Series             0    //串联模式
#define Mode_Parallel           1    //NTC0016并联模式
//1.1   Station_Type[]
#define Station_Type_24I16O      0    //NIO0808R/NIO1616R/NIO2416                           8/16/24输入8/16/16输出模块        
#define Station_Type_48I32O      1    //NIO4832/NIO3232/NIO4000                             48/32/40输入32/32/0输出模块          
#define Station_Type_4D          2    //NMC1200R/NMC1400/NMC3400/NMC3401/NMC3201            2/4轴网络总线运动控制卡
#define Station_Type_8D          3    //NMC5800/NMC5600R/NMC1800/NMC1600R                   6/8轴网络总线运动控制卡
#define Station_Type_16D         4    //NMC5120R/NMC5160                                    12/16轴网络总线运动控制卡
#define Station_Type_DM          5    //LMC3400/LMC3100                                     网络运动控制激光卡
#define Station_Type_8E24I16O    6    //EIO0840                                             8路编码器24输入16输出模块
#define Station_Type_NAD0804     7    //NAD0804/NAD0402/NAD0808/NAD0400/NAD0004/NAD0002     模拟量4路输出8路输入 数字量12输入12输出
#define Station_Type_NIO0040     8    //NIO0040                                             40路输出模块 
/********************************************************************************************************************************************************************
                                                      2 输入输出函数
********************************************************************************************************************************************************************/
//2.3.1 Bit_Output_Number
#define Bit_Output_0             0    //输出0
#define Bit_Output_1             1    //输出1
#define Bit_Output_2             2    //输出2
#define Bit_Output_3             3    //输出3
#define Bit_Output_4             4    //输出4
#define Bit_Output_5             5    //输出5
#define Bit_Output_6             6    //输出6
#define Bit_Output_7             7    //输出7
#define Bit_Output_8             8    //输出8
#define Bit_Output_9             9    //输出9
#define Bit_Output_10            10   //输出10
#define Bit_Output_11            11   //输出11
#define Bit_Output_12            12   //输出12
#define Bit_Output_13            13   //输出13
#define Bit_Output_14            14   //输出14
#define Bit_Output_15            15   //输出15
#define Bit_Output_16            16   //NMC3400/NMC5800扩展卡输出16
#define Bit_Output_17            17   //NMC3400/NMC5800扩展卡输出17
#define Bit_Output_18            18   //NMC3400/NMC5800扩展卡输出18
#define Bit_Output_19            19   //NMC3400/NMC5800扩展卡输出19
#define Bit_Output_20            20   //NMC3400/NMC5800扩展卡输出20
#define Bit_Output_21            21   //NMC3400/NMC5800扩展卡输出21
#define Bit_Output_22            22   //NMC3400/NMC5800扩展卡输出22
#define Bit_Output_23            23   //NMC3400/NMC5800扩展卡输出23
#define Bit_Output_24            24   //NMC3400/NMC5800扩展卡输出24
#define Bit_Output_25            25   //NMC3400/NMC5800扩展卡输出25
#define Bit_Output_26            26   //NMC3400/NMC5800扩展卡输出26
#define Bit_Output_27            27   //NMC3400/NMC5800扩展卡输出27
//2.3.2 Bit_Output_Logic
#define Bit_Output_Close         0    //触点闭合,硬件灯亮
#define Bit_Output_Open          1    //触点打开,硬件灯灭
//2.4.1 Bit_Input_Number
#define Bit_Input_0              0    //输入0
#define Bit_Input_1              1    //输入1
#define Bit_Input_2              2    //输入2
#define Bit_Input_3              3    //输入3
#define Bit_Input_4              4    //输入4
#define Bit_Input_5              5    //输入5
#define Bit_Input_6              6    //输入6
#define Bit_Input_7              7    //输入7
#define Bit_Input_8              8    //输入8
#define Bit_Input_9              9    //输入9
#define Bit_Input_10             10   //输入10
#define Bit_Input_11             11   //输入11
#define Bit_Input_12             12   //输入12
#define Bit_Input_13             13   //输入13
#define Bit_Input_14             14   //输入14
#define Bit_Input_15             15   //输入15
#define Bit_Input_16             16   //NMC3400扩展卡输入16
#define Bit_Input_17             17   //NMC3400扩展卡输入17
#define Bit_Input_18             18   //NMC3400/NMC5800扩展卡输入18
#define Bit_Input_19             19   //NMC3400/NMC5800扩展卡输入19
#define Bit_Input_20             20   //NMC3400/NMC5800扩展卡输入20
#define Bit_Input_21             21   //NMC3400/NMC5800扩展卡输入21
#define Bit_Input_22             22   //NMC3400/NMC5800扩展卡输入22
#define Bit_Input_23             23   //NMC3400/NMC5800扩展卡输入23
#define Bit_Input_24             24   //NMC3400/NMC5800扩展卡输入24
#define Bit_Input_25             25   //NMC3400/NMC5800扩展卡输入25
#define Bit_Input_26             26   //NMC3400/NMC5800扩展卡输入26
#define Bit_Input_27             27   //NMC3400/NMC5800扩展卡输入27
#define Bit_Input_28             28   //NMC3400/NMC5800扩展卡输入28
#define Bit_Input_29             29   //NMC3400/NMC5800扩展卡输入29
#define Bit_Input_30             30   //NMC3400/NMC5800扩展卡输入30
#define Bit_Input_31             31   //NMC3400/NMC5800扩展卡输入31
//2.4.2 Bit_Input_Logic
#define Bit_Input_Close          0    //触点闭合,硬件灯亮
#define Bit_Input_Open           1    //触点打开,硬件灯灭
//2.7   Bit_Input_Fall
#define Bit_Input_Fall_Check     0    //下降沿检查中
#define Bit_Input_Fall_Trigger   1    //下降沿触发
/********************************************************************************************************************************************************************
                                                      3 轴专用输入输出函数
********************************************************************************************************************************************************************/
//3.1 Servo_Logic
#define Servo_Close              0    //触点闭合 
#define Servo_Open               1    //触点打开
//3.2 Alarm_Logic
#define Alarm_Close              0    //触点闭合 
#define Alarm_Open               1    //触点打开
//3.3 Servo_Alarm_State
#define Servo_Alarm_Close        0    //触点闭合
#define Servo_Alarm_Open         1    //触点打开
//3.4 Servo_INP_State
#define Servo_INP_Close          0    //触点闭合
#define Servo_INP_Open           1    //触点打开
//3.5 Z_State
#define Z_Logic_L                0    //Z相低电平
#define Z_Logic_H                1    //Z相高电平
//3.6 Home_State
#define Home_Close               0    //触点闭合,硬件灯亮
#define Home_Open                1    //触点打开,硬件灯灭
//3.7 Positive_Limit_State
#define Positive_Limit_Close     0    //触点闭合,硬件灯亮
#define Positive_Limit_Open      1    //触点打开,硬件灯灭
//3.8 Negative_Limit_State
#define Negative_Limit_Close     0    //触点闭合,硬件灯亮
#define Negative_Limit_Open      1    //触点打开,硬件灯灭

/********************************************************************************************************************************************************************
                                                      4 轴设置函数
********************************************************************************************************************************************************************/
//4.1 Pulse_Mode
#define Pulse_Dir_H              0    //脉冲方向(默认)
#define Pulse_Dir_L              1    //脉冲方向
#define Pulse_CW_CCW             2    //双脉冲
#define Pulse_CCW_CW             3    //双脉冲
#define Pulse_AB                 4    //AB相位
#define Pulse_BA                 5    //AB相位
/********************************************************************************************************************************************************************
                                                      5 轴硬件触发停止运动函数
********************************************************************************************************************************************************************/
//5.1   EMG_Mode
#define EMG_Trigger_Close        0    //不使用紧急停止功能
#define EMG_Trigger_Low_IMD      1    //低电平触发紧急停止
#define EMG_Trigger_Low_DEC      2    //低电平触发减速停止
#define EMG_Trigger_High_IMD     3    //高电平触发紧急停止
#define EMG_Trigger_High_DEC     4    //高电平触发减速停止
//5.3 Trigger_Mode    
#define Soft_Limit_Close         0    //软件限位关闭
#define Soft_Limit_Open          1    //软件限位打开
//5.4 Trigger_Mode    
#define Trigger_Close            0    //关闭电平触发(默认)
#define Trigger_Low_IMD          1    //低电平触发紧急停止
#define Trigger_Low_DEC          2    //低电平触发减速停止
#define Trigger_High_IMD         3    //高电平触发紧急停止
#define Trigger_High_DEC         4    //高电平触发减速停止
/********************************************************************************************************************************************************************
                                                      6 轴回原点函数
********************************************************************************************************************************************************************/
//6.1.1 Limit_Logic      
//6.1.2 Home_Logic      
//6.1.3 Index_Logic      
#define Low_Logic                0    //低电平触发
#define High_Logic               1    //高电平触发 
/********************************************************************************************************************************************************************
                                                      7 点位运动控制函数
********************************************************************************************************************************************************************/
//7.7 Axis_Stop_Mode
#define Axis_Stop_IMD            0    //紧急停止
#define Axis_Stop_DEC            1    //减速停止 
/********************************************************************************************************************************************************************
                                                      8 插补运动控制函数
********************************************************************************************************************************************************************/
//8.8 Coordinate_Stop_Mode
#define Coordinate_Stop_IMD      0    //紧急停止
#define Coordinate_Stop_DEC      1    //减速停止

/********************************************************************************************************************************************************************
                                                      9 缓冲区函数
********************************************************************************************************************************************************************/
//9.2 Coordinate_Stop_Mode
#define Buffer_Stop_IMD          0    //紧急停止
#define Buffer_Stop_DEC          1    //减速停止
//9.5 Velocity_Ratio_Enable
#define Velocity_Ratio_Clsoe     0    //速度倍率关闭
#define Velocity_Ratio_Open      1    //速度倍率打开
//9.16 Execute_Mode 
#define Execute_Mode_0           0    //按照用户曲线参数执行
#define Execute_Mode_1           1    //根据两轴运动指令间的拐角做速度规划
/********************************************************************************************************************************************************************
                                                      10 示波器10K采样频率数据捕捉函数
********************************************************************************************************************************************************************/
//10.2 Capture_Mode
#define Capture_Keep              0     //连续采样
#define Capture_Rise_Input_15     1     //输入Bit_Input_15上升沿触发捕抓开始
#define Capture_Fall_Input_15     2     //输入Bit_Input_15下升沿触发捕抓开始

//10.2 Capture_State
#define Data_Keep              0     //数据更新
#define Data_Updata            1     //数据保持
//10.7 Capture_Frequency
#define Frequency_1K              1    //1K采样周期
#define Frequency_2K              2    //2K采样周期
#define Frequency_5K              5    //5K采样周期
#define Frequency_10K             10   //10K采样周期
/********************************************************************************************************************************************************************
                                                      11 电子齿轮控制函数
********************************************************************************************************************************************************************/
//11.1.1 Follow_Source
#define Follow_Command           0    //跟随命令
#define Follow_Encode            1    //跟随编码器
//11.1.2 Dir
#define Dir_P_T_P                0    //跟随正同方向走 
#define Dir_N_T_N                1    //跟随负同方向走
#define Dir_PN_T_PN              2    //跟随正负同方向走 
#define Dir_PN_T_P               3    //跟随正负都往正方向走
#define Dir_PN_T_N               4    //跟随正负都往负方向走 
//11.2 Gear_Enable
#define Gear_Close               0    //电子齿轮关闭
#define Gear_Open                1    //电子齿轮打开
/********************************************************************************************************************************************************************
                                                      12 位置比较输出函数
********************************************************************************************************************************************************************/
/********************************************************************************************************************************************************************
                                                      13 PWM输出函数
********************************************************************************************************************************************************************/
//13.1.1 Channel
#define PWM_Channel_1               0    //第1通道
#define PWM_Channel_2               1    //第2通达
//13.1.2 Enable
#define PWM_Disable                 0    //PWM不使能
#define PWM_Enable                  1    //PWM使能
//13.1.3 Output_Port_Config
#define PWM_Output_14               14   //输出点14
#define PWM_Output_15               15   //输出点15
#define PWM_AXIS_4_Pluse            22   //轴4脉冲输出点
#define PWM_AXIS_4_Dir              23   //轴4方向输出点
//13.1.3 Output_Start_Logic
#define PWM_Start_Low_Logic         0    //PWM默认触点闭合,硬件灯亮
#define PWM_Start_High_Logic        1    //PWM默认触点打开,硬件灯灭
//13.3.1 *Finish
#define PWM_Free                    0    //PWM输出空闲
#define PWM_Busy                    1    //PWM输出忙








