

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


#include "pro_data.h"
#include "md5_code.h"


#include "app_can_recv.h"
#include "app_iap.h"

#include "app_fifo.h"
#include "app_gb4.h"
#include "app_packet.h"
#include "app_lte.h"
#include "app_at.h"

#define BUFFSIZE            4224

#define READ_IAP_SIZE				4096


const uint8_t secret_key[16] = {0x12,0x5B,0xA9,0xF1,0xCF,0xE9,0x78,0x9A,0x15,0x67,0xD7,0x7C,0x12,0xAC,0xEF,0x2D};


/***************本地全局变量********************/

static uint8_t                  local_data[BUFFSIZE] = {0};   //数据缓冲区

static uint32_t                 file_total_size = 0;     //文件总长度

static uint32_t 								file_crc_value = 0;      //文件CRC校验值

static uint32_t                 read_offset = 0;         //偏移量

static  uint32_t                file_size = 0;           //要升级的文件大小

static uint8_t                  iap_num = 0;             //链接FPT的次数

static uint8_t                  iap_state = 0;           //IAP线程状态0:未升级；1：升级中
	
static struct iap_ftp_str       iap_info = {0};      		//临时先使用全局变量


/*************************
**
**************************/

uint8_t read_iap_state(void)
{
	uint8_t rv;
	
	rv = iap_state;
	
	return rv;
}



/*************************
**
**************************/

uint32_t read_vector_table_flag(void)
{
	rt_device_t inner_mem_dev = RT_NULL;
	uint32_t rv = 0;
	rt_uint32_t mem_args = ADDR_SECTOR1;
	
	inner_mem_dev = rt_device_find("inner_mem");
	rt_device_open(inner_mem_dev,RT_DEVICE_OFLAG_RDWR);

	rt_device_read(inner_mem_dev,mem_args,(uint8_t *)&rv,4);
	rt_device_close(inner_mem_dev);
	
  return rv;
}





/*************************
**
**************************/

uint8_t write_vector_table_flag(uint32_t flag)
{
	rt_device_t inner_mem_dev = RT_NULL;
	
	rt_uint32_t mem_args = ADDR_SECTOR1;
	
	inner_mem_dev = rt_device_find("inner_mem");
	rt_device_open(inner_mem_dev,RT_DEVICE_OFLAG_RDWR);

	rt_device_write(inner_mem_dev,mem_args,(uint8_t *)&flag,4);
	rt_device_close(inner_mem_dev);
	
  return 0;
}




/***********************************
**	写入下载下来的应用程序到 单片机闪存备份区域
************************************/
uint8_t write_ftp_data_to_back(uint8_t *data,uint32_t addr,uint16_t len)
{
  rt_device_t inner_mem_dev = RT_NULL;
	
	inner_mem_dev = rt_device_find("inner_mem");
	rt_device_open(inner_mem_dev,RT_DEVICE_OFLAG_RDWR);
	rt_uint32_t mem_args;
	
	mem_args = ADDR_APP_BACK + addr;

	rt_device_write(inner_mem_dev,mem_args,(uint8_t *)data,len);
	rt_device_close(inner_mem_dev);
	
  return 0;
}


/*************************************
** 写升级标志位
***************************************/

static uint8_t write_iap_upgrade_info(uint32_t flag)
{
  rt_device_t 								bsram_dev = RT_NULL;       //
	
	bsram_dev = rt_device_find("back_sram");
	rt_device_open(bsram_dev, RT_DEVICE_OFLAG_RDWR);
	
	rt_device_write(bsram_dev,4,(uint8_t *)&flag,4);
	
	rt_device_close(bsram_dev);
	
  return 1;
}



/*****************************
**	擦除备份应用程序区域
******************************/

uint8_t erase_back_app_setcors(void)
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

uint8_t down_and_wirte_flash_ota(uint8_t *file_name)
{
	 uint32_t 	read_size = 0;
   uint32_t 	data_read = 0;
   uint8_t 		err;

   while(file_size > 0)
   {
			if(file_size > READ_IAP_SIZE)
      {
				read_size = READ_IAP_SIZE;
      }
      else
      {
				read_size = file_size;
      }
			
			data_read = at_download_ftp_files(file_name, (uint8_t *)local_data, read_offset, read_size);
			if(data_read > 0)
      {
				//printf("-- recv ftp file data len ... %d\r\n",data_read);
        err = write_ftp_data_to_back(local_data,read_offset, data_read);
        if(err != 0)
        {
					rt_kprintf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
          return 2;
        }
        file_size -= data_read;
        read_offset += data_read;
				
				read_size = (read_offset * 1000.0) / file_total_size;
				
        rt_kprintf("-- Download ota file from ftp... %d.%d%, %u,%u,%u,%u\r\n",(read_size / 10),(read_size % 10), file_size,read_offset,file_total_size,data_read);
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

uint8_t end_ftp_iap(void)
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






/***********************************
**
************************************/

uint8_t end_ftp_iap_md5(void)
{
	MD5_CTX 			md5;
	uint8_t 			array[32] = {0};
	uint8_t 			*start = NULL;

	uint32_t 			read_size = 0;
	
	rt_device_t 	inner_mem_dev = RT_NULL;
	
	rt_kprintf("-- Start ota data verfy ... %d\r\n",file_total_size);		//开始校验
	
	MD5Init(&md5);
	
	inner_mem_dev = rt_device_find("inner_mem");
	rt_device_open(inner_mem_dev,RT_DEVICE_OFLAG_RDWR);
	
	//file_total_size = 1;
	
	file_size = file_total_size - 16;
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
		
		MD5Update(&md5,local_data,read_size);
		read_offset += read_size;
		file_size -= read_size;
	}
	
	MD5Update(&md5, (uint8_t *)secret_key,sizeof(secret_key));
	MD5Final(array,&md5);
	
	rt_kprintf("-- The download updata.bin file md5 value:  ");
	for (int i = 0; i < 16; i++)
	{
		rt_kprintf("%02x", array[i]);
	}
	rt_kprintf("\r\n");
	
	start = (uint8_t *)(ADDR_APP_BACK + file_total_size - 16);
	memcpy(local_data,start,16);   //读取到
	rt_kprintf("-- The download OTA file md5 value:  ");
	for (int i = 0; i < 16; i++)
	{
		rt_kprintf("%02x", local_data[i]);
	}
	rt_kprintf("\r\n");
	
	if(str_compare(local_data,array,16) != 1)
	{
		rt_kprintf("-- The OTA Data Verfy Fail.....\r\n");
		write_iap_upgrade_info('R');
		return 1;
	}
	
	write_iap_upgrade_info('Y');
	
	return 0;
}



/*********************************
**	固件升级任务
**********************************/

void thread_entry_iap(void *parameter)
{	
	uint8_t 								step = 0;
  struct ftp_info_str     sa = {0};
	rt_thread_t 						tmp_tid = NULL;
	uint8_t                 res = 0;
	
	if(parameter != NULL)
  {
		iap_info = *(struct iap_ftp_str *)parameter;
    rt_kprintf("\r\n-- IAP FtpUserName:%s\r\n",iap_info.user);
		rt_kprintf("-- IAP FtpUserPassd:%s\r\n",iap_info.passwd);
		rt_kprintf("-- IAP Server Addr:%s\r\n",iap_info.host);
		rt_kprintf("-- IAP Server Port:%d\r\n",iap_info.port);
		rt_kprintf("-- IAP File Name:%s\r\n",iap_info.files);	
  }
	
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
				
				rt_kprintf("-- del thread_entry_iap 2.....\r\n");
				rt_thread_delete(tmp_tid);    //删除任务自身
				rt_thread_yield();
				file_total_size = 0;     //文件总长度
				file_crc_value = 0;      //文件CRC校验值
				read_offset = 0;         //偏移量
				file_size = 0;           //要升级的文件大小
				iap_num = 0;             //链接FPT的次数
				iap_state = 0;           //IAP线程状态0:未升级；1：升级中
			}
			continue;
    }

		switch(step)
		{
				case 0:
					memcpy(sa.host,iap_info.host,sizeof(iap_info.host));   //主机地址
          sa.port = iap_info.port;
          memcpy(sa.user,iap_info.user,sizeof(iap_info.user));
          memcpy(sa.passwd,iap_info.passwd,sizeof(iap_info.passwd));
                
          if(at_config_ftp_account(&sa) == 0)  //配置FTP服务器用户名和密码,可以多次配置
          {
						 rt_kprintf("-- config ftp account ok......\r\n");
          }
          else
          {
             step = 5;
          }

          if(at_config_ftp_transmode(1) == 0)    //配置传输模式
          {
              rt_kprintf("-- config ftp transmode ok......\r\n");
          }
          else
          {
              step = 5;
          }

          if(at_config_ftp_rsptimeout(20) == 0)   //
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
               printf("-- Creat ftp connect ok......(1)\r\n");
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
          if(file_total_size <= 0 && file_total_size >= (224 * 1024))
          {
            rt_kprintf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
            step = 5;
            break;
          }
          file_size = file_total_size - read_offset;    //   (???)
          step++;
					rt_kprintf("-- Start Down and write flash ota... %d\r\n",file_size);
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
						 {
							 rt_kprintf("-- close ftp connect......ok\r\n");
						 }
             else
						 {
							 rt_kprintf("-- close ftp connect......fail\r\n");
						 }
             
						 step = 2;   //重新链接
						 rt_thread_delay(100);
             break; 
           }
           else
           {
              step = 5;   //升级失败
           }
					 break;
				case 4:            //结束升级
					if(end_ftp_iap_md5() == 0)
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
          step = 0;
					break;
				case 5:          //升级失败
					write_iap_upgrade_info('R');
					rt_reboot_sys();
					break;
				default:
					break;
			}
	}	
}


/******************** 一下为BOOT IAP升级 *******************/


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
	
	mem_args = ADDR_SECTOR1;
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
	rt_device_close(inner_mem_dev);
	
	return 0;
}





/*****************************
**	擦除备份应用程序区域
******************************/

uint8_t erase_app_setcors_boot(void)
{
	rt_err_t res = RT_EOK;
	
	rt_device_t inner_mem_dev = RT_NULL;
	
	inner_mem_dev = rt_device_find("inner_mem");
	
	rt_device_open(inner_mem_dev,RT_DEVICE_OFLAG_RDWR);
	rt_uint32_t mem_args;
	
	mem_args = ADDR_SECTOR0;
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
	
	rt_device_close(inner_mem_dev);
	
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
	
	rt_kprintf("-- Start boot ota data verfy ... %d\r\n",file_total_size);		//开始校验
	
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
		rt_device_read(inner_mem_dev,ADDR_SECTOR1 + read_offset,(uint8_t *)local_data,read_size);
		
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
	
	rt_kprintf("-- The boot ota data verfy value:0x%X,0x%X\r\n",CRC16,file_crc_value);
  
	if(file_crc_value != CRC16)
	{
		rt_kprintf("-- The boot OTA Data Verfy Fail.....\r\n");
		
		return 1;
	}
	
	if(erase_app_setcors_boot() == 0)
		rt_kprintf("-- Erase Boot App Setcors..... Ok \r\n");
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
		
		if(rt_device_read(inner_mem_dev,ADDR_SECTOR1 + read_offset,(uint8_t *)local_data,read_size) == read_size)
		{
			rt_kprintf("-- Read Boot OTA Data: %d,%d\r\n",read_size,read_offset);
		}
		
		if(rt_device_write(inner_mem_dev,ADDR_SECTOR0 + read_offset,(uint8_t *)local_data,read_size) == read_size)
		{
			rt_kprintf("-- Write Boot OTA Data: %d,%d\r\n",read_size,read_offset);
		}
		
		read_offset += read_size;
		file_size -= read_size;
	}
	
	
	
	return 0;
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
	
	mem_args = ADDR_SECTOR1 + addr;

	rt_device_write(inner_mem_dev,mem_args,(uint8_t *)data,len);
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
        err = write_ftp_data_to_back_boot(local_data,read_offset, data_read);
				//esp_ota_write(update_handle, (const void *), data_read);
        if(err != 0)
        {
					rt_kprintf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
          return 2;
        }
        file_size -= data_read;
        read_offset += data_read;

        rt_kprintf("-- Download boot ota file from ftp... %u,%u,%u,%u\r\n",file_size,read_offset,file_total_size,data_read);
      }
      else
      {
				return 1;
      }
    }

    return 0;
}




/*********************************
**	固件升级任务
**********************************/

void thread_entry_boot_iap(void *parameter)
{	
	uint8_t 								step = 0;
	struct iap_ftp_str      iap_info = {0};
  struct ftp_info_str     sa = {0};
	rt_thread_t 						tmp_tid = NULL;
	uint8_t                 res = 0;
	
	if(parameter != NULL)
  {
		iap_info = *(struct iap_ftp_str *)parameter;
    rt_kprintf("\r\n-- IAP Boot FtpUserName:%s\r\n",iap_info.user);
		rt_kprintf("-- IAP Boot FtpUserPassd:%s\r\n",iap_info.passwd);
		rt_kprintf("-- IAP Boot Server Addr:%s\r\n",iap_info.host);
		rt_kprintf("-- IAP Boot Server Port:%d\r\n",iap_info.port);
		rt_kprintf("-- IAP Boot File Name:%s\r\n",iap_info.files);	
  }
	
	if(get_file_verify((char *)&iap_info.files,&file_crc_value) > 0)
	{
		rt_kprintf("-- Get Boot IAP File Crc16 Value Fail....");
		tmp_tid = rt_thread_self();   //
		if(tmp_tid != NULL)
		{
			rt_kprintf("-- del arch thread 1.....\r\n");
			rt_thread_delete(tmp_tid);    //删除任务自身
			rt_thread_yield();
		}
	}
	
	rt_kprintf("-- Boot IAP File Crc16 Value:0x%X\r\n",file_crc_value);
	file_total_size = 0;
  read_offset = 0;
  file_size = 0;
	iap_state = 1;
	
  memset((uint8_t *)&sa,'\0',sizeof(sa));
	for(;;)
	{
		rt_kprintf("-- thread_entry_boot_iap runing....\r\n");
		if(read_lte_net_init_state() == 0)
    {
			rt_thread_delay(100);
			
			tmp_tid = rt_thread_self();   //
			if(tmp_tid != NULL)
			{
				rt_kprintf("-- del boot iap thread 2.....\r\n");
				rt_thread_delete(tmp_tid);    //删除任务自身
				rt_thread_yield();
				file_total_size = 0;     //文件总长度
				file_crc_value = 0;      //文件CRC校验值
				read_offset = 0;         //偏移量
				file_size = 0;           //要升级的文件大小
				iap_num = 0;             //链接FPT的次数
				iap_state = 0;           //IAP线程状态0:未升级；1：升级中
			}
			
			continue;
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

          if(at_config_ftp_rsptimeout(20) == 0)
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
					if(erase_back_app_setcors_boot() == 0)
          {
						rt_kprintf("-- start ftp boot iap ok.....\r\n");
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
               printf("-- Creat ftp connect ok......(2)\r\n");
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
          file_size = file_total_size - read_offset;    //   (???)
          step++;
					rt_kprintf("-- Start Down and write flash ota boot...\r\n");
          break;		
				case 3:
					 res = down_and_wirte_flash_ota_boot(iap_info.files);
           if(res == 0)
           {
               rt_kprintf("-- down load boot iap files ok.....\r\n");
               step++;
           }
           else if(res == 1)   //下载过程中断开，可以重新链接
           {
						 rt_kprintf("-- ftp down boot disconnect......\r\n");
             if(at_close_ftp_connect() == 0)
							 rt_kprintf("-- close ftp boot connect......ok\r\n");
                     
             step = 2;   //重新链接
             break; 
           }
           else
           {
              step = 5;   //升级失败
           }
					 break;
				case 4:            //结束升级
					if(end_ftp_iap_boot() == 0)
					{
						rt_kprintf("-- The End Boot Ftp Iap OK....\r\n");
					}
					else
					{
						rt_kprintf("-- The End Boot Ftp Iap Fail...\r\n");
					}
          if(at_close_ftp_connect() == 0)
             printf("-- close ftp connect......ok\r\n");
          printf("-- Reset boot from fpt iap ...\r\n");
					
					rt_reboot_sys();
          step = 0;
					break;
				case 5:    //升级失败
					break;
				default:
					break;
			}
	}	
}




