	// 深圳摩升泰科技保留更更新增加宏定义数值的权利，建议客户直接使用宏定义，以免由于更新造成的数值不对
//重复打开卡               
#define ERR_Open_Already               -24  //控制卡已经打开
//系统时间超时               
#define ERR_Command_Time_Out           -23  //超时
//数据捕获错误               
#define ERR_Capture_Empty              -22  //数据捕获空
//控制卡系统错误
#define ERR_Beyond_Station_Type        -21  //超出站点类型
#define ERR_Beyond_Station_Length      -20  //超出站点长度
#define ERR_Beyond_Station_Number      -19  //超出站点设置
#define ERR_Open_Station_Fail          -18  //打开站点不成功
#define ERR_Link_Break                 -17  //链接中断
//指令参数错误
#define ERR_Axis_Number                -16  //没有此轴号/相机号
#define ERR_Axis_Inter_Number          -15  //表示轴数量超出设置范围
#define ERR_NOT_Set_Profile            -14  //没有设置对应的规划参数
#define ERR_Set_Profile_ERR            -13  //错误设置的规划参数
#define ERR_Profile_Calculate          -12  //表示曲线参数计算出错
#define ERR_Arc_Radius                 -11  //MCF_Arc_Radius_Net目标和源坐标不能设置为同意点
// 坐标系参数错误
#define ERR_Coordinate_Number          -10  //没有此坐标系
//输入输出参数错误
#define ERR_Output_Number              -9   //没有此输出点
#define ERR_Input_Number               -8   //没有此输入点
//缓冲区参数错误
#define ERR_Buffer_Malloc_Fail         -7   //动态开辟本地线程内存失败
#define ERR_Buffer_Number              -6   //没有此缓存
#define ERR_Buffer_Space_Enough        -5   //缓冲区空间不足
#define ERR_Buffer_Inter_Number        -4   //超出缓冲区最大插补轴
#define ERR_Buffer_NO_Start            -3   //没有开启/启动缓冲区
#define ERR_Buffer_NO_Profile          -2   //没有设置缓冲曲线参数
#define ERR_Buffer_NO_End_Buffer       -1   //没有结束缓冲区

//正常状态
#define Funtion_Success                0    //正常命令执行成功

//轴运动执行状态
#define ERR_Axis_Busy                  1    //正在执行
#define IMD_STOP_AT_EMG                2    //EMG立即紧急停止
#define DEC_STOP_AT_EMG                3    //EMG减速紧急停止
#define IMD_STOP_AT_ALM                4    //ALM立即停止   
#define DEC_STOP_AT_ALM                5    //ALM减速停止      
#define IMD_STOP_AT_Servo              6    //伺服使能立即停止      
#define DEC_STOP_AT_Servo              7    //伺服使能减速停止     
#define IMD_STOP_AT_Pos_Error          8    //指令编码器误差立即停止      
#define DEC_STOP_AT_Pos_Error          9    //指令编码器误差减速停止   
#define IMD_STOP_AT_Index              10   //Index立即停止       
#define DEC_STOP_AT_Index              11   //Index减速停止
#define IMD_STOP_AT_Home               12   //原点立即停止       
#define DEC_STOP_AT_Home               13   //原点减速停止
#define IMD_STOP_AT_ELP                14   //正硬限位立即停止 
#define DEC_STOP_AT_ELP                15   //正硬限位减速停止
#define IMD_STOP_AT_ELN                16   //负硬限位立即停止    
#define DEC_STOP_AT_ELN                17   //负硬限位减速停止    
#define IMD_STOP_AT_SOFT_ELP           18   //正软限位立即停止  
#define DEC_STOP_AT_SOFT_ELP           19   //正软限位减速停止 
#define IMD_STOP_AT_SOFT_ELN           20   //负软限位立即停止
#define DEC_STOP_AT_SOFT_ELN           21   //负软限位减速停止  
#define IMD_STOP_AT_CMD                22   //命令立即停止     
#define DEC_STOP_AT_CMD                23   //命令减速停止
#define IMD_STOP_AT_OTHER              24   //其它原因立即停止   
#define IMD_STOP_AT_LINK               25   //网络通讯中断立即停止 
#define IMD_STOP_AT_UNKOWN             26   //未知原因立即停止   
#define DEC_STOP_AT_UNKOWN             27   //未知原因减速停止   
#define DEC_STOP_AT_IO                 28   //外部IO减速停止  
// 缓冲区执行状态
#define ERR_Buffer_Excute              29   //缓冲区正在执行
#define ERR_Buffer_Stop                30   //缓冲区IO等待超时停止
// 回原点执行状态
#define ERR_Home_Wrong                 31   //回原点错误
#define ERR_Home_Excute                32   //正在回原点
// 位置比较缓冲区状态
#define ERR_Compare_Full               33   //位置比较缓冲区满
// PWM通道超出范围
#define ERR_PWM_Number                 34   //PWM通道太大
// 命令限制
#define ERR_Command_Limit              35   //卡不支持此命令

//筛选保护机制自动停止轴
#define ERR_Input_0_TimeOut            100   //物件检测无料超时停止轴运动 
#define ERR_Trig_Blow_OK_TimeOut       101   //物件吹气OK超时停止轴运动
#define ERR_Trig_Blow_NG_NumberOut     102   //物件吹气连续NG停止轴运动
#define ERR_Sorting_Forbid_Command     103   //筛选过程中禁止设置该函数

//筛选保护机制自动停止轴
