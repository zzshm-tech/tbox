



#include "drv_timer.h"

TIM_HandleTypeDef htim3;


/****************************
**	初始化定时器3
*****************************/

void TIM3_Init(void)
{
	TIM_ClockConfigTypeDef sClockSourceConfig;
	TIM_MasterConfigTypeDef sMasterConfig;
	htim3.Instance=TIM3;
	htim3.Init.Prescaler = 42000-1;//预分频值，本例使用外部8MHz，倍频后是32M，32000分频后为1kHz，预分频值为32000-1，计算公式为：CK_INT/(TIM_Perscaler+1)
	htim3.Init.CounterMode=TIM_COUNTERMODE_UP;//上升计数
	htim3.Init.Period= 20 - 1;//2000-1 1s;//计数值，1kHz即计数1000为1s，本例定时1s，计数值为1000-1
	htim3.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
	if(HAL_TIM_Base_Init(&htim3)!=HAL_OK)
	{
		rt_kprintf("HAL_TIM_Base_Init error.\n");
	}
	sClockSourceConfig.ClockSource=TIM_CLOCKSOURCE_INTERNAL;//设置定时器时钟为内部时钟
	if(HAL_TIM_ConfigClockSource(&htim3,&sClockSourceConfig)!=HAL_OK)
	{
		rt_kprintf("HAL_TIM_ConfigClockSource error.\n");
	}
	sMasterConfig.MasterOutputTrigger=TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode=TIM_MASTERSLAVEMODE_DISABLE;
	if(HAL_TIMEx_MasterConfigSynchronization(&htim3,&sMasterConfig)!=HAL_OK)
	{
		rt_kprintf("HAL_TIMEx_MasterConfigSynchronization error.\n");
	}
	HAL_TIM_Base_Start(&htim3);//启动定时器
	HAL_TIM_Base_Start_IT(&htim3);//启动定时器中断
}




/******************************
**
*******************************/


void HAL_TIM_Base_MspInit(TIM_HandleTypeDef*htim_base)
{
	if(htim_base->Instance==TIM3)
	{
		/*USERCODEBEGINTIM3_MspInit0*/
		/*USERCODEENDTIM3_MspInit0*/
		/*Peripheralclockenable*/
		__HAL_RCC_TIM3_CLK_ENABLE();
		/*TIM3interruptInit*/
		HAL_NVIC_SetPriority(TIM3_IRQn,0,0);
		HAL_NVIC_EnableIRQ(TIM3_IRQn);
		/*USERCODEBEGINTIM3_MspInit1*/
		/*USERCODEENDTIM3_MspInit1*/
	}
}
// 10ms
//void rt_hw_timer3_init(void)
//{
//	TIM3_Init();
//}

int rt_hw_timer_init(void)
{
//    rt_hw_timer3_init();
	TIM3_Init();
  return 0;
}

INIT_BOARD_EXPORT(rt_hw_timer_init);


static void (*rt_irq_timer3_hook1)();
static void (*rt_irq_timer3_hook2)();
static void (*rt_irq_timer3_hook3)();
static void (*rt_irq_timer3_hook4)();       //
static void (*rt_irq_timer3_hook5)();
static void (*rt_irq_timer3_hook6)();       //


void rt_irq_timer3_sethook(void (*hook)(void))
{
    if (rt_irq_timer3_hook1 == RT_NULL)
        rt_irq_timer3_hook1 = hook;
    else if (rt_irq_timer3_hook2 == RT_NULL)
        rt_irq_timer3_hook2 = hook;
    else if (rt_irq_timer3_hook3 == RT_NULL)
        rt_irq_timer3_hook3 = hook;
		else if(rt_irq_timer3_hook4 == RT_NULL)
				rt_irq_timer3_hook4 = hook;
		else if(rt_irq_timer3_hook5 == RT_NULL)
				rt_irq_timer3_hook5 = hook;
		else if(rt_irq_timer3_hook6 == RT_NULL)
				rt_irq_timer3_hook6 = hook;
    else 
    {
        rt_kprintf("rt_irq_timer3_sethook failed\r\n");
    }
}



/**********************************
**	注意这些函数
***********************************/

void rt_irq_timer3_resethook(void)
{
	rt_irq_timer3_hook1 = RT_NULL;
	rt_irq_timer3_hook2 = RT_NULL;
	rt_irq_timer3_hook3 = RT_NULL;
	rt_irq_timer3_hook4 = RT_NULL;
	rt_irq_timer3_hook5 = RT_NULL;
	rt_irq_timer3_hook6 = RT_NULL;
}




/*********************
**	
***********************/

void TIM3_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&htim3);
	
  if (rt_irq_timer3_hook1 != RT_NULL)
     rt_irq_timer3_hook1();

  if (rt_irq_timer3_hook2 != RT_NULL)
     rt_irq_timer3_hook2();

  if (rt_irq_timer3_hook3 != RT_NULL)
     rt_irq_timer3_hook3();
	
	if(rt_irq_timer3_hook4 != RT_NULL)
		rt_irq_timer3_hook4();
	
	if(rt_irq_timer3_hook5 != RT_NULL)
		rt_irq_timer3_hook5();
	
	if(rt_irq_timer3_hook6 != RT_NULL)
		rt_irq_timer3_hook6();
	
}
