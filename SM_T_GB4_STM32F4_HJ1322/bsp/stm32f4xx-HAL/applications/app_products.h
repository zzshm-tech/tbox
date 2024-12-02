




#ifndef _APP_PRODUCTS_H
#define _APP_PRODUCTS_H

#include <stdint.h>
#include <rtthread.h>




#define UART_CONFIG_CMD							0
#define SET_GATEWAY_ADDR 						1
#define SET_UPLOAD_CYCLE 						2
#define SET_LOCK_SECRET 						3

#define SET_SLEEP_TIME							4




#pragma pack(1)




/*********************************
**	自定义UART_CMD参数
*************************************/

struct products_data_t
{
	uint32_t		 cmd;           //参数类型
	uint16_t 	 	 len;					//不同的参数类型，代表不同的解析方法
	uint8_t		 	 data[320];
};




struct config_str
{
	uint8_t 				product_mode;    								//1 生产模式 0 正常模式; 1 生产模式 
	uint8_t 				terminal_id[18];      					//2 设备ID
	uint8_t         terminal_type;		 							//3 设备类型
	uint8_t 				fir_ver;			 									//4 固件版本号  (预留)
	uint8_t 				enterprise_gw_addr[50];      		//5 企标网关地址
	uint16_t 				enterprise_gw_port;     	 			//6 企标网关端口   
	uint32_t 				run_time;            	 					//7 断开ACC后运行时间
	uint32_t 				sleep_time;           					//8 睡眠时间
	uint32_t 				travel_upload_cycle;    				//9 行驶上传周期 (企标数据上传周期)
	uint32_t        work_upload_cycle;      				//10 作业上传周期
	uint16_t  		  distance_upload;     	 					//11 定距离报位
	uint16_t  		  azimuth_upload;       					//12 航向角报位
	uint8_t 				car_type;             					//13 安装车辆类型
	uint8_t 				can_num;												//14 CAN协议号
	uint8_t 				user_code;            					//15 用户编码					
	uint8_t         apn[20];												//16 GSM模块接入点
	uint8_t					user[20];												//17 GSM模块APN用户名
	uint8_t					password[20];										//18 GSM模块APN密码
	uint8_t 				hard_ware;											//19 硬件版本
	uint8_t 				dev_id[3];											//20 设备ID (针对潍柴MD5锁车使用)
	uint8_t					dev_secret[3];									//21 设备秘钥(针对潍柴MD5锁车使用))
	uint8_t 			  archivel_gw_addr[50];      			//22 备案网关地址
	uint16_t 			  archivel_gw_port;     	 				//23 备案网关端口 
	uint8_t 				gateway_addr3[50];      				//24 预留
	uint16_t 			  gateway_port3;     	 						//25 预留  
	uint8_t 				gateway_addr4[50];      				//26 预留
	uint16_t 			  gateway_port4;     	 						//27 预留
	uint8_t 			  pin[20];												//28 VIN vin[0] == 0x5A
	uint8_t  			  public_key[64];									//29 公钥
	uint8_t  			  acl_chipid[16];									//30 加密芯片ID  预留
	uint8_t 				archival_state;									//31 备案状态
	uint8_t 				dev_password[64];								//32 设备秘钥
	uint32_t 				gb_four_upload_cycle;   				//33 国标数据上传周期
	uint16_t 				delay_shutdown_time;						//34 电锁关闭
	uint8_t 				emission;  											//35 排放阶段
	uint8_t         nc[17];													//
  uint32_t        crc_value;											//
};



#pragma pack()




rt_mq_t get_products_mq(void);


uint8_t read_config_state(void);
uint32_t read_config_travel_upload_cycle(void);
uint32_t read_config_work_upload_cycle(void);
uint8_t  read_config_terminal_id(uint8_t *buf,uint8_t buf_size);
uint8_t read_config_car_type(void);
uint8_t read_enterprise_gw_addr(uint8_t *buf,uint8_t buf_size);
uint16_t read_enterprise_gw_port(void);
void read_config_dev_id(uint8_t *buf,uint8_t buf_size);
void read_config_dev_secret(uint8_t *buf,uint8_t buf_size);
rt_uint16_t read_config_sleep_time(void);
uint8_t read_config_hard_ware(void);
void close_rs232_module(void);
uint8_t read_config_product_mode(void);
uint8_t read_config_vin_info(uint8_t *buf,uint8_t size_buf);
uint8_t read_config_vin_state(void);
uint8_t  read_archival_gw_addr(uint8_t *buf,uint8_t buf_size);
uint16_t  read_archival_gw_port(void);
uint8_t read_archival_state(void);
uint32_t read_config_gb_four_upload_cycle(void);
void read_boot_loader_info(uint8_t *buf,uint8_t buf_size);
uint8_t read_config_archival_state(void);
uint32_t read_boot_loaer_ver_num(void);
uint16_t read_config_delay_shutdown_time(void);
uint8_t read_config_user_code(void);
uint8_t read_config_archivel_gw_addr(uint8_t *buf,uint8_t buf_size);
uint16_t read_config_archivel_gw_port(void);
uint8_t read_config_emission(void);

void read_config_gps_id(uint8_t *buf,uint8_t buf_size);
void read_config_mask(uint8_t *buf,uint8_t buf_size);
uint8_t load_products_cfg_info(void);
uint8_t reset_config_info(void);

uint8_t read_config_public_key(uint8_t *buf,uint8_t buf_size);

void thread_entry_products(void *parameter);


#endif
