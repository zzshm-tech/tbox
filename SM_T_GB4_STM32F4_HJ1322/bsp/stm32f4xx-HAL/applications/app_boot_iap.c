





/************************************
**
*************************************/


#include <stdio.h>
#include <string.h>
#include <stdint.h>


#include "pro_data.h"

#include <rtthread.h>


#include "board.h"
#include "drv_mem.h"
#include "drv_can.h"


#include "app_can.h"
#include "app_iap.h"
#include "app_ddp.h"
#include "app_fifo.h"
#include "app_acl16.h"
#include "app_rtc.h"
#include "app_lte.h"

#define BUFFSIZE            2176


/***************本地全局变量********************/

static uint8_t                  local_data[BUFFSIZE] = {0};   //数据缓冲区

static uint32_t                 file_total_size = 0;     //文件总长度

static uint32_t 								file_crc_value = 0;      //文件CRC校验值

static uint32_t                 read_offset = 0;         //偏移量

static  uint32_t                file_size = 0;           //要升级的文件大小

static uint8_t                  iap_num = 0;             //链接FPT的次数

static uint8_t                  iap_state = 0;           //IAP线程状态0:未升级；1：升级中
	
static rt_mq_t 									iap_mq	= RT_NULL;       //CAN锁车队列





/********************************
**
**********************************/

rt_mq_t get_iap_mq(void)
{
	return iap_mq;
}



/*************************
**
**************************/

uint8_t read_iap_boot_state(void)
{
	uint8_t rv;
	
	rv = iap_state;
	
	return rv;
}


/***********************************
**	写入下载下来的应用程序到 单片机闪存备份区域
************************************/
uint8_t write_ftp_data_to_back_boot(uint8_t *data,uint32_t addr,uint16_t len)
{
  rt_device_t inner_mem_dev = RT_NULL;
	
	inner_mem_dev = rt_device_find("inner_mem");
	rt_device_open(inner_mem_dev,RT_DEVICE_OFLAG_RDWR);
	rt_uint32_t mem_args;
	
	mem_args = ADDR_APP_BACK + addr;

	rt_device_write(inner_mem_dev,mem_args,(rt_uint8_t *)data,len);
	rt_device_close(inner_mem_dev);
	
  return 0;
}



/*****************************
**	擦除备份应用程序区域
******************************/

uint8_t erase_back_app_setcors_boot(void)
{
	rt_err_t res = RT_EOK;
	
	rt_device_t inner_mem_dev = RT_NULL;
	
	inner_mem_dev = rt_device_find("inner_mem");
	
	rt_device_open(inner_mem_dev,RT_DEVICE_OFLAG_RDWR);
	rt_uint32_t mem_args;
	
	mem_args = ADDR_SECTOR6;
	res = rt_device_control(inner_mem_dev,RT_DEVICE_CTRL_INNER_MEM_ERASE, &mem_args); 
	if(res == RT_EOK)
	{
		rt_kprintf("-- Erase app back setcors :%x\r\n",mem_args);
	}
	else
	{
		return 1;
	}
	
	rt_thread_delay(10);
	
	mem_args = ADDR_SECTOR7;
	res = rt_device_control(inner_mem_dev,RT_DEVICE_CTRL_INNER_MEM_ERASE, &mem_args); 
	if(res == RT_EOK)
	{
		rt_kprintf("-- Erase app back setcors :%x\r\n",mem_args);
	}
	else
	{
		return 1;
	}
	
	rt_device_close(inner_mem_dev);
	
	return 0;
}


/**************************
**	下载数据并写入对应的闪存地址
***************************/

uint8_t down_and_wirte_flash_ota_boot(uint8_t *file_name)
{
	 uint32_t 	read_size = 0;
   uint32_t 	data_read = 0;
   uint8_t 		err;

   while(file_size > 0)
   {
			if(file_size > 2048)
      {
				read_size = 2048;
      }
      else
      {
				read_size = file_size;
      }
			
			data_read = at_download_ftp_files(file_name, (uint8_t *)local_data, read_offset, read_size);
			if(data_read > 0)
      {
				//printf("-- recv ftp file data len ... %d,%u\r\n",data_read,update_handle);
        err = write_ftp_data_to_back(local_data,read_offset, data_read);
				//esp_ota_write(update_handle, (const void *), data_read);
        if(err != 0)
        {
					rt_kprintf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
          return 2;
        }
        file_size -= data_read;
        read_offset += data_read;

        rt_kprintf("-- Download ota file from ftp... %u,%u,%u,%u\r\n",file_size,read_offset,file_total_size,data_read);
      }
      else
      {
				return 1;
      }
    }

    return 0;
}




/***********************************
**
************************************/

uint8_t end_ftp_iap_boot(void)
{
	uint32_t 			size;
	uint32_t	 		i;
	uint8_t	 			LSB;				
	uint16_t 			CRC16 = 0xFFFF;
	uint8_t 			*start;
	uint32_t 			read_size = 0;
	
	rt_device_t 	inner_mem_dev = RT_NULL;
	
	rt_kprintf("-- Start ota data verfy ... %d\r\n",file_total_size);		//开始校验
	
	inner_mem_dev = rt_device_find("inner_mem");
	rt_device_open(inner_mem_dev,RT_DEVICE_OFLAG_RDWR);
	
	file_size = file_total_size;
	read_offset = 0;
	
	while(file_size > 0)
  {
		if(file_size > 2048)
    {
				read_size = 2048;
		}
    else
    {
				read_size = file_size;
    }
		rt_device_read(inner_mem_dev,ADDR_APP_BACK + read_offset,(uint8_t *)local_data,read_size);
		
		start = local_data;
		size = read_size;
		
		while(size > 0) 
		{
			*(uint8_t *)&CRC16 ^= *start;
			for (i = 0; i < 8; i++) 
			{
				LSB = (unsigned char)CRC16 & 0x01 ? 1 : 0;
				CRC16 >>= 1;
				if(LSB) 
					CRC16 ^= 0xA001;
			}
			start++;
			size--;
		}
		read_offset += read_size;
		file_size -= read_size;
	}
	
	rt_kprintf("-- The ota data verfy value:0x%X,0x%X\r\n",CRC16,file_crc_value);
  
	if(file_crc_value != CRC16)
	{
		rt_kprintf("-- The OTA Data Verfy Fail.....\r\n");
		write_iap_upgrade_info('R');
		return 1;
	}
	
	write_iap_upgrade_info('Y');
	
	return 0;
}




/*********************************
**	单片机Boot  升级线程
**********************************/

void thread_entry_boot_iap(void *parameter)
{	
	uint8_t 								step = 0;
	struct iap_ftp_str      iap_info = {0};
  struct ftp_info_str     sa = {0};
	rt_thread_t 						tmp_tid = NULL;
	uint8_t                 res = 0;
	
	
	memcpy(iap_info.user,"ftadmin",sizeof("ftadmin"));
	memcpy(iap_info.passwd,"ftadmin81645",sizeof("ftadmin81645"));
	memcpy(iap_info.host,"223.223.187.35",sizeof("223.223.187.35"));
	iap_info.port = 1050;
	memcpy(iap_info.files,"BT_5397.bin",sizeof("BT_5397.bin"));
	
		
    
	rt_kprintf("\r\n-- IAP FtpUserName:%s\r\n",iap_info.user);
	rt_kprintf("-- IAP FtpUserPassd:%s\r\n",iap_info.passwd);
	rt_kprintf("-- IAP Server Addr:%s\r\n",iap_info.host);
	rt_kprintf("-- IAP Server Port:%d\r\n",iap_info.port);
	rt_kprintf("-- IAP File Name:%s\r\n",iap_info.files);	
  
	
	if(get_file_verify((char *)&iap_info.files,&file_crc_value) > 0)
	{
		rt_kprintf("-- Get IAP File Crc16 Value Fail....");
		tmp_tid = rt_thread_self();   //
		if(tmp_tid != NULL)
		{
			rt_kprintf("-- del arch thread 1.....\r\n");
			rt_thread_delete(tmp_tid);    //删除任务自身
			rt_thread_yield();
		}
	}
	
	rt_kprintf("-- IAP File Crc16 Value:0x%X\r\n",file_crc_value);
	file_total_size = 0;
  read_offset = 0;
  file_size = 0;
	iap_state = 1;
	
  memset((uint8_t *)&sa,'\0',sizeof(sa));
	for(;;)
	{
		if(read_lte_net_init_state() == 0)
    {
			rt_thread_delay(100);
			
			tmp_tid = rt_thread_self();   //
			if(tmp_tid != NULL)
			{
				rt_kprintf("-- del arch thread 2.....\r\n");
				rt_thread_delete(tmp_tid);    //删除任务自身
				rt_thread_yield();
			}
    }

		switch(step)
		{
				case 0:
					memcpy(sa.host,iap_info.host,sizeof(iap_info.host));   //主机地址
          sa.port = iap_info.port;
          memcpy(sa.user,iap_info.user,sizeof(iap_info.user));
          memcpy(sa.passwd,iap_info.passwd,sizeof(iap_info.passwd));
                
          if(at_config_ftp_account(&sa) == 0)
          {
						 rt_kprintf("-- config ftp account ok......\r\n");
          }
          else
          {
             step = 5;
          }

          if(at_config_ftp_transmode(1) == 0)
          {
              rt_kprintf("-- config ftp transmode ok......\r\n");
          }
          else
          {
              step = 5;
          }

          if(at_config_ftp_rsptimeout(90) == 0)
          {
              rt_kprintf("-- config ftp rsptimeout ok......\r\n");
          }
          else
          {
              step = 5;
          }
                 
          if(at_config_ftp_file_type(0) == 0)
          {
              rt_kprintf("-- config ftp file_type ok......\r\n");
              step++;
          }
          else
          {
             step = 5;
          }
          break;                                 //直接重启
				
				case 1:
					if(erase_back_app_setcors() == 0)
          {
						rt_kprintf("-- start ftp iap ok.....\r\n");
            step++;
          }
          else
          {
						step = 5;
          }
          break;
				case 2:
					if(iap_num++ > 2)
          {
             step = 5;
             break;
          }
          if(at_creat_ftp_connect(&sa) == 0)
          {
               printf("-- Creat ftp connect ok......\r\n");
          }
          else
          {
               printf("-- Creat ftp connect Fail......\r\n");
               step = 5;
               break;
          }
					/*获取文件大小*/
          if((file_total_size = at_get_ftp_files_size(iap_info.files)) == 0)
          {
						rt_kprintf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
            step = 5;
                    
						break;
          }
          rt_kprintf("-- OTA file total size = %d\r\n", file_total_size);
          if(file_total_size <= 0)
          {
            rt_kprintf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
            step = 5;
            break;
          }
          file_size = file_total_size - read_offset;    //   (???)
          step++;
					rt_kprintf("-- Start Down and write flash ota...\r\n");
          break;		
				case 3:
					 res = down_and_wirte_flash_ota(iap_info.files);
           if(res == 0)
           {
               rt_kprintf("-- down load iap files ok.....\r\n");
               step++;
           }
           else if(res == 1)   //下载过程中断开，可以重新链接
           {
						 rt_kprintf("-- ftp down disconnect......\r\n");
             if(at_close_ftp_connect() == 0)
							 rt_kprintf("-- close ftp connect......ok\r\n");
                     
             step = 2;   //重新链接
             break; 
           }
           else
           {
              step = 5;   //升级失败
           }
					 break;
				case 4:            //结束升级
					if(end_ftp_iap() == 0)
					{
						rt_kprintf("-- The End Ftp Iap OK....\r\n");
					}
					else
					{
						rt_kprintf("-- The End Ftp Iap Fail...\r\n");
					}
          if(at_close_ftp_connect() == 0)
             printf("-- close ftp connect......ok\r\n");
          printf("-- Reset be from fpt iap ...\r\n");
					
					rt_reboot_sys();
//          a_mq.state = 1;

//          qu_t =  get_app_main_queue();
//          if(qu_t != NULL)
//          {
//                    xQueueSend(qu_t,&a_mq,sizeof(struct app_main_mq_str));
//          }
//          vTaskDelay(1000);
          step = 0;
					break;
				default:
					break;
			}
	}	
}
