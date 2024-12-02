


#include <stdio.h>
#include <string.h>
#include <math.h>

#include <rtthread.h>
#include <rtdevice.h>
#include <rtdevice.h>
#include <rthw.h>



#include "drv_rtc.h"


#include "pro_data.h"

#include "app_gnss.h"
#include "drv_gpio.h"
#include "drv_timer.h"
#include "drv_usart.h"
#include "app_packet.h"
#include "app_shell.h"



const uint8_t 								cold_cmd[9] = {0x23,0x3E,0x02,0x01,0x01,0x00,0x01,0x05,0x12};


/****************** 鏈湴鍏ㄥ眬鍙橀噺 *********************/

struct gnss_str 							gnss = {0};												//GNSS模块相关

struct gnss_info_str 					gnss_info = {0};   								//GNSS定位信息缓冲区

static uint8_t 								gnss_data_recv_buf[1024] = {0};   //GNSS数据接收缓冲区

static uint8_t 								gnss_module_flag = 0;             //GNSS模块类型标志         

static rt_device_t 						gnss_uart_dev = RT_NULL;       		//GNSS接收数据串口




/****************************
**
*****************************/

uint8_t init_gnss_debug_state(uint8_t n)
{
	gnss.debug_state = n;
	
	return 0;
}


/**********************************************
**
***********************************************/
uint8_t	read_gnss_info(struct gnss_info_str *source)
{
    if(source == NULL)
			return 1;
		
		memcpy((uint8_t *)source,(uint8_t *)&gnss_info,sizeof(struct gnss_info_str));

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

    if ((ascii_low >= 0x30) && (ascii_low <= 0x39)) //
        ascii_low -= 0x30;
    else if ((ascii_low >= 0x41) && (ascii_low <= 0x46)) //
        ascii_low -= 0x37;
    else if ((ascii_low >= 0x61) && (ascii_low <= 0x66)) //
        ascii_low -= 0x57;

    return (ascii_high << 4) | ascii_low;
}


/*******************************************
**	鏍￠獙NEMA淇℃伅
*******************************************/

static uint8_t gnss_data_verify(uint8_t *gps_data, uint16_t *len)
{
    uint8_t i = 0, k = 0;
    uint8_t *tmp = RT_NULL;
    uint8_t data_check_value = 0; //
    uint8_t recv_check_value = 0; //

    tmp = gps_data;
    for (i = 0; (tmp[i] != '*'); i++)
    {
        if (i > 100) //
            return 0;
    }

    for (k = 1; k < i; k++)
        data_check_value ^= tmp[k];

    *len = i + 1 + 2; //
    recv_check_value = gps_ascii_to_hex(tmp[i + 1], tmp[i + 2]);

    if (data_check_value == recv_check_value)
        return 1; //
    else
        return 0;
}


/****************************************
**	 
*****************************************/

static void parse_gga(uint8_t *data,uint16_t len)
{
	uint8_t 	tmp = 0;
	uint8_t 	*p = NULL;
	uint8_t 	tmp_buf[20];
	
	if(data == RT_NULL)
		return;
	
	p = data;

	//使用卫星数量
	if((tmp = get_data_str(7,8,p,tmp_buf,len)) > 0)       
	{
		tmp_buf[tmp] = '\0';
		gnss_info.satellite_num = (uint8_t)fr_atof((const char *)tmp_buf);
	}
	
  //水平进度因子 HDOP
	if((tmp = get_data_str(8,9,p,tmp_buf,len)) > 0)      
	{
		tmp_buf[tmp] = '\0';
		//rt_kprintf("-- The hop:%s\r\n",tmp_buf);
		gnss_info.hdop = (uint16_t)(fr_atof((const char *)tmp_buf) * 10);
	}
	
	
  //海拔 高度
	if((tmp = get_data_str(9,10,p,tmp_buf,len)) > 0)    
	{
		tmp_buf[tmp] = '\0';
		gnss_info.altitude = (uint8_t)fr_atof((const char *)tmp_buf);
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
	double 		tmp_dou;
  uint8_t 	tmp = 0;
	uint8_t 	*p = NULL;
	uint8_t 	tmp_buf[20];
	
	struct rt_tm now;
	
	if(data == RT_NULL)
		return;
	
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
			gnss.state = 'A';
		}
		else
		{
			gnss.state = 'V';
		}
	}
	
	if((tmp = get_data_str(3,4,p,tmp_buf,len)) > 0)      //GNSS-维度
	{
		tmp_buf[tmp] = '\0';
		tmp_dou = fr_atof((const char *)tmp_buf);
		gnss_info.latitude_nmea =  tmp_dou *100000;
		gnss.real_latitude = ((unsigned int)(tmp_dou / 100) + (tmp_dou - (unsigned int)(tmp_dou / 100 ) * 100) / 60) * 1000000;
	}
	
	if(get_data_str(4,5,p,tmp_buf,len) > 0)							//gnss_info-维度-标志
	{
		gnss_info.latitude_sn = tmp_buf[0];
	}
	
	if((tmp = get_data_str(5,6,p,tmp_buf,len)) > 0)             //GNSS-经度
	{
		tmp_buf[tmp] = '\0';
		tmp_dou = fr_atof((const char *)tmp_buf);
		gnss_info.longitude_nmea = tmp_dou * 10000;
		gnss.real_longitude = ((unsigned int)(tmp_dou / 100) + (tmp_dou - (unsigned int)(tmp_dou / 100 ) * 100) / 60) * 1000000;
		
	}
	
	if(get_data_str(6,7,p,tmp_buf,len) > 0)            //GNSS-经度标志
	{
		gnss_info.longitude_ew = tmp_buf[0];
	}
	
	if((tmp = get_data_str(7,8,p,tmp_buf,len)) > 0)    //速度
	{
		tmp_buf[tmp] = '\0';
		gnss_info.speed = fr_atof((const char *)tmp_buf) * 1.852 * 100;
	}
	else
	{
		gnss_info.speed = 0;
	}
	
	if((tmp = get_data_str(8,9,p,tmp_buf,len)) > 0)    //航向角度
	{
		tmp_buf[tmp] = '\0';
		gnss_info.heading = (unsigned short int)fr_atof((const char *)tmp_buf);
	}
	else
	{
		gnss_info.heading = 0;
	}
	
	if(get_data_str(9,10,p,tmp_buf,len) > 0) 
	{
		struct rt_tm *pt;
		
		gnss_info.utc_time.year = (tmp_buf[4] - '0') * 10 + (tmp_buf[5] - '0');
		gnss_info.utc_time.mon  = (tmp_buf[2] - '0') * 10 + (tmp_buf[3] - '0');
		gnss_info.utc_time.day	= (tmp_buf[0] - '0') * 10 + (tmp_buf[1] - '0');
		
		now.year = gnss_info.utc_time.year;
		now.mon = gnss_info.utc_time.mon ;
		now.day = gnss_info.utc_time.day;
	
		now.hour = gnss_info.utc_time.hour;
		now.min = gnss_info.utc_time.min;
		now.sec = gnss_info.utc_time.sec;
		
		
		gnss_info.timestamp = rt_mktime(&now) + 28800;         //GNSS
		
		pt = rt_localtime(&gnss_info.timestamp);
		
		gnss_info.btc_time.year = pt->year;                //年
		gnss_info.btc_time.mon = pt->mon;                  //月
		gnss_info.btc_time.day = pt->day;                  //日
		gnss_info.btc_time.hour = pt->hour;                //时
		gnss_info.btc_time.min = pt->min;                  //分
		gnss_info.btc_time.sec = pt->sec;                  //秒
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
	
	if(data == RT_NULL)
		return;
	
	p = data;
		
	if((tmp = get_data_str(3,4,p,tmp_buf,len)) > 0)      //GNSS-维度
	{
		tmp_buf[tmp] = '\0';
		gnss_info.gps_sate_num = fr_atof((const char *)tmp_buf);
	}
}



/**************************************
**	解析北斗定位可视卫星数目
***************************************/

static void parse_bdgsv(uint8_t *data,uint16_t len)
{
  uint8_t 		tmp = 0;
	uint8_t 		*p = NULL;
	uint8_t 		tmp_buf[20];
	
	if(data == RT_NULL)
		return;
	
	p = data;
		
	if((tmp = get_data_str(3,4,p,tmp_buf,len)) > 0)      //GNSS-维度
	{
		tmp_buf[tmp] = '\0';
		gnss_info.bd_sate_num = fr_atof((const char *)tmp_buf);
	}
}



/****************************
**	GNSS信息解析
**	
*****************************/

void gnss_info_handler(uint8_t *data_buf)
{
    char 				*p = RT_NULL;
    uint16_t 		data_len = 0;
    const char 	*data = RT_NULL;
    

    data = (const char *)data_buf;
		
    /* 判断使用的定位模块*/ 
    if(gnss_module_flag == 0)
    {
       if(rt_strstr(data, "$GNRMC") != RT_NULL)
			 {
					gnss_module_flag = 1;           //
			 }
			 else  if(rt_strstr(data, "$GPRMC") != RT_NULL)
			 {
				 gnss_module_flag = 2;
			 }
			 else
			 {
				 gnss_info.module_state = 1;
				 return;
			 }
    }

    p = RT_NULL;
    if(gnss_module_flag == 1)
        p = rt_strstr(data, "$GNRMC");
    else if(gnss_module_flag == 2)
        p = rt_strstr(data, "$GPRMC");

    if (p != RT_NULL)
    {
        if(gnss_data_verify((uint8_t *)p, &data_len))
        {
//					rt_kprintf("-- RMC ..... %d\r\n",data_len);
					parse_rmc((uint8_t *)p,data_len);
        }
        else
        {
           //rt_kprintf("- GxRMC info verify failed.\r\n");
        }
    }
    else
		{
			//rt_kprintf("- Gnss module error.....GxRMC!\r\n");
		}
        
    p = RT_NULL;
    if (gnss_module_flag == 1)
        p = rt_strstr(data, "$GNGGA");
    else if (gnss_module_flag == 2)
        p = rt_strstr(data, "$GPGGA");
   
    if (p != RT_NULL)
    {
        if(gnss_data_verify((uint8_t *)p, &data_len))
        {
            parse_gga((uint8_t *)p,data_len);
        }
        else
        {
						//rt_kprintf("- GxGGA info verify failed.\r\n");
        }
    }
    else
		{
			//rt_kprintf("- Gnss module error.....GxGGA!\r\n");
		}
		
		p = RT_NULL;
		p = rt_strstr(data, "$GPGSV");         //解析GPS可视卫星数量
		if(p != RT_NULL)
		{
			if(gnss_data_verify((uint8_t *)p, &data_len))
			{
				parse_gpgsv((uint8_t *)p,data_len);
			}
		}
		
		p = RT_NULL;
		p = rt_strstr(data, "$BDGSV");               //解析北斗可视卫星数量
		if(p != RT_NULL)
		{
			if(gnss_data_verify((uint8_t *)p,&data_len))
			{
				parse_bdgsv((uint8_t *)p,data_len);
			}
			else
			{
				//rt_kprintf("- BDGSV info verify failed.\r\n");
			}
		}
}



/*************************************
**
**************************************/

static rt_err_t gnss_data_rx_call_fun(rt_device_t dev, rt_size_t size)
{
    gnss.ticks = rt_tick_get();
    gnss.data_len = size;
	
    return RT_EOK;
}


/************************************
 **
************************************/

static void gnss_tick_handle(void)
{
    if (gnss.ticks == 0xffffffff)
        return;

    if(rt_tick_get() - gnss.ticks > 2)
    {
        gnss.ticks = 0xffffffff;
        rt_sem_release(&gnss.gnss_sign);
    }
}



/*******************************
**	定时保存定位信息
********************************/

static uint8_t save_gnss_info_back(void)
{
	rt_device_t eeprom_dev = RT_NULL;
	uint16_t result = 0;
	
  eeprom_dev = rt_device_find("at24cxx");
	rt_device_open(eeprom_dev,RT_DEVICE_OFLAG_RDWR);
	
	result = rt_device_write(eeprom_dev,1024,(uint8_t *)&gnss_info,sizeof(gnss_info));
	if(result == sizeof(gnss_info))
	{
		rt_device_close(eeprom_dev);           //注意这个地方的代
		//rt_kprintf("-- save gnss info.........\r\n");
	}
	
	return 1;
}




/*******************************************
 **从EEPROM读取GNSS保存的信息
**	
 ********************************************/

static uint8_t read_gnss_info_back(void)
{
	rt_device_t eeprom_dev = RT_NULL;
	rt_err_t result = RT_EOK;
	
  eeprom_dev = rt_device_find("at24cxx");
	
	rt_device_open(eeprom_dev,RT_DEVICE_OFLAG_RDWR);
	
	result = rt_device_read(eeprom_dev,1024,(uint8_t *)&gnss_info,sizeof(gnss_info));

	if(result == RT_EOK )
	{
		if(gnss_info.state != 'V' && gnss_info.state != 'A')
		{
			gnss_info.state = 'V';
			gnss_info.latitude_nmea = 39915020;
			gnss_info.longitude_nmea = 11640395;
			gnss_info.utc_time.year = 22;
			gnss_info.utc_time.mon = 1;
			gnss_info.utc_time.day = 1;
			gnss_info.utc_time.hour = 0;
			gnss_info.utc_time.min = 0;
			gnss_info.utc_time.sec = 0;
		}
		//rt_kprintf("-- Read gnss info.........OK\r\n");
		
		return 0;
	}
	
	rt_kprintf("-- Read gnss info....Fail\r\n");
	
	return 1;
}





/********************************************
**	过滤GNSS数据
**	
*********************************************/

static uint8_t filter_gnss_data(void)
{
	double tmp;

	tmp = ComputeDistance((double)(gnss.real_latitude / 1000000.0),(double)(gnss.real_longitude / 1000000.0),(double)(gnss.prev_latitude / 1000000.0),(double)(gnss.prev_longitude / 1000000.0)) * 1000;

	if(tmp > 70)             //120Km/h  
	{
		return 0;
	}
	
	return 1;
}




/************************************
**	GNSS模块的状态
*************************************/

uint8_t read_gnss_module_state(void)
{
	uint8_t rv;
	
	rv = gnss_info.module_state;
	
	return rv;
}




/**************************************
**	返回定位状态
** 	'A'：定位
**	'V':不定位
***************************************/

uint8_t read_gnss_positing_state(void)
{
	uint8_t rv;
	
	rv = gnss_info.state;
	//rv = 'A';
	
	return rv;
}




/*****************************
**	返回nema数据包数量
*******************************/

uint32_t read_gnss_nmea_cnt(void)
{
	uint32_t rv;
	
	rv = gnss_info.nmea_cnt;
	
	return rv;
}


/**************************************
**	返回GNSS海拔高度
****************************************/

uint16_t read_gnss_altitude(void)
{
	uint16_t rv;
	
	rv = gnss_info.altitude;
	
	//rt_kprintf("-- the gnss info altitude:%d\r\n",rv);
	
	return rv;
}



/***********************************
**	可视卫星数据
***********************************/

uint8_t read_gnss_satellite_num(void)
{
	uint8_t rv;
	
	rv = gnss_info.satellite_num;
	
	return rv;
}


 
 /************************
 **	航向角
 *************************/
 
uint16_t read_gnss_heading(void)     					// 航向 度*/
{
	uint16_t rv;
	
	rv = gnss_info.heading;
	
	return rv;
}

 
 
 /**********************
*	可视卫星数量(北斗)
***********************/
uint8_t	read_gnss_bd_sate_num(void) 					// 可视卫星数量(北斗)
{
	uint8_t rv;
	
	rv = gnss_info.bd_sate_num;

	return rv;
}

/**********************
*	可视微信数量()
***********************/

uint8_t read_gnss_gps_sate_num(void)					// 可视微信数量()
{
	uint8_t rv;
	
	rv = gnss_info.gps_sate_num;
	
	return rv;
}


/**********************
*	位置经度因子
***********************/

uint16_t read_gnss_hdop(void)        					// 水平精度因子*/
{
	uint16_t rv;
	
	rv = gnss_info.hdop;
	
	return rv;
}

 
/****************************
** 经度 dddmm.mmmm
****************************/
uint32_t read_gnss_longitude(uint8_t n)
{
	uint32_t rv;
	
	if(n == 0)
		rv = gnss_info.longitude_normal;
	else
		rv = gnss_info.longitude_nmea;
	
	return rv;
}

/****************************
** 维度 ddmm.mmmm
**	输入参数：n-0：度*1000000
							n-1：度分分
****************************/
uint32_t read_gnss_latitude(uint8_t n)
{
	uint32_t rv;
	
	if(n == 0)
		rv = gnss_info.latitude_normal;
	else
		rv = gnss_info.latitude_nmea;
	
	return rv;
}


/*********************
**	读取GNSS速度
**	返回整数，保留一位有效小数
************************/
uint16_t read_gnss_speed(void)
{
	uint16_t rv;
	
	if(gnss_info.speed < 20)
		rv = 0;
	else
		rv = gnss_info.speed;
	
	//rv = 2000;
	
	return rv;
}


/*********************
**	读取UTC时间拷贝
**	UTC时间通过GNSS定位模块获取的原始时间
**	
************************/

void read_gnss_utc_time(struct rt_tm *ptime)
{
	if(gnss_info.state != 'A')
		memset((char *)&gnss_info.utc_time,0,sizeof(struct rt_tm));
	
	memcpy((char *)ptime,(char *)&gnss_info.utc_time,sizeof(struct rt_tm));
}






/**************************
**	返回经度标志
**************************/

uint8_t read_gnss_longitude_ew(void)
{
	uint8_t rv;
	
	rv = gnss_info.longitude_ew;
	
	return rv;
}




/**************************
**	返回经度标志
**************************/

uint8_t read_gnss_latitude_sn(void)
{
	uint8_t rv;
	
	rv = gnss_info.latitude_sn;
	
	return rv;
}



/*********************************
**	 返回GNSS的北京时间戳
**********************************/

uint32_t read_gnss_btc_timetamp(void)
{
	uint32_t rv;
	
	rv = gnss_info.timestamp;
	
	return rv;
}


/***************************
**	重启GNSS模块
****************************/

void gnss_power_reset(void)
{
	gnss_uart_dev = rt_device_find(RT_GPS_DEVICE_NAME);
	
	gnss.cnt_reset = 0;
  rt_gnss_power_off();
  //rt_gnss_vback_off();
	if(gnss_uart_dev != RT_NULL)
	{
		gnss_uart_dev->flag &= 0xFFEF;
		rt_device_close(gnss_uart_dev);     //关闭串口
	}
  rt_thread_delay(100);	
	rt_gnss_power_on();
  //rt_gnss_vback_on();
  rt_device_open(gnss_uart_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);    //
}





/*******************************************
**	关闭
*******************************************/
void close_gnss_module(void)
{
	if(gnss_uart_dev != RT_NULL)
	{
		rt_device_close(gnss_uart_dev);     //关闭串口
		gnss_uart_dev->flag &= 0xFFEF;
	}
	
	rt_gnss_power_off();                 //关闭GNSS电源							
}










/*******************************************
**	GNSS模块任务
********************************************/

void thread_entry_gnss(void *parameter)
{
	gnss.ticks = 0xffffffff;
  gnss.cnt_reset = 0;
	
	
	rt_thread_delay(200);

	rt_sem_init(&gnss.gnss_sign, "gnss_sign", 0, 0); 																//GNSS数据接收信号量
	read_gnss_info_back();
	
  gnss_uart_dev = rt_device_find(RT_GPS_DEVICE_NAME);
	rt_gnss_power_on();           																									//打开GNSS电源
  rt_device_set_rx_indicate(gnss_uart_dev, gnss_data_rx_call_fun); 								// 添加回掉函数
  rt_irq_timer3_sethook(gnss_tick_handle);                   											// 添加定时器回调函数 (定时器回调函数)  
	rt_device_init(gnss_uart_dev);
	rt_device_open(gnss_uart_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);    //
	
	//rt_gnss_vback_on();
	gnss.state = 'V';  									//默认定位状态'V'
	gnss_info.state = 'V';
	gnss_info.nmea_cnt = 0;
	
	for(;;)
  {
		if(rt_sem_take(&gnss.gnss_sign, 1000) != RT_EOK)
    {
			gnss_info.module_state = 1;        																					//(报N303模块故障)
			rt_kprintf("-- gnss nema fail........\r\n");
      continue;
    }
		
		gnss_info.nmea_cnt++;   																											//每次收到一次GNSS模块数据自动加1
		
    rt_memset(gnss_data_recv_buf,'\0', 1024);
    gnss.data_len = rt_device_read(gnss_uart_dev, 0, gnss_data_recv_buf, sizeof(gnss_data_recv_buf));          //读取接收到的数据
		//rt_kprintf("-- nema num:%d,%d,%d     %c\r\n",gnss_info.nmea_cnt,gnss.data_len,gnss.cnt_reset,gnss_info.state);
		if(gnss.data_len > 1024)           																						//注意N303总数据长度  总长度不超过1K，
			gnss.data_len = 1024;

		switch(gnss.debug_state)
		{
			case 1:  //输出GNSS信息
				rt_kprintf("\r\n\r\n");
				send_data_to_shell_dev(gnss_data_recv_buf,gnss.data_len);
				break;
		}
		
		gnss_info_handler(gnss_data_recv_buf);        															//进行定位信息解析
		gnss_info.module_state = 0;

		if(gnss.state == 'A')
		{
			switch(gnss.step)
			{
				case 0:
					gnss.prev_latitude = gnss.real_latitude;
					gnss.prev_longitude = gnss.real_longitude;
					gnss.step++;
					break;
				case 1:
				case 2:
				case 3:
					if(filter_gnss_data() > 0)
					{
						gnss.prev_latitude = gnss.real_latitude;
						gnss.prev_longitude = gnss.real_longitude;
						gnss.step++;
					}
					else
					{
						gnss.step = 0;
					}
					break;
				case 4:                                      //状态4，实时定位信息
					if(filter_gnss_data() > 0)
					{
						gnss.prev_latitude = gnss.real_latitude;
						gnss.prev_longitude = gnss.real_longitude;
						gnss_info.latitude_normal = gnss.real_latitude;
						gnss_info.longitude_normal = gnss.real_longitude;
						gnss_info.state = gnss.state;
						gnss.erro_cnt = 0;
						if(gnss.state_back != 'A')
						{
							save_gnss_info_back();
							gnss.state_back = gnss.state;
							//rt_kprintf("--save gnss..........%u,%u\r\n",gnss_info.latitude_normal,gnss_info.longitude_normal);
						}
					}
					else
					{
						if(gnss.erro_cnt++ > 3)
						{
							gnss.step = 0;
						}
					}
					break;
				default:
					gnss.step = 0;
					break;	
			}
		}
		else
		{
			gnss.state_back = 'V';      //
			gnss_info.state = 'V';      //
		}
		
		if(gnss_info.state == 'A')            //用来校时
		{
			rt_set_rtc(&gnss_info.btc_time);
			gnss.cnt_reset = 0;
			if(++gnss.nmea_cnt >= 600)         //每10分钟保存一次定位位置
			{
				gnss.nmea_cnt = 0;
				save_gnss_info_back();
				//rt_kprintf("-- save gnss info data:%u\r\n",now_timestamp);
			}
			
		}
		else
		{
			if(gnss.cnt_reset++ > 600)
			{
				gnss.cnt_reset = 0;
				//rt_device_write(gnss_uart_dev, 0, cold_cmd, 9);
				//rt_kprintf("-- Send cold boot cmd ... \r\n");
			}
		}
		
		//rt_kprintf("-- Gnss info:%d,%u,%u,%u,%c\r\n",gnss_info.speed,gnss_info.longitude_normal,gnss_info.latitude_nmea,gnss_info.longitude_nmea,gnss_info.state);
		//rt_kprintf("%d,%d,\r\n",gnss_info.speed,read_gnss_speed() * 256 / 100);
		//rt_kprintf("- Gnss info:%d,%u,%c\r\n",gnss.data_len,gnss_info.timestamp,gnss_info.state);
		//rt_kprintf("%u,%u,%c\r\n",gnss_info.latitude,gnss_info.longitude,gnss.state);
		//rt_kprintf("%d,%d,%d-%d:%d:%d    %u\r\n",gnss_info.utc_time.year,gnss_info.utc_time.mon,gnss_info.utc_time.day,gnss_info.utc_time.hour,gnss_info.utc_time.min,gnss_info.utc_time.sec,gnss_info.timestamp);
		//rt_kprintf("the btc time:%d,%d,%d-%d:%d:%d\r\n",gnss_info.btc_time.year,gnss_info.btc_time.mon,gnss_info.btc_time.day,gnss_info.btc_time.hour,gnss_info.btc_time.min,gnss_info.btc_time.sec);
		//rt_kprintf("the the :%d,%d,%d\r\n",gnss_info.satellite_num,gnss_info.gps_sate_num,gnss_info.bd_sate_num);
  }
}




