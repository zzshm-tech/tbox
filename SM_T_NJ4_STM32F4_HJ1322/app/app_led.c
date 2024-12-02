
#include <stdio.h>
#include <stdint.h>


#include "FreeRTOS.h"
#include "task.h"

#include "drv_gpio.h"





/***********************************
**	指示LTE状态
**	成功连接到企业平台：3S闪烁周期
**	
************************************/

static void led_green_handle(uint32_t cnt)
{
	static  uint8_t state = 0;
	
	if(cnt % 50 == 0)
	{
		if(state == 0)
		{
			rt_led_green_on();
			state = 1;
		}
		else
		{
			rt_led_green_off();
			state = 0;
		}
	}
		
}




/****************************
 ** LED指示任务
 ****************************/

void thread_entry_led(void *parameter)
{
	uint32_t cnt = 0;

  parameter = parameter;
 
  for (;;)
  {
		cnt++;
    vTaskDelay(1);
		led_green_handle(cnt);
		
  }
}

