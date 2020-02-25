

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
//

#include "gnss.h"
#include "gsm.h"
#include "rtc.h"
#include "config.h"
#include "mon.h"
#include "can.h"
#include "rtc.h"
#include "pro_data.h"
#include "ddp.h"
#include "fifo.h"


//一下信息为新添加

#include "board.h"
#include "fr_drv_uart.h"
#include "fr_drv_timer.h"
#include "fr_drv_gpio.h"
#include "fr_drv_adc.h"
#include "fr_drv_mem.h"
#include "fr_drv_rtc.h"



static unsigned char 	run_state;


/***************任务句柄*******************/

TaskHandle_t 			task_handler_gnss;						//GNSS任务
TaskHandle_t 			task_handler_gsm;							//gsm连接网络任务
TaskHandle_t 			task_handler_rtc;							//rtc任务
TaskHandle_t			task_handler_in;							//输入任务
TaskHandle_t			task_handler_config;					//配置任务
TaskHandle_t			task_handler_can_lock;				//can锁车任务
TaskHandle_t			task_handler_can_rx;					//can接收任务
TaskHandle_t 			task_handler_up_load;
TaskHandle_t      task_handler_build_data;

/*****************************************/

static TickType_t 							mon_ticks = 0xFFFFFFFF;     //

//static unsigned char 						mon_recv_buf[4096];         //接收缓冲区

static SemaphoreHandle_t 							mon_semaphore = NULL;				//


/****************************
**	接收完成回调函数
******************************/
static void mon_ticks_handle(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken;
	
	if(mon_ticks == 0xFFFFFFFF)
		return;
	if(xTaskGetTickCount() - mon_ticks > 2)
	{
		mon_ticks = 0xFFFFFFFF;
		xSemaphoreGiveFromISR(mon_semaphore,&xHigherPriorityTaskWoken);
	}
}


/**********************
**	收到GNSS数据
************************/

static void mon_rx_handle(void)
{
	mon_ticks = xTaskGetTickCount();
}



/***********************
**	配置任务
**	生产配置任务
************************/

void task_config(void *data)
{
	unsigned char buf[200];
	//unsigned char 						mon_recv_buf[4096]
		
	BaseType_t mon_state;
	int index;
	
	data = data;
	
	for(;;)
	{
		mon_state = xSemaphoreTake(mon_semaphore,100);     //等待回应
		if(mon_state > 0)
		{
			mon_state = read_uart_pkt(2,buf,sizeof(buf));
			if(strstr((const char *)buf, "HOMER3ETESTOVER!") != NULL)     //系统服务，或者重新启动
			{
				save_config_info();
				run_state = 1;
				
				vTaskDelete(task_handler_config);  		//删除任务
				break;
			}
		
			if((index = look_for_str(buf, (unsigned char *)"BDWMODIF:",mon_state)) >= 0)
			{
				mon_state = mon_state - 9;
				analysis_config_info(buf + index + 8,mon_state);
			}
			
			if(strstr((const char *)buf, "Reset") != NULL)     //系统服务，或者重新启动
			{
				vTaskDelay(100);
				fr_reset_sys();
				break;
			}
		}
		memset(buf,'\0',sizeof(buf));
		index = build_config_info((char *)buf,sizeof(buf),0);
		fr_send_console((const char *)buf,index);
	}
}

/*********************************
**	上传包
*********************************/

void task_up_load(void *data)
{
	unsigned short int len;
	unsigned char buf[400];
	
	data = data;
	
	for(;;)
	{
		vTaskDelay(100);
		if(read_net_state() != 2)          //网络未连接成功，不发送任何数据
			continue;
		len = ReadCmdDataBuf(buf);     //(?????? ?????????)
		if(len > 0)
		{
			if(gsm_send_data(1,buf,len) > 0)
			{
				ClearCmdData();
				//fr_printf("Send CMD..................\r\n");
				continue;
			}
		}
		
		len = ReadSendQueue(buf);
		if(len > 0)
		{
			if(gsm_send_data(1,buf,len) > 0)
			{
				fr_printf("Send Up load data........\r\n");
				ClearSendQueue();
			}
			
		}
	}
}


/*********************************
**	数据推送
**	
*********************************/

void task_build_data(void *data)
{
	unsigned int 					next_time_posi;
	unsigned int 					next_time_comp;
	unsigned char 				step;
	unsigned int 					tmp;
	unsigned short int		pack_len;
	unsigned char 				work_state;
	unsigned char 				data_packet_buf[512];
	data = data;
	
	next_time_posi = 0;
	next_time_comp = 0;
	step = 0;
	vTaskDelay(300);
	
	for(;;)
	{
		vTaskDelay(10);
		work_state = (read_can_actual_rotate() >= 2200) ? 1 : 0;    //工作判断条件
		tmp = read_timestamp();
		switch(step)
		{
			case 0:													//  
				if(tmp >= next_time_comp)
				{
					next_time_comp =  tmp + read_config_travel_upload_cycle();
					pack_len = build_complete_packet(data_packet_buf,0);
					
					if(pack_len > 0)
					{
						//fr_printf("build_complete_packet:%u\r\n",pack_len);
						WriteSendQueue(data_packet_buf,pack_len);         //
						break;
					}
				}
			
				if(work_state > 0)                   //工作状态
					step++;
				break;
			case 1:                   		//   
				if(work_state == 0)                              //
				{
					pack_len = build_position_packet(data_packet_buf,1);  //结束打包
					if(pack_len > 0)
					{	
						//fr_printf("acc over build_position_packet:%u\r\n",pack_len);
						WriteSendQueue(data_packet_buf,pack_len);    //
						step = 0;
						break;
					}
				}
			
				if(tmp >= (next_time_comp - 1))
				{	
					next_time_posi = tmp + read_config_work_upload_cycle();
					pack_len = build_position_packet(data_packet_buf,1);         	//
					if(pack_len > 0)
					{
						//fr_printf("work over build_position_packet:%u\r\n",pack_len);
						WriteSendQueue(data_packet_buf,pack_len);   				//
						step++;
						break;
					}

				}
				if(tmp < next_time_posi)           						//
					break;
				next_time_posi = tmp + read_config_work_upload_cycle();
				pack_len = build_position_packet(data_packet_buf,0);
				if(pack_len > 0)
				{
					//fr_printf("work build_position_packet:%u\r\n",pack_len);
					WriteSendQueue(data_packet_buf,pack_len);    					//
				}
				break;
			case 2:
				if(tmp >= next_time_comp)
				{
					next_time_comp =  tmp + read_config_travel_upload_cycle();
					next_time_posi =  tmp + read_config_work_upload_cycle();

					pack_len = build_complete_packet(data_packet_buf,0);
					if(pack_len > 0)
					{
						//fr_printf("work build_complete_packet:%u\r\n",pack_len);
						WriteSendQueue(data_packet_buf,pack_len); 
					}
					step = 1;
				}
			
				break;
			default: 
				step = 0;
				break;
		}
		
		//发送数据
	}
}


/*

void task_test(void *data)
{
	unsigned char step = 0;
	int i;
	
	data = data;
	
	for(;;)
	{
		switch(step)
		{
			case 0:
				step++;
				vTaskDelay(100);
			storage_medium_init();	
				break;
			case 1:
				fr_printf("Write Ex Flash\r\n");
				for(i = 0;i < 4096;i++)
					mon_recv_buf[i] = i;
			taskENTER_CRITICAL(); 
				w25qxx_flash_write(mon_recv_buf, 0,1);
			taskEXIT_CRITICAL(); 
				step++;
				break;
			case 2:
				vTaskDelay(100);
				step++;
				break;
			case 3:
				fr_printf("Read Ex Flash\r\n");
				memset(mon_recv_buf,0,sizeof(mon_recv_buf));
				w25qxx_flash_read(mon_recv_buf, 0, 1);
				step++;
				break;
			case 4:
				vTaskDelay(100);
				fr_printf("Test tast run.............\r\n");
				break;
		}
	}
}


*/



/**************************
**	系统任务开始
**	根据运行状态设备开始运行
**	
***************************/

void task_start(void *data)
{
	unsigned portBASE_TYPE stack_;
	BaseType_t mon_state;
	unsigned int acc_close_cnt;
	unsigned char tmp;
	unsigned char 						mon_recv_buf[256];
	
	data = data;

	acc_close_cnt = 0;
	run_state = 0;
	init_fifo_sema();
	
	tmp = 1;
	if(tmp == 1)
		fr_printf("init storage ok...............\r\n");
	
	for(;;)
	{
		switch(run_state)
		{
			case 0:													//系统初始化（创建任务）
				
				taskENTER_CRITICAL();    	
						
				fr_led_red_on(); 							//点亮红色LED
				read_config_info();           //读取配置信息（设备相关配置信息）
	
				xTaskCreate((TaskFunction_t)task_gnss,			(const char *)"task_gnss",			100,NULL,3,&task_handler_gnss);  	//创建测试任务
				xTaskCreate((TaskFunction_t)task_gsm,				(const char *)"task_gsm",				128,NULL,4,&task_handler_gsm);  		//创建测试任务
				xTaskCreate((TaskFunction_t)task_rtc,				(const char *)"task_rtc",				100,NULL,5,&task_handler_rtc);  		//创建测试任务
				xTaskCreate((TaskFunction_t)task_in,				(const char *)"task_in",				100,NULL,6,&task_handler_in);  		//创建测试任务
				xTaskCreate((TaskFunction_t)task_can_lock,	(const char *)"task_can_lock",	128,NULL,7,&task_handler_can_lock); 
				xTaskCreate((TaskFunction_t)task_can_rx,		(const char *)"task_can_rx",		100,NULL,8,&task_handler_can_rx);  	
				xTaskCreate((TaskFunction_t)task_build_data,(const char *)"task_build_data",256,NULL,9,&task_handler_build_data); 
				//xTaskCreate((TaskFunction_t)task_test,(const char *)"task_test",512,NULL,15,&task_handler_build_data); 
				
				
				taskEXIT_CRITICAL();             //退出临界区
				
				//if(mon_semaphore == NULL)
					mon_semaphore = xSemaphoreCreateBinary();    //信号量
				fr_set_uart2_rx_hook(mon_rx_handle);
				fr_timer3_sethook(mon_ticks_handle);
				vTaskDelay(100);      //
				if(read_config_state() == 0x56)
				{
					xTaskCreate((TaskFunction_t)task_config,			(const char *)"task_config",		512,NULL,10,&task_handler_config);  		//创建测试任务
					run_state = 3;
					break;
				}
				
				xTaskCreate((TaskFunction_t)task_up_load,		(const char *)"task_up_load",		256,NULL,2,&task_handler_up_load);  	
				
				fr_init_wdt(5);
				acc_close_cnt = 0;
				run_state = 1;
				break;
			case 1:														//设备正常运行
				fr_reset_iwdg();
				
				mon_state = xSemaphoreTake(mon_semaphore,(TickType_t)70);     //等待回应
				if(mon_state > 0)
				{
					mon_state = read_uart_pkt(2,mon_recv_buf,sizeof(mon_recv_buf));
					if(strstr((const char *)mon_recv_buf, "Reset") != NULL)     //系统服务，或者重新启动
					{
						fr_printf("Reset from mon.....\r\n");
						vTaskDelay(100);
						fr_reset_sys();
						break;
					}
					if(strstr((const char *)mon_recv_buf, "AT+Test") != NULL)     //系统服务，或者重新启动
					{
						vTaskDelete(task_handler_up_load);            //删除
						vTaskDelay(10);
						xTaskCreate((TaskFunction_t)task_config,			(const char *)"task_config",		512,NULL,31,&task_handler_config);  		//创建测试任务
						run_state = 3;
						break;
					}
				}
				
				fr_led_red_on();
				vTaskDelay(30);
				fr_led_red_off();
				
				/*
				acc_close_cnt++;
				if(acc_close_cnt++ > 30)
				{
					acc_close_cnt = 0;
					tmp = gsm_send_data(1,(unsigned char *)"123456789abcdef\r\n",sizeof("123456789abcdef\r\n"));
					if(tmp > 0)
						fr_printf("Send Data OK...........\r\n");
					if(tmp == 0)
						fr_printf("Send Data Fail...........\r\n");
				}
				
				test_SendQueue();
				*/
				stack_ = uxTaskGetStackHighWaterMark(task_handler_up_load);
				//fr_printf("task ta.....%d\r\n",stack_);
				if(read_acc_state() == 1)
				{
					acc_close_cnt = 0;
					break;
				}
				
				acc_close_cnt++;
				fr_printf("Acc close.....%d\r\n",acc_close_cnt);
				if(acc_close_cnt > read_config_info_run_time() || read_batter_vol()  <= 34)
					run_state = 2; 
				//mon_state = read_config_info_sleep_time() + read_timestamp();   //自动唤醒时间
				mon_state = 3600 + read_timestamp();   //自动唤醒时间
				break;
			case 2:					//休眠模式(休眠的时候整个系统停止调度)
				//保存相关运行数据
				fr_init_wdt(25);
				vTaskDelete(task_handler_up_load);							//删除任务
				vTaskDelete(task_handler_gnss);  								//删除任务
				vTaskDelete(task_handler_gsm);  								//删除任务
				vTaskDelete(task_handler_rtc);  								//删除任务
				vTaskDelete(task_handler_in);  									//删除任务
				vTaskDelete(task_handler_build_data);
				vTaskDelete(task_handler_can_lock);   					//删除任务
				vTaskDelete(task_handler_can_rx);  							//删除任务			
					
				close_gnss_modle();
				gsm_modle_close();
				fr_can_power_off();
				fr_power_flash_off();
				
			  fr_led_red_off();
				fr_led_blue_off();
				fr_led_green_off();
				
				
				fr_init_hsi_8mhz();      //系统切换8MHz主频
				do
				{
					fr_reset_iwdg();
					fr_enter_sleep();      //进入休眠
					
					fr_init_adc();
					
					tmp	 = 0;
					while(tmp < 10)
					{
						tmp++;
						delay_ms(10);
						process_in();
					}
					                  //
				}while((read_acc_state() == 0 && mon_state >= fr_read_rtc()) || read_batter_vol() <= 34);
				//fr_reset_sys();      //系统复位
				fr_exit_sleep();
				run_state = 0;
				break;	
			case 3:          //配置模式()
				vTaskDelay(10);
				fr_led_red_on();            //红色LED灯指示设备处在快闪模式
				vTaskDelay(10);
				fr_led_red_off();	
				fr_reset_iwdg();			
				if(read_acc_state() == 0)
					run_state = 2;
				break;
			default:
				run_state = 0;
				break;
		}
	}
}


/*************************
**	主功能函数：
**************************/
int main(void)				 
{
	fr_init_board();
	
	xTaskCreate((TaskFunction_t)task_start,(const char *)"task_start",200,NULL,1,NULL);   //删除任务
	
	vTaskStartScheduler();   				//任务开始调度 
	
	return 0;

}


/************************************************
*  函数名称：vApplicationIdleHook
*  功    能：空闲任务钩子函数
*************************************************/
void vApplicationIdleHook( void )
{
   // __WFI();
}



/********************File End***********************/


