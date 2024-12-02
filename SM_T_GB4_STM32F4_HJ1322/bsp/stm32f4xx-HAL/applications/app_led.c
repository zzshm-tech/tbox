

/********************************
**
***2020.3.22 Creat
2020.3.22 添加任务
************************************/

#include <stdio.h>
#include <string.h>

#include <rtthread.h>


#include "app_led.h"
#include "drv_gpio.h"
#include "drv_timer.h"

#include "app_can_recv.h"
#include "app_products.h"
#include "app_gnss.h"
#include "app_mon.h"
#include "app_lte.h"
#include "app_shell.h"
#include "app_mon.h"
#include "app_at.h"





static rt_event_t 									led_event = RT_NULL;

static struct led_cnt_str 					led_cnt;

/*******************************
**	
*******************************/

void send_led_event(void)
{
	if(led_event != RT_NULL)
		rt_event_send(led_event,1);
}



/******************************************
**	色LED
** 快速闪烁：约50ms  ，CAN1，或者CAN2  没有连接
**	
uint8_t read_can_connect_state(uint8_t ch)
*******************************************/

void led_red_handle(uint32_t led_ticks)
{	
	if(read_config_car_type() == 0x3F)  //新能源车辆  2路CAN
	{
		if(read_can_connect_state(1) > 0 || read_can_connect_state(2) > 0)
		{
			if(led_ticks % 5 == 0)
				rt_led_red_flip();
			
			return;
		}
	}
	else           //燃油车，单路CAN
	{
		if(read_can_connect_state(1) > 0)
		{
			if(led_ticks % 5 == 0)
				rt_led_red_flip();
			
			return;
		}
	}
	
	if(read_input_shell_state() > 0)    //外壳 (红灯常亮)
	{
		rt_led_red_on();
		return;
	}
	
	if(read_input_ant_state() > 0)      //天线断开 (1秒周期闪烁)
	{
		if(led_cnt.red_cnt_off <= led_ticks)
		{
			led_cnt.red_cnt_off = led_ticks + 70;
			led_cnt.red_cnt_on = led_ticks + 30;
			rt_led_red_on();
		}
		else
		{
			if(led_ticks >= led_cnt.red_cnt_on)
			{
				rt_led_red_off();
			}
		}
		return;
	}
	
	rt_led_red_off();
	
}

/******************************************
**	黄色LED灯--指示网络状态
**	常亮：LTE模块处于开机状态
**	1S闪烁：LTE模块已经成功开机，且附着GPRS网络
**	3S闪烁：LTE模块已经连接到主网关地址
**	
*******************************************/

void led_yellow_handle(uint32_t led_ticks)
{
	static uint32_t  num = 0;
	static uint32_t  cnt = 0;
	
	
	if(read_lte_module_state() == RT_FALSE)
	{
		rt_led_yellow_on();           				//如果LTE模块开机失败，黄灯常亮
		return;
	}
	
	if(read_lte_sim_state() == 0)    //SIM卡故障或者附着网络
	{
		rt_led_yellow_on();   
		return;
	}
	
	if(read_lte_send_num() != num)      //发送状态
	{
		if(led_ticks % 5 == 0)
		{
			rt_led_yellow_flip();
			if(cnt++ >= 10)
			{
				num = read_lte_send_num();
				cnt = 0;
			}
		}
		
		return;
	}
	
	if(read_gb4_socket_state() == 1)           //连接到服务器网络状态
	{
		if(led_cnt.yellow_cnt_off <= led_ticks)
		{
			led_cnt.yellow_cnt_off = led_ticks + 270;
			led_cnt.yellow_cnt_on = led_ticks + 30;
			rt_led_yellow_on();
		}
		else
		{
			if(led_ticks >= led_cnt.yellow_cnt_on)
			{
				rt_led_yellow_off();
			}
		}
	}
	else                                           //已经成功连接到网络
	{
		if(led_cnt.yellow_cnt_off <= led_ticks)
		{
			led_cnt.yellow_cnt_off = led_ticks + 70;
			led_cnt.yellow_cnt_on = led_ticks + 30;
			rt_led_yellow_on();
		}
		else
		{
			if(led_ticks >= led_cnt.yellow_cnt_on)
			{
				rt_led_yellow_off();
			}
		}
	}
}



/************************************
**	接收到一条nema数据，闪烁一下蓝色LED
***************************************/

void led_blue_handle(uint32_t led_ticks)
{
	static rt_uint32_t tmp = 0;
	static rt_uint32_t cnt = 0;
	

	if(read_gnss_nmea_cnt() != tmp)
		{
			rt_led_blue_on();
			tmp =  read_gnss_nmea_cnt(); //
			cnt = led_ticks + 30;
			return;
		}
		
    if(read_gnss_positing_state() != 'A' && cnt <= led_ticks)
    {
				rt_led_blue_off();
    }
}


/************************************
**	绿灯快速闪烁：发送数据
*************************************/

void led_green_handle(rt_int32_t led_ticks)
{
	if(read_config_state() == 0x56)
	{
		if(led_ticks % 5 == 0)
			rt_led_green_flip();
		return;
	}
	

	if(led_cnt.green_cnt_off <= led_ticks)
	{
		led_cnt.green_cnt_off = led_ticks + 70;
		led_cnt.green_cnt_on = led_ticks + 30;
		rt_led_green_on();
	}
	else
	{
		if(led_ticks >= led_cnt.green_cnt_on)
		{
			rt_led_green_off();
		}
	}	
}



/**************************
**	LED线程
***************************/

void thread_entry_led(void *parameter)
{
	rt_int32_t led_ticks = 0;
	rt_err_t 				res = RT_EOK;
	rt_uint32_t 		recved = 0;
	
	parameter = parameter;
	
	led_event = rt_event_create("led", 1);
	if(led_event != RT_NULL)
	{
		//rt_kprintf("--Create len event :%d\r\n",led_ticks);
	}
	rt_irq_timer3_sethook(send_led_event); 
	memset((char *)&led_cnt,0,sizeof(led_cnt));
	
	for(;;)
	{
		res = rt_event_recv(led_event, 1, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &recved);
    if (res != RT_EOK)
		{
			rt_kprintf("rt_event_recv failed\r\n");   //打印错误信息
			continue;  //
		}
		
		led_ticks++;
		
		led_yellow_handle(led_ticks);   //黄色LED-指示网络连接
		led_blue_handle(led_ticks);     //蓝色LED-指示定位状态
		led_green_handle(led_ticks);    //绿色LED-指示运行状态
		led_red_handle(led_ticks);      //红色LED-指示报警状态
	}
}



/*******************************
**	
********************************/

void close_led(void)
{
	
}
