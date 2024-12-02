
/****************
**
**
******************************/

#include <stdio.h>
#include <string.h>


#include "FreeRTOS.h"
#include "task.h"

#include "board.h"

#include "app_led.h"
#include "app_shell.h"
#include "app_gnss.h"
#include "app_packet.h"
#include "app_can_recv.h"
#include "app_can_send.h"
#include "app_at.h"
#include "app_lte.h"
#include "app_sms.h"
#include "app_gb4.h"






/***********************
**	主功能函数：
*************************/

int main(void)
{
	uint8_t step = 0;
	
	xTaskCreate(thread_entry_led,      "thread_entry_led",    	512,   NULL,   8,  NULL);  //创建LED处理任务
	xTaskCreate(thread_entry_shell,    "thread_entry_shell",  	512,   NULL,  12,  NULL);  //创建SHELL任务
  xTaskCreate(thread_entry_gnss, 		 "thread_entry_gnss",   	512,   NULL,   9,  NULL);  //创建GNSS处理任务
	xTaskCreate(thread_entry_packet,   "thread_entry_packet", 	512,   NULL,   13,  NULL);  //创建组包数据任务
	xTaskCreate(thread_entry_can_recv, "thread_entry_can_recv", 512,   NULL,  11,  NULL);  //创建CAN接收任务
  xTaskCreate(thread_entry_can_send, "thread_entry_can_send", 512,   NULL,  10,  NULL);  //创建GNSS处理任务
	xTaskCreate(thread_entry_lte,      "thread_entry_lte",      512,   NULL,   6,  NULL);  //创建LTE模块健康任务
  xTaskCreate(thread_entry_at, 			 "thread_entry_at",       512,   NULL,   14,  NULL);  //创建LTE命令解析AT命令
	xTaskCreate(thread_entry_sms, 		 "thread_entry_sms",      512,   NULL,   15,  NULL);  //创建LTE命令解析AT命令
	xTaskCreate(thread_entry_gb4, 		 "thread_entry_gb4",      512,   NULL,   16,  NULL);  //创建LTE命令解析AT命令
	
	
	for(;;)
	{
		switch(step)
		{
			case 0:
				step++;
				break;
			case 1:
	
				break;
		}
		
		vTaskDelay(1);
		rt_feed_iwdg();
	}
}

/**************** End File ***************/
