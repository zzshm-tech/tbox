

#include <stdint.h>
#include <string.h>
#include <stdio.h>



#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"



#include "app_products.h"




/************** 本地全局变量 ******************/

static QueueHandle_t  							products_queue = NULL;   //生产队列

static struct config_str        		config_info = {0};    //？配置信息读写要增加  信号保护？？？？？

static uint8_t 											products_port = 0;    //默认的生产端口CAN  0  1：通过SHELL口





/***************************************
**	返回设备号
****************************************/

uint8_t read_config_terminal_id(uint8_t *buf,uint8_t size)
{
	if(size < 16)
		return 0;
	memcpy(buf,config_info.terminal_id,16);

	return 16;
}




/*************************************************
**
************************************************/

QueueHandle_t get_products_queue(void)
{
	return products_queue;
}





/****************************
**	
****************************/

void thread_entry_products(void *parameter)
{
	parameter = parameter;

	for(;;)
	{
		vTaskDelay(100);
		
	}
}


