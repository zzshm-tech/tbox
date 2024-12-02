



#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "board.h"
#include "drv_uart.h"
#include "drv_rtc.h"


#include "common.h"
#include "pro_data.h"
#include "version.h"

#include "app_products.h"
#include "app_shell.h"
#include "app_can_recv.h"
#include "app_can_send.h"
#include "app_iap.h"
#include "app_gnss.h"
#include "app_main.h"
#include "app_gb4.h"
#include "app_in.h"
#include "app_lte.h"






/****************** 本地全局变量 *****************/

static uint8_t              shell_buff[SHELL_BUFF] = {0};   //
 
static uint16_t             shell_len = 0;                 //接收数据长度

static  uint16_t            delay_net_time = 0;
//static struct debug_str     debug_t;                      //控制调试信息



uint16_t read_dealy_net_time(void)
{
    uint16_t rv = 0;

    rv = delay_net_time;

    return rv;
}


/***********************
**	故障码
*********************/

void view_obd_fault_code(void)
{
	uint32_t i;
	struct gb1939_dm_str tmp;

	printf("\r\n");
	printf("\r\n");
	
	printf("---- OBD国标信息(国标信息) ----\r\n");
	printf("-- 排放控制报警灯状态... %d\r\n",read_mil_light_state());
	
    read_ecu_dm1_data(&tmp);
	printf("-- ECU 故障数量... %d\r\n",tmp.num);
	for(i = 0;i < tmp.num;i++)
	{
		printf("-- 故障码【%d】 SPN:%d,FMI:%d\r\n",i,tmp.spn[i],tmp.fmi[i]);
	}
	
	printf("-------------------------\r\n");
	printf("\r\n");
	printf("\r\n");
}





/*******************************
**  显示国标排放数据流
*******************************/

void view_ecu_data(void)
{
    //uint32_t                    tmp = 0;
    struct ecu_data_str         *p_ecu;
    uint8_t                     array[256];

    p_ecu = (struct ecu_data_str *)array;
	read_ecu_data(p_ecu);	

    printf("\r\n");
	printf("\r\n");
	
	printf("-- 整车数据(ECU广播的) ----\r\n");
	
    printf("-- ECU类型:%d\r\n",p_ecu->manu_type);							//车速  (使用GPS速度)

	printf("-------------------------\r\n");
	printf("\r\n");
	printf("\r\n");
}



/*******************************
**  显示国标排放数据流
*******************************/

void view_gb4_data(void)
{
    
}



/***********************************
**	打印定位信息
************************************/

static void view_gnss_info(void)
{
    struct gnss_info_str gnss;

    read_gnss_info(&gnss);

	printf("\r\n*********** GNSS INFO ***********\r\n");

    printf("- Gnss info:%u,%u,%c\r\n",gnss.latitude_normal,gnss.longitude_normal,gnss.state);
		
    printf("\r\n*********************************\r\n");
}



/*******************************
**  处理SHELL命令
********************************/

static void shell_cmd_handler(uint8_t *data,uint16_t len)
{
    char *p = NULL;

    if(data == NULL)
        return;

    p = strstr((char *)data,"AT+MON=");
    if(p != NULL)
    {
        struct can_send_mq_t tmp_mq;
		QueueHandle_t t_q = NULL;
        uint32_t m = 0;

        sscanf((char *)data,"AT+MON=%d\r\n",&m);
        //printf("-- AT+Mon=%d\r\n",m);

		tmp_mq.cmd = 4;
		tmp_mq.len = 2;
		tmp_mq.data[0] = 0;
		tmp_mq.data[1] = (m > 1) ? 1 : m;

		t_q = get_can_send_queue();
		if(t_q != NULL)
			xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
				
				
	    printf("-- Shell mon cmd....%d\r\n",tmp_mq.data[1]);
    }

    p = strstr((char *)data,"AT+LOCK=");
    if(p != NULL)
    {
        uint32_t m = 0;

        sscanf((char *)data,"AT+LOCK=%d\r\n",&m);
        //printf("-- AT+Lockn=%d\r\n",m);

        if(m == 0)
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
			printf("-- Shell unlock cmd...%d\r\n",tmp_mq.data[1]);
        }
        else
        {
            struct can_send_mq_t tmp_mq;
		    QueueHandle_t t_q = NULL;

		    tmp_mq.cmd = 4;
			tmp_mq.len = 2;
			tmp_mq.data[0] = 1;
			tmp_mq.data[1] = (m > 2) ? 2 : m;
			t_q = get_can_send_queue();
			if(t_q != NULL)
			xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
								
			printf("-- Shell lock cmd...%d\r\n",tmp_mq.data[1]);
        }
    }

    
    p = strstr((char *)data,"Reset");
    if(p != NULL)
    {
        QueueHandle_t  	            qu_t = NULL;          //生产队列
        struct app_main_mq_str      a_mq = {0};

        printf("-- Reset be from shell cmd ...\r\n");
        a_mq.state = 1;

        qu_t =  get_app_main_queue();
        if(qu_t != NULL)
        {
            xQueueSend(qu_t,&a_mq,sizeof(struct app_main_mq_str));
        }

        
        vTaskDelay(100);
    }


    p = strstr((char *)data,"AT+NEMA=");
    if(p != NULL)
    {
        uint32_t m = 0;
        

        sscanf((char *)data,"AT+NEMA=%d\r\n",&m);


        //printf("-- Set Neam:%d\r\n",m);
        init_gnss_debug_state((uint8_t)m);

        return;
    }

    p = strstr((char *)data,"AT+PIN?\r\n");
    if(p != NULL)
    {
        uint8_t  array[32] = {0};
        
        memset(array,'\0',sizeof(array));

        if(read_config_vin_state() == 0x5A)
            read_config_vin_info(array,sizeof(array));
   
        printf("-- PIN:%s\r\n",array);
        return ;
    }

    p = strstr((char *)data,"shutdown");  //???????
    if(p != NULL)
    {
        QueueHandle_t  	            qu_t = NULL;          //????????
        struct app_main_mq_str      a_mq = {0};

        if(read_in_acc_state() > 0)
        {
            printf("-- Acc Open ........ \r\n");
            return ;
        }
        a_mq.state = 2;
        qu_t =  get_app_main_queue();
        if(qu_t != NULL)
        {
            xQueueSend(qu_t,&a_mq,sizeof(struct app_main_mq_str));
        }
        vTaskDelay(100);
        return;
    }

    p = strstr((char *)data,"AT+VERSION\r\n");
    if(p != NULL)
    {
        show_sys_version();
    }


    p = strstr((char *)data,"AT+DEV?");   //
    if(p != NULL)
    {
        uint8_t array[32];

        memset(array,'\0',32);

        read_config_terminal_id(array,32);
        printf("-- The Terminal ID:%s\r\n",array);
        
        return;
    }

    p = strstr((char *)data, "DEBUG_TEST");         //
    if(p != NULL)
    {
        //view_gb4_data();
        view_obd_fault_code();   //显示故障代码  1939
        //view_gb27145_dm();
        //view_ecu_data();
    }
    


        p = strstr((char *)data, "AT+LTE_INFO?");         //
    if(p != NULL)
    {
        view_lte_info();
    }


    p = strstr((char *)data,"AT+KEY?");   //
    if(p != NULL)
    {
        uint8_t array[100];
       
        
        printf("--------------------- 加密处理器信息 ---------------------\r\n");
       
        printf("-- 加密处理器状态：");
        if(read_acl16_work_state() == 0)
        {
            printf("异常\r\n");
        }   
        else 
        {
            printf("正常\r\n");
            memset(array,'\0',32);
            read_encryption_chip_id(array,32);
            printf("\r\n-- 加密处理器ID:%s\r\n",array);
            printf("\r\n-- 加密处理器应用程序版本：%d\r\n", read_acl16_app_version());
            read_config_public_key(array,sizeof(array));
            printf("\r\n-- 公钥信息：");
            mem_printf(LOG_ERROR,PRINT_HEX,array,64);
        }
        printf("\r\n---------------------------------------------------------\r\n");
        return;
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
        if(erase_products_cfg_info() == 0)
            printf("-- erase products config info ok... \r\n");

        delete_gnss_info_files();
        delete_login_out_num();
        vTaskDelay(100);
		
        return;
    }

    p = strstr((char *)data,"AT+RTC?");   //读取时间
    if(p != NULL)
    {
        struct rt_tm tm;

        get_rtc_time(&tm);
        printf("-- RTC:20%02d.%02d.%02d : %02d-%02d-%02d\r\n",tm.year,tm.mon,tm.day,tm.hour,tm.min,tm.sec);
       
        return;
    }

    p = strstr((char *)data,"AT+ERASE\r\n");
    if(p != NULL)
    {
        erase_vehicle_args_info();
    }


     p = strstr((char *)data,"AT+GNSS\r\n");
    if(p != NULL)
    {
        view_gnss_info();
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
			return;
		}	
      
        #if 1
		memcpy(iap_ftp.user,(uint8_t *)"FTPServer",sizeof("FTPServer"));
		memcpy(iap_ftp.passwd,(uint8_t *)"Shme92635887",sizeof("Shme92635887"));
	 	memcpy(iap_ftp.host,(uint8_t *)"124.222.139.175",sizeof("124.222.139.175"));
		iap_ftp.port = 1050;
        #else
        memcpy(iap_ftp.user,(uint8_t *)"ftp01",sizeof("ftp01"));
		memcpy(iap_ftp.passwd,(uint8_t *)"Wang92635887wei",sizeof("Wang92635887wei"));
	 	memcpy(iap_ftp.host,(uint8_t *)"47.96.248.39",sizeof("47.96.248.39"));
		iap_ftp.port = 21;
        #endif

        printf("\r\n-- FTP FtpUserName:%s\r\n",iap_ftp.user);
		printf("-- FTP FtpUserPassd:%s\r\n",iap_ftp.passwd);
	    printf("-- FTP Server Addr:%s\r\n",iap_ftp.host);
		printf("-- FTP Server Port:%d\r\n",iap_ftp.port);
		printf("-- FTP File Name:%s\r\n",iap_ftp.files);
     
	    if(read_iap_state() == 0)
        {
            xTaskCreate(thread_entry_iap, "thread_entry_iap",  4096,&iap_ftp,  15,  NULL);  //????FTP????????
        }
        vTaskDelay(100);    						
    }
}



/****************************
 ** SHELL处理任务
****************************/


////*
void thread_entry_shell(void *parameter)
{
    parameter = parameter;

    for (;;)
    {
       shell_len = read_data_from_uart0(shell_buff, sizeof(shell_buff) - 1,5000,150);
       //shell_len = uart_read_bytes(UART_NUM_0, shell_buff,1023, pdMS_TO_TICKS(100));
        if(shell_len > 0)
        {
            //printf("-- the recv shell ... %d\r\n",shell_len);
            shell_cmd_handler(shell_buff,shell_len);
            memset(shell_buff,0,sizeof(shell_buff));
            shell_len = 0;
        }
        
        //printf("-- the thread entry shell....%d\r\n",shell_len);
    }
    
}

///*/

/****************************
 ** SHELL处理任务
****************************/

// void thread_entry_shell(void *parameter)
// {
//     parameter = parameter;
//     for (;;)
//     {
//     }
// }





