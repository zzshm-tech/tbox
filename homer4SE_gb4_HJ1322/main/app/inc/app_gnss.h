


#ifndef _APP_GNSS_H
#define _APP_GNSS_H


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_task_wdt.h"
#include "esp_netif.h"
#include <esp_event.h>


#include "drv_rtc.h"


/************************** GNSS定位信息 ****************************/

#pragma pack(1)

struct gnss_info_str
{	
    uint8_t 		        state;        			//是否定位 0:未定位
    uint8_t 				longitude_ew; 			//东西经 0:东 1:西
    uint8_t 				latitude_sn;  			//南北纬 0:北 1:南
    uint32_t 				longitude_normal;   	//经度 xxxyyyyyy = xxx.yyyyyy(度)*1000000
    uint32_t 				latitude_normal;    	//纬度  度
    uint32_t 				altitude;    			//海拔 米  
    uint16_t 				heading;     			//航向 度
    uint16_t 				speed;       			//速度 公里/小时  保留一位有效小数 扩大10倍  
    uint8_t 				satellite_num; 			//使用卫星个数
    uint16_t 			    hdop;        			//水平精度因子
	uint32_t 				nmea_cnt;  				//接收到nmea信息
    uint8_t 	 			gps_sate_num;			//GPS可视卫星数量
	uint8_t 				bd_sate_num;			//北斗可视卫星数量
	struct rt_tm		    utc_time;				//世界时间（未加时区）
	struct rt_tm 			btc_time;				//北京时间
	uint32_t 				utc_unix;				//时间时间时间戳（1970.1.1 0.0.0）
	uint32_t				btc_unix;				//北京时间时间戳（1970.1.1 0.0.0）
	uint8_t					rtc_set_state;			//设备授时标志
	uint8_t  				module_state; 			//GNSS模块状态
	uint8_t					ant_state;			    //
		/**
		定位模块状态：默认0：正常
											1：
											2：
											3：
		**/
	uint32_t 				longitude_nmea;         //经度 度分.分
	uint32_t				latitude_nmea;          //维度 度分.分  

	uint8_t 				ant;  
};


/************************** GNSS定位信息 ****************************/

struct gnss_str
{
	SemaphoreHandle_t 			mutex;
 
    uint16_t 					cnt_reset; 			//定位失败重启的时间
	uint32_t 					nmea_cnt;			//接收到Nmea数据包自动加一
	uint32_t 					real_longitude;   	//经度  当前经度
    uint32_t 					real_latitude;      //纬度  当前维度
	uint32_t					prev_longitude;		//上一个定位点
	uint32_t 					prev_latitude;      //上一个定位点
	uint8_t						state;              //
	uint8_t						step;               //
	uint8_t						erro_cnt;           //错误计数器
	uint8_t						state_back;         //定位状态备份
	uint32_t					ticks;   
	uint8_t						debug_state;		//
};

#pragma pack()



uint8_t	read_gnss_info(struct gnss_info_str *source);
uint8_t read_gnss_positing_state(void);

/***********************************
**	      //维度(经纬度)
************************************/

uint32_t read_gnss_latitude(uint8_t n);
uint32_t read_gnss_longitude(uint8_t n);       
uint16_t read_gnss_speed(void);
uint16_t read_gnss_heading(void);
uint8_t read_gnss_utc_time(struct rt_tm *ptime);
uint8_t read_gnss_satellite_num(void);
uint8_t read_gnss_ant_state(void);

uint16_t read_gnss_altitude(void);
uint8_t read_gnss_bd_sate_num(void);
uint8_t  read_gnss_gps_sate_num(void);
uint16_t read_gnss_hdop(void);
uint8_t read_gnss_latitude_sn(void);
uint8_t read_gnss_longitude_ew(void);
uint32_t read_gnss_module_state(void);
void  delete_gnss_info_files(void);
uint8_t init_gnss_debug_state(uint8_t n);
void thread_entry_gnss(void *parameter);



#endif


