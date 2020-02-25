


#ifndef _DATA_TYPE_H
#define _DATA_TYPE_H


#define BLIND_NUM  20        	/****/                                 
#define BLIND_BUF  390	      /****/


struct time_str
{
	unsigned char year;
	unsigned char mon;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
};


/**************************
**	CAN数据结构体
***************************/

struct can_tx_data_str
{
    unsigned int id;													//CANID号
    unsigned char data0;											//CAN数据区域   工8字节
    unsigned char data1;
    unsigned char data2;
    unsigned char data3;
    unsigned char data4;
    unsigned char data5;
    unsigned char data6;
    unsigned char data7;
};



/**********************
**	
***********************/

struct can_rx_data_str
{
	unsigned int can_id;								//CAN消息ID
	unsigned char data[8];						  //CAN消息数据
};


/****************************
**	CAN锁车状态
*****************************/

#pragma pack(1)
struct lock_expect_str
{
	unsigned char lock_expect_state;   	//0:解锁；1：一级锁车；2：二级锁车
	unsigned char mon_expect_state;    	//期望监控状态  0：锁车功能打开;1:锁车功能关闭
	unsigned char crc_area;							//数据校验区域
};
#pragma pack()




struct lock_current_str
{
	unsigned char lock_current_state;			//当前锁车状态
	unsigned char mon_current_state;			//当前激活状态
	unsigned char handshake_state;         //握手状态
	unsigned char	lock_execute_state;			//命令执行状态
	unsigned char mon_execute_state;      //命令执行状态。
};



//
typedef union  
{
	unsigned short int			value;
	unsigned char						byte[2];
}int16_to_char;


//
typedef union  
{
	unsigned int						value;
	unsigned char						byte[4];
}int32_to_char;



/*****************************************
**
**
*******************************************/
#pragma pack(1)
struct can_info_str
{
	unsigned short int 				sys_vol;           	 					//系统电压
	unsigned char 						temp_water;           				//冷却水温
	unsigned short int 				engine_actual_rotate;		 			//发动机实际转速
	unsigned short int 				engine_set_rotate;     				//发动机设定转速
	unsigned char 						engine_torque;								//发动机实际扭矩
	unsigned char 						fuel_temp;										//燃油温度
	unsigned short int 				oil_temp;         						//既有温度
	unsigned char 						air_pressure;         				//大气压力
	unsigned short int   			engine_nacelle_temp;					//发动机仓内温度
	unsigned short int 				air_temp;											//空气温度
	unsigned char 						entere_air_temp;							//发动机进气温度
	unsigned int							engine_work_time;							//发动机工作时间
	unsigned short int 				travel_speed;									//行驶速度
	unsigned int 							once_travel;									//单次形式速度
	unsigned int 							total_travel;  								//总形式历程
	unsigned int 							once_fuel;      							//单次油耗
	unsigned int 	   					total_fuel;  									//总油耗
	unsigned char							relative_oil_pressure;				//相对机油压力
	unsigned char 						absolute_oil_pressure;				//绝对机油家里
	unsigned char							relative_add_pressure;					//相对增压压力
	unsigned char 						absolute_add_pressure;					//绝对增压压力
	unsigned short int				fuel_position;								//燃油位置
	unsigned char		 					fuel_percent;									//燃油百分比
	unsigned char 						oil_position;									//机油液位
	unsigned short int  			crankcase_pressure;   				//曲轴箱压力
	unsigned char 						cool_pressure;  							//冷却液压力
	unsigned char 						cool_position;								//冷却液位置
	unsigned char     				lock_car_state;								//锁车状态
	unsigned char							activate_status;   						//设备监控状态
	unsigned char 						key_status;     							//Key验证状态
	unsigned char 						ter_id_status;								//设备ID状态
	unsigned char 						work_flag;   									//作业工作标志
	unsigned char 						car_warn_value1;							//车辆报警1
	unsigned char 						car_warn_value2;							//车辆报警2
	unsigned char 						food_full_warn;								//粮满报警
	unsigned char 						clutch_state;  								//离合器状态
	unsigned short int	 			strip_rotate;									//剥皮机转速
	unsigned short int	 			lift_rotate;									//升运器转速
	unsigned short int				cut_table_high;								//割台高度
};
#pragma pack()



/**********************
**	GNSS定位信息
*************************/

struct gnss_info_str
{
		double 							longitude;   					// 经度 xxxyyyyyy = xxx.yyyyyy(度)*1000000*/
    double 							latitude;    					// 纬度
			
		unsigned char 			longitude_ew; 				// 东西经 0:东 1:西
    unsigned char 			latitude_sn;  				// 南北纬 0:北 1:南
    struct time_str			utc_time;
		struct time_str 		btc_time;
		unsigned int 				timestamp;         		// 时间戳
    unsigned char 			state;        				// 是否定位 "A":未定位
    
    unsigned short int	altitude;    					// 海拔 米
		unsigned short int 	heading;     					// 航向 度
		
		double 							speed;       					// 速度 公里/小时
		double              hdop;        					// 水平精度因子
  
    unsigned char 			satellite_num; 				// 使用卫星个数
		unsigned char 			bd_sate_num; 					// 可视卫星数量(北斗)
		unsigned char 			gps_sate_num;					// 可视微信数量()
};




/***********************
**	GPRS联网信息
************************/

struct gsm_info_str
{
		unsigned char 			socket[4];							//socket序号
		unsigned char 			server_addr[50];				//网关地址
		unsigned short int	port;										//网关端口
	
    unsigned char	 			gsm_up_state;         	// GSM模块开机状态 
		unsigned char 			gsm_at_state;
		unsigned char	  		sim_state;            	// sim卡状态: 0 正常; 1 异常	
		unsigned int 				csq;                 		// 信号强度*/
		unsigned char 			iccid[25];           		// ICCID 
		unsigned int 				net_register;       		// 网络注册状态 1 本地网络 5 漫游网络 
		unsigned char 			pdp_state;           		// pdp激活状态 1 已激活 0 未激活 
		unsigned int 				net_state;          		// 0：开机或者未注册到电信供应商网络；1：已经注册到网络，且以附着GPRS网络;2：已经连接到服务器
		unsigned char 			send_state;							//发送状态
		unsigned char 			led_state;							/* 0：开机或者未注册到电信供应商网络，LED灯不亮；
																								 * 1：已经注册到网络，LED灯1秒周期闪烁；
																								 * 2：已经连接到服务器，LED灯3秒周期闪烁；
																								 * 3：正在发送数据，LED灯0.1S闪烁；
																								 */
		unsigned char 			link_type;							//连接类型 0：不连接；1 TCP连接；2：FTP连接
		unsigned char 			reset_link;							//重新连接标志
		unsigned char 			ftp_server[50];					//FTP服务器地址
		unsigned short int  ftp_port;								//FTP端口
		unsigned char 			ftp_admin[20];					//FTP用户名
		unsigned char 			ftp_passd[20];             //FTP密码
		unsigned char 			update_file[20];
		unsigned int 				update_crc;              //升级文件校验码
	
    unsigned char	 			disable;              	// GPRS 任务处理开关 (使用FTP时 禁用) 
    
     
    unsigned char 			sms_run_flag;         	// 短信内容解析标志: RT_FALSE 未接收到短信; RT_TRUE 接收到短信 
    
    
    
    unsigned int 	  		dbm;               			// 信道误码率 
		 
		
    unsigned char 			socket_status;        	// socket链接状态 RT_TRUE建立链接 RT_FALSE链接断开  
    unsigned short int 	cnt_reconnect;      		// socket重新连接次数 如果连续3次初始化失败 则要复位系统 ()
    unsigned short int 	cnt_reset;          		// 初始化失败后重启时的延时时间计数 
    unsigned  int 			cnt_total_send;    			// 本次启动累计发送成功数据条数 
		
		unsigned int 				local_ip[4];
		unsigned char 			send_buf[1024];					//socket发送缓冲区
		unsigned char 			send_len;               //发送长度
		unsigned char 			send_socket;						//socket序列号
};




/************************************************
**	
************************************************/


struct config_str
{
	unsigned char 					flag;                    //配置标志
	
	unsigned char 					terminal_id[18];      					//设备ID
	unsigned char           terminal_type;		 							//设备类型
	unsigned char 					fir_ver;			 									//固件版本号
	unsigned char 					gateway_addr1[50];      					//网关地址
	unsigned int short 			gateway_port1;     	 						//网关端口   
	unsigned int 						run_time;            	 					//断开ACC后运行时间
	unsigned int 						sleep_time;           					//睡眠时间
	unsigned int 						travel_upload_cycle;    				//行驶上传周期
	unsigned int        		work_upload_cycle;      				//作业上传周期
	unsigned short int  		distance_upload;     	 					//定距离报位
	unsigned short int  		azimuth_upload;       					//航向角报位
	unsigned char 					car_type;             					//安装车辆类型
	unsigned char 					user_code;            					//用户编码
	unsigned short int			can_num;												//CAN协议号 					
	unsigned char           apn[20];												//GSM模块接入点
	unsigned char						user[20];												//GSM模块APN用户名
	unsigned char						password[20];										//GSM模块APN密码
	unsigned char 					icc_id[25];											//SIM卡ICC-ID号
	unsigned char 					hard_ware;											//硬件版本
	unsigned char 					dev_id[3];											//设备ID
	unsigned char						dev_secret[3];									//设备秘钥
	
	
	unsigned char 					gateway_addr2[50];      				//网关地址
	unsigned int short 			gateway_port2;     	 						//网关端口  
	
	unsigned char 					gateway_addr3[50];      				//网关地址
	unsigned int short 			gateway_port3;     	 						//网关端口  
	
	unsigned char 					gateway_addr4[50];      				//网关地址
	unsigned int short 			gateway_port4;     	 						//网关端口  
	
	unsigned char 					socket1;
	unsigned char 					socket2;
	
	unsigned char 					socket3;
	unsigned char 					socket4;
};



/***************************
**	设备信息
****************************/
#pragma pack(1)

struct in_info_str
{
	unsigned char 				acc_state;								//ACC状态  0  1
	unsigned char 				gnss_ant_state;           //Gnss天线连接状态  0:正常；1：未连接;2:未启用
	unsigned char 				shell_state;							//外壳状态 					0:正常；1：未连接;2:未启用
	unsigned short int 		power_vol;               //外部供电电压
	unsigned short int 		battery_vol;             //电池电压
	unsigned short int 		mcu_temp;                //单片机温度
};

#pragma pack()


#pragma pack(1)
struct run_data_str
{

	unsigned int 				acc_total_time;             //ACC总计工作时间，以分钟为单位
	unsigned int				engine_total_time;					//发动机总工作时间 ，发动机转速大于 750转的时间
	
	double 							longitude;   								//经度 （保存最后一次位置）
  double 							latitude;    								// 纬度
};
#pragma pack()





//消息头
#pragma pack(1)
typedef struct 
{
    unsigned char 				frame_start[3];			//
    unsigned char					msg_id;          		//
    unsigned char 				device_id[16];   		// 
    unsigned char 				CarTypeNum;             // 
    unsigned char 				DataPackFlag;			//
    unsigned char 				msg_body_num;     		//
    unsigned short int 		msg_len;        		//
}MsgHeadStr;

#pragma pack()


#pragma pack(1)
typedef struct 
{
	unsigned short int 		MsgDeviceType;                  //
	unsigned short int 		MsgDeviceLen;                 	//
	
	unsigned char      		MsgManuNum;            			//    	      	
	unsigned char      		MsgTerminalType;        		// 
	unsigned char 				MsgUserNum;                     //
	unsigned char      		MsgAppVer1;              		// 
	unsigned char 				MsgAppVer2;           			//
	unsigned char      		MsgHardwareVer;             	//
	
}MsgDeviceStr;
#pragma pack()


/***********************************************
**	
************************************************/
#pragma pack(1)

typedef struct 
{
	unsigned short int  	MsgGnssType;                  	//
	unsigned short int 		MsgGnssLen;                   	//
	
	unsigned int       		MsgGnssLatitude;        		//
	unsigned int       		MsgGnssLongitude;             	//
	unsigned short int	 	MsgGnssSpeed;           		//
	unsigned short int	 	MsgGnssAzimuth;         		//
	short int 						MsgGnssAltitude;        		//
	unsigned char      		MsgGnssYear;                  	//
	unsigned char      		MsgGnssMon;                   	//
	unsigned char      		MsgGnssDay;						//
	unsigned char      		MsgGnssHour;                 	//
	unsigned char      		MsgGnssMin;                   	//
	unsigned char      		MsgGnssSec;                   	//
	unsigned char      		MsgGnssSatelliteNum;         	//
	unsigned char      		MsgGnssViewNum;              	//
	unsigned short 				MsgGhdopV;						//
	unsigned char      		MsgGnssStatus;              	//
}MsgGnssStr;

#pragma pack()



/*************************************************
**
*************************************************/
#pragma pack(1)

typedef struct
{
	unsigned short int   			MsgInputType;					//
	unsigned short int 				MsgInputLen;					//
		
	unsigned char      				MsgInputIo;						//
	unsigned char 						MsgAcc;     					//
	unsigned char 						MsgMoto;						//
	unsigned short int 				MsgInputFrq1;					//
	unsigned short int 				MsgInputFrq2;					//
	unsigned short int 				MsgInputFrq3;					//
	unsigned short int 				MsgInputFrq4;					//
	unsigned short int 				MsgPowVol;						//
	unsigned short int 				MsgBatteryVol;					//
	unsigned short int 				MsgInputVol1;					//
	unsigned short int 				MsgInputVol2;					//
	unsigned int							MsgWarnValue;					//
	unsigned char 						MsgLine;   						//
} MsgInputStr;

#pragma pack()



/************************
**	定义玉米机 CAN消息体
**	
*************************/

#pragma pack(1)
typedef struct
{
	unsigned short int 					MsgMaizeMechType;				//
	unsigned short int					MsgMaizeMechLen;        		//

	unsigned short int 					MsgSysVol;           	 		//
	unsigned char 							MsgTempWater;           		//
	unsigned short int 					MsgEngineRotate;		 		//
	unsigned short int 					MsgEngineRotateSet;     		//
	unsigned char 							MsgEngineTorque;				//
	unsigned char 							MsgFuelTemp;					//
	unsigned short int 					MsgOilTemp;         			//
	unsigned char 							MsgAirPressure;         		//
	unsigned short int   				MsgEngineNacelleTemp;			//
	unsigned short int 					MsgAirTemp;						//
	unsigned char 							MsgEnteredAirTemp;				//
	unsigned int								MsgEngineWorkTime;				//
	unsigned short int 					MsgTravelSpeed;					//
	unsigned int 								MsgOnceTravel;					//
	unsigned int 								MsgTotalTravel;  				//
	unsigned int 								MsgOnceFuel;      				//
	unsigned int 	   						MsgTotalFuel;  					//
	unsigned char								MsgRelativeOilPressure;			//
	unsigned char 							MsgAbsoluteOilPressure;			//
	unsigned char								MsgRelativeAddPressure;			//
	unsigned char 							MsgAbsoluteAddPressure;			//
	unsigned short int					MsgFuelNum;						//
	unsigned char		 						MsgFuelPercent;					//
	unsigned char 							MsgOilPosition;					//
	unsigned short int  				MsgCrankcasePressure;   		// 
	unsigned char 							MsgCoolPressure;  				//
	unsigned char 							MsgCoolPosition;				//
	unsigned char     					MsgLockCarState;				//
	unsigned char								MsgActivateStatus;   			//
	unsigned char 							MsgKeyStatus;     				//
	unsigned char 							MsgTerIDStatus;					//
	unsigned char 							MsgWorkFlag;   					//
	unsigned char 							MsgCarWarnValue1;				//
	unsigned char 							MsgCarWarnValue2;				//
	unsigned char 							MsgFoodFullWarn;				//
	unsigned char 							MsgClutchState;  				//
	unsigned short int	 				MsgStripRotate;					//
	unsigned short int	 				MsgLiftRotate;					//
	unsigned short int					MsgCutTableHigh;				//
	unsigned short int    			MsgNC1;							//
	unsigned short int     			MsgNC2;							//
	unsigned int  							MsgNC3;							//
	unsigned int 								MsgNC4;     					//
}MsgCanMaizeStr;



/******************************************
**	
**
******************************************/
#pragma pack(1)
typedef struct
{
	unsigned char 			FrameStart[3];			// 
  unsigned char				msg_id;          		//
  unsigned char 			device_id[16];   		// 
  unsigned short int 	DataPackFlag;			//
  unsigned char 			msg_body_num;     		// 	
  unsigned short int 	msg_len;        		//  
}SysCmdStr;
#pragma pack()




/************************************************
**	
*************************************************/
#pragma pack(1)

typedef struct
{
	unsigned char 			DataBuf[BLIND_BUF];     				//
	unsigned short int 	DataLen;          				//
	unsigned char 			DataCrc;          				//
	unsigned char       DataBig;          				//
	
}FiFoStr;
#pragma pack()

/*************************************************************
**	
*************************************************************/
#pragma pack(1)

typedef struct
{
	unsigned short int 		QNum;             						//
	unsigned short int 		QWrite;           						//
	unsigned short int 		QRead;            						//
	FiFoStr            		QData[BLIND_NUM];       				//
}SendQueueStr;
#pragma pack()






#endif

