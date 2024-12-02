



#include <stdio.h>
#include <string.h>
#include <stdint.h>


#include <rtthread.h>
#include <rtdevice.h>



#include "board.h"


#include "pro_data.h"
#include "app_iap.h"
#include "app_sms.h"
#include "app_iap.h"
#include "common.h"
#include "app_lte.h"
#include "app_files.h"
#include "app_at.h"
#include "app_main.h"




/******************* 本地全局变量 *************************/
static rt_mq_t   			             					 sms_queue = NULL;           //短消息

static struct sms_down_str                   sms_data = {0};			//


/****************************
**	
*****************************/

rt_mq_t   get_sms_queue(void)
{
	return sms_queue;
}


/*******************************************
**  
*******************************************/

uint8_t app_sms_parse(char *data,uint16_t len)
{
	char                *index = NULL;
	uint8_t 		    		array[128];
  uint32_t            tmp = 0;

	if(data == NULL || len == 0 || len > 128)
        return 0;

	index = strstr(data, "CLOSE_NET");
	if(index != NULL)
	{
    struct lte_mq_t event;
		
		rt_mq_t			tmp_mq;
	
		event.cmd = 1;
		
		tmp_mq = get_lte_link_mq();
		rt_kprintf("-- SMS cmd close net......\r\n");
		if(tmp_mq != NULL)
			rt_mq_send(get_lte_link_mq(),&event,sizeof(struct lte_mq_t));   //关闭网络
	}
	
	index = strstr(data,"FILE_SYSTEM_FORMAT");
	if(index != NULL)
	{
		struct app_main_mq_t      	amq = {0};
		rt_mq_t 										tmp = NULL;
		
		mkfs_files_sys();   //格式化文件系统
		amq.state = 1;
		tmp = get_app_main_queue();
		if(tmp != NULL)
			rt_mq_send(tmp,&amq,sizeof(struct app_main_mq_t));
	}

  index = strstr(data, "SMS_RESET");  //通过短信息重启
	if(index != NULL)
	{
		struct app_main_mq_t      	amq = {0};
		rt_mq_t 										tmp = NULL;
		
		amq.state = 1;
		tmp = get_app_main_queue();
		if(tmp != NULL)
			rt_mq_send(tmp,&amq,sizeof(struct app_main_mq_t));
	}
	// 设置主服务器地址 SetSever0:123.127.244.154,13013, 
	index = strstr(data, "SetSever0:");
	if(index != NULL)
	{
		
	}
	// 设置从服务器地址 
	index = strstr(data, "SetSever1:");
	if(index != NULL)
	{

	}
	// 设置token服务器地址 
	index = strstr(data, "SetSever2:");
	if(index != NULL)
	{
							
	}
	// 设置从2服务器地址 
	index = strstr(data, "SetSever3:");
	if(index != NULL)
	{
						
	}
	index = strstr(data, "SetSever4:");
	if(index != NULL)
	{
							
	}
	// 设置参数 
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
			rt_kprintf("-- FTP Server Addr:%s\r\n",iap_ftp.host);	
		}
    else
    {
       return 1;
    }

    memset(array,'\0',sizeof(array));
		if(get_buf_str(1,2,',',(uint8_t  *)index,array,len) > 0)         
		{
      iap_ftp.port = fr_atof((const char *)array);   //FTP端口号
			rt_kprintf("-- FTP Server Port:%d\r\n",iap_ftp.port);					
		}	
    else
    {
      return 1;
    }							

    memset(array,'\0',sizeof(array)); 
		if(get_buf_str(2,3,',',(uint8_t *)index,array,len) > 0)     
		{
      memcpy(iap_ftp.user,array,sizeof(iap_ftp.user));
			rt_kprintf("-- FTP FtpUserName:%s\r\n",iap_ftp.user);
		}	
    else
    {
      return 1;
    }	
		
    memset(array,'\0',sizeof(array));
		if(get_buf_str(3,4,',',(uint8_t *)index,array,len) > 0)       
		{
			memcpy(iap_ftp.passwd,array,sizeof(iap_ftp.passwd));          //FTP用户密码
			rt_kprintf("-- FTP FtpUserPassd:%s\r\n",iap_ftp.passwd);
		}
    else
    {
      return 1;
    }
		
    memset(array,'\0',sizeof(array));
		if(get_buf_str(4,5,',',(uint8_t *)index,array,len) > 0)      
		{
			memcpy(iap_ftp.files,array,sizeof(iap_ftp.files));         //要升级的文件名称
			rt_kprintf("-- FTP File Name:%s\r\n",iap_ftp.files);	
		}
    else
    {
      return 1;
    }
		
		memset(array,'\0',sizeof(array));
		if(get_buf_str(5,6,',',(uint8_t *)index,array,len) > 0) 
		{
			
		}
		else
		{
			return 1;
		}
		
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


/**********************************
**  处理短信
**********************************/

void thread_entry_sms(void *parameter)
{
	uint8_t array[70] = {0};

  parameter = parameter;

	rt_thread_delay(100);

	
  sms_queue = rt_mq_create("sms_mq",sizeof(struct sms_down_str),1,RT_IPC_FLAG_FIFO);
	
	for(;;)
  {
		memset((uint8_t *)&sms_data,0,sizeof(sms_data));
		if(rt_mq_recv(sms_queue,&sms_data,sizeof(struct sms_down_str),100) == RT_EOK) //接收队列== RT_EOK)         //
		{
			
			switch(sms_data.cmd)
			{
				case 0:   //读取短信
					//rt_kprintf("-- run this is.... %d\r\n",sms_data.data[0]);
					at_read_sms_text(sms_data.data[0]);
					break;
				case 1:    //删除所有短信
					
					break;
				case 2:   //处理短信
					if(sms_data.len > 0)
					{
						char 			*p_s = NULL;
						char 			*p_e = NULL;
						uint32_to_byte	m_tmp;

						p_s = strstr((const char *)sms_data.data, "\r\n");
						p_e = strstr((const char *)sms_data.data, "3010535A521B805452A83011");
						//rt_kprintf("-- 12 recv sms ...%d\r\n%s\r\n",sms_data.len,p_s);
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
							//rt_kprintf("-- recv sms ... %d  %s\r\n",sms_data.len,array);
						}
					}
				break;
			}
    }     
   }
}
    





