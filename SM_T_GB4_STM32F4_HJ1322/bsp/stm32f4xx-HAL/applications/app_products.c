

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "drv_iwg.h"
#include "drv_timer.h"
#include "drv_usart.h"
#include "drv_gpio.h"

#include "pro_data.h"
#include "common.h"

#include "app_shell.h"
#include "app_ver.h"
#include "app_lte.h"
#include "app_mon.h"
#include "app_ver.h"
#include "app_can_recv.h"
#include "app_gnss.h"
#include "app_fifo.h"
#include "app_can_send.h"
#include "app_files.h"
#include "app_iap.h"
#include "app_archive.h"
#include "app_edge.h"
#include "app_shell.h"
#include "app_products.h"
#include "app_main.h"



#define 	DEBUG_PLAT  0     /** 0:生产平台；1：测试平台 **/



/***************** 本地全局变量定义 *********************/

static rt_mq_t 											products_mq	= NULL;      //

static uint8_t 											products_port = 0;    //默认的生产端口CAN  0  1：通过SHELL口

static struct config_str						config_info;    						//设备配置信息


/********************************
**
**********************************/

rt_mq_t get_products_mq(void)
{
	return products_mq;
}




/*************************
**	 显示配置信息
**************************/

void view_config_info(void)
{
	rt_kprintf("---- 配置信息 ----\r\n");
	
	rt_kprintf("-------------------------\r\n");
	rt_kprintf("-- PIN 状态：0x%x\r\n",config_info.pin[0]);
	rt_kprintf("-- PIN:%s\r\n",&config_info.pin[1]);
	rt_kprintf("-- 备案状态:%x\r\n",config_info.archival_state);  //备案状态
	rt_kprintf("-- 设备编号：%s\r\n",config_info.terminal_id);
	rt_kprintf("-- 企业平台网关地址:%s\r\n",config_info.enterprise_gw_addr);
	rt_kprintf("-- 企业平台网关端口:%d\r\n",config_info.enterprise_gw_port);
	rt_kprintf("-- 企标数据上传周期:%d\r\n",config_info.travel_upload_cycle);
	rt_kprintf("-- 国标数据上传周期:%d\r\n",config_info.gb_four_upload_cycle);
	
	
	rt_kprintf("\r\n");
	rt_kprintf("\r\n");
}



/*************************************
**	读取BooTLoader信息
**************************************/

void read_boot_loader_info(uint8_t *buf,uint8_t buf_size)
{
	rt_device_t 								bsram_dev = RT_NULL;       //
	
	if(buf_size < 4 || buf == NULL)
		return;
	
	bsram_dev = rt_device_find("back_sram");
	rt_device_open(bsram_dev, RT_DEVICE_OFLAG_RDWR);
	rt_device_read(bsram_dev,0,buf,buf_size);
	rt_device_close(bsram_dev);
}



/*****************************
**
******************************/

uint32_t read_boot_loaer_ver_num(void)
{
	uint32_to_byte 		m_tmp = {0};
	uint32_t rv;
	
	read_boot_loader_info((uint8_t *)&m_tmp.value,4);
	
	rv = m_tmp.byte[2] * 10 + m_tmp.byte[3];
	
	return rv;
}




/***************************************
**	配置信息保存在外部EEPROM内部
****************************************/

static uint8_t save_products_cfg_info(void)
{
	rt_device_t eeprom_dev = RT_NULL;
	rt_size_t rv;
	
  eeprom_dev = rt_device_find("at24cxx");
	rt_device_open(eeprom_dev,RT_DEVICE_OFLAG_RDWR);
	config_info.product_mode  = 0x55;
	rv = rt_device_write(eeprom_dev, 0,(uint8_t *)&config_info,sizeof(config_info));
	
	rt_kprintf("-- write config info :%d,%u\r\n",rv,sizeof(config_info));
	if(rv != sizeof(config_info))
	{
		rt_device_close(eeprom_dev);
		return 1;
	}
	
	rt_device_close(eeprom_dev);
	rt_kprintf("-- save config info ....OK ...\r\n");
	
	return 0;
}





/***************************************
**	配置信息保存在外部EEPROM内部
**	默认参数：
**	
****************************************/

uint8_t load_products_cfg_info(void)
{
	rt_device_t eeprom_dev = RT_NULL;
	rt_err_t result = RT_EOK;
	
  eeprom_dev = rt_device_find("at24cxx");
	rt_device_open(eeprom_dev,RT_DEVICE_OFLAG_RDWR);
	
	result = rt_device_read(eeprom_dev, 0,(uint8_t *)&config_info,sizeof(config_info));
	//rt_kprintf("-- read config info:%d,%d\r\n",result,config_info.product_mode);
	if(result != RT_EOK || config_info.product_mode != 0x55)
	{
		config_info.product_mode = 0x56;
		
		rt_memset(config_info.terminal_id,'\0',sizeof(config_info.terminal_id));          //设备号
		rt_memcpy(config_info.enterprise_gw_addr,(char *)"was.g4.yingxuan.bcnyyun.com",sizeof("was.g4.yingxuan.bcnyyun.com"));              //调试使用
		config_info.enterprise_gw_port = 19003;     	 				//网关端口 
		
		rt_memcpy(config_info.archivel_gw_addr,"fdlpfjk.vecc.org.cn",sizeof("fdlpfjk.vecc.org.cn")); 
		config_info.archivel_gw_port = 61003;
		
		config_info.terminal_type = 0;		 							//设备类型
		config_info.fir_ver = 10;			 									//固件版本号
		
		config_info.run_time = 300;            	 					//断开ACC后运行时间(单位：秒，例如：300S，5分钟)
		config_info.sleep_time = 21600;          					//睡眠时间(单位：秒)
		config_info.travel_upload_cycle = 60;    				//行驶上传周期(单位：秒)
		config_info.gb_four_upload_cycle = 300;         //国标数据上传周期
		config_info.work_upload_cycle = 1;      				//作业上传周期(单位：秒)
		config_info.distance_upload = 200;     	 					//定距离报位 (单位：米)
		config_info.azimuth_upload = 300;       					//航向角报位(单位：度)
		config_info.car_type = 0x02;             					//安装车辆类型
		config_info.user_code = 26;            					//用户编码
		config_info.can_num = 0x02;												//CAN协议号 
		config_info.emission = 0x04;										  //排放阶段
		config_info.delay_shutdown_time = 300;            //300S 5分钟
		memcpy(config_info.apn,"CMIOT",sizeof("CMIOT"));    //APN
		memcpy(config_info.user,"",sizeof(""));												//GSM模块APN用户名
		memcpy(config_info.password,"",sizeof(""));										//GSM模块APN密码						
		config_info.hard_ware = 11;									//硬件版本
		memset(config_info.dev_id,0,3);
		memset(config_info.dev_secret,0,3);
		config_info.archival_state = 0;
	}
	rt_device_close(eeprom_dev); 
		
	if(config_info.enterprise_gw_port != 19003)     	 				//网关端口 
	{
		rt_memcpy(config_info.enterprise_gw_addr,(char *)"was.g4.yingxuan.bcnyyun.com",sizeof("was.g4.yingxuan.bcnyyun.com"));              //调试使用
		config_info.enterprise_gw_port = 19003;     	 				//网关端口 
		rt_memcpy(config_info.archivel_gw_addr,"fdlpfjk.vecc.org.cn",sizeof("fdlpfjk.vecc.org.cn")); 
		config_info.archivel_gw_port = 61003;
		
		rt_thread_delay(10);
		save_products_cfg_info();
	}
	

	if(config_info.car_type != 0x02)
	{
		config_info.car_type = 0x02;
		rt_thread_delay(50);
	}
	
	
	memcpy(config_info.terminal_id,"YXZ4112103051003",sizeof("YXZ4112103051003"));          //设备号
	
	return result;
}



/************************
**	返回排放阶段
*************************/

uint8_t read_config_emission(void)
{
	uint8_t rv = 0;
	
	rv = config_info.emission;
	
	return rv;
}


/***************************
**	返回备案状态
****************************/

uint8_t read_config_archival_state(void)
{
	uint8_t rv = 0;
	
	rv  = config_info.archival_state;
	
	return rv;
}



/***************************
**	返回设备内部VIN状态
****************************/

uint8_t read_config_vin_info(uint8_t *buf,uint8_t size_buf)
{
	if(size_buf < 17)
		return 0;
	
	memcpy(buf,(uint8_t *)&config_info.pin[1],17);
	
	return 17;
	
}



/***************************
**	返回设备内部VIN状态
**	0x5A标识已经正确读取到VIN
**	
****************************/

uint8_t read_config_vin_state(void)
{
	uint8_t rv;
	
	rv = config_info.pin[0];

	return rv;
}

/*******************************
**
*******************************/

uint8_t read_config_user_code(void)
{
	uint8_t rv;
	
	rv = config_info.user_code;
	rv = 26;
	
	return rv;
}

/*************************************************

*************************************************/
uint32_t get_product_add(uint32_t addr)
{
	uint32_t rv = 0;
	
	rv = addr - (uint32_t)&config_info.product_mode;
	
	return rv;
}


/********************************************
**	
********************************************/

void modification_config_info(uint32_t index,void *data, uint32_t len)
{
	rt_device_t eeprom_dev = RT_NULL;
	rt_size_t rv;
	uint32_t offset = 0;
	
	if(read_in_power_vol() < 80)  //防止没有外电操作EEPROM
		return;
	
	eeprom_dev = rt_device_find("at24cxx");
	rt_device_open(eeprom_dev,RT_DEVICE_OFLAG_RDWR);
	
	switch(index)
	{
		case 5:			//修改企标网关地址
			offset = get_product_add((uint32_t)&config_info.enterprise_gw_addr);
			memset(config_info.enterprise_gw_addr,'\0',sizeof(config_info.enterprise_gw_addr));
			memcpy(config_info.enterprise_gw_addr,(uint8_t *)data,len);
			rv = rt_device_write(eeprom_dev,offset,(uint8_t *)config_info.enterprise_gw_addr, sizeof(config_info.enterprise_gw_addr));
			if(rv == sizeof(config_info.enterprise_gw_addr))
			{
				rt_kprintf("-- Save config info enterprise_gw_addr ok ... \r\n");
			}
			else
			{
				rt_kprintf("-- Save config info enterprise_gw_addr fail ... \r\n");
			}
	
			rt_thread_delay(10);
			offset = get_product_add((uint32_t)&config_info.enterprise_gw_port);
			config_info.enterprise_gw_port = *(uint16_t *)((uint8_t *)data + 49);
			rv = rt_device_write(eeprom_dev,offset,(uint8_t *)&config_info.enterprise_gw_port, 2);
			if(rv == 2)
			{
				rt_kprintf("-- Save config info enterprise_gw_port ok ... %u\r\n",config_info.enterprise_gw_port);
			}
			else
			{
				rt_kprintf("-- Save config info enterprise_gw_port fail ... \r\n");
			}
			break;
		case 28:    //设置机械环保代码
			if(str_compare((uint8_t *)data,(uint8_t *)"00000000000000000",17) == 1)
			{
				offset = get_product_add((uint32_t)&config_info.pin);
				memset(config_info.pin,'\0',sizeof(config_info.pin));
				rv = rt_device_write(eeprom_dev,offset,(uint8_t *)config_info.pin, sizeof(config_info.pin));
				if(rv == sizeof(config_info.pin))
				{
					rt_kprintf("-- Clear config info PIN ok  %s... \r\n",config_info.pin);
				}
				else
				{
					rt_kprintf("-- Clear config info VIN fail ... \r\n");
				}
				
				rt_thread_delay(10);  //
				offset = get_product_add((uint32_t)&config_info.archival_state);
				config_info.archival_state = 0;
				rv = rt_device_write(eeprom_dev,offset,&config_info.archival_state, len);
				if(rv == len)
				{
					rt_kprintf("-- Clear config info archival state ok ... %d \r\n",config_info.archival_state);
				}
				else
				{
					rt_kprintf("-- Clear config info archival state fail ... \r\n");
				}
			}			
			else if(str_compare(&config_info.pin[1],(uint8_t *)data,17) == 0) 
			{
				offset = get_product_add((uint32_t)&config_info.pin);
				memset(config_info.pin,'\0',sizeof(config_info.pin));
				config_info.pin[0] = 0x5A;
				memcpy(&config_info.pin[1],(uint8_t *)data,sizeof(config_info.pin) - 1);
				rv = rt_device_write(eeprom_dev,offset,(uint8_t *)config_info.pin, sizeof(config_info.pin));
				if(rv == sizeof(config_info.pin))
				{
					rt_kprintf("-- Save config info PIN ok  %s... \r\n",config_info.pin);
				}
				else
				{
					rt_kprintf("-- Save config info VIN fail ... \r\n");
				}
			}
			
			if(config_info.archival_state != 1 && config_info.archival_state != 2 && config_info.pin[0] == 0x5A && config_info.emission == 4)
			{
				rt_thread_t tid_arch;
				
				tid_arch = rt_thread_create("archival",	thread_entry_archivel, 			RT_NULL,3076,27, 20);
				if(tid_arch !=  NULL)
				{
					rt_kprintf("-- Cread thread entry archivel OK...\r\n");
					rt_thread_startup(tid_arch);
				}
			}
			
			break;
		case 31:    //保存备案状态
			offset = get_product_add((uint32_t)&config_info.archival_state);
			config_info.archival_state = *(uint8_t *)data;
			rv = rt_device_write(eeprom_dev,offset,&config_info.archival_state, len);
			if(rv == len)
			{
				rt_kprintf("-- Save config info archival state ok ... %d \r\n",config_info.archival_state);
			}
			else
			{
				rt_kprintf("-- Save config info archival state fail ... \r\n");
			}
			break;
		case 33:   //修改国标数据上传周期
			offset = get_product_add((uint32_t)&config_info.gb_four_upload_cycle);
			config_info.gb_four_upload_cycle = *(uint32_t *)data;
			rv = rt_device_write(eeprom_dev,offset,(uint8_t *)&config_info.gb_four_upload_cycle, sizeof(config_info.gb_four_upload_cycle));
			if(rv == sizeof(config_info.gb_four_upload_cycle))
			{
				rt_kprintf("-- Save config info gb_four_upload_cycle ok ... %d \r\n",config_info.gb_four_upload_cycle);
			}
			else
			{
				rt_kprintf("-- Save config info gb_four_upload_cycle fail ... \r\n");
			}
			break;
		case 9:  //修改企标上传数据
			offset = get_product_add((uint32_t)&config_info.travel_upload_cycle);
			config_info.travel_upload_cycle = *(uint32_t *)data;
			rv = rt_device_write(eeprom_dev,offset,(uint8_t *)&config_info.travel_upload_cycle, 4);
			if(rv == 4)
			{
				rt_kprintf("-- Save config info travel_upload_cycle ok ... %d\r\n",config_info.travel_upload_cycle);
			}
			else
			{
				rt_kprintf("-- Save config info travel_upload_cycle fail ... \r\n");
			}
			break;    //修改企标数据上传周期
		case 8:
			offset = get_product_add((uint32_t)&config_info.sleep_time);
			config_info.sleep_time = *(uint32_t *)data;
			rv = rt_device_write(eeprom_dev,offset,(uint8_t *)&config_info.sleep_time, 4);
			if(rv == 4)
			{
				rt_kprintf("-- Save config info sleep time ok ... %d\r\n",config_info.sleep_time);
			}
			else
			{
				rt_kprintf("-- Save config info sleep time fail ... \r\n");
			}
			break;    //修改企标数据上传周期
		case 35:
			offset = get_product_add((uint32_t)&config_info.emission);
			config_info.emission = *(uint32_t *)data;
			rv = rt_device_write(eeprom_dev,offset,(uint8_t *)&config_info.emission, 1);
			if(rv == 1)
			{
				rt_kprintf("-- Save config info emission ok ... %d\r\n",config_info.emission);
			}
			else
			{
				rt_kprintf("-- Save config info sleep time fail ... \r\n");
			}
			break;    //
		case 29:
				offset = get_product_add((uint32_t)&config_info.public_key);
				memcpy(config_info.public_key,data,64);
				rv = rt_device_write(eeprom_dev,offset,(uint8_t *)config_info.public_key, 64);
				if(rv == 64)
				{
					//rt_kprintf("-- Slave Public Key OK ....... \r\n");
				}
				else
				{
					rt_kprintf("-- Slave Public Key Fail ....... \r\n");
				}
			break;
		default:
			break;
	}
	
	rt_device_close(eeprom_dev);
}


/**********************************
**	复位配置信息
**	擦除整个EEPROM存储2K存储区
***********************************/

uint8_t reset_config_info(void)
{
	rt_device_t eeprom_dev = RT_NULL;
	uint16_t i = 0;
	uint8_t array[256];
	
  eeprom_dev = rt_device_find("at24cxx");
	rt_device_open(eeprom_dev,RT_DEVICE_OFLAG_RDWR);
	
	memset(array,0xFF,256);
	for(i = 0;i < 2048;i += 256)
	{
		rt_device_write(eeprom_dev,i,array,256);
		rt_thread_delay(10);
		//rt_kprintf("-- errar i:%d\r\n",i);
	}

	rt_device_close(eeprom_dev);
	
	return 1;
}


/***************************************
**	配置信息保存在外部EEPROM内部
****************************************/
uint8_t read_config_hard_ware(void)
{
	uint8_t rv;
	
	rv = 11;//硬件版本

	return rv;
}



/******************************
**  
******************************/

uint8_t read_config_product_mode(void)
{
	uint8_t rv;
	
	rv = config_info.product_mode;
	
	return rv;
}



/*********************************
**	配置信息保存在外部EEPROM内部
**********************************/

uint16_t read_config_run_time(void)
{
	uint16_t rv;
	
	rv = config_info.run_time;

	return rv;
}



/***************************************
**	返回配置信息睡眠时间
****************************************/
uint16_t read_config_sleep_time(void)
{
	uint16_t rv;
	
	rv = config_info.sleep_time;
	//rv = 30;
	return rv;
}



/**************************************
**	返回延时关机时间  （S）
***************************************/

uint16_t read_config_delay_shutdown_time(void)
{
	uint16_t rv;

	rv = config_info.delay_shutdown_time;

	return rv;
}


/***************************************
**	返回设备号
****************************************/

uint8_t read_config_terminal_id(uint8_t *buf,uint8_t buf_size)
{
	if(buf_size < 16)
		return 0;
	
	rt_memcpy(buf,config_info.terminal_id,16);
	
	return 16;
}


/************************************
**	返回网关地址
*************************************/

uint8_t read_enterprise_gw_addr(uint8_t *buf,uint8_t buf_size)
{
	if(buf_size < 50)
		return 0;
	
	memcpy(buf,config_info.enterprise_gw_addr,50);
	
	return 50;
}


/************************************
**	返回网关地址
*************************************/
uint16_t read_enterprise_gw_port(void)
{
	uint16_t rv;
	
	rv = config_info.enterprise_gw_port;

	return rv;
}



/************************
**	读取车辆类型
*******************/

uint8_t read_config_car_type(void)
{
	uint8_t rv;
	
	rv = config_info.car_type;
	
	return rv;
}


/*****************************
**	返回配置状态
******************************/

uint8_t read_config_state(void)
{
	uint8_t rv;
	
	rv = config_info.product_mode;

	return rv;
}


/*****************************
**	返回配置-上传时间周期
******************************/

uint32_t read_config_travel_upload_cycle(void)
{
	uint32_t rv;
	
	rv = config_info.travel_upload_cycle;
	//rv = 20;
	
	if(rv > 600)
		rv = 600;
	
	return rv;
}



/*****************************
**	返回配置-国标上传时间周期
******************************/

uint32_t read_config_gb_four_upload_cycle(void)
{
	uint32_t rv;
	
	rv = config_info.gb_four_upload_cycle;
	
	if(rv > 600)
		rv = 600;
	//rv = 2;
	return rv;
}



/************************************
**	返回网关地址
*************************************/

uint8_t read_config_public_key(uint8_t *buf,uint8_t buf_size)
{
	if(buf_size < 64)
		return 0;
	memcpy(buf,&config_info.public_key[0],64);

	return 50;
}


/************************************************
 *  玉柴使用 （英轩）
************************************************/

void read_config_gps_id(uint8_t *buf,uint8_t buf_size)
{
	buf[0] = 'Y';  //
	buf[1] = 'X';	 //
	buf[2] = 'Z';  //
	buf[3] = 'G';  //
}




/************************************************
 *  玉柴使用 英轩重工
************************************************/

void read_config_mask(uint8_t *buf,uint8_t buf_size)
{
	if(buf_size < 4)
		return;
	
	buf[0] = 0x74;
	buf[1] = 0xE0;
	buf[2] = 0x27;
	buf[3] = 0x89;
}




/**************************************************
**	函数名称: 解析配置信息
***************************************************/

static uint8_t analysis_products_cfg_info(uint8_t *source,uint16_t len)
{
	uint8_t 		*p = source;
	uint8_t 		i;
	uint8_t			tmp_s[40];
	
	if(*(p) != ':')
		return 0;
	
	p += 1;
	memset(config_info.terminal_id,'\0',sizeof(config_info.terminal_id));    //解析设备号
	for(i = 0;i < 16;i++)  
	{	
		config_info.terminal_id[i] = *(p + i);
	}

	p += i;
	if(*p != ',')
		return 0;
				
	i = get_data_str(1,2,p,tmp_s,len);    //网关地址
	if(i > 0)
	{
		tmp_s[i] = '\0';
		memset(config_info.enterprise_gw_addr,'\0',sizeof(config_info.enterprise_gw_addr));
		memcpy(config_info.enterprise_gw_addr,tmp_s,i + 1);          	//				
	}
				
	i = get_data_str(2,3,p,tmp_s,len);              //端口号
	if(i > 0)
	{	
		tmp_s[i] = '\0';
		config_info.enterprise_gw_port = fr_atof((const char *)tmp_s);         			//
	}
				
	i = get_data_str(3,4,p,tmp_s,len);  
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.sleep_time = fr_atof((const char *)tmp_s);         			//休眠时间
	}
				
	i = get_data_str(4,5,p,tmp_s,len);  
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.gb_four_upload_cycle = fr_atof((const char *)tmp_s);   			//定距上传
	}


	i = get_data_str(5,6,p,tmp_s,len);  
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.travel_upload_cycle = fr_atof((const char *)tmp_s);  			//数据上传周期
	}

	i = get_data_str(6,7,p,tmp_s,len);  
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.hard_ware = fr_atof((const char *)tmp_s);      			//
	}
				
	i = get_data_str(7,8,p,tmp_s,len);  
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.can_num = fr_atof((const char *)tmp_s);   			//解析CAN协议号
					
		if(config_info.can_num == 0x02)                    //燃油装载机
		{
			config_info.car_type = 0x02;                     //
		}
		
		else
		{
			config_info.can_num = 0x02;                       	//
			config_info.car_type = 0x02;												//
		}
	}
	i = get_data_str(8,9,p,tmp_s,len);  
	if(i > 0)
	{
		memcpy(config_info.apn,tmp_s,i);           			//设备APN
		config_info.apn[i] = '\0';
	}
	
	i = get_data_str(9,10,p,tmp_s,len);               //激活状态            
	if(i > 0)
	{
		struct rt_can_event 				event = {0};
		rt_mq_t											t_mq = NULL;
		
		event.cmd = 6;
		event.arg1 = tmp_s[0];
		
		t_mq = get_can_lock_mq();
		if(t_mq != NULL)
			rt_mq_send(t_mq,&event,sizeof(event));
	}
	
	i = get_data_str(11,12,p,tmp_s,len);               //激活状态            
	if(i > 0)
	{
		if(tmp_s[0] == '1')
		{
			mkfs_files_sys();       //格式化文件系统(暂时未使用)
		}
	}
	
	i = get_data_str(13,14,p,tmp_s,len);    //网关地址
	if(i > 0)
	{
		tmp_s[i] = '\0';
		memset(config_info.archivel_gw_addr,'\0',sizeof(config_info.archivel_gw_addr));
		memcpy(config_info.archivel_gw_addr,tmp_s,i + 1);          	//				
	}
				
	i = get_data_str(14,15,p,tmp_s,len);              //端口号
	if(i > 0)
	{	
		tmp_s[i] = '\0';
		config_info.archivel_gw_port = fr_atof((const char *)tmp_s);         			//
	}
	
	for(i = 0;i < 3;i++)                            //
	{
		len = 0;
		len = tmp_s[i * 2 + 14] - 0x30;
		len <<= 4;
		len += (tmp_s[i * 2 + 15] - 0x30);
		config_info.dev_secret[i] = len;
	}
	
	for(i = 0;i < 3;i++)                           //
	{
		len = 0;
		len = config_info.terminal_id[i * 2 + 10] - 0x30;
		len <<= 4;
		len += (config_info.terminal_id[i * 2 + 11] - 0x30);
		config_info.dev_id[i] = len;
	}
	
	return 1;
}







/**********************
**	返回
***********************/

uint32_t read_config_work_upload_cycle(void)
{
	rt_uint32_t rv;
	
	rv = config_info.work_upload_cycle;
	
	return rv;
}


/**********************
**	返回
***********************/

void read_config_apn(unsigned char *buf,unsigned char buf_size)
{
	if(buf_size < 20)
		return;
	memcpy(buf,config_info.apn,20);
}



/************************************
**	返回网关地址
*************************************/

uint8_t read_config_archivel_gw_addr(uint8_t *buf,uint8_t buf_size)
{
	if(buf_size < 50)
		return 0;
	
	memcpy(buf,config_info.archivel_gw_addr,50);
	
	return 50;
}


/************************************
**	返回网关地址
*************************************/

uint16_t read_config_archivel_gw_port(void)
{
	uint16_t rv;
	
	rv = config_info.archivel_gw_port;

	return rv;
}



/**************************
**	返回备案状态
****************************/
uint8_t read_archival_state(void)
{
	uint8_t rv;
	
	rv = config_info.archival_state;
	
	return rv;
}


/************************************
**	返回网关地址
*************************************/

void read_config_dev_id(unsigned char *buf,unsigned char buf_size)
{
	if(buf_size < 3)
		return;
	config_info.dev_id[0] = 'Y';
	config_info.dev_id[1] = 'X';
	config_info.dev_id[2] = 'Z';
	memcpy(buf,config_info.dev_id,3);
}





/************************************
**	返回网关地址
*************************************/

void read_config_dev_secret(unsigned char *buf,unsigned char buf_size)
{
	if(buf_size < 3)
		return;
	memcpy(buf,config_info.dev_secret,3);
}






/**************************************************
**	函数名称:
**	功能描述:发送配置信息
**	相关配置信息
**	flag:终检或者配置
**	
***************************************************/
static uint16_t build_config_info(uint8_t *buf,uint16_t buf_size,uint8_t flag)
{
	uint16_t 						len;
	uint32_t 						tmp;
	uint8_t							i;
	uint8_t							tmp_c[50];
	uint8_t 						tmp_buf[20];
	
	uint32_to_byte			m_tmp;
	struct rt_tm				*ptime;
	
	if(buf_size < 220)
		return 0;
	
	len = 0; 
	
	memcpy((char *)buf,(char *)"homer4L:",sizeof("homer4L:") - 1);
	if(flag == 2)
		memcpy((char *)buf,(char *)"homer3t:",sizeof("homer3t:") - 1);
	len += sizeof("homer4L:") - 1;	
	
	for(i = 0;i < 16;i++)
		*(buf + len + i) = config_info.terminal_id[i];   			//设备号
	len += i;
	*(buf + len) = ',';                               		//
	len++;
	
	read_lte_icc_id((unsigned char *)buf + len,20);

	len += 20;
	buf[len++] = ',';                                		//
	
	
	tmp = strlen((const char *)config_info.enterprise_gw_addr);
	memcpy(buf + len,config_info.enterprise_gw_addr,tmp);     //网关地址
	len += tmp;
	buf[len++] = ',';                                //

	tmp = int_to_str(config_info.enterprise_gw_port,(char *)tmp_c,sizeof(tmp_c));
	for(i = 0;i < tmp;i++)            //网关端口号
		buf[len++] = tmp_c[i];
	buf[len++] = ',';

	tmp = strlen((const char *)config_info.apn);
	memcpy(buf + len,config_info.apn,tmp);        // APN 
	len += tmp;
	buf[len++] = ',';                             //
					
	
	if(read_gnss_positing_state() == 'A')                    //定位状态
	{
		buf[len] = '8';
		buf[len + 1] = '0';
	}
	else
	{
		buf[len] = '0';
		buf[len + 1] = '0';
	}
	len += 2;
					
	tmp = read_gnss_latitude(1);                   //维度(经纬度)
	
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));
	memset(buf + len,'0',9);
	memcpy(buf + len,tmp_c,tmp);              								//
	len += 9;
	tmp = read_gnss_longitude(1);                  //经度
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));                       
	memset(buf + len,'0',9);
	memcpy(buf + len,tmp_c,tmp);              //
	len += 9;

	tmp = sprintf((char *)&buf[len],(char *)"%04d%04d",read_gnss_speed(),read_gnss_heading());
	
	len += 8;

	ptime = (struct rt_tm *)tmp_c;
	read_gnss_utc_time(ptime);
	//rt_kprintf("\r\n-- the rtc:%d,%d,%d - %d:%d:%d\r\n",ptime->year,ptime->mon,ptime->day,ptime->hour,ptime->min,ptime->sec);
	tmp = sprintf((char *)&buf[len],(char *)"%02d%02d%02d%02d%02d%02d",ptime->year,ptime->mon,ptime->day,ptime->hour,ptime->min,ptime->sec);
	len += tmp;
	buf[len++] = ',';
	
	tmp = read_gnss_satellite_num();		        //使用卫星的数量	
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));
	memset(buf + len,'0',9);
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';
	
	tmp = read_lte_csq();                              				//信号值
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));
	memset(buf + len,'0',9);
	memcpy(buf + len,tmp_c,tmp);
	len += 2;
	buf[len++] = ',';
					
	tmp = read_in_power_vol();   
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));          //外部供电电压
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';
					
	tmp = read_ai_board_vol();
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));          //板卡电压
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';
					
	buf[len++] = read_in_acc_state() + 0x30;   										// ACC状态
	buf[len++] = ',';
		

	if(read_lte_net_init_state() == 0 || read_lte_sim_state() == 0)
		buf[len] = '0';   										//网络连接状态 0：未注册网络 1：已经附着网络	2：已经连接到服务器
	else
		buf[len] = '1';
	
	
	if(read_gb4_socket_state() == 1)           //链网状态
		buf[len] = '2';
	
	len++;
	
	buf[len++] = ',';
					
	tmp = read_user_ver();                    //单片机版本号
	buf[len++] = tmp / 10 + 0x30;
	buf[len++] = '.';
	buf[len++] = tmp % 10 + 0x30;
					
	buf[len++] = ',';

	
	tmp = int_to_str(config_info.sleep_time,(char *)tmp_c,sizeof(tmp_c));      		//睡眠时间
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';

	
	tmp = int_to_str(config_info.gb_four_upload_cycle,(char *)tmp_c,sizeof(tmp_c));      		//定距离报位时间
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';

	tmp = int_to_str(config_info.travel_upload_cycle,(char *)tmp_c,sizeof(tmp_c));      //数据推送周期
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';

	config_info.hard_ware = 11;
	buf[len++] = config_info.hard_ware / 10 + 0x30;      //硬件版本号
	buf[len++] = '.';
	buf[len++] = config_info.hard_ware % 10 + 0x30;
	buf[len++] = ',';
					
	buf[len++] = ((read_files_sys_state() == 0) ? 0 : 1 ) + 0x30;   							//SD卡状态
	buf[len++] = ',';

	buf[len++] = '1';									//EEPROM    18
	buf[len++] = ',';

	buf[len++] = '1';								 //Ex_RTC     19
	buf[len++] = ',';
					
	buf[len++] = read_input_ant_state() + 0x30;    //天线状态    20
	buf[len++] = ',';
			 
	buf[len++] = read_input_shell_state() + 0x30; 	//外壳状态    21
	buf[len++] = ',';

	buf[len++] = '0';                                //22
	buf[len++] = ',';

	buf[len++] = '1';								//23
	buf[len++] = ',';                         	//24

	read_boot_loader_info((uint8_t *)&m_tmp.value,4);              //BOOTLoader版本号
	tmp = hex_to_str(&m_tmp.byte[0],2,(unsigned char *)tmp_c,sizeof(tmp_c));      //"C"  Homer4C-T-7 (英轩重工)
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	
	buf[len++] = m_tmp.byte[2] + 0x30;
	buf[len++] = m_tmp.byte[3] + 0x30;
	
	config_info.hard_ware = 11;
	buf[len++] = config_info.hard_ware / 10 + 0x30;      //硬件版本号
	buf[len++] = config_info.hard_ware % 10 + 0x30;
	
	tmp = read_app_version();                //单片机版本
	buf[len++] = tmp / 10 + 0x30;     
	buf[len++] = tmp % 10 + 0x30;
	
	tmp = read_acl16_app_version();                 //嵌入式系统软件版本号
	buf[len++] = tmp / 10 + 0x30;   //
	buf[len++] = tmp % 10 + 0x30;   //
	
	tmp = read_user_ver();                 //嵌入式系统软件版本号
	buf[len++] = tmp / 10 + 0x30;   //
	buf[len++] = tmp % 10 + 0x30;   //
	
	buf[len++] = ',';
	
	tmp = int_to_str(config_info.can_num,(char *)tmp_c,sizeof(tmp_c));      //CAN协议号  25
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';

	//
	buf[len++] = read_can_connect_state(1) + 0x30;  //CAN1链接状态   26
	buf[len++] = ',';
	
	buf[len++] = read_can_connect_state(2) + 0x30;  //CAN2链接状态     27
	buf[len++] = ',';
	
	tmp = read_app_version();
	buf[len++] = tmp / 10 + 0x30; ;   //软件版本   28
	buf[len++] = '.';
	buf[len++] = tmp % 10 + 0x30; ;   //
	buf[len++] = ',';
	
	tmp = read_expect_mon_state();
	if(tmp == 1)
		buf[len++] = '1';   //激活期望状态     29
	else 
		buf[len++] = '0';
	
	buf[len++] = ',';
	
	tmp = read_lock_expect_state();     //锁车状态
	if(tmp == 1)
		buf[len++] = '1';	 //锁车期望状态   30  
	else 
		buf[len++] = '0';     
	
	buf[len++] = ',';
	
	buf[len++] = read_di_charg_state() + 0x30;		  //充电状态			    31 			
	buf[len++] = ',';
	
	buf[len++] = read_di_stdby_state() + 0x30;      //电池充满状态     32 
	buf[len++] = ',';
	
	buf[len++] = read_di_batter_state() + 0x30;     //电池状态     33
	buf[len++] = ',';
	
	
	tmp = read_in_acc_vol();                        //ACC电压

	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));          //ACC电压   34
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';
	
	
	tmp = read_in_batter_vol();
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));         //电池电压  35
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';

	if(flag == 0)  
	{
		tmp = read_lte_imei_id((unsigned char *)tmp_c,sizeof(tmp_c));       //LTE模块IMEI号   36
		memcpy(buf + len,tmp_c,tmp);
		//memcpy(buf + len,"866222051446535",sizeof("866222051446535"));
	}
	else
	{
		tmp = read_lte_imei_id((unsigned char *)tmp_c,sizeof(tmp_c));       //LTE模块IMEI号   36
		memcpy(buf + len,tmp_c,tmp);
		//memcpy(buf + len,"866222051446535",sizeof("866222051446535"));
	}
	
	len += 15;
	buf[len++] = ','; 
	      //第三十七段 蓝牙状态
	buf[len++] = ',';
                      //第三十八段 蓝牙强度
	buf[len++] = ',';    												// 39 段蓝牙名称
	*(buf + len) = ',';                               		//
	len++;
	
	if(read_acl16_work_state() == 1)
		buf[len++] = 0x30;
	else
		buf[len++] = 0x31;
  
	buf[len++] = ','; //第四十段   加密芯片状态
	
	tmp = read_encryption_chip_id(tmp_buf,sizeof(tmp_buf));
	//tmp = hex_to_str(tmp_buf,16, tmp_c,sizeof(tmp_c));
	memcpy(buf + len,tmp_buf,tmp);
	len += tmp;
  buf[len++] = ','; //第四十一段 加密芯片ID
	
	tmp = read_acl16_app_version();
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));     
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
  buf[len++] = ','; //第四十二段 加密芯片版本

	tmp = strlen((const char *)config_info.archivel_gw_addr);
	memcpy(buf + len,config_info.archivel_gw_addr,tmp);     //网关地址
	len += tmp;
	buf[len++] = ',';                                //

	tmp = int_to_str(config_info.archivel_gw_port,(char *)tmp_c,sizeof(tmp_c));
	for(i = 0;i < tmp;i++)            //网关端口号
		buf[len++] = tmp_c[i];
	buf[len++] = ',';
	
	
	buf[len++] = 0x0d;
	buf[len++] = 0x0a;   //回车换行符
	
	return len;
}




/*************************
**	通过SHELL端口发送生产信息
**	(暂时这样使用)
**************************/

uint16_t send_products_info(uint8_t *data,uint16_t len)
{
	uint16_t 			rv = 0;
	rt_device_t  	t_dev = NULL;
	
	t_dev = rt_device_find("uart1");
	
	if(t_dev == NULL)
		return 0;
	
	rv = rt_device_write(t_dev, 0, data, len);
	
	rt_device_close(t_dev);
	
	return rv;
}



/*****************************
**	
******************************/

void thread_entry_products(void *parameter)
{
	struct products_data_t 		products_data;
	
	parameter = parameter;
	
	products_mq = rt_mq_create("products_mq",sizeof(struct products_data_t),3,RT_IPC_FLAG_FIFO);    //
	
	for(;;)
	{
		if(rt_mq_recv(products_mq,&products_data,sizeof(products_data),100) == RT_EOK)
		{
			switch(products_data.cmd)
			{
				case 0:				//  解析下行配置信息
					if(config_info.product_mode != 0x55)
					{
						analysis_products_cfg_info(products_data.data,products_data.len);
						//rt_kprintf("-- Recv products Info:%d,%s\r\n",products_data.len,products_data.data);
					}
					break;
				case 1:          //结束配置
					if(config_info.product_mode != 0x55)
					{
						if(save_products_cfg_info() == 0)
						{
							struct lte_mq_t event;
		
							rt_mq_t			tmp_mq;
														
							rt_thread_delay(50);
							load_products_cfg_info();
							products_data.len = build_config_info(products_data.data,sizeof(products_data.data),2);
							
							if(products_data.len > 0)
							{
								if(products_port == 1)
								{
									send_data_to_shell_dev(products_data.data,products_data.len); 
								}	
								else if(products_port == 2)
								{
									//write_data_to_can(products_mq.data,products_mq.len);    //通过CAN接口发送
								}
								else
								{
									send_data_to_shell_dev(products_data.data,products_data.len); 
									//write_data_to_can(products_mq.data,products_mq.len); 
								}
							}
							
							event.cmd = 0;
							tmp_mq = get_lte_link_mq();
							rt_kprintf("-- config end reboot net ......\r\n");
							if(tmp_mq != NULL)
								rt_mq_send(get_lte_link_mq(),&event,sizeof(struct lte_mq_t));   //关闭网络
						}
				  }
					break;
				case 2:  //进入生产模式
					config_info.product_mode = 0x56;
					products_port = 1;
					break;
				case 3:  //通过平台设置，修改配置信息
					modification_config_info(products_data.data[0],&products_data.data[1],products_data.len);
					break;
				case 4:  //进入生产模式，通过CAN接口
					config_info.product_mode = 0x56;
					products_port = 2;
					break;
				case 5:
				{						
						uint8_t		array[16] = {0};

						read_config_terminal_id(array,16);
						if(*(uint32_t *)(array + 8) != *(uint32_t *)&products_data.data[0] || *(uint32_t *)(array + 12) != *(uint32_t *)&products_data.data[4])
							break;
						
						products_data.len = build_config_info(products_data.data,sizeof(products_data.data),1);
						if(products_data.len > 0)
						{
								write_buf_to_can1(products_data.data,products_data.len); 
						}	
					}
					break;
				default:
					break;
			}
		}
		
		if(config_info.product_mode == 0x56)
		{
			products_data.len = build_config_info(products_data.data,sizeof(products_data.data),1);
			
			if(products_data.len > 0)
			{
				if(products_port == 1)
				{
					//rt_kprintf("-- send products info.....\r\n");
					send_data_to_shell_dev(products_data.data,products_data.len);   //通过串口发送
				} 
				else if(products_port == 2)
				{
					//write_data_to_can(products_mq.data,products_mq.len);    //通过CAN接口发送
				}
				else
				{
					send_data_to_shell_dev(products_data.data,products_data.len);    //通过串口发送
					rt_thread_delay(10);
					//write_data_to_can(products_mq.data,products_mq.len);    //通过CAN接口发送
				}
			}
		}
	}
}




