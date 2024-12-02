

#include <stdint.h>
#include <stdio.h>
#include <string.h>


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


#include "app_sms.h"





static QueueHandle_t  			             sms_queue = NULL;           //¶ÌÏûÏ¢

static struct sms_down_str               sms_data = {0};			//





QueueHandle_t  get_sms_queue(void)
{
	return sms_queue;
}


/****************************
**
*****************************/

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
				printf("-- thread entry\r\n");
			}
		}
	}
}


