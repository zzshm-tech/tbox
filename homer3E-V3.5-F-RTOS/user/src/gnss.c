
/*************************
**
**	2019.1.11
*****************************/

#include <stdio.h>
#include <string.h>

#include "fr_drv_timer.h"
#include "fr_drv_uart.h"
#include "fr_drv_gpio.h"
#include "data_type.h"
#include "pro_data.h"
#include "mon.h"
#include "rtc.h"



#include "FreeRTOS.h"     																		//



extern QueueHandle_t  					set_time_queue;   				//定义队列变量-设置时间（?????）


/*********************************************************/

static TickType_t 							gnss_ticks = 0xFFFFFFFF;     //

static unsigned char 						gnss_recv_buf[1000];         //接收缓冲区

struct gnss_info_str						gnss_info;									 //定位信息

static SemaphoreHandle_t 				gnss_semaphore = NULL;				//

/****************************
**	接收完成回调函数
**	(???回调函
******************************/
static void gnss_ticks_handle(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken; 
	
	xHigherPriorityTaskWoken = pdTRUE;          //注意这里的信号量问题
	
	if(gnss_ticks == 0xFFFFFFFF)
		return;
	if(xTaskGetTickCount() - gnss_ticks > 1)
	{
		gnss_ticks = 0xFFFFFFFF;
		xSemaphoreGiveFromISR(gnss_semaphore,&xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}


/**********************
**	收到GNSS数据
************************/

static void gnss_rx_handle(void)
{
	gnss_ticks = xTaskGetTickCount();
}


/**********************
**	复位GPS
************************/

void gnss_reset(void)
{
	fr_power_gps_off();                    //()
	fr_power_gps_vback_off();
	vTaskDelay(30);         		//
	fr_power_gps_vback_on();
	fr_power_gps_on();                    //()
}



/***********************
**	解析RMC
**	经度
**	维度
**	UTC时间
**	经度标志
**	维度标志
**	
************************/

static void parse_rmc(unsigned char *data)
{
  unsigned char len = 0;
	
  unsigned char tmp = 0;
	unsigned char *p = NULL;
	unsigned char tmp_buf[20];
	struct time_str *p_time;
	
	len = nmea_verfy(data);
	if(len == 0 || data == NULL)
		return;      //校验不正确
	
	p = data;
	
	if(get_data_str(1,2,p,tmp_buf,len) > 0)       //UTC-time
	{
		gnss_info.utc_time.hour = (tmp_buf[0] - '0') * 10 + (tmp_buf[1] - '0');
		gnss_info.utc_time.min  = (tmp_buf[2] - '0') * 10 + (tmp_buf[3] - '0');
		gnss_info.utc_time.sec  = (tmp_buf[4] - '0') * 10 + (tmp_buf[5] - '0');
	}
	
	if(get_data_str(2,3,p,tmp_buf,len) > 0)     //GNSS-state 定位状态 “A”：定位 “V”
	{
		if(tmp_buf[0] == 'A')
		{
			gnss_info.state = 'A';
		}
		else
		{
			gnss_info.state = 'V';
		}
	}
	
	if((tmp = get_data_str(3,4,p,tmp_buf,len)) > 0)      //GNSS-维度
	{
		tmp_buf[tmp] = '\0';
		gnss_info.latitude = fr_atof((const char *)tmp_buf);
		gnss_info.latitude = ((unsigned int)(gnss_info.latitude / 100) + (gnss_info.latitude - (unsigned int)(gnss_info.latitude / 100 ) * 100) / 60);
	}
	
	if(get_data_str(4,5,p,tmp_buf,len) > 0)							//gnss_info-维度-标志
	{
		gnss_info.latitude_sn = tmp_buf[0];
		
	}
	
	if((tmp = get_data_str(5,6,p,tmp_buf,len)) > 0)             //GNSS-经度
	{
		tmp_buf[tmp] = '\0';
		gnss_info.longitude = fr_atof((const char *)tmp_buf);
		gnss_info.longitude = ((unsigned int)(gnss_info.longitude / 100) + (gnss_info.longitude - (unsigned int)(gnss_info.longitude / 100) * 100) / 60);
		
	}
	
	if(get_data_str(6,7,p,tmp_buf,len) > 0)            //GNSS-经度标志
	{
		gnss_info.longitude_ew = tmp_buf[0];
	}
	
	if((tmp = get_data_str(7,8,p,tmp_buf,len)) > 0)    //速度
	{
		tmp_buf[tmp] = '\0';
		gnss_info.speed = fr_atof((const char *)tmp_buf) * 1.85;
	}
	
	if((tmp = get_data_str(8,9,p,tmp_buf,len)) > 0)    //航向角度
	{
		tmp_buf[tmp] = '\0';
		gnss_info.heading = (unsigned short int)fr_atof((const char *)tmp_buf);
	}
	
	if(get_data_str(9,10,p,tmp_buf,len) > 0) 
	{
		gnss_info.utc_time.year = (tmp_buf[4] - '0') * 10 + (tmp_buf[5] - '0');
		gnss_info.utc_time.mon  = (tmp_buf[2] - '0') * 10 + (tmp_buf[3] - '0');
		gnss_info.utc_time.day	= (tmp_buf[0] - '0') * 10 + (tmp_buf[1] - '0');
		
		gnss_info.timestamp = calendar_to_timestamp(&gnss_info.utc_time) + 28800;
		p_time = timestamp_to_calendar(gnss_info.timestamp);
		
		gnss_info.btc_time.year = p_time->year;
		gnss_info.btc_time.mon = p_time->mon;
		gnss_info.btc_time.day = p_time->day;
		gnss_info.btc_time.hour = p_time->hour;
		gnss_info.btc_time.min = p_time->min;
		gnss_info.btc_time.sec = p_time->sec;
	}
}


/***********************
**	使用卫星数量：
**	水平精度因子
**	海拔高度
************************/

static void parse_gga(unsigned char *data)
{
	unsigned char len = 0;
  unsigned char tmp = 0;
	unsigned char *p = NULL;
	unsigned char tmp_buf[20];
	
	len = nmea_verfy(data);
	if(len == 0 || data == NULL)
		return;      //校验不正确
	
	p = data;
	
	if((tmp = get_data_str(7,8,p,tmp_buf,len)) > 0)    //使用卫星数量
	{
		tmp_buf[tmp] = '\0';
		gnss_info.satellite_num = (unsigned char )fr_atof((const char *)tmp_buf);
	}
	
	if((tmp = get_data_str(8,9,p,tmp_buf,len)) > 0)    //水平经度因子
	{
		tmp_buf[tmp] = '\0';
		gnss_info.hdop = fr_atof((const char *)tmp_buf);
	}
	
	if((tmp = get_data_str(9,10,p,tmp_buf,len)) > 0)    //
	{
		tmp_buf[tmp] = '\0';
		gnss_info.hdop = (unsigned short int)fr_atof((const char *)tmp_buf);
	}
}



/***********************
**	GPS可视卫星数量
************************/

static void parse_gpgsv(unsigned char *data)
{
	unsigned char len = 0;
  unsigned char tmp = 0;
	unsigned char *p = NULL;
	unsigned char tmp_buf[20];
	
	len = nmea_verfy(data);
	if(len == 0 || data == NULL)
		return;      //校验不正确
	
	p = data;
	
	if((tmp = get_data_str(3,4,p,tmp_buf,len)) > 0)    //使用卫星数量
	{
		tmp_buf[tmp] = '\0';
		gnss_info.gps_sate_num = (unsigned char )fr_atof((const char *)tmp_buf);
	}
}




/***********************
**	解析北斗可视卫星数量
************************/

static void parse_bdgsv(unsigned char *data)
{
	unsigned char len = 0;
  unsigned char tmp = 0;
	unsigned char *p = NULL;
	unsigned char tmp_buf[20];
	
	len = nmea_verfy(data);
	if(len == 0 || data == NULL)
		return;      //校验不正确
	
	p = data;
	
	if((tmp = get_data_str(3,4,p,tmp_buf,len)) > 0)    //使用卫星数量
	{
		tmp_buf[tmp] = '\0';
		gnss_info.bd_sate_num = (unsigned char )fr_atof((const char *)tmp_buf);
	}
}



/**********************
**	收到GNSS数据
************************/

void gps_info_handler(unsigned char *data_buf,unsigned short int size)
{
	int index;
	
	if((index = look_for_str(data_buf,(unsigned char *)"GNRMC",size)) > 0 || (index = look_for_str(data_buf,(unsigned char *)"GPRMC",size)) > 0)
	{
		parse_rmc(data_buf + index);
	}
	else
	{
		gnss_info.state = 'V';          //（？？？？？？？？？？）
	}
	if((index = look_for_str(data_buf,(unsigned char *)"GNGGA",size)) > 0 || (index = look_for_str(data_buf,(unsigned char *)"GPGGA",size)) > 0)
	{
		parse_gga(data_buf + index);
	}
	
	if((index = look_for_str(data_buf,(unsigned char *)"GNGSV",size)) > 0)
	{
		parse_gpgsv(data_buf + index);
	}
	
	if((index = look_for_str(data_buf,(unsigned char *)"BDGSV",size)) > 0)
	{
		parse_bdgsv(data_buf + index);
	}
	
}



/*************************
**	处理GNSS任务
**	BUG1:2020.02.25
**	
**************************/

void task_gnss(void *data)
{
	BaseType_t sem_state;
	unsigned char cnt_time = 0;
	unsigned char cnt_reset = 0;
	
	data = data;
	fr_power_gps_on();                    //()
	fr_power_gps_vback_on();
	fr_init_uart1();
	fr_set_uart1_rx_hook(gnss_rx_handle);
	fr_timer3_sethook(gnss_ticks_handle);
	cnt_time = 5;
	if(gnss_semaphore == NULL)
		gnss_semaphore = xSemaphoreCreateBinary();
	
	for(;;)
	{
		sem_state = xSemaphoreTake(gnss_semaphore,200);   //2秒钟如果收不到定位信息数据报错
		if(sem_state > 0)
		{
			sem_state = read_uart_pkt(1,gnss_recv_buf,sizeof(gnss_recv_buf));
			//fr_printf("recv gnss data:%d\r\n",sem_state);
			if(sem_state > 0 && sem_state < 1000)
			{
				if(read_ant_state() == 0)    //定位天线有问题不进行解析
					gps_info_handler(gnss_recv_buf,sizeof(gnss_recv_buf));             //解析GNSS信息
			
				
				fr_led_blue_on();           //点亮蓝色LED
			}
			else
			{
				gnss_info.state = 'V';
				fr_led_blue_off(); 
				cnt_time = 5;
			}
			if(gnss_info.state == 'A')
			{
				if(cnt_time > 0)
				{
					cnt_time--;
					continue;
				}
				sem_state = xQueueSendToBack(set_time_queue,&gnss_info.timestamp, 0);
				if(sem_state != pdPASS )
				{
					fr_printf("gnss  Could not send to the queue.........\r\n");
				}
				continue;
			}
		}
		gnss_info.state = 'V';
		vTaskDelay(30);         		//LED亮300ms 
		fr_led_blue_off();					//关闭LED灯，
		
		if(cnt_reset++ > 240)
		{
			cnt_reset = 0;
			fr_power_gps_off();                    //()
			fr_power_gps_vback_off();
			vTaskDelay(30);         		//
			fr_power_gps_vback_on();
			fr_power_gps_on();                    //()
			fr_printf("gnss_reset.........\r\n");
		}
		
		fr_printf("gnss state.............%c\r\n",gnss_info.state);
	}
}

/****************************
** 关闭GNSS模块
****************************/

void close_gnss_modle(void)
{
	fr_close_uart1();
	fr_power_gps_off();
}



/****************************
** 读取gnss信息
****************************/

//void read_gnss_info(struct gnss_info_str *p_arg)
//{
//	if(p_arg == NULL)
//		return;
//	//
//	memcpy((unsigned char *)p_arg,(const unsigned char *)&gnss_info,sizeof(gnss_info));
//}

 /************************
 **	
 *************************/

 unsigned short int read_gnss_altitude(void)    					// 海拔 米*/
 {
	 return gnss_info.altitude;
 }

 
 /************************
 **	
 *************************/
 
 unsigned short int read_gnss_heading(void)     					// 航向 度*/
 {
	 return gnss_info.heading;
 }

 
 /**********************
*	可视卫星数量(北斗)
***********************/
unsigned char	read_bd_sate_num(void) 					// 可视卫星数量(北斗)
{
	return gnss_info.bd_sate_num;
}

/**********************
*	可视微信数量()
***********************/

unsigned char read_gps_sate_num(void)					// 可视微信数量()
{
	return gnss_info.gps_sate_num;
}


/**********************
*	位置经度因子
***********************/

unsigned short int read_gnss_hdop(void)        					// 水平精度因子*/
{
	return gnss_info.hdop * 10;
}

 
/****************************
** 经度 dddmm.mmmm
****************************/
double read_gnss_longitude(void)
{
	return gnss_info.longitude;
}

/****************************
** 维度 ddmm.mmmm
****************************/
double read_gnss_latitude(void)
{
	return gnss_info.latitude;
}

/*********************
**	读取gnss定位状态
**	'A' 已经定位
**	'V" 未定位
************************/

unsigned char read_gnss_state(void)
{
	return gnss_info.state;
}


/*********************
**	读取GNSS速度
**	返回整数，保留一位有效小数
************************/
unsigned short int read_gnss_speed(void)
{
	return gnss_info.speed * 10;
}


/*********************
**	读取BTC时间拷贝
**	BTC时间是在UTC时间基础之上加
************************/
void read_gnss_btc_time(struct time_str *ptime)
{
	memcpy((char *)ptime,(char *)&gnss_info.btc_time,sizeof(struct time_str));
}

/*********************
**	读取UTC时间拷贝
**	UTC时间通过GNSS定位模块获取的原始时间
************************/

void read_gnss_utc_time(struct time_str *ptime)
{
	memcpy((char *)ptime,(char *)&gnss_info.utc_time,sizeof(struct time_str));
}



/*********************
**	读取使用卫星数量
************************/

unsigned char read_satellite_num(void)
{
	return gnss_info.satellite_num;
}


/******************File End***********************/

