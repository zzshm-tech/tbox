





/**********************************
**  动态国家平台备案
**  
***********************************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>



#include "pro_data.h"
#include "common.h"


#include "app_lte.h"
#include "app_products.h"
#include "app_shell.h"
#include "app_archive.h"
#include "app_gb4.h"
#include "app_ver.h"
#include "app_at.h"




#define ARCHIVE_SOCKET_ID                      2   /**** 备案Socket序号 ****/ 



static struct socket_down_str                  archive_down_data = {0};			//

struct archive_plat_str                    	   archive_plat = {0};

static rt_mq_t                           			 archive_down_qh = NULL; 			//

static uint8_t                                 local_buff[1256] = {0};            //



/**********************************************************
**  解析国家备案平台下行数据
***********************************************************/

uint16_t archive_platform_down_parse(uint8_t *data, uint16_t len,uint8_t *buf,uint16_t size)
{
	struct start_str    		    *p_start;
	uint8_t 				        		*index_p = NULL;
	uint8_t     	              array[100];

	//rt_kprintf("\r\n-- Archive Platform  receive:");
  //mem_printf(LOG_ERROR, PRINT_HEX, data, len);

  if(data == NULL || len == 0)
	{
		return 0;
	}
        

	if(len < sizeof(struct start_str))
		return 0;	
   	
	p_start = (struct start_str *)data;
	
	if((p_start->head[0] != 0x23) && (p_start->head[1] != 0x23))
	{
		rt_kprintf("-- Header error.......\r\n");
		return 0;
	}
	
	
	if((*(data + len - 1)) != (calc_xor_verify(data + 2,len - 3)))
	{
		rt_kprintf("-- Check the error.......\r\n");
		return 0;
	}
	
	memset(array,0,sizeof(array));
	
	switch(p_start->cmd)
	{
		case 0x08:            //备案应答
		  index_p = data + sizeof(struct start_str);
		  if(1 == *index_p)
			{
				printf("-- Recode success... \r\n");
				archive_plat.state = 1;	
			}
			else
			{
				index_p++;
				if(*index_p == 1)
				{
					archive_plat.state = 2;   //备案失败-芯片已经备案
				}
				else if(*index_p == 2)
				{
					archive_plat.state = 3;   //备案失败-芯片ID错误
				}
				else if(*index_p == 3)
				{
					archive_plat.state = 4;   //备案失败-芯片ID错误
				}
				else
				{
					archive_plat.state = 5;   //备案失败-验签失败，
				}
				rt_kprintf("-- Recode fail... %d\r\n",*index_p);
				//archive_plat.state = 0;
			}
			
			rt_kprintf("-- Archive status :%d,%d\r\n",archive_plat.state,*index_p);
			archive_plat.res = 1;           //收到国家平台的应答
			//printf("-- the runing is.....%d\r\n",archive_plat.res);
			
			break;
		default:
      break;
    }

    return 0;
}




/********************************************
**	动态备案线程
*********************************************/

uint16_t build_vehicle_archival_info(uint8_t *buf,uint16_t size,uint8_t type)
{
	struct emissions_activate_t			*p_emission = NULL;
	struct position_activate_t 			*p_position = NULL;
	struct start_str            		*p_start = NULL;
	uint16_t 												len =  0;
	uint8_t                     		array[256] = {0};


  len = sizeof(struct start_str);
	
	if(type == 0)
  {
		p_emission = (struct emissions_activate_t *)array;
		memset((uint8_t *)p_emission,0,sizeof(struct emissions_activate_t));
		build_rtc_time(p_emission->time,6);                         //数据采集时间 
		if(64 == acl_write_then_read(CRY_GENKEY,NULL,p_emission->public_key,0))   //备案的时候获取一次公钥
		{
			rt_mq_t 									t_mq = NULL;
			struct products_data_t 		pd = {0};	
			pd.cmd = 3;
			pd.len = 64;
			pd.data[0] = 29;
			memset(&pd.data[1],'\0',64);			
			memcpy(&pd.data[1],p_emission->public_key,64);   //拷贝机械环保代码
			t_mq = get_products_mq();
			if(t_mq != NULL)
				rt_mq_send(t_mq,&pd,sizeof(struct products_data_t));
			//mem_printf(LOG_ERROR,PRINT_HEX,p_emission->public_key,64);
		}
		else
		{
			printf("-- Get Public Key Fail......\r\n");
			return 2;
		}

    read_encryption_chip_id(p_emission->chip_id,16);    //芯片ID
		if(read_config_vin_state() == 0x5A)
		{
			read_config_vin_info(p_emission->vehicle_vin,17);
		}
		else
		{
			return 1;
		}
		
		memcpy(buf + len,array,sizeof(struct emissions_activate_t));
    len += sizeof(struct emissions_activate_t);
		read_encryption_chip_id(array,16);
		memcpy(buf + len,array,16);
		if(64 != acl_write_then_read(CRY_SIGN,buf + sizeof(struct start_str),array,sizeof(struct emissions_activate_t) + 16))
		{
			rt_kprintf("-- CRY_SIGN is error...\r\n");
      return 1;
		}
		*(buf + len++) = 0x20;
		memcpy(buf + len,(array + 0),32);
		len += 32;
		*(buf + len++) = 0x20;
		memcpy(buf + len,(array + 32),32);
		len += 32;
	}
	else
	{
		p_position = (struct position_activate_t *)array;
		if(read_config_vin_state() == 0x5A)
		{
			read_config_vin_info(p_position->vehicle_vin,17);
		}
		else
		{
			return 1;
		}
		memcpy(buf + len,array,sizeof(struct position_activate_t));
    len += sizeof(struct position_activate_t);
	}

	p_start = (struct start_str *)array;
	p_start->head[0] = 0x23;
	p_start->head[1] = 0x23;
	p_start->cmd = 0x07;
	memset(p_start->vin,0,17);

  read_config_vin_info(p_start->vin,17);
	p_start->soft_ver = (uint8_t)read_user_ver();
	p_start->encrypt = SM2;
  p_start->len = swap_uint16_t(len - sizeof(struct start_str));
  memcpy(buf, array, sizeof(struct start_str));

  *(buf + len) = calc_xor_verify(buf + 2, len - 2);
	len++;
	
	return len;
}



/*************************
**	备案任务
**************************/

void thread_entry_archivel(void *parameter)
{
  struct socket_addr_str      	tmp;
  uint16_t                    	send_len = 0;
	rt_thread_t 									tmp_tid = NULL;
  
	parameter = parameter;

	archive_down_qh = rt_mq_create("uart_cmd_mq",sizeof(struct socket_down_str),2,RT_IPC_FLAG_FIFO);  
	rt_thread_delay(100);
	memset((uint8_t *)&archive_plat,0,sizeof(archive_plat));
	archive_plat.res = 0;
	
	for(;;)
	{
		if(rt_mq_recv(archive_down_qh,&archive_down_data,sizeof(archive_down_data),100) == RT_EOK)
    {
			if(strstr((char *)archive_down_data.data,"+QIURC:") != NULL)
			{
				if(at_close_socket_connect(ARCHIVE_SOCKET_ID) == 0)
				{
					rt_kprintf("-- close archive socket ok.......\r\n");
				}
				
				archive_plat.state = 10;
				archive_plat.step = 3;
				continue;
			}
				
      archive_platform_down_parse(archive_down_data.data,archive_down_data.len,local_buff,sizeof(local_buff));
    }

		//printf("-- thread entry archivel...%d\r\n",archive_plat.step);
		switch(archive_plat.step)
		{
			case 0:   //创建Socket  链接国家平台  (备案网关地址)
				if(at_query_socket_state(ARCHIVE_SOCKET_ID) > 0)
				{
					if(at_close_socket_connect(ARCHIVE_SOCKET_ID) == 0)
					{
						rt_kprintf("-- close socket archive plat connect....\r\n");
					}
				}

        read_config_archivel_gw_addr(tmp.addr,sizeof(tmp.addr));
        tmp.port = read_config_archivel_gw_port();

        if(at_creat_socket_connect(ARCHIVE_SOCKET_ID,&tmp,archive_down_qh) == 0)
        {
						rt_kprintf("-- Creat Archive socket onnect:%s:%d.....OK\r\n",tmp.addr,tmp.port);  
            send_len = build_vehicle_archival_info(local_buff,sizeof(local_buff),0);
						if(send_len > 2)   //组包备案信息
						{
							//mem_printf(LOG_ERROR, PRINT_HEX,local_buff, send_len);
							if(at_send_socket_data(ARCHIVE_SOCKET_ID,local_buff,send_len) == 0)
							//if(at_send_socket_data(1,local_buff,send_len) == 0)
							{
								archive_plat.step++;
								printf("-- send archival info OK .....\r\n");
							}
							else
							{
								archive_plat.step = 2;
								archive_plat.state = 7;  //归类为链接国家平台（备案平台）失败
								rt_kprintf("-- send archival info Fail .....\r\n");
							}	
						}
						else if(send_len == 1)
						{
							archive_plat.step = 2;
							archive_plat.state = 6;    //没有机械环保代码  
							rt_kprintf("-- build archival info Fail .....\r\n");
						}
						else
						{
							archive_plat.step = 2;
							archive_plat.state = 9;    //加密芯片错误 
							rt_kprintf("-- build archival info Fail .....\r\n");
						}
          }   
					else        //链接国家平台失败
					{
						archive_plat.step = 2;    //删除该任务
						archive_plat.state = 7; 
						rt_kprintf("-- Creat Archive socket onnect:%s:%d.....Fail\r\n",tmp.addr,tmp.port);
					}      
            
				break;
			case 1:                     //等待备案结果 
				rt_kprintf("-- wait for archival res....%d,%d\r\n",archive_plat.res,archive_plat.count);
				if(archive_plat.count++ < 30 && archive_plat.res == 0)
					break;
				if(archive_plat.res == 0)
				{
					archive_plat.state = 8;//  备案平台无应答，备案超时。
				}
					
				archive_plat.step++;
				break;
			case 2:                 //关闭Socket  
				{
					rt_mq_t 								t_mq = NULL;
					struct products_data_t 		pd = {0};
					
					pd.cmd = 3;
					pd.len = 1;
					pd.data[0] = 31;
					pd.data[1] = archive_plat.state;
				
					t_mq = get_products_mq();
					if(t_mq != NULL)
						rt_mq_send(t_mq,&pd,sizeof(struct products_data_t));
					
					if(at_close_socket_connect(ARCHIVE_SOCKET_ID) == 0)            
						rt_kprintf("-- close archival socket ok....\r\n");
					if(archive_down_qh != NULL)
					{
						if(rt_mq_delete(archive_down_qh) == RT_EOK)
							rt_kprintf("-- Del archive queue OK ... \r\n");
						archive_down_qh = NULL;
					}
					
					rt_thread_delay(10);
					tmp_tid = rt_thread_self();   //删除任务自身
					if(tmp_tid != NULL)
					{
						rt_kprintf("-- del arch thread.....\r\n");
						rt_thread_delete(tmp_tid);
						rt_thread_yield();
					}
				}
				break;
			default:
				archive_plat.step = 2;
				break;
		}	
	}
}


