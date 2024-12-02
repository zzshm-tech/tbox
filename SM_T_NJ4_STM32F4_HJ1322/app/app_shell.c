

#include <stdio.h>
#include <string.h>
#include <stdint.h>


#include "FreeRTOS.h"
#include "task.h"

#include "drv_rtc.h"
#include "drv_timer.h"
#include "drv_uart.h"



#include "app_iap.h"
#include "app_shell.h"
#include "app_products.h"


static struct shell_info shell = {0};

static uint8_t	data_buff[256] = {0};



/**************************************
 * @desc  : 串口回调函数
 * @param : dev 使用的串口
 *          size 当前所接收到的数据量
 * @return: 
 *************************************/
 
void shell_rx_data_handle(uint16_t size)
{
    shell.ticks = xTaskGetTickCount(); /* 取收到数据时的tick值*/
    shell.len = size;          /* 当前的接收的数据长度*/
}

/********************************************************
 * @desc  : 串口接收完毕判断,定时器回调
 * @param : none
 * @return: none
 *********************************************************/
void shell_ticks_handle(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if(shell.ticks == 0xffffffff)
		return;

  if(xTaskGetTickCount() - shell.ticks > 1)
  {
		shell.ticks = 0xffffffff;     /* tick值复位*/
      
		if(shell.semaphore != NULL)
			xSemaphoreGiveFromISR(shell.semaphore, &xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}






/*******************************
**  处理SHELL命令
********************************/

static void shell_cmd_handler(uint8_t *data,uint16_t len)
{
    char *p = NULL;

    if(data == NULL)
        return;
    
    p = strstr((char *)data,"Reset");
    if(p != NULL)
    {
        QueueHandle_t  	            qu_t = NULL;          //生产队列
        //struct app_main_mq_str      a_mq = {0};

        printf("-- Reset be from shell cmd ...\r\n");
        //a_mq.state = 1;

        qu_t =  NULL;//get_app_main_queue();
        if(qu_t != NULL)
        {
            //xQueueSend(qu_t,&a_mq,sizeof(struct app_main_mq_str));
        }

        
        vTaskDelay(100);
    }

    p = strstr((char *)data, "DEBUG_TEST");         //
    if(p != NULL)
    {
//        view_gb4_data();
//        view_obd_fault_code();   //显示故障代码
//        view_gb27145_dm();
    }
    
	p = strstr((char *)data, "AT+Test");           //进入配置模式  （手动进入配置模式）
    if(p != NULL)
    {
        QueueHandle_t t_q = NULL;
        struct products_mq_str	t_m = {0};
        
        t_m.cmd = 2;
        t_q = get_products_queue();
        if(t_q != NULL)
            xQueueSend(t_q,&t_m,sizeof(struct products_mq_str));
	    return;
    }
    
    p = strstr((char *)data, "BDWMODIF:");          //配置信息 （收到配合信息）
    if(p != NULL)
    {
        QueueHandle_t t_q = NULL;
        struct products_mq_str	t_m = {0};

        t_m.cmd = 0;
        t_m.len = len - 8;
        memcpy(t_m.data,p + 8,len - 8);

        //printf("-- products len:%d\r\n",t_m.len);
        t_q = get_products_queue();
        if(t_q != NULL)
            xQueueSend(t_q,&t_m,sizeof(struct products_mq_str));

        return;
    }
    
	p = strstr((char *)data, "HOMER3ETESTOVER!");     //结束配置     
    if(p != NULL)
    {
        QueueHandle_t t_q = NULL;
        struct products_mq_str	t_m = {0};
        
        t_m.cmd = 1;
        //memcpy(t_m.data,p + sizeof("BDWMODIF:"),len - sizeof("BDWMODIF:"));
        t_q = get_products_queue();
        if(t_q != NULL)
            xQueueSend(t_q,&t_m,sizeof(struct products_mq_str));
	    return;
    }
    
		p = strstr((char *)data, "000000RESET!");             //回复出厂设置
    if(p != NULL)
    {
        
		
        return;
    }

    p = strstr((char *)data,"AT+RTC?");   //读取时间
    if(p != NULL)
    {
        struct rt_tm tm;
        get_rtc_time(&tm);
        printf("+RTC:20%02d.%02d.%02d : %02d-%02d-%02d\r\n",tm.year,tm.mon,tm.day,tm.hour,tm.min,tm.sec);

        return;
    }

    p = strstr((char *)data,"AT+GNSS\r\n");
    if(p != NULL)
    {
        //view_gnss_info();
    }

    p = strstr((char *)data,"AT+IAP");   //调试使用
    if(p != NULL)
    {
        struct iap_ftp_str   iap_ftp = {0};                      //

        memset((uint8_t *)&iap_ftp,'\0',sizeof(struct iap_ftp_str));
        sscanf((char *)data,"AT+IAP=%s\r\n",iap_ftp.files);
        memcpy(iap_ftp.user,(uint8_t *)"ftadmin",sizeof("ftadmin"));
				memcpy(iap_ftp.passwd,(uint8_t *)"ftadmin81645",sizeof("ftadmin81645"));
				memcpy(iap_ftp.host,(uint8_t *)"223.223.187.35",sizeof("223.223.187.35"));
				iap_ftp.port = 1050;
		//memcpy(iap_ftp.files,(uint8_t *)"Homer4SE-GB4.bin",sizeof("Homer4SE-GB4.bin"));	

        // printf("\r\n-- FTP FtpUserName:%s\r\n",ftp_info.user);
		// printf("-- FTP FtpUserPassd:%s\r\n",ftp_info.passwd);
		// printf("-- FTP Server Addr:%s\r\n",ftp_info.host);
		// printf("-- FTP Server Port:%d\r\n",ftp_info.port);
		// printf("-- FTP File Name:%s\r\n",ftp_info.files);
        
	    //xTaskCreate(thread_entry_iap, "thread_entry_iap",  4096,&iap_ftp,  14,  NULL);  //创建FTP升级任务
        
        vTaskDelay(100);    
        						
    }
}





/****************************
 ** SHELL处理任务
****************************/

void thread_entry_shell(void *parameter)
{
	uint16_t len = 0;
	
	parameter = parameter;
	
	shell.semaphore = xSemaphoreCreateBinary();
	
	if(shell.semaphore == NULL)
  {
		printf("-- creat packet sph fail....\r\n");
  }
	
	rt_irq_tim3_sethook(shell_ticks_handle);
	rt_irq_uart_sethook(1,shell_rx_data_handle);
	
  for(;;)
  {
		if(xSemaphoreTake(shell.semaphore, 100) == pdTRUE)
		{
			memset(data_buff,'\0',sizeof(data_buff));
			len =  rt_read_uart_buf(1, data_buff, shell.len);
			if(len > 0)
			{
				shell_cmd_handler(data_buff,len);
			}
		}
  } 
}



