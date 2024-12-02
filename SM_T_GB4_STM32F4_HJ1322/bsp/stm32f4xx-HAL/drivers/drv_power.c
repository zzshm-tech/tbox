


#include <stdio.h>
#include <stdint.h>

#include "board.h"
#include "drv_power.h"


/*****************************
**	进入休眠模式
******************************/

uint8_t rt_enter_sleep_model(void)
{
	SysTick->CTRL  &= ~SysTick_CTRL_ENABLE_Msk;           //停止滴答时钟
	HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);       //进入睡眠
				
	return 0;
}



/*******************************
**	退出休眠模式
********************************/

uint8_t rt_exit_sleep_model(void)
{
	rt_hw_sys_clock_init();                                    //初始化系统失踪
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; 						 // 使能滴答定时器
				
	return 0;
}



