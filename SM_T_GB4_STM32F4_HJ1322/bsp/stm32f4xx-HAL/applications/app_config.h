

#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_



#include <rtthread.h>


/************************************************
**	
************************************************/

#pragma pack(1)
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
#pragma pack()



rt_int8_t read_config_state(void);
rt_err_t read_config_info(void);
rt_uint8_t save_config_info(void);
rt_uint16_t build_config_info(rt_uint8_t *buf,rt_uint16_t buf_size,rt_uint8_t flag);
rt_uint8_t analysis_config_info(rt_uint8_t *source,rt_uint16_t len);
unsigned int read_config_travel_upload_cycle(void);
unsigned int read_config_work_upload_cycle(void);
unsigned char read_config_terminal_id(unsigned char *buf,unsigned char buf_size);
unsigned char read_config_car_type(void);

unsigned char read_gateway_addr1(unsigned char *buf,unsigned char buf_size);
unsigned short int read_gateway_port1(void);
void read_config_dev_id(unsigned char *buf,unsigned char buf_size);
void read_config_dev_secret(unsigned char *buf,unsigned char buf_size);


#endif






