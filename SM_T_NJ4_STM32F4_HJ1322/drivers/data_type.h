
/**************************************
**
**	
***************************************/

#ifndef _DATA_TYPE_H
#define _DATA_TYPE_H




#define   GPS_VALID   		1   				// 
#define   GPS_INVALID		0 						// 
	 
#define   NORTH_LATITUDE    1  				//
#define   SOUTH_LATITUDE	0	 					// 
	 
#define   EAST_LONGTITUDE   1  				// 
#define   WEST_LONGTITUDE	0  					// 



struct stopwatch16 
{					
	unsigned short int	 start_time;
};



struct stopwatch32 
{					
	unsigned int	 start_time;
};



/*************************
**	系统时间
**	整个系统处理的时间范围是：2000-1-1,0:0:0 ----- 
**************************/

#pragma pack(1)
typedef struct
{
	unsigned char Year;				//系统时间-年
	unsigned char Mon;				//系统时间-月
	unsigned char Mday;				//系统时间-日
	unsigned char Wday;				//系统时间-周
	unsigned char Hour;				//系统时间-时
	unsigned char Min;				//系统时间-分
	unsigned char Sec;				//系统时间-秒
	unsigned int UnixTime;		//系统时间-时间戳，2000年1月1日0时0分0秒开始计算
}TimeStr;
#pragma pack()







/***************************
**	定位数据结构
**	原始定位数据
****************************/
typedef struct 
{
	double 								Latitude;										//维度
	double 								Longitude;	  							//精度
	unsigned short int 		Altitude;       						//海拔高度
	unsigned short int 		Azimuth;	  								//航向角度
	float 								Speed;		            			//行驶速度
	float  								Ghdop_v;      							//定位因子
	unsigned char 				Satellite_num;         	 		//使用卫星数量
	unsigned char 				Satellite_view_num; 				//可视卫星数量
	unsigned char 				bd_view_num;								//可视北斗卫星数量
	unsigned char 				Status;                 		//定位状态
	unsigned char					TYear;                      //GNSS时间-年 
	unsigned char 				TMon;												//GNSS时间-月
	unsigned char 				TDay;												//GNSS时间-日
	unsigned char 				THour;											//GNSS时间-时
	unsigned char					TMin;												//GNSS时间-分
	unsigned char 				TSec;                       //GNSS时间-秒                         
	unsigned char 				latitude_ns;            		//维度标识
	unsigned char 				longitude_ew;           		//经度标识
}GNSSSTR; 






/***************************
**	
****************************/
#pragma pack(1)

typedef struct 
{
	unsigned char 				AccState;								//
	unsigned char 				MotoState;							//
	unsigned char 				GnssAntState;           //
	unsigned char 				GnssAntShort;						//
	unsigned char 				ShellState;							//
	unsigned short int 		PowerVol;               //
	unsigned short int 		BatteryVol;             //
	unsigned short int 		McuTemp;                //					
}SysDataInfoStr;

#pragma pack()




/**************************
**	CAN数据结构体
***************************/

typedef struct
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
}Str_CanTxdData;




/**************************
**	CAN数据结构体
***************************/

typedef struct
{
    unsigned int id;													//CANID号
    unsigned char data[8];											//CAN数据区域   工8字节
}RxCanDataStr;




typedef struct 
{
	unsigned char di_in_1;
	unsigned char di_in_2;
	unsigned char di_in_3;
	unsigned char di_in_4;
	
	unsigned short int ai_in1;
	unsigned short int ai_in2;
	unsigned short int ai_in3;
	unsigned short int ai_in4;
}sys_in_str;




/************************************************
**	系统配置信息
************************************************/
typedef struct
{

	unsigned char 					device_id[18];      			//设备ID
	unsigned char           device_type;		 					//设备类型
	unsigned char 					gateway_addr[50];      		//
	unsigned short int			gateway_prot;      				//网关端口
	unsigned char 					hw_ver_num;			 					//硬件版本号
	unsigned int 						sleep_time;           		//休眠时间
	unsigned int 						upload_time;    					//数据自动推送周期
	unsigned short int  		uplaod_distance;     	 		//距离报位(暂时保留)
	unsigned short int  		uplaod_azimuth;       		// 
	unsigned char 					car_type;             		//车类型
	unsigned char 					user_code;            		//用户代码
	unsigned char 					icc_id[25];								//GSM模块ICCID	
}config_str;





#endif












