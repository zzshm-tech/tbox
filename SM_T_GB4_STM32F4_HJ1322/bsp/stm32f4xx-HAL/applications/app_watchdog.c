

#include <rtthread.h>
#include <board.h>

#include "drv_watchdog.h"

/***********************
**
************************/

uint8_t feed_watchdog_fun(void)
{
	rt_device_t 		watchdog_dev = RT_NULL;       //看门狗设备
	uint32_t 				tmp = 0;
	
	watchdog_dev = rt_device_find("watchdog");
	if(watchdog_dev == NULL)
		return 1;
	
	rt_device_control(watchdog_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, &tmp);        //设置看门狗时间
	
	return 0;
}



/***********************
**	设置看门狗
************************/

uint8_t set_watchdog_cycle(uint8_t cycle)
{
	rt_device_t 		watchdog_dev = RT_NULL;       //看门狗设备
	uint32_t 				tmp = 0;

	tmp = cycle;
	
	watchdog_dev = rt_device_find("watchdog");
	rt_device_control(watchdog_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &tmp);        //设置看门狗时间
	
	return 0;
}




