


/**********************************
**  动态国家平台备案
**  
***********************************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "pro_data.h"
#include "common.h"

#include "drv_acl16.h"

#include "app_lte.h"
#include "app_products.h"
#include "app_at.h"
#include "app_archive.h"
#include "app_gb4.h"
#include "app_acl16.h"




#define ARCHIVE_SOCKET_ID                       3

static struct socket_down_str                  archive_down_data = {0};			//

struct archive_plat_str                    	   archive_plat = {0};

static QueueHandle_t                           archive_down_qh = NULL; 			//

static uint8_t                                 archive_buff[256] = {0};            //




/********************************
 * 标注该线程一启动
********************************/

uint8_t read_archive_en_state(void)
{
	uint8_t rv;

	rv = archive_plat.ententry_state;

	return rv;
}




/**********************************************************
**  解析国家备案平台下行数据
***********************************************************/

uint16_t archive_platform_down_parse(uint8_t *data, uint16_t len,uint8_t *buf,uint16_t size)
{
	struct start_str    		    *p_start;
	uint8_t 				        *index_p = NULL;
	uint8_t     	                array[100];

	//printf("\r\n-- Archive Platform  receive:");
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
		printf("-- Header error.......\r\n");
		return 0;
	}
	
	
	if((*(data + len - 1)) != (calc_xor_verify(data + 2,len - 3)))
	{
		printf("-- Check the error.......\r\n");
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
				printf("-- Recode fail... %d\r\n",*index_p);
				//archive_plat.state = 0;
			}
			archive_plat.res = 1;           //收到国家平台的应答
			//printf("-- the runing is.....%d\r\n",archive_plat.res);
			
			break;
        default:
            break;
    }

    return 0;
}




/********************************************
**	
*********************************************/

uint16_t build_vehicle_archival_info(uint8_t *buf,uint16_t size)
{
	struct archival_info_str 	*p_archival = NULL;
	struct start_str            *p_start = NULL;
	uint16_t 					len =  0;
	uint8_t                     array[256] = {0};


    len = sizeof(struct start_str);

    p_archival = (struct archival_info_str *)array;
    memset(p_archival,0,sizeof(struct archival_info_str));
	build_rtc_time(p_archival->time,6);                         //数据采集时间 
	

	//注意这里的问题
	if(64 == acl16_write_then_read(CRY_GENKEY,NULL,p_archival->public_key,0))   //备案的时候获取一次公钥
	{
		printf("-- Get public key ok.......\r\n");
		
	}
	else
	{
		printf("-- Get Public Key Fail......\r\n");
		return 2;
	}

    read_encryption_chip_id(p_archival->chip_id,16);    //芯片ID

	if(read_config_vin_state() == 0x5A)
	{
		read_config_vin_info(p_archival->vehicle_vin,17);
		//return 1;  //调试使用
	}
	else
	{
		return 1;
	}

    memcpy(buf + len,array,sizeof(struct archival_info_str));
    len += sizeof(struct archival_info_str);
	//签明改用芯片ID
    read_encryption_chip_id(array,16);
	memcpy(buf + len,array,16);
	
	if(64 != acl16_write_then_read(CRY_SIGN,buf + sizeof(struct start_str),array,sizeof(struct archival_info_str) + 16))
	{
		printf("-- CRY_SIGN is error...\r\n");
        return 1;
	}
	*(buf + len++) = 0x20;
	memcpy(buf + len,(array + 0),32);
	len += 32;
	*(buf + len++) = 0x20;
	memcpy(buf + len,(array + 32),32);
	len += 32;

	
	
    p_start = (struct start_str *)array;
	p_start->head[0] = 0x23;
	p_start->head[1] = 0x23;
	p_start->cmd = 0x07;
	memset(p_start->vin,0,17);

    read_config_vin_info(p_start->vin,17);
	p_start->soft_ver = SOFEWARE_VER;
	p_start->encrypt = SM2;
    p_start->len = swap_uint16_t(len - sizeof(struct start_str));
    memcpy(buf, array, sizeof(struct start_str));

    *(buf + len) = calc_xor_verify(buf + 2, len - 2);
	len++;

	printf("\r\n-- the archival info len:%d\r\n",len);
	
	return len;
}



/*************************
**	备案任务
**************************/

void thread_entry_archivel(void *parameter)
{
	uint8_t 					res = 0;
    struct socket_addr_str      tmp;
    uint16_t                    send_len = 0;

    parameter = parameter;

    archive_down_qh = xQueueCreate(2,sizeof(struct socket_down_str));
	
	vTaskDelay(100);
	memset((uint8_t *)&archive_plat,0,sizeof(archive_plat));
	archive_plat.res = 0;

	for(;;)
	{
		if(xQueueReceive(archive_down_qh,&archive_down_data,100) == pdTRUE)
        {
           	send_len = archive_platform_down_parse(archive_down_data.data,archive_down_data.len,archive_buff,sizeof(archive_buff));
            if(send_len > 0)
			{
				if(at_send_socket_data(ARCHIVE_SOCKET_ID,archive_buff,send_len) == 0)
                {
                   	printf("-- Send archive plat Down Cmd Res OK......%d\r\n",send_len);
                }
			}
        }

		archive_plat.ententry_state = 1;
		//printf("-- thread entry archivel...%d\r\n",archive_plat.step);
		switch(archive_plat.step)
		{
			case 0:   //创建Socket  链接国家平台  (备案网关地址)
				if((res = at_query_socket_state(ARCHIVE_SOCKET_ID)) > 0)
				{
					if(at_close_socket_connect(ARCHIVE_SOCKET_ID) == 0)
						printf("-- close socket archive plat connect....\r\n");
				}

                read_config_archivel_gw_addr(tmp.addr,sizeof(tmp.addr));
                tmp.port = read_config_archivel_gw_port();

                if(at_creat_socket_connect(ARCHIVE_SOCKET_ID,&tmp,archive_down_qh) == 0)
                {
					printf("-- Creat Archive socket onnect:%s:%d.....OK\r\n",tmp.addr,tmp.port);  
                	send_len = build_vehicle_archival_info(archive_buff,sizeof(archive_buff));
					if(send_len > 2)   //组包备案信息
					{
						//mem_printf(LOG_ERROR, PRINT_HEX,local_buff, send_len);
						if(at_send_socket_data(ARCHIVE_SOCKET_ID,archive_buff,send_len) == 0)
						{
							archive_plat.step++;
							printf("-- send archival info OK .....\r\n");
						}
						else
						{
							archive_plat.step = 2;
							archive_plat.state = 7;  //归类为链接国家平台（备案平台）失败
							printf("-- send archival info Fail .....\r\n");
						}
							
					}
					else if(send_len == 1)
					{
						archive_plat.step = 2;
						archive_plat.state = 6;    //没有机械环保代码  
						printf("-- build archival info Fail .....\r\n");
					}
					else
					{
						archive_plat.step = 2;
						archive_plat.state = 9;    //加密芯片错误 
						printf("-- build archival info Fail .....\r\n");
					}
                }   
				else        //链接国家平台失败
				{
					archive_plat.step = 2;    //删除该任务
					archive_plat.state = 7; 
					printf("-- Creat Archive socket onnect:%s:%d.....Fail\r\n",tmp.addr,tmp.port);
				}      
            
				break;
			case 1:                     //等待备案结果 
				//printf("-- wait for archival res....%d,%d\r\n",archive_plat.res,archive_plat.count);
				if(archive_plat.count++ < 30 && archive_plat.res == 0)
					break;
				if( archive_plat.res == 0)
				{
					archive_plat.state = 8;//  备案平台无应答，备案超时。
				}
					
				archive_plat.step++;
				break;
			case 2:                 //关闭Socket  
				{
					struct products_mq_str 		mq = {0};
					QueueHandle_t			 	t_q = NULL;

					mq.cmd = 3;
					mq.len = 1;
					mq.data[0] = 32;
					mq.data[1] = archive_plat.state;
					t_q = get_products_queue();
        			if(t_q != NULL)
            			xQueueSend(t_q,&mq,sizeof(struct products_mq_str));	

					if(at_close_socket_connect(ARCHIVE_SOCKET_ID) == 0)            
						printf("-- close archival socket ok....\r\n");

					if(archive_down_qh != NULL)
					{
						vQueueDelete(archive_down_qh);
						archive_down_qh = NULL;
					}	

					printf("-- Del thread entry archivel...\r\n");
               		vTaskDelete(NULL);   //删除自身任务
				}
				break;
			default:
				archive_plat.step = 2;
				break;
		}	
	}
}


