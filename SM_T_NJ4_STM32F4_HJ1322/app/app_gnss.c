

#include <stdio.h>
#include <string.h>
#include <stdint.h>


#include "FreeRTOS.h"
#include "task.h"

#include "pro_data.h"

#include "drv_gpio.h"
#include "drv_uart.h"
#include "drv_timer.h"

#include "app_gnss.h"





static struct gnss_info  				gnss = {0};

static struct gnss_data_t 			gnss_data = {0};   								//GNSS定位信息缓冲区

static uint8_t 									data_buff[1024] = {0};

static uint8_t 						    gnss_module_flag    = 0;             //GNSS模块类型标志 


uint8_t read_gnss_utc_time(struct rt_tm *ptime)
{
	if(ptime == NULL)
		return 1;
	
	memcpy((char *)ptime,(char *)&gnss_data.utc_time,sizeof(struct rt_tm));

	return 0;
}

/***********************************
**	返回定位状态
************************************/
uint8_t read_gnss_positing_state(void)
{
	uint8_t rv = 'V';

    rv = gnss_data.state;
        
	return rv;
}




/**************************************
**
**************************************/
 
void gnss_rx_data_handle(uint16_t size)
{
    gnss.ticks = xTaskGetTickCount(); /* 取收到数据时的tick值*/
    gnss.len = size;          /* 当前的接收的数据长度*/
}

/********************************************************
**
********************************************************/
void gnss_ticks_handle(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if(gnss.ticks == 0xffffffff)
		return;

  if(xTaskGetTickCount() - gnss.ticks > 1)
  {
		gnss.ticks = 0xffffffff;     /* tick值复位*/
      
		if(gnss.semaphore != NULL)
			xSemaphoreGiveFromISR(gnss.semaphore, &xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}




/*********************************
**	返回模块状态
**********************************/

uint32_t read_gnss_module_state(void)
{
	uint32_t rv;

	rv = gnss_data.module_state;

	return rv;
}



/********************************************
**	
*********************************************/

uint8_t read_gnss_latitude_sn(void)
{
	uint8_t rv;

	rv = gnss_data.latitude_sn;

	return rv;
}



/********************************************
**	
*********************************************/
uint8_t read_gnss_longitude_ew(void)
{
	uint8_t rv;

	rv = gnss_data.longitude_ew;

	return rv;
}



/******************************
**
*******************************/
uint16_t read_gnss_altitude(void)
{
	uint16_t rv;

	rv = gnss_data.altitude;

	return rv;
}



/******************************
**
*******************************/
uint8_t read_gnss_bd_sate_num(void)
{
	uint8_t rv;

	rv = gnss_data.bd_sate_num;

	return rv;
}




/******************************
**
*******************************/
uint8_t  read_gnss_gps_sate_num(void)
{
	uint8_t rv;

	rv = gnss_data.gps_sate_num;

	return rv;
}   



/******************************
**
*******************************/
uint16_t read_gnss_hdop(void)
{
	uint16_t rv;

	rv = gnss_data.hdop;

	return rv;
}




/********************************************
**	过滤GNSS数据
**	
*********************************************/

static uint8_t filter_gnss_data(void)
{
	double tmp;

	tmp = ComputeDistance((double)(gnss_data.real_latitude / 1000000.0),(double)(gnss_data.real_longitude / 1000000.0),(double)(gnss_data.prev_latitude / 1000000.0),(double)(gnss_data.prev_longitude / 1000000.0)) * 1000;

	if(tmp > 70)             //120Km/h  
	{
		return 0;
	}
	
	return 1;
}



/****************************************************
**	
*****************************************************/
static uint8_t gps_ascii_to_hex(uint8_t ascii_high, uint8_t ascii_low)
{
    if ((ascii_high >= 0x30) && (ascii_high <= 0x39))
        ascii_high -= 0x30;
    else if ((ascii_high >= 0x41) && (ascii_high <= 0x46))
        ascii_high -= 0x37;
    else if ((ascii_high >= 0x61) && (ascii_high <= 0x66))
        ascii_high -= 0x57;

    if ((ascii_low >= 0x30) && (ascii_low <= 0x39)) //数字0-9
        ascii_low -= 0x30;
    else if ((ascii_low >= 0x41) && (ascii_low <= 0x46)) //大写字母A-F
        ascii_low -= 0x37;
    else if ((ascii_low >= 0x61) && (ascii_low <= 0x66)) //小写字母a-f
        ascii_low -= 0x57;

    return (ascii_high << 4) | ascii_low;
}


/*******************************************
**	
*******************************************/

static uint8_t gnss_data_verify(uint8_t *gps_data, uint16_t *len)
{
    uint8_t i = 0, k = 0;
    uint8_t *tmp = NULL;
    uint8_t data_check_value = 0; //计算的校验和
    uint8_t recv_check_value = 0; //接收的校验和

    tmp = gps_data;
    for (i = 0; (tmp[i] != '*'); i++)
    {
        if (i > 100) //校验星号的存在，如不存在或数据过长都不对
            return 0;
    }

    for (k = 1; k < i; k++)
        data_check_value ^= tmp[k];

    *len = i + 1 + 2; //返回校验数据的长度
    recv_check_value = gps_ascii_to_hex(tmp[i + 1], tmp[i + 2]);

    if (data_check_value == recv_check_value)
        return 1; //校验通过
    else
        return 0;
}


/******************************************
**	 功能  : 解析GxGGA数据
**	 输入参数：data：执行GxGGA数据包
**	返回: None
** 	日期: 2018-4-20 13:49:06
**	解析：使用卫星数量
          水平经度因子
					海拔高度
 ******************************************/

static void parse_gga(uint8_t *data,uint16_t len)
{
	uint8_t tmp = 0;
	uint8_t *p = NULL;
	uint8_t tmp_buf[20];
	
	if(data == NULL)
		return;
	
	p = data;

	//使用卫星数量
	if((tmp = get_data_str(7,8,p,tmp_buf,len)) > 0)       
	{
		tmp_buf[tmp] = '\0';
		gnss_data.satellite_num = (uint8_t)fr_atof((const char *)tmp_buf);
	}
	
  //水平进度因子 HDOP
	if((tmp = get_data_str(8,9,p,tmp_buf,len)) > 0)      
	{
		tmp_buf[tmp] = '\0';
		gnss_data.hdop = (uint16_t)(fr_atof((const char *)tmp_buf) * 100);
	}
	
	
  //海拔 高度
	if((tmp = get_data_str(9,10,p,tmp_buf,len)) > 0)    
	{
		tmp_buf[tmp] = '\0';
		gnss_data.altitude = (uint8_t)fr_atof((const char *)tmp_buf);
	} 
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

static void parse_rmc(uint8_t *data,uint16_t len)
{
	double tmp_dou;
    uint8_t tmp = 0;
	uint8_t *p = NULL;
	uint8_t tmp_buf[20];
	
	struct rt_tm now;
	
	if(data == NULL)
		return;
	
	p = data;
	
	if(get_data_str(1,2,p,tmp_buf,len) > 0)       //UTC-time
	{
		gnss_data.utc_time.hour = (tmp_buf[0] - '0') * 10 + (tmp_buf[1] - '0');
		gnss_data.utc_time.min  = (tmp_buf[2] - '0') * 10 + (tmp_buf[3] - '0');
		gnss_data.utc_time.sec  = (tmp_buf[4] - '0') * 10 + (tmp_buf[5] - '0');
	}
	
	if(get_data_str(2,3,p,tmp_buf,len) > 0)     //GNSS-state 定位状态 “A”：定位 “V”
	{
		if(tmp_buf[0] == 'A')
		{
			gnss_data.state = 'A';
		}
		else
		{
			gnss_data.state = 'V';
		}
	}
	
	if((tmp = get_data_str(3,4,p,tmp_buf,len)) > 0)      //GNSS-维度
	{
		tmp_buf[tmp] = '\0';
		tmp_dou = fr_atof((const char *)tmp_buf);
		gnss_data.latitude_nmea =  tmp_dou *100000;
		gnss_data.real_latitude = ((unsigned int)(tmp_dou / 100) + (tmp_dou - (unsigned int)(tmp_dou / 100 ) * 100) / 60) * 1000000;
	}
	
	if(get_data_str(4,5,p,tmp_buf,len) > 0)							//gnss_info-维度-标志
	{
		gnss_data.latitude_sn = tmp_buf[0];
	}
	
	if((tmp = get_data_str(5,6,p,tmp_buf,len)) > 0)             //GNSS-经度
	{
		tmp_buf[tmp] = '\0';
		tmp_dou = fr_atof((const char *)tmp_buf);
		gnss_data.longitude_nmea = tmp_dou * 10000;
		gnss_data.real_longitude = ((unsigned int)(tmp_dou / 100) + (tmp_dou - (unsigned int)(tmp_dou / 100 ) * 100) / 60) * 1000000;
		
	}
	
	if(get_data_str(6,7,p,tmp_buf,len) > 0)            //GNSS-经度标志
	{
		gnss_data.longitude_ew = tmp_buf[0];
	}
	
	if((tmp = get_data_str(7,8,p,tmp_buf,len)) > 0)    //速度
	{
		tmp_buf[tmp] = '\0';
		gnss_data.speed = fr_atof((const char *)tmp_buf);
	}
	else
	{
		gnss_data.speed = 0;
	}
	
	if((tmp = get_data_str(8,9,p,tmp_buf,len)) > 0)    //航向角度
	{
		tmp_buf[tmp] = '\0';
		gnss_data.heading = (unsigned short int)fr_atof((const char *)tmp_buf);
	}
	else
	{
		gnss_data.heading = 0;
	}
	
	if(get_data_str(9,10,p,tmp_buf,len) > 0) 
	{
		struct rt_tm *pt;
		
		gnss_data.utc_time.year = (tmp_buf[4] - '0') * 10 + (tmp_buf[5] - '0');
		gnss_data.utc_time.mon  = (tmp_buf[2] - '0') * 10 + (tmp_buf[3] - '0');
		gnss_data.utc_time.day	= (tmp_buf[0] - '0') * 10 + (tmp_buf[1] - '0');
		
		now.year = gnss_data.utc_time.year;
		now.mon = gnss_data.utc_time.mon ;
		now.day = gnss_data.utc_time.day;
	
		now.hour = gnss_data.utc_time.hour;
		now.min = gnss_data.utc_time.min;
		now.sec = gnss_data.utc_time.sec;
		
		
		gnss_data.utc_unix = rt_mktime(&now);         //GNSS
		gnss_data.btc_unix = gnss_data.utc_unix + 28800;
		
		pt = rt_localtime(&gnss_data.btc_unix);
		
		gnss_data.btc_time.year = pt->year;                //年
		gnss_data.btc_time.mon = pt->mon;                  //月
		gnss_data.btc_time.day = pt->day;                  //日
		gnss_data.btc_time.hour = pt->hour;                //时
		gnss_data.btc_time.min = pt->min;                  //分
		gnss_data.btc_time.sec = pt->sec;                  //秒
	}
}


/**************************************
**	解析北斗定位可视卫星数目
***************************************/

static void parse_gpgsv(uint8_t *data,uint16_t len)
{
  uint8_t tmp = 0;
	uint8_t *p = NULL;
	uint8_t tmp_buf[20];
	
	if(data == NULL)
		return;
	
	p = data;
		
	if((tmp = get_data_str(3,4,p,tmp_buf,len)) > 0)      //GNSS-维度
	{
		tmp_buf[tmp] = '\0';
		gnss_data.gps_sate_num = fr_atof((const char *)tmp_buf);
	}
}



/**************************************
**	解析北斗定位可视卫星数目
***************************************/

static void parse_bdgsv(uint8_t *data,uint16_t len)
{
  uint8_t tmp = 0;
	uint8_t *p = NULL;
	uint8_t tmp_buf[20];
	
	if(data == NULL)
		return;
	
	p = data;
		
	if((tmp = get_data_str(3,4,p,tmp_buf,len)) > 0)      //GNSS-维度
	{
		tmp_buf[tmp] = '\0';
		gnss_data.bd_sate_num = fr_atof((const char *)tmp_buf);
	}
}





/****************************
**	GNSS信息解析
**	
*****************************/

void gnss_info_handler(uint8_t *data_buf,uint16_t len)
{
    char         *p = NULL;
    uint16_t     data_len = 0;
    const char   *data = NULL;
    

    data = (const char *)data_buf;
		
    /* 判断使用的定位模块*/ 
    if(gnss_module_flag == 0)
    {
       if(strstr((char *)data, "$GNRMC") != NULL)
			 {
					gnss_module_flag = 1;           //
			 }
			 else  if(strstr(data, "$GPRMC") != NULL)
			 {
				 gnss_module_flag = 2;
			 }
			 else
			 {
				return;
			 }
    }

    p = NULL;
    if(gnss_module_flag == 1)
        p = strstr(data, "$GNRMC");
    else if(gnss_module_flag == 2)
        p = strstr(data, "$GPRMC");

    if (p != NULL)
    {
        if(gnss_data_verify((uint8_t *)p, &data_len))
        {
					parse_rmc((uint8_t *)p,data_len);
//					*(p + data_len - 5) = '\0';
        }
        else
        {
        }
    }
    else
		{
			
		}
        
    p = NULL;
    if (gnss_module_flag == 1)
        p = strstr(data, "$GNGGA");
    else if (gnss_module_flag == 2)
        p = strstr(data, "$GPGGA");
   
    if (p != NULL)
    {
        if(gnss_data_verify((uint8_t *)p, &data_len))
        {
            parse_gga((uint8_t *)p,data_len);
        }
        else
        {
        }
    }
    else
		{
			
		}
		
		p = NULL;
		p = strstr(data, "$GPGSV");         //解析GPS可视卫星数量
		if(p != NULL)
		{
			if(gnss_data_verify((uint8_t *)p, &data_len))
			{
				parse_gpgsv((uint8_t *)p,data_len);
			}
			else
			{
			}
		}
		
		p = NULL;
		p = strstr(data, "$BDGSV");               //解析北斗可视卫星数量
		if(p != NULL)
		{
			if(gnss_data_verify((uint8_t *)p,&data_len))
			{
				parse_bdgsv((uint8_t *)p,data_len);
			}
			else
			{
			
			}
		}


		p = strstr(data,"$GNTXT");             //解析天线状态
		if(p != NULL)
		{
			if(gnss_data_verify((uint8_t *)p,&data_len))
			{
				p = strstr(data,"ANTENNA OK");
				if(p != NULL)
				{
					gnss_data.ant_state = 0;
				}
				p = strstr(data,"ANTENNA OPEN");
				if(p != NULL)
				{
					gnss_data.ant_state = 1;
				}
				p = strstr(data,"ANTENNA SHORT");
				if(p != NULL)
				{
					gnss_data.ant_state = 2;
				}
			}
		}

	//过滤GNSS经纬度
	if(gnss_data.state == 'A')
	{
		switch(gnss_data.step)
		{
			case 0:
				gnss_data.prev_latitude = gnss_data.real_latitude;
				gnss_data.prev_longitude = gnss_data.real_longitude;
				gnss_data.step++;
				break;
			case 1:
			case 2:
			case 3:
				if(filter_gnss_data() > 0)
				{
					gnss_data.prev_latitude = gnss_data.real_latitude;
					gnss_data.prev_longitude = gnss_data.real_longitude;
					gnss_data.step++;
				}
				else
				{
					gnss_data.step = 0;
				}
				break;
			case 4:                                      //状态4，实时定位信息
				if(filter_gnss_data() > 0)
				{
					gnss_data.prev_latitude = gnss_data.real_latitude;
					gnss_data.prev_longitude =gnss_data.real_longitude;
					gnss_data.latitude_normal = gnss_data.real_latitude;
					gnss_data.longitude_normal = gnss_data.real_longitude;
					gnss_data.state = gnss_data.state;
					gnss_data.erro_cnt = 0;
					if(gnss_data.state_back != 'A')
					{
						//save_gnss_info_back();
						gnss_data.state_back = gnss_data.state;
						gnss_data.nmea_cnt = 1000;
						//printf("-- save gnss..........%u,%u\r\n",gnss_info.latitude_normal,gnss_info.longitude_normal);
					}
					//rt_kprintf("the run is..........\r\n");
				}
				else
				{
					if(gnss_data.erro_cnt++ > 3)
					{
						gnss_data.step = 0;
					}
				}
				break;
			default:
				gnss_data.step = 0;
				break;	
			}
		}
		else
		{
			gnss_data.state_back = 'V';      //
			gnss_data.state = 'V';      //
		}
		
		if(gnss_data.state == 'A')            //用来校时
		{
			int32_t tmp;
			//printf("-- tun is :%d...\r\n",gnss.nmea_cnt);
			if(++gnss_data.nmea_cnt >= 600)         //每10分钟保存一次定位位置
			{
				gnss_data.nmea_cnt = 0;
				//printf("-- Save gnss info....\r\n");
				//save_gnss_info();
			}

			tmp = get_rtc_timestamp() - gnss_data.btc_unix;
			if(tmp > 0 || tmp < -5)	
			{
				printf("-- Gnss calibration rtc time......%u\r\n",gnss_data.btc_unix);
				//set_rtc_time(&gnss_info.btc_time);
			}
		}
		else
		{
			if(gnss_data.cnt_reset++ > 240)
			{
				gnss_data.cnt_reset = 0;
				//gnss_power_reset();
				//printf("-- Gnss module reset........\r\n");
			}
		}
}






/***********************************
**	定位状态
***********************************/

uint8_t read_gnss_ant_state(void)    //天线状态    20
{
	uint8_t rv;

	rv = gnss_data.ant_state;
	//rv = 1;

	return rv;
}

/***********************************
**	可视卫星数据
***********************************/

uint8_t read_gnss_satellite_num(void)
{
	uint8_t rv;
	
	rv = gnss_data.satellite_num;
	
	return rv;
}




/***********************************
**	      //维度(经纬度)
************************************/

uint32_t read_gnss_latitude(uint8_t n)   
{
	uint32_t  rv;

	if(n == 1)
		rv = gnss_data.latitude_nmea;
	else
		rv = gnss_data.latitude_normal;

	return rv;
}        
	

/***********************************
**	      //维度(经纬度)
************************************/

uint16_t read_gnss_speed(void)
{
	uint16_t rv;

	rv = gnss_data.speed;

	return rv;
}


/***********************************
**	     航向角度
************************************/

uint16_t read_gnss_heading(void)
{
	uint16_t rv;

	rv = gnss_data.heading;

	return rv;

}


/***********************************
**	返回
************************************/
uint32_t read_gnss_longitude(uint8_t n)   
{
	uint32_t rv;


	if(n == 1)
        rv = gnss_data.longitude_nmea;
	else
		rv = gnss_data.longitude_normal;

	return rv;
}         





/****************************
 ** 处理GNSS定位
 ****************************/

void thread_entry_gnss(void *parameter)
{
	uint16_t len = 0;
	
	parameter = parameter;
	
	gnss.semaphore = xSemaphoreCreateBinary();
	
	if(gnss.semaphore == NULL)
  {
		printf("-- creat packet sph fail....\r\n");
  }
	
	rt_gnss_power_on();
	rt_irq_tim3_sethook(gnss_ticks_handle);
	rt_irq_uart_sethook(3,gnss_rx_data_handle);
	
	for (;;)
  {
		if(xSemaphoreTake(gnss.semaphore, 1000) == pdTRUE)
		{
			memset(data_buff,'\0',sizeof(data_buff));
			len =  rt_read_uart_buf(3, data_buff, gnss.len);
			if(len > 0)
			{
				gnss_info_handler(data_buff, gnss.len);
				//printf(" %s\r\n",data_buff);
				//printf("-- the gnss ant state:%d\r\n",gnss_data.ant_state);
			}
		}
  }  
}




