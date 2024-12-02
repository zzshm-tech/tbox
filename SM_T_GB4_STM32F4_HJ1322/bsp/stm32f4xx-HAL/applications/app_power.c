

#include <rtthread.h>
#include <rtdevice.h>

#include "board.h"
#include "drv_gpio.h"
#include "drv_watchdog.h"
#include "drv_gpio.h"
#include "drv_rtc.h"
#include "app_lte.h"
#include "app_led.h"


#include "app_cmd.h"
#include "app_gnss.h"
#include "app_mon.h"




static rt_device_t 									watchdog_dev = RT_NULL;       //

static rt_uint8_t										sleep_work_flag = 0;          //


/*****************************
**
*****************************/

void acc_irq_call_back(void *args)
{
	sleep_work_flag = 1;
}


/*****************************
**
******************************/

void alarm_irq_call_back(void)
{
	sleep_work_flag = 1;
}





/*****************************
**	
*****************************/

void runing_to_sleep(void)
{
	
}


extern void RT_Close_GPIO(void);

/**********************************
**	设备低功耗处理任务
*************************************/

void thread_entry_low_power(void *parameter)			//
{
	rt_device_t rtc_dev;
//	struct rt_lte_event event;
	rt_uint8_t step = 0;
	rt_uint8_t state = 0;
	rt_uint32_t cnt = 0;
	rt_uint16_t sleep_time;
	
	parameter =  parameter;
	
	cnt = 20;
	watchdog_dev = rt_device_find("watchdog");
	rt_device_control(watchdog_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &cnt);        //设置看门狗时间
	rt_device_open(watchdog_dev, RT_DEVICE_OFLAG_RDWR);
	rt_pin_attach_irq(PIN_ACC_STATE,PIN_IRQ_MODE_RISING,acc_irq_call_back,RT_NULL);
	rt_pin_irq_enable(PIN_ACC_STATE,PIN_IRQ_ENABLE);
	rtc_dev = rt_device_find("rtc");
				
	for(;;)
	{
		rt_thread_delay(100);

		state = read_sys_run_status();            //暂时这样使用  ？？？
		switch(step)
		{
			case 0:
				if(read_acc_state() > 0)   
					break;
				sleep_time = read_config_sleep_time();
				step++;
				break;
			case 1:
				if(read_acc_state() > 0)
				{
					step = 0;
					break;
				}
				if(state == 0)            //预留5S，关闭网络
					break;
				cnt = 0;
				step++;
				break;
			case 2:
				//rt_kprintf("-- the sys status %d\r\n",cnt);
				if(cnt++ < 5)
					break;
				cnt = 25;
				rt_device_control(watchdog_dev,RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &cnt);  //重新初始化喂狗周期 
				sleep_work_flag = 0;
				step++;
				break;
			case 3:
				cnt = 20;
				rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_WKUP, &cnt);        //设置喂狗周期
				
				SysTick->CTRL  &= ~SysTick_CTRL_ENABLE_Msk;           //停止滴答时钟
			
				digital_close();                    //
			  rt_led_yellow_off();								//					
				rt_led_blue_off();									//
				rt_led_green_off();									//
				close_gnss_module();                //
				rt_rs232_power_off();
				rt_can1_stb_on();
				rt_can2_stb_on();
				rt_mem_power_on();
				rt_lte_power_on();
				RT_Close_GPIO();  //注意这个函数(重新写该函数)
			
				for(;;)
				{
					cnt = 20;
					rt_device_control(watchdog_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, &cnt);  //喂狗 
					
					HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);
					sleep_time -= 21;
					if(rt_read_acc_state() > 0 || sleep_work_flag > 0 || sleep_time < 21)         //电锁信号打开
					{
						rt_reboot_sys();            //系统重启
					}
				}
				
			default:
				step = 0;
				break;
		}
		rt_device_control(watchdog_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, &cnt);  //喂狗   
	}
}





