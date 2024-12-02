


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_system.h"

#include "pro_data.h"
#include "common.h"

#include "drv_uart.h"
#include "drv_rtc.h"
#include "drv_gpio.h"


#include "app_gnss.h"



const uint8_t 							cold_cmd[9] = {0x23,0x3E,0x02,0x01,0x01,0x00,0x01,0x05,0x12};

/******************** 本地全局变量 ************************/
static struct gnss_str                  gnss                = {0};

static struct gnss_info_str 			gnss_info           = {0};   								//GNSS定位信息缓冲区

static uint8_t 						    gnss_module_flag    = 0;             //GNSS模块类型标志 

static uint8_t 							nema_buf[1536]      = {0};	

static uint16_t 						nema_len            = 0;




/****************************
**
*****************************/

uint8_t init_gnss_debug_state(uint8_t n)
{
	gnss.debug_state = n;
	
	return 0;
}


void  delete_gnss_info_files(void)
{
	if(remove("/nandflash/gnss.txt") == 0)
		printf("-- delete gnss.txt file ok...\r\n");
	else
		printf("-- delete gnss.txt file fial...\r\n");
}


/*********************************
**	保存GNSS信息
**********************************/
void save_gnss_info(void)
{
	FILE *fd = NULL;
	int res = 0;
	uint8_t array[100];

	fd = fopen("/nandflash/gnss.txt", "w");
    if(fd != NULL)
    {
		memset(array,'\0',50);
    	sprintf((char *)array,"%u,%u,%u,%u,%c\r\n",gnss_info.latitude_normal,gnss_info.longitude_normal,gnss_info.latitude_nmea,gnss_info.longitude_nmea,gnss_info.state);  
		//printf("%s\r\n",array);
        fseek(fd,0,SEEK_SET);       
        res = fwrite((uint8_t *)array,50,1,fd);
		//printf("-- run hsi d df asd (1):%d\r\n",res);
		if(res > 0)
		{
			printf("-- Save gnss info OK...\r\n");
		}
		else
		{
			printf("-- Save gnss info Fail...\r\n");
		}
        res = fclose(fd);
        if(res != 0)
	    {
		    printf("-- GNSS info log files close failed\r\n");
        }
    }
	else
	{
		printf("-- GNSS info log files Open failed\r\n");
	}
}



/*********************************
**	返回模块状态
**********************************/

void init_gnss_info(void)
{
	FILE 		*fd;
	int 		res;
	uint8_t 	array[100];

	fd = fopen("/nandflash/gnss.txt", "r");
    if(fd != NULL)
    {
		memset(array,'\0',50);
        fseek(fd,0,SEEK_SET);
		res = fread((uint8_t *)array,100,1,fd);
        sscanf((const char *)array,"%u,%u,%u,%u,%c\r\n",&gnss_info.latitude_normal,&gnss_info.longitude_normal,&gnss_info.latitude_nmea,&gnss_info.longitude_nmea,&gnss_info.state);
		printf("-- init gnss info ok...\r\n");     
        res = fclose(fd);
        if(res != 0)
	    {
			
		    printf("-- Init GNSS info log files close failed 1\r\n");
        }
    }
	else
	{
		gnss_info.latitude_normal = 39913568;			
		gnss_info.longitude_normal = 116397252;    //默认经纬度 北京天安门广场午门

		gnss_info.latitude_nmea = 39912140;        //  度分分
		gnss_info.longitude_nmea = 116394351;	   //  度分分
		printf("-- Init GNSS info log files Open failed 2\r\n");
	}
}


/*********************************
**	返回模块状态
**********************************/

uint32_t read_gnss_module_state(void)
{
	uint32_t rv;

	rv = gnss_info.module_state;

	return rv;
}



/********************************************
**	
*********************************************/

uint8_t read_gnss_latitude_sn(void)
{
	uint8_t rv;

	rv = gnss_info.latitude_sn;

	return rv;
}



/********************************************
**	
*********************************************/
uint8_t read_gnss_longitude_ew(void)
{
	uint8_t rv;

	rv = gnss_info.longitude_ew;
	
	return rv;
}



/******************************
**
*******************************/
uint16_t read_gnss_altitude(void)
{
	uint16_t rv;

	rv = gnss_info.altitude;

	return rv;
}



/******************************
**
*******************************/
uint8_t read_gnss_bd_sate_num(void)
{
	uint8_t rv;

	rv =  gnss_info.bd_sate_num;

	return rv;
}




/******************************
**
*******************************/
uint8_t  read_gnss_gps_sate_num(void)
{
	uint8_t rv;

	rv = gnss_info.gps_sate_num;

	return rv;
}   



/******************************
**
*******************************/
uint16_t read_gnss_hdop(void)
{
	uint16_t rv;

	rv = gnss_info.hdop;

	return rv;
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
		gnss_info.satellite_num = (uint8_t)fr_atof((const char *)tmp_buf);
	}
	
  //水平进度因子 HDOP
	if((tmp = get_data_str(8,9,p,tmp_buf,len)) > 0)      
	{
		tmp_buf[tmp] = '\0';
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
		
		
		gnss_info.utc_unix = rt_mktime(&now);         //GNSS
		gnss_info.btc_unix = gnss_info.utc_unix + 28800;
		
		pt = rt_localtime(&gnss_info.btc_unix);
		
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
	
	if(data == NULL)
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
    uint8_t tmp = 0;
	uint8_t *p = NULL;
	uint8_t tmp_buf[20];
	
	if(data == NULL)
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

uint8_t  gnss_info_handler(uint8_t *data_buf,uint16_t len)
{
    char         *p = NULL;
    uint16_t     data_len = 0;
    const char   *data = NULL;
    

    data = (const char *)data_buf;
		
    /* 判断使用的定位模块*/ 
    if(gnss_module_flag == 0)
    {
		if(list_for_str((uint8_t *)data, (uint8_t *)"$GNRMC",len) >= 0 || list_for_str((uint8_t *)data, (uint8_t *)"$GNGSA",len) >= 0)  //综合定位
		{
			gnss_module_flag = 1;           //
		}
		else if(list_for_str((uint8_t *)data, (uint8_t *)"$GPRMC",len) >= 0 || list_for_str((uint8_t *)data, (uint8_t *)"$GPGSA",len) >= 0)   //单GPS
		{
			gnss_module_flag = 2;
		}
		else if(list_for_str((uint8_t *)data, (uint8_t *)"$BDRMC",len) >= 0 || list_for_str((uint8_t *)data, (uint8_t *)"$BDGSA",len) >= 0)   //单独北斗
		{
			gnss_module_flag = 3;
		}
		else
		{
			return 1;
		}
    }


    p = NULL;
    if(gnss_module_flag == 1)
        p = strstr(data, "$GNRMC");
    else if(gnss_module_flag == 2)
        p = strstr(data, "$GPRMC");
	else if(gnss_module_flag == 3)
		p = strstr(data, "$BDRMC");
    if(p != NULL)
    {
        if(gnss_data_verify((uint8_t *)p, &data_len))
        {
			parse_rmc((uint8_t *)p,data_len);
        }
    }
        
    p = NULL;
    if (gnss_module_flag == 1)
        p = strstr(data, "$GNGGA");
    else if (gnss_module_flag == 2)
        p = strstr(data, "$GPGGA");
    else if(gnss_module_flag == 3)
		p = strstr(data, "$BDGGA");

    if(p != NULL)
    {
        if(gnss_data_verify((uint8_t *)p, &data_len))
        {
            parse_gga((uint8_t *)p,data_len);
        }
    }
	
	p = NULL;
	p = strstr(data, "$GPGSV");         //解析GPS可视卫星数量
	if(p != NULL)
	{
		if(gnss_data_verify((uint8_t *)p, &data_len))
		{
			parse_gpgsv((uint8_t *)p,data_len);
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

	}


	p = strstr(data,"$GNTXT");             //解析天线状态
	if(p != NULL)
	{
		if(gnss_data_verify((uint8_t *)p,&data_len))
		{
			p = strstr(data,"ANTENNA OK");
			if(p != NULL)
			{
				gnss_info.ant_state = 0;
			}
			
			p = strstr(data,"ANTENNA OPEN");
			if(p != NULL)
			{
				gnss_info.ant_state = 1;
			}
				
			p = strstr(data,"ANTENNA SHORT");
			if(p != NULL)
			{
				gnss_info.ant_state = 2;
			}
		}
	}

	p = strstr(data,"$BDTXT");             //解析天线状态（北斗）
	if(p != NULL)
	{
		if(gnss_data_verify((uint8_t *)p,&data_len))
		{
			p = strstr(data,"ANTENNA OK");
			if(p != NULL)
			{
				gnss_info.ant_state = 0;   //外部天线OK
			}
			p = strstr(data,"ANTENNA OPEN");
			if(p != NULL)
			{
				gnss_info.ant_state = 1;  //外部定位天性断开
			}
			
			p = strstr(data,"ANTENNA SHORT");
			if(p != NULL)
			{
				gnss_info.ant_state = 2;  //外部定位天线短路
			}
		}
	}

	//过滤GNSS经纬度
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
						//save_gnss_info_back();
						gnss.state_back = gnss.state;
						gnss.nmea_cnt = 1000;
						//printf("-- save gnss..........%u,%u\r\n",gnss_info.latitude_normal,gnss_info.longitude_normal);
					}
					//rt_kprintf("the run is..........\r\n");
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
			int32_t tmp;
			//printf("-- tun is :%d...\r\n",gnss.nmea_cnt);
			if(++gnss.nmea_cnt >= 600)         //每10分钟保存一次定位位置
			{
				gnss.nmea_cnt = 0;
				//printf("-- Save gnss info....\r\n");
				save_gnss_info();
			}

			tmp = get_rtc_timestamp() - gnss_info.btc_unix;
			if(tmp > 0 || tmp < -5)	
			{
				printf("-- Gnss calibration rtc time......%u\r\n",gnss_info.btc_unix);
				set_rtc_time(&gnss_info.btc_time);
			}
		}

		return 0;
}



/************************************************
 * @desc  : 获取GNSS信息 对外接口
 * @param : gnss_data:返回gnss信息
 * @return: NULL
 * @Date  : 2021-6-12
 ************************************************/
uint8_t	read_gnss_info(struct gnss_info_str *source)
{
    if(pdTRUE == xSemaphoreTake(gnss.mutex, pdMS_TO_TICKS(1000)))
    {
        *source = gnss_info;
        xSemaphoreGive(gnss.mutex);
        return 0;
    }

    return 1;
}



/***********************************
**	定位状态
***********************************/

uint8_t read_gnss_ant_state(void)    //天线状态    20
{
	uint8_t rv;

	rv = gnss_info.ant_state;
	//rv = 1;

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


/*********************************
**	返回UTC时间
**	
*********************************/

uint8_t read_gnss_utc_time(struct rt_tm *ptime)
{
	if(ptime == NULL)
		return 1;
	
	memcpy((char *)ptime,(char *)&gnss_info.utc_time,sizeof(struct rt_tm));

	return 0;
}

/***********************************
**	返回定位状态
************************************/
uint8_t read_gnss_positing_state(void)
{
	uint8_t rv = 'V';

    rv = gnss_info.state;
    //rv = 'A';
	return rv;
}



/***********************************
**	      //维度(经纬度)
************************************/

uint32_t read_gnss_latitude(uint8_t n)   
{
	uint32_t  rv;

	if(n == 1)
		rv = gnss_info.latitude_nmea;
	else
		rv = gnss_info.latitude_normal;
		
	//rv = 38685702;

	return rv;
}        
	

/***********************************
**	      //维度(经纬度)
************************************/

uint16_t read_gnss_speed(void)
{
	uint16_t rv;

	rv = gnss_info.speed;
	//rv = 6789;
	return rv;
}


/***********************************
**	     航向角度
************************************/

uint16_t read_gnss_heading(void)
{
	uint16_t rv;

	rv = gnss_info.heading;

	return rv;

}


/***********************************
**	返回
************************************/
uint32_t read_gnss_longitude(uint8_t n)   
{
	uint32_t rv;


	if(n == 1)
        rv = gnss_info.longitude_nmea;
	else
		rv = gnss_info.longitude_normal;
	//rv = 100635308;
	return rv;
}         






/****************************
 ** 处理GNSS定位
 ****************************/

void thread_entry_gnss(void *parameter)
{
	uint8_t 	step = 0;

    parameter = parameter;

	rt_gnss_power_on();
	vTaskDelay(200);         //
	gnss.mutex = xSemaphoreCreateMutex();
    if (gnss.mutex == NULL)
    {
        printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
    }
	
	init_gnss_info();
    gnss_info.state = 'V';

	for (;;)
    {
        memset(nema_buf,0,sizeof(nema_buf));
		nema_len = read_data_from_uart2(nema_buf, sizeof(nema_buf) - 1, 5000, 150);
			
		//printf("-- recv nema info len: %d    ,,,,,     %d\r\n",nema_len,gnss.cnt_reset);
        if(nema_len == 0)
		{
			if(gnss.cnt_reset++ > 6)
			{
				gnss.cnt_reset = 0;
				gnss_info.module_state = 0;
				gnss_info.state = 'V';
				rt_gnss_power_off();
				vTaskDelay(100);
				rt_gnss_power_on();
				printf("-- Gnss module reset........\r\n");
			}
				//printf("-- Recv nema data......faile\r\n"); 			//
			continue;
		}

		//if(1)
		if(gnss.debug_state == 1)
		{
			printf("\r\n\r\n");
			write_data_to_uart0(nema_buf,nema_len);
		}
		
		//自适配定位模块波特率
		switch(step)
		{
			case 0:
				if(pdTRUE == xSemaphoreTake(gnss.mutex, pdMS_TO_TICKS(10)))
				{
					if(gnss_info_handler(nema_buf,nema_len) == 0)
					{

						write_data_to_uart2((uint8_t *)"$CCMSG,T01,1,1*20\r\n",sizeof("$CCMSG,T01,1,1*20\r\n"));
						step = 2;
						xSemaphoreGive(gnss.mutex);
						break;
					}
					xSemaphoreGive(gnss.mutex);
				}
				//uart_is_driver_installed
				rt_hw_init_uart2(9600);      //GNSS
				step++;
				continue;
			case 1:     //单北斗  115200
				if(pdTRUE == xSemaphoreTake(gnss.mutex, pdMS_TO_TICKS(10)))
				{
					if(gnss_info_handler(nema_buf,nema_len) == 0)
					{
						step = 2;
						xSemaphoreGive(gnss.mutex);
						break;
					}
					xSemaphoreGive(gnss.mutex);
				}

				step = 0;
				rt_hw_init_uart2(115200);      //GNSS
				continue;;
			default:    //北斗+GPS 9600
				break;
		}

		//printf("%s\r\n",nema_buf);
		if(pdTRUE == xSemaphoreTake(gnss.mutex, pdMS_TO_TICKS(10)))
		{
			if(gnss_info_handler(nema_buf,nema_len) == 0)
			{
				gnss_info.module_state = 1;
				gnss.cnt_reset = 0;
			}
			
			xSemaphoreGive(gnss.mutex);
		}	
		//printf("-- GNSS INfo:%u,%u\r\n",gnss.ticks,nema_len);
		gnss.ticks++;
		if(gnss.ticks % 300 == 0 && gnss_info.state == 'V')  
		//if(gnss.ticks % 100 == 0)   //5分钟不定位，冷启动一次定位模块
		{
			write_data_to_uart2((uint8_t *)cold_cmd,9);
		}
		
    }
    
}


