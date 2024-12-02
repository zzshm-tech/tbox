


#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "drv_gpio.h"

#include "common.h"

#include "app_led.h"
#include "app_gnss.h"
#include "app_can_recv.h"
#include "app_gb4.h"
#include "app_lte.h"
#include "app_at.h"
#include "app_products.h"


/*********************************
**
**********************************/

static uint8_t app_led_control(led_list_e led, led_status_e status, uint32_t tick, uint32_t count)
{
    if (0 == tick % count)
    {
        rt_led_control(led, status);
		return 0;
    }

	return 1;
}




/***********************************
**	指示LTE状态
**	成功连接到企业平台：3S闪烁周期
**	
************************************/

static void led_green_handle(uint32_t led_ticks)
{
    static uint8_t flag = 0;
	static uint16_t  back = 0;

	if(read_gb4_socket_state() > 0)    //链接
	{
		//if(app_led_control(LED_G, LED_ON, led_ticks, 250)  == 0)
		//发送数据，快速闪烁绿色LED 5ms

		if(get_lte_send_num() != back)
		{
			flag = 20;
			back = get_lte_send_num();
		}

		if(flag > 1)
		{
			if(app_led_control(LED_G, LED_FLIP, led_ticks, 5) == 0)
			{
				flag--;
			}
			if(flag == 1)
			{
				app_led_control(LED_G, LED_OFF, 50, 50);
			}	
			return;
		}

		if(flag == 0)
		{
			if(app_led_control(LED_G, LED_OFF, led_ticks, 50) == 0)
			{
				flag = 1;
			}
		} 
		else
		{

			if(app_led_control(LED_G, LED_ON, led_ticks, 300)  == 0)
			{
				flag = 0;
			}
		}
	}
	else
	{
		if(read_lte_net_reg_state() > 0 || read_lte_csq() > 10)     //
		{
			app_led_control(LED_G, LED_FLIP, led_ticks, 50);
		}
		else
		{
			app_led_control(LED_G, LED_ON, led_ticks, 100);
		}
	}
	
}




/********************************
**	红色LED
*********************************/

static void led_red_handle(uint32_t led_ticks)
{
	if(read_can_connect_state() > 0)
	{
		app_led_control(LED_R, LED_FLIP, led_ticks, 5);
	}
	else
	{
		app_led_control(LED_R, LED_FLIP, led_ticks, 50);
	}
}



/****************************
**	蓝色LED灯 指示定位状态
****************************/

static void led_blue_handle(uint32_t led_ticks)
{
	if(read_gnss_ant_state() > 0)   //
	{
		app_led_control(LED_B, LED_FLIP, led_ticks, 5);
		return;
	}


	if(read_gnss_positing_state() == 'A')
	{
		app_led_control(LED_B, LED_ON, led_ticks,50) ;//
		return;
	}

	if(read_gnss_module_state() > 0)
	{
		app_led_control(LED_B, LED_FLIP, led_ticks, 50);
	}
	else
	{
		app_led_control(LED_B, LED_OFF, led_ticks, 50);
	}
	
}


/****************************
 ** LED指示任务
 ****************************/

void thread_entry_led(void *parameter)
{
    uint32_t cnt = 0;

    parameter = parameter;

	vTaskDelay(100);
    
    for (;;)
    {
		cnt++;
        vTaskDelay(1);
        if(get_products_cfg_model() == 0x55)
		{
			led_green_handle(cnt);
        	led_red_handle(cnt);
        	led_blue_handle(cnt);
		}
		else
		{
			app_led_control(LED_ALL, LED_FLIP, cnt, 50);
		}
    }
    
}









