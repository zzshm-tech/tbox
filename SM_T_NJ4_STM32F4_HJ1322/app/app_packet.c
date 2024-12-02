
#include <stdio.h>
#include <string.h>
#include <stdint.h>


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "semphr.h"
#include "event_groups.h"

#include "drv_rtc.h"


/*************************/

static SemaphoreHandle_t  	packet_sph = NULL;  //二值信号量



void packet_call_back(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if(packet_sph != NULL)
		xSemaphoreGiveFromISR(packet_sph, &xHigherPriorityTaskWoken);

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}




void thread_entry_packet(void *parameter)
{
	uint32_t thread_cnt = 0;
	struct rt_tm tm;
	
	parameter = parameter;
	
	packet_sph = xSemaphoreCreateBinary();
	
	if(packet_sph == NULL)
  {
		printf("-- creat packet sph fail....\r\n");
  }
		
	rt_irq_wkup_sethook(packet_call_back);
			
	for(;;)
	{
		if(xSemaphoreTake(packet_sph, 500) == pdTRUE)
		{
			thread_cnt++;
			get_rtc_time(&tm);
			//printf("--the RTC: %d,%d,%d:%d-%d-%d    %d\r\n",tm.year,tm.mon,tm.day,tm.hour,tm.min,tm.sec,get_rtc_timestamp());

			//printf("-- shell %d\r\n",thread_cnt);
			continue;
		}
		//vTaskDelay(100);
		
		
		printf("-- SemaphoreTake Fail... %d\r\n",thread_cnt);
	}
}




