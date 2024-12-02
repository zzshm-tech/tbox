



#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_task_wdt.h"


#include "common.h"

#include "pro_data.h"
#include "app_lte.h"
#include "app_at.h"
#include "app_iap.h"
#include "app_products.h"
#include "app_sms.h"
#include "app_iap.h"
#include "app_main.h"
#include "app_can_recv.h"
#include "app_can_send.h"



/*******************  *************************/
static QueueHandle_t  			             sms_queue = NULL;           //短消息

static struct sms_down_str                   sms_data = {0};			//




QueueHandle_t  get_sms_queue(void)
{
	return sms_queue;
}

/********************************************************
**  
*********************************************************/
uint8_t app_sms_parse(char *data,uint16_t len)
{
	char                *index = NULL;
	uint8_t 		    array[512];
    uint32_t            tmp = 0;

	if(data == NULL || len == 0 || len > 512)
        return 0;

	

    index = strstr(data, "Reset");
	if (index != NULL)
	{
       	QueueHandle_t  	            qu_t = NULL;          //生产队列
        struct app_main_mq_str      a_mq = {0};

        printf("-- Reset be from SMS ...\r\n");
        a_mq.state = 1;

        qu_t =  get_app_main_queue();
        if(qu_t != NULL)
        {
            xQueueSend(qu_t,&a_mq,sizeof(struct app_main_mq_str));
        }
        vTaskDelay(100);
	}
	// 设置主服务器地址 SetSever0:123.127.244.154,13013, 
	index = strstr(data, "SetSever0:");
	if(index != NULL)
	{
		struct can_send_mq_t tmp_mq;
		QueueHandle_t t_q = NULL;

		tmp_mq.cmd = 4;
		tmp_mq.len = 2;
		tmp_mq.data[0] = 0;
		tmp_mq.data[1] = 0;

		t_q = get_can_send_queue();
		if(t_q != NULL)
			xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
				
				
	    printf("-- SMS mon cmd....%d\r\n",tmp_mq.data[1]);
	}
	/* 设置从服务器地址 */
	index = strstr(data, "SetSever1:");
	if(index != NULL)
	{
		struct can_send_mq_t tmp_mq;
		QueueHandle_t t_q = NULL;

		tmp_mq.cmd = 4;
		tmp_mq.len = 2;
		tmp_mq.data[0] = 0;
		tmp_mq.data[1] = 1;

		t_q = get_can_send_queue();
		if(t_q != NULL)
			xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
				
				
	    printf("-- SMS mon cmd....%d\r\n",tmp_mq.data[1]);
	}
	// 设置token服务器地址 
	index = strstr(data, "SetSever2:");
	if(index != NULL)
	{
	 	struct can_send_mq_t tmp_mq;
		QueueHandle_t t_q = NULL;

		tmp_mq.cmd = 4;
		tmp_mq.len = 2;
		tmp_mq.data[0] = 1;
		tmp_mq.data[1] = 0;
		t_q = get_can_send_queue();
		if(t_q != NULL)
			xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
		printf("-- SMS unlock cmd...%d\r\n",tmp_mq.data[1]);
	}
	/* 设置从2服务器地址 */
	index = strstr(data, "SetSever3:");  
	if(index != NULL)
	{
		struct can_send_mq_t tmp_mq;
		QueueHandle_t t_q = NULL;

		tmp_mq.cmd = 4;
		tmp_mq.len = 2;
		tmp_mq.data[0] = 1;
		tmp_mq.data[1] = 1;
		t_q = get_can_send_queue();
		if(t_q != NULL)
		xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
								
		printf("-- SMS lock cmd...%d\r\n",tmp_mq.data[1]);		
	}
	index = strstr(data, "SetSever4:");
	if(index != NULL)
	{
		struct can_send_mq_t tmp_mq;
		QueueHandle_t t_q = NULL;

		tmp_mq.cmd = 4;
		tmp_mq.len = 2;
		tmp_mq.data[0] = 1;
		tmp_mq.data[1] = 2;
		t_q = get_can_send_queue();
		if(t_q != NULL)
		xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
								
		printf("-- SMS lock cmd...%d\r\n",tmp_mq.data[1]);				
	}
	/* 设置参数 */
	index = strstr(data, "SetParameter:");
	if(index != NULL)
	{
						
	}

	//远程升级 Down:223.223.187.35,1050,ftadmin,ftadmin81645,BC_03ff.bin,T,
	index = strstr(data, "Down:");
	if(index != NULL)
	{
        struct iap_ftp_str  iap_ftp;
	
        memset((uint8_t *)&iap_ftp,'\0',sizeof(struct iap_ftp_str));
		index += 5;

		tmp = search_char(1,',',(uint8_t *)index,len);
		if(tmp > 0)
		{
            memset(iap_ftp.host,'\0',sizeof(iap_ftp.host));
			memcpy(iap_ftp.host,index,tmp - 1);     //FTP主机地址   		
			printf("-- FTP Server Addr:%s\r\n",iap_ftp.host);	
		}
        else
        {
            return 1;
        }

        memset(array,'\0',sizeof(array));
		if(get_buf_str(1,2,',',(uint8_t  *)index,array,len) > 0)         
		{
            iap_ftp.port = fr_atof((const char *)array);   //FTP端口号
			printf("-- FTP Server Port:%d\r\n",iap_ftp.port);					
		}	
        else
        {
            return 1;
        }							

        memset(array,'\0',sizeof(array)); 
		if(get_buf_str(2,3,',',(uint8_t *)index,array,len) > 0)     
		{
            memcpy(iap_ftp.user,array,sizeof(iap_ftp.user));
			printf("-- FTP FtpUserName:%s\r\n",iap_ftp.user);
		}	
        else
        {
            return 1;
        }	
		
        memset(array,'\0',sizeof(array));
		if(get_buf_str(3,4,',',(uint8_t *)index,array,len) > 0)       
		{
			memcpy(iap_ftp.passwd,array,sizeof(iap_ftp.passwd));          //FTP用户密码
			printf("-- FTP FtpUserPassd:%s\r\n",iap_ftp.passwd);
		}
        else
        {
            return 1;
        }
		
        memset(array,'\0',sizeof(array));
		if(get_buf_str(4,5,',',(uint8_t *)index,array,len) > 0)      
		{
			memcpy(iap_ftp.files,array,sizeof(iap_ftp.files));         //要升级的文件名称
			printf("-- FTP File Name:%s\r\n",iap_ftp.files);	
		}
        else
        {
            return 1;
        }

		// printf("\r\n-- FTP FtpUserName:%s\r\n",ftp_info.user);
		// printf("-- FTP FtpUserPassd:%s\r\n",ftp_info.passwd);
		// printf("-- FTP Server Addr:%s\r\n",ftp_info.host);
		// printf("-- FTP Server Port:%d\r\n",ftp_info.port);
		// printf("-- FTP File Name:%s\r\n",ftp_info.files);

        if(read_iap_state() == 0)
		{
			xTaskCreate(thread_entry_iap, "thread_entry_iap",4096,&iap_ftp,  15,  NULL);  //创建FTP升级任务
			vTaskDelay(100);
		}
								
		vTaskDelay(100);
	}

	//vTaskDelay(100);
	// fflush(stdout);
	// esp_restart();

    return 0;
}


/**********************************
**  处理短信
**********************************/

void thread_entry_sms(void *parameter)
{
	uint8_t array[70] = {0};

    parameter = parameter;

	vTaskDelay(100);

    sms_queue = xQueueCreate(2,sizeof(struct sms_down_str ));
	
	for(;;)
    {
        if(xQueueReceive(sms_queue,&sms_data,100) == pdTRUE)
        {
			if(sms_data.len > 0)
			{
				char 			*p_s = NULL;
				char 			*p_e = NULL;
				uint32_to_byte	m_tmp;

				p_s = strstr((const char *)sms_data.data, "\r\n");
				p_e = strstr((const char *)sms_data.data, "3010535A521B805452A83011");
				//printf("-- 11 recv sms ...%d\r\n%s\r\n",sms_data.len,p_s);
				if(p_s != NULL && p_e != NULL && (p_e > (p_s + 2)))
				{	
					p_s += 2;
					sms_data.len = (p_e - p_s) / 4;
					memset(array,'\0',sizeof(array));
					m_tmp.value = 0;

					for(int i = 0;i < sms_data.len;i++)
					{
						m_tmp.byte[0] = *(p_s + i * 4 + 3);
						m_tmp.byte[1] = *(p_s + i * 4 + 2);
						m_tmp.byte[2] = *(p_s + i * 4 + 1);
						m_tmp.byte[3] = *(p_s + i * 4 + 0);

						if(m_tmp.byte[0] == '0' || m_tmp.byte[0] <= '9')
							m_tmp.byte[0] -= 0x30;
						else
							m_tmp.byte[0] -= 55;
						
						if(m_tmp.byte[1] == '0' || m_tmp.byte[1] < '9')
							m_tmp.byte[1] -= 0x30;
						else
							m_tmp.byte[1] -= 55;

						array[i] = m_tmp.byte[1] * 16 + m_tmp.byte[0];
						//printf("-- the 0x%X\r\n",array[i]);
					}

					app_sms_parse((char *)array,sms_data.len);
               		printf("-- recv sms ... %d  %s\r\n",sms_data.len,array);
				}
				
            }
        }     
    }
}
    





