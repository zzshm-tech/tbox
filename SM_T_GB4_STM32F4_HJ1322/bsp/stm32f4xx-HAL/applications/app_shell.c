


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
#include "app_main.h"
#include "app_products.h"






#define  SHELL_BUFF_LEN 			512



/***************** 本地全局变量 *******************/
static rt_device_t 						shell_dev = RT_NULL;						//命令串口

static struct shell_info_t 		shell_info = {0};

static uint8_t 								shell_buff[512] = {0};





/*******************************************
**	关闭
*******************************************/
void close_shell_module(void)
{
	if(shell_dev != RT_NULL)
	{
		rt_device_close(shell_dev);     //关闭串口
		shell_dev->flag &= 0xFFEF;
	}
	
	rt_rs232_power_off();                 //关闭GNSS电源							
}





/**************************************
**	通过SHELL接口发送数据
***************************************/

uint16_t send_data_to_shell_dev(uint8_t *data,uint16_t len)
{
	uint16_t rv = 0;
	
	if(shell_dev == NULL)
		return 0;
	
	rv = rt_device_write(shell_dev, 0, data, len);
	if(rv != len)
		return 0;
	
	return rv;
}



/*************************
**	
**************************/
void view_stream_data(void)
{
	struct ecu_data_str		*p;
	uint8_t 							tmp_buf[200];
	uint16_t 							tmp;
	
	p = (struct ecu_data_str *)tmp_buf;
	read_ecu_data(p,sizeof(tmp_buf));
	
	rt_kprintf("\r\n");
	rt_kprintf("\r\n");
	
	rt_kprintf("-------\r\n");
	
	tmp = read_gnss_speed() * 256 / 100;
	
	rt_kprintf("-- 行驶速度 : %d\r\n",tmp);
	rt_kprintf("-- 大气压力(kPa) : %d\r\n",p->air_pressure);
	rt_kprintf("-- (%%) : %d\r\n",p->engine_torque);
	rt_kprintf("-- (%%) : %d\r\n",p->friction_torque);
	rt_kprintf("-- (rpm) : %d\r\n",p->engine_rotate);
	rt_kprintf("-- (L/h) : %d\r\n",p->engine_fuel_flow);
	rt_kprintf("-- (ppm) : %d\r\n",p->scr_upstream_nox);
	rt_kprintf("-- (ppm) : %d\r\n",p->scr_downstream_nox);
	rt_kprintf("-- (%%) : %d\r\n",p->reactant_allowance);
	rt_kprintf("-- (kg/h) : %d\r\n",p->enter_volume);
	rt_kprintf("-- () %d\r\n",p->scr_entrance_temp);
	rt_kprintf("-- SCR() %d \r\n",p->scr_exit_temp);
	rt_kprintf("-- (kPa) : %d\r\n",p->dpf_diffPressure);
	rt_kprintf("-- : %d\r\n",p->coolant_temp);
	rt_kprintf("-- (%%) : %d\r\n",p->fuel_percent);
	rt_kprintf("--  : %d\r\n",p->egr_opening);
	rt_kprintf("--  : %d\r\n",p->egr_setting);
	
	rt_kprintf("-------------------------------------\r\n");
	rt_kprintf("\r\n");
	rt_kprintf("\r\n");
}



/********************
**
*********************/

void view_vehicel_data(void)
{
	struct ecu_data_str				*p_can;
	uint8_t 									tmp_buf[256];
	
	p_can = (struct ecu_data_str *)tmp_buf;
	read_ecu_data(p_can,sizeof(tmp_buf));
	
	rt_kprintf("\r\n");
	rt_kprintf("\r\n");
	rt_kprintf("-------------------------\r\n");
	rt_kprintf("\r\n");
	rt_kprintf("\r\n");
}





/********************
**
*********************/

void view_tcu_data(void)
{
	struct ecu_data_str				*p_can;
	uint8_t 									tmp_buf[256];
	
	p_can = (struct ecu_data_str *)tmp_buf;
	read_ecu_data(p_can,sizeof(tmp_buf));
	
	rt_kprintf("\r\n");
	rt_kprintf("\r\n");
	
	rt_kprintf("-------------------------\r\n");
	rt_kprintf("\r\n");
	rt_kprintf("\r\n");
}


/***********************************
**	鎵撳嵃瀹氫綅淇℃伅
************************************/

static void view_gnss_info(void)
{
	struct gnss_info_str gnss;

  read_gnss_info(&gnss);

	printf("\r\n***********  ***********\r\n");

  printf("-- 定位状态 ：%c\r\n",gnss.state);  											//	定位状态		
  printf("-- :%d\r\n",gnss.longitude_ew); 						//	
  printf("-- :%d\r\n",gnss.latitude_sn);  						//
  printf("-- :%d\r\n",gnss.longitude_normal);   					//
  printf("-- :%d\r\n",gnss.latitude_normal);    					//
  printf("-- :%d\r\n",gnss.altitude);    									//  
  printf("-- :%d\r\n",gnss.heading);     							//
  printf("-- :%d\r\n",gnss.speed);       									// 
  printf("-- :%d\r\n",gnss.satellite_num); 				//
  printf("-- :%d\r\n",gnss.hdop);        							//
  printf("-- :%d\r\n",gnss.gps_sate_num);					//
	printf("-- :%d\r\n",gnss.bd_sate_num);					//
  printf("-- :%d,%d,%d  %d:%d:%d\r\n",gnss.utc_time.year,gnss.utc_time.mon,gnss.utc_time.day,gnss.utc_time.hour,gnss.utc_time.min,gnss.utc_time.sec);		//????????????????
	printf("-- %d\r\n",gnss.module_state); 						//
	printf("-- %d\r\n",gnss.ant_state);			    			//
  printf("\r\n*********************************\r\n");
}


/***********************
**	
*********************/

void view_qb4_fault_code(void)
{
	uint32_t i;
	struct gb1939_dm_str tmp;

	rt_kprintf("\r\n");
	rt_kprintf("\r\n");
	
	rt_kprintf("---- OBD国标信息(国标信息) ----\r\n");
	rt_kprintf("-- 排放控制报警灯状态... %d\r\n",read_mil_light_state());
	
  read_ecu_qb4_dm_data(&tmp);
	rt_kprintf("-- ECU 故障数量... %d\r\n",tmp.num);
	for(i = 0;i < tmp.num;i++)
	{
		rt_kprintf("-- 故障码【%d】 SPN:%d,FMI:%d\r\n",i,tmp.spn[i],tmp.fmi[i]);
	}
	
	rt_kprintf("-------------------------\r\n");
	rt_kprintf("\r\n");
	rt_kprintf("\r\n");
}




/***********************
**	故障码
*********************/

void view_gb4_fault_code(void)
{
	uint32_t i;
	struct gb1939_dm_str tmp;

	rt_kprintf("\r\n");
	rt_kprintf("\r\n");
	
	rt_kprintf("---- OBD国标信息(国标信息) ----\r\n");
	rt_kprintf("-- 排放控制报警灯状态... %d\r\n",read_mil_light_state());
	
  read_ecu_gb4_dm_data(&tmp);
	rt_kprintf("-- ECU 故障数量... %d\r\n",tmp.num);
	for(i = 0;i < tmp.num;i++)
	{
		rt_kprintf("-- 故障码【%d】 SPN:%d,FMI:%d\r\n",i,tmp.spn[i],tmp.fmi[i]);
	}
	
	rt_kprintf("-------------------------\r\n");
	rt_kprintf("\r\n");
	rt_kprintf("\r\n");
}





/***********************
**	故障码
*********************/

void view_tcu_fault_code(void)
{
	uint32_t i;
	struct gb1939_dm_str tmp;

	rt_kprintf("\r\n");
	rt_kprintf("\r\n");
	
	rt_kprintf("---- TCU故障信息 ----\r\n");
	
  read_tcu_gb4_dm_data(&tmp);
	rt_kprintf("-- TCU 故障数量... %d\r\n",tmp.num);
	for(i = 0;i < tmp.num;i++)
	{
		rt_kprintf("-- 故障码【%d】 SPN:%d,FMI:%d\r\n",i,tmp.spn[i],tmp.fmi[i]);
	}
	
	rt_kprintf("-------------------------\r\n");
	rt_kprintf("\r\n");
	rt_kprintf("\r\n");
}


/*****************************
**	SHELL 处理
******************************/

uint8_t shell_cmd_handler(const char *data,rt_uint16_t len)
{
	char *p = RT_NULL;
	
	p = rt_strstr(data,"AT+FE");
	if(p != NULL)
	{
		reset_edge_data();
	}
	
  p = rt_strstr(data, "Reset");
  if (p != RT_NULL)
  {
		struct app_main_mq_t      	amq = {0};
		rt_mq_t 										tmp = NULL;
		
		amq.state = 1;
		tmp = get_app_main_queue();
		if(tmp != NULL)
			rt_mq_send(tmp,&amq,sizeof(struct app_main_mq_t));
		return 0;
  }
	
	p = rt_strstr(data, "reboot");
  if (p != RT_NULL)
  {
		struct app_main_mq_t      	amq = {0};
		rt_mq_t 										tmp = NULL;
		
		amq.state = 1;
		tmp = get_app_main_queue();
		if(tmp != NULL)
			rt_mq_send(tmp,&amq,sizeof(struct app_main_mq_t));
		return 0;
  }
	
	
	p = rt_strstr(data, "shutdown");
  if (p != RT_NULL)
  {
		struct app_main_mq_t      	amq = {0};
		rt_mq_t 										tmp = NULL;
		if(rt_read_acc_state() > 0)
		{
			rt_kprintf("-- Acc Open ...........\r\n");
			return 0;
		}
		amq.state = 2;
		tmp = get_app_main_queue();
		if(tmp != NULL)
			rt_mq_send(tmp,&amq,sizeof(struct app_main_mq_t));
		return 0;
  }
	
	p = strstr((char *)data,"AT+VERSION\r\n");
  if(p != NULL)
	{
		//打印程序版本号
	}
	p = rt_strstr(data, "AT+NEMA=");         //
  if (p != RT_NULL)
  {
		uint32_t cmd = 0;
		
		sscanf(p,"AT+NEMA=%d\r\n",&cmd);
		
		init_gnss_debug_state((uint8_t)cmd);
		//rt_kprintf("-- Set NEMA state : %d\r\n",cmd);
  }
    
	
	p = rt_strstr(data, "DEBUG_TEST");         //
  if (p != RT_NULL)
  {
		uint32_t cmd = 0;
		sscanf(p,"DEBUG_TEST=%d\r\n",&cmd);
		switch(cmd)
		{
			case 1:
				view_gnss_info();
				break;
			case 2:
				view_stream_data();
				break;
			case 3:
				view_qb4_fault_code();   //DM1故障码
				break;
			case 4:
				view_vehicel_data();    //国四数据
				break;
			case 5:  //现实配置信息
				//view_config_info();
				break;
			default:
				break;
		}
			//view_tcu_data();
			//view_config_info();
  }
    
	p = rt_strstr(data, "AT+Test");           //手动进入配置模式
  if (p != NULL)
  {
		rt_mq_t t_mq = NULL;
		struct products_data_t t_data = {0};
		t_data.cmd = 2;
		t_mq = get_products_mq();
		if(t_mq != NULL)
			rt_mq_send(t_mq,(uint8_t *)&t_data,sizeof(struct products_data_t));
			
		return 0;
  }
  p = rt_strstr(data, "BDWMODIF:");          //配置信息
  if (p != RT_NULL)
  {
    rt_mq_t t_mq = NULL;
		struct products_data_t t_data = {0};
		
		t_data.cmd = 0;
		t_data.len = len - 8;
		memcpy(t_data.data,p + 8,len - 8);
		t_mq = get_products_mq();
		if(t_mq != NULL)
			rt_mq_send(t_mq,(uint8_t *)&t_data,sizeof(struct products_data_t));
			
    return 0;
  }
    
	p = rt_strstr(data, "HOMER3ETESTOVER!");     //结束配置     
  if (p != NULL)
  {
		rt_mq_t t_mq = NULL;
		struct products_data_t t_data = {0};
		t_data.cmd = 1;
		
		t_mq = get_products_mq();
		if(t_mq != NULL)
			rt_mq_send(t_mq,(uint8_t *)&t_data,sizeof(struct products_data_t));
			
		return 0;
  }
    
	p = rt_strstr(data, "000000RESET!");             //回复出厂设置
  if(p != RT_NULL)
  {
		struct app_main_mq_t      	amq = {0};
		rt_mq_t 										tmp = NULL;
				
		reset_config_info();     //擦除EEPROM
		rt_kprintf("-- Reset factory data...\r\n");
		amq.state = 1;
		tmp = get_app_main_queue();
		if(tmp != NULL)
			rt_mq_send(tmp,&amq,sizeof(struct app_main_mq_t));
  }
		
		//调试备案
	p = rt_strstr(data,"AT+ARCH");
		
	if(p != NULL)
	{
		rt_thread_t tid_arch;
	
		tid_arch = rt_thread_create("archival",	thread_entry_archivel, 			RT_NULL,3076,27, 20);
		if(tid_arch !=  NULL)
		{
			rt_kprintf("-- Cread thread entry archivel OK...\r\n");
			rt_thread_startup(tid_arch);
		}
	}
	
	
	 p = strstr((char *)data,"AT+PIN=");   //??????
    if(p != NULL)
    {
        uint8_t array[32];
        rt_mq_t 									t_mq = NULL;
				struct products_data_t 		pd = {0};
					
				sscanf((char *)data,"AT+PIN=%s\r\n",array);				
				pd.cmd = 3;
				pd.len = 18;
				pd.data[0] = 28;
				memset(&pd.data[1],'\0',20);
								
				memcpy(&pd.data[1],array,17);   //拷贝机械环保代码
				t_mq = get_products_mq();
				if(t_mq != NULL)
					rt_mq_send(t_mq,&pd,sizeof(struct products_data_t));
										
				rt_kprintf("-- Set PIN OK ......  %s\r\n",&pd.data[1]);
    }

		p = strstr((char *)data,"AT+RTC?");   //??????
    if(p != NULL)
    {
        struct rt_tm tm;

        rt_get_rtc(&tm);
        printf("-- RTC:20%02d.%02d.%02d : %02d-%02d-%02d\r\n",tm.year,tm.mon,tm.day,tm.hour,tm.min,tm.sec);
       
        return 0;
    }
		
		p = strstr((char *)data,"AT+PIN?\r\n");
    if(p != NULL)
    {
        uint8_t  array[32] = {0};
        
        memset(array,'\0',sizeof(array));

        if(read_config_vin_state() == 0x5A)
            read_config_vin_info(array,sizeof(array));
   
        printf("-- PIN:%s\r\n",array);
    }
		
		p = strstr((char *)data,"AT+DEV?");   //
    if(p != NULL)
    {
        uint8_t array[32];

        memset(array,'\0',32);

        read_config_terminal_id(array,32);
        rt_kprintf("-- The Terminal ID:%s\r\n",array);
        
        return 0;
    }

    p = strstr((char *)data,"AT+KEY?");   //
    if(p != NULL)
    {
        uint8_t array[100];
       
        
        rt_kprintf("--------------------- 加密处理器信息 ---------------------\r\n");
       
        rt_kprintf("-- 加密处理器状态：");
        if(read_acl16_work_state() == 1)
        {
            rt_kprintf("异常\r\n");
        }   
        else 
        {
            rt_kprintf("正常\r\n");
						memset(array,0,sizeof(array));
            read_encryption_chip_id(array,32);
            rt_kprintf("\r\n-- 加密处理器ID:%s\r\n",array);
            rt_kprintf("\r\n-- 加密处理器应用程序版本：%d\r\n", read_acl16_app_version());
            read_config_public_key(array,sizeof(array));
            rt_kprintf("\r\n-- 公钥信息：");
            mem_printf(LOG_ERROR,PRINT_HEX,array,64);
        }
        rt_kprintf("\r\n---------------------------------------------------------\r\n");
        return 0;
    }

	p = strstr(data, "AT+CLOSE_NET");
	if(p != NULL)
	{
    struct lte_mq_t event;
		
		rt_mq_t			tmp_mq;
	
		event.cmd = 1;
		
		tmp_mq = get_lte_link_mq();
		rt_kprintf("-- Shell cmd close net ......\r\n");
		if(tmp_mq != NULL)
			rt_mq_send(get_lte_link_mq(),&event,sizeof(struct lte_mq_t));   //
	}
	
	p = strstr(data, "AT+OPEN_NET");
	if(p != NULL)
	{
    struct lte_mq_t event;
		
		rt_mq_t			tmp_mq;
	
		event.cmd = 2;
		
		tmp_mq = get_lte_link_mq();
		rt_kprintf("-- Shell cmd open net ......\r\n");
		if(tmp_mq != NULL)
			rt_mq_send(get_lte_link_mq(),&event,sizeof(struct lte_mq_t));   //
	}
	
	p = rt_strstr(data, "AT+GB4?");           //
  if (p != NULL)
  {
		view_stream_data();
		return 0;
  }
	
	p = strstr((char *)data,"AT+GNSS?\r\n");
  if(p != NULL)
  {
		view_gnss_info();	
  }
	
	
	p = strstr((char *)data,"AT+files?\r\n");
  if(p != NULL)
  {
		view_gnss_info();
  }
		
	p = strstr(data,"FILE_SYSTEM_FORMAT");
	if(p != NULL)
	{
		struct app_main_mq_t      	amq = {0};
		rt_mq_t 										tmp = NULL;
		
		
		if(mkfs_files_sys() == 0)   //
			rt_kprintf("-- FILE_SYSTEM_FORMAT ... OK\r\n");
    
		amq.state = 1;
		tmp = get_app_main_queue();
		if(tmp != NULL)
			rt_mq_send(tmp,&amq,sizeof(struct app_main_mq_t));
	}
	
	
	p = strstr((char *)data,"AT+VERSION\r\n");
  if(p != NULL)
  {
		rt_kprintf("-- 应用程序版本：%d.%d\r\n",read_app_version() / 10,read_app_version() % 10);
  }
	
	
	 p = strstr((char *)data,"AT+FilseSys?\r\n");
   if(p != NULL)
   {
		 show_files_sys_info();
   }
	
	p = strstr((char *)data,"AT+GB_CYCLE=");
  if(p != NULL)
  {
		uint32_t 									m = 0;
		rt_mq_t 									t_mq = NULL;
		struct products_data_t 		pd = {0};
							
		sscanf((char *)data,"AT+GB_CYCLE=%d\r\n",&m);
    rt_kprintf("-- Set GB_CYCLE=:%d\r\n",m);
		
		pd.cmd = 3;
		pd.len = 4;
		pd.data[0] = 33;

		*(uint16_t *)&pd.data[1] = m;

		t_mq = get_products_mq();
		if(t_mq != NULL)
			rt_mq_send(t_mq,&pd,sizeof(struct products_data_t));
		return 0;
  }
	
	p = strstr((char *)data,"AT+OTA");   //
  if(p != NULL)
  {
		uint8_t array[64];
		struct iap_ftp_str   iap_ftp = {0};                      //

		memset((uint8_t *)&iap_ftp,'\0',sizeof(struct iap_ftp_str));
		memset(array,'\0',sizeof(array)); 
		if(get_buf_str(1,2,',',(uint8_t *)data,array,len) > 0)     
		{
			memcpy(iap_ftp.files,array,sizeof(iap_ftp.files));
		}	
		else
		{
			return 1;
		}	
      
		memcpy(iap_ftp.user,(uint8_t *)"ftadmin",sizeof("ftadmin"));
		memcpy(iap_ftp.passwd,(uint8_t *)"ftadmin81645",sizeof("ftadmin81645"));
	 	memcpy(iap_ftp.host,(uint8_t *)"223.223.187.35",sizeof("223.223.187.35"));
		iap_ftp.port = 1050;

    rt_kprintf("\r\n-- FTP FtpUserName:%s\r\n",iap_ftp.user);
		rt_kprintf("-- FTP FtpUserPassd:%s\r\n",iap_ftp.passwd);
	  rt_kprintf("-- FTP Server Addr:%s\r\n",iap_ftp.host);
		rt_kprintf("-- FTP Server Port:%d\r\n",iap_ftp.port);
		rt_kprintf("-- FTP File Name:%s\r\n",iap_ftp.files);
     
	  if(read_iap_state() == 0)
		{
			rt_thread_t	 tid_iap = NULL;
		
			tid_iap =  rt_thread_create("ftp_iap",thread_entry_iap,&iap_ftp,2048, 28, 20); 
			if(tid_iap != RT_NULL)
			{
				rt_kprintf("-- Create Ftp iap thread OK ... \r\n");
				rt_thread_startup(tid_iap);
			}
			rt_thread_delay(100);
		} 						
  }
		
	return 0;
}


/**************************************
**	
***************************************/
 
rt_err_t shell_data_rx_call_fun(rt_device_t dev, rt_size_t size)
{
	shell_info.ticks = rt_tick_get(); /* 取收到数据时的tick值*/
  shell_info.len = size;          /* 当前的接收的数据长度*/
    
	return RT_EOK;
}

/**********************************************
**
**********************************************/
void shell_tick_handle(void)
{
	if(shell_info.ticks == 0xffffffff)
        return;

  if(rt_tick_get() - shell_info.ticks > 1)
  {
		shell_info.ticks = 0xffffffff;     /* tick值复位*/
    rt_sem_release(&shell_info.sign); /* 释放信号量,激活命令解析任务*/
  }
}




/***********************************************
**  处理加密芯片验签
************************************************/

uint8_t acl16_verify(uint8_t *data,uint16_t size)
{
    uint16_t        tmp = 0;
    uint8_t         array[64];
    uint8_t         verfy_buf[64];
    //uint8_t         buf[200];
    uint16_t        len;

    if(data == NULL || size < 5)
        return 1;
    
    if(*data != 0x5A)
        return 1;

    tmp = calc_xor_verify(data,size - 1);
    if(tmp != *(data + size - 1))
        return 1;
    
    switch(*(data + 1))
    {
        case 0xA0:
            //printf("-- VerfyCMD\r\n");
            array[0] = 0x5A;
            array[1] = 0xA0;   
            array[2] = 0x00;
            array[3] = 0x02;
            array[4] = 0x90;
            array[5] = 0x00;
            array[6] = 0xE6;
            send_data_to_shell_dev(array,7);
            break;
        case 0xA5:
            tmp = swap_uint16_t(*(uint16_t *)(data + 2));
            //printf("-- recv send len:%d\r\n",tmp);
            memcpy(array,data + 4,tmp);
            //mem_printf(LOG_ERROR, PRINT_HEX, array, tmp);
            shell_buff[0] = 0x5A;
            shell_buff[1] = 0xA5;
            shell_buff[2] = 0x00;
            shell_buff[3] = 0x90;
            len = 4;
            read_config_public_key(shell_buff + len,64);
            len += 64;
            read_encryption_chip_id(shell_buff + len,16);
            len += 16;
            
	        //mem_printf(LOG_ERROR, PRINT_HEX, buff, len + 16);
						read_encryption_chip_id(array + tmp,16);
            if(64 != acl_write_then_read(CRY_SIGN,array,verfy_buf,tmp + 16))
            {
                rt_kprintf("-- CRY_SIGN is error...\r\n");
                //return 0;
            }
	
            memcpy(shell_buff +len,verfy_buf,64);
            len += 64;
            *(shell_buff + len) =  calc_xor_verify(shell_buff,len);
            len += 1;
						//rt_kprintf("-- the data len  %d\r\n",len);
            send_data_to_shell_dev(shell_buff,149);
						rt_thread_delay(10);
            break;
    }
    //printf("-- the crc sum 8  0x%X\r\n",tmp);
    return 0;
}


/*********************************
**	SHELL澶勭悊绾跨▼
**********************************/

void thread_entry_shell(void *parameter)
{
	parameter = parameter;
	
	 rt_sem_init(&shell_info.sign, "shell_info", 0, RT_IPC_FLAG_FIFO); //
	shell_info.ticks = 0xffffffff;
	shell_dev = rt_device_find(RT_CONSOLE_DEVICE_NAME);        //
  rt_device_set_rx_indicate(shell_dev, shell_data_rx_call_fun); //
	
	rt_irq_timer3_sethook(shell_tick_handle);                     //
	rt_device_read(shell_dev, 0, shell_buff, sizeof(shell_buff));           //
	
	
	
	for(;;)
	{
		if(rt_sem_take(&shell_info.sign, 100) != RT_EOK)
    {
      continue;
    }

		rt_memset(shell_buff, 0,sizeof(shell_buff));
    shell_info.len = rt_device_read(shell_dev, 0, shell_buff,sizeof(shell_buff));          //
		
		if(shell_info.len > SHELL_BUFF_LEN)           																				//
			shell_info.len = SHELL_BUFF_LEN;
		
		if(acl16_verify(shell_buff,shell_info.len) > 0)
		{
			shell_cmd_handler((const char *)shell_buff,shell_info.len);
		}
	}
}




