



#ifndef _APP_PRODUCTS_H
#define _APP_PRODUCTS_H

#include <stdint.h>

#include "queue.h"


#pragma pack(1)

struct config_str
{
	uint8_t 				product_mode;    					// 生产模式 0 正常模式; 1 生产模式  		  1
	uint8_t 				terminal_id[18];      				//设备ID                           		    2
	uint8_t                 terminal_type;		 				//设备类型                          		 3
	uint8_t 				fir_ver;			 				//固件版本号  (预留)                          4
	uint8_t 				enterprise_gw_addr[50];      		//企标网关地址							      5
	uint16_t 				enterprise_gw_port;     	 		//企标网关端口   							6
	uint32_t 				run_time;            	 			//断开ACC后运行时间							7
	uint32_t 				sleep_time;           				//睡眠时间									8
	uint32_t 				travel_upload_cycle;    			//行驶上传周期 (企标数据上传周期)			9
	uint32_t                work_upload_cycle;      			//作业上传周期								10
	uint16_t  		        distance_upload;     	 			//定距离报位								11
	uint16_t  		        azimuth_upload;       				//航向角报位								12
	uint8_t 				car_type;             				//安装车辆类型								13
	uint8_t 				can_num;							//CAN协议号									14
	uint8_t 				user_code;            				//用户编码									15
	uint8_t                 apn[20];							//GSM模块接入点								17
	uint8_t					user[20];							//GSM模块APN用户名							18
	uint8_t					password[20];						//GSM模块APN密码							19
	uint8_t 				hard_ware;							//硬件版本									20
	uint8_t 				dev_id[3];							//设备ID (针对潍柴MD5锁车使用)					21
	uint8_t					dev_secret[3];						//设备秘钥(针对潍柴MD5锁车使用))			22
	uint8_t 			    archivel_gw_addr[50];      			//备案网关地址								23
	uint16_t 			    archivel_gw_port;     	 			//备案网关端口 								24
	uint8_t 				gateway_addr3[50];      			//预留										25  
	uint16_t 			    gateway_port3;     	 				//预留  									26
	uint8_t 				gateway_addr4[50];      			//预留										27
	uint16_t 			    gateway_port4;     	 				//预留										28
	uint8_t 			    vin[20];							//VIN vin[0] == 0x5A						29
	uint8_t  			    public_key[64];						//公钥										30
	uint8_t  			    acl_chipid[16];						//加密芯片ID  预留							31
	uint8_t 				archival_state;						//备案状态									32  
	uint8_t 				dev_password[64];					//设备秘钥									33
	uint32_t 				gb_four_upload_cycle;   			//国标数据上传周期							34
  	uint8_t                 token[32];                     		//token										35
	uint8_t					nj_state;							//农业3和1补贴状态							36
	uint32_t 				farm_manu;							//农机补贴 厂商编码							37
	uint16_t 				waring_time_ms;       				//报警上传间隔 单位ms						38
    uint8_t 				heartbeat_time_sec;    				//心跳上传间隔 单位秒						39
    uint16_t 				blind_time_sec;      				//盲区上传间隔 单位秒						40
	uint16_t				delay_shutdown_time;				//唤醒后的工作时间							41
	uint8_t					eco_mark[4];						//环保标志符
  	uint32_t                crc_value;                  		//配置信息校验值							
};








/*********************************
**	自定义UART_CMD参数
*************************************/

struct products_mq_str
{
	uint8_t		 	cmd;           //参数类型
	uint32_t 	 	len;					//不同的参数类型，代表不同的解析方法
	uint8_t		 	data[320];
};


#pragma pack()

uint8_t load_products_cfg_info(void);
uint8_t get_products_cfg_model(void);
uint32_t read_config_qb4_upload_cycle(void);
uint32_t read_config_gb4_upload_cycle(void);
uint8_t read_config_vin_state(void);
uint8_t read_config_vin_info(uint8_t *buf,uint8_t size_buf);
uint8_t read_config_car_type(void);
uint16_t read_config_farm_manu(void);
uint8_t read_config_terminal_id(uint8_t *buf,uint8_t size);
uint8_t read_config_token(uint8_t *data,uint8_t size);
uint16_t build_config_info(uint8_t *buf,uint16_t size,uint8_t flag);
uint8_t read_enterprise_gw_addr(uint8_t *buf,uint8_t buf_size);
uint16_t read_enterprise_gw_port(void);
uint8_t read_config_archivel_gw_addr(uint8_t *buf,uint8_t buf_size);
uint16_t read_config_archivel_gw_port(void);
uint8_t read_config_nj_state(void);
uint32_t read_config_sleep_cycle(void);
uint16_t read_config_delay_shutdown_time(void);
uint8_t read_config_user_code(void);
uint8_t erase_products_cfg_info(void);
uint8_t read_config_eco_mark(uint8_t *buf,uint8_t size);
uint8_t read_config_eco_mark(uint8_t *buf,uint8_t size);


QueueHandle_t get_products_queue(void);

void thread_entry_products(void *parameter);



#endif










