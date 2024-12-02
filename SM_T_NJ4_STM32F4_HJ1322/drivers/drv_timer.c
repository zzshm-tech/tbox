

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "stm32f4xx.h"




static void (*rt_irq_timer0_hook1)();
static void (*rt_irq_timer0_hook2)();
static void (*rt_irq_timer0_hook3)();
static void (*rt_irq_timer0_hook4)();       //


void rt_irq_tim3_sethook(void (*hook)(void))
{
	if(rt_irq_timer0_hook1 == NULL)
		rt_irq_timer0_hook1 = hook;
  else if (rt_irq_timer0_hook2 == NULL)
    rt_irq_timer0_hook2 = hook;
  else if (rt_irq_timer0_hook3 == NULL)
    rt_irq_timer0_hook3 = hook;
	else if(rt_irq_timer0_hook4 == NULL)
		rt_irq_timer0_hook4 = hook;
}


uint8_t rt_hw_init_tim3(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///使能TIM3时钟
	
  TIM_TimeBaseInitStructure.TIM_Period = 100 - 1; 	//自动重装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 8400 - 1;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//初始化TIM3
	
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //允许定时器3更新中断
	TIM_Cmd(TIM3,ENABLE); //使能定时器3
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //定时器3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	rt_irq_timer0_hook1 = NULL;
	rt_irq_timer0_hook2 = NULL;
	rt_irq_timer0_hook3 = NULL;
	rt_irq_timer0_hook4 = NULL;
	
	return 0;
}




void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //溢出中断
	{
		if(rt_irq_timer0_hook1 != NULL)
     rt_irq_timer0_hook1();

		if (rt_irq_timer0_hook2 != NULL)
			 rt_irq_timer0_hook2();

		if (rt_irq_timer0_hook3 != NULL)
			 rt_irq_timer0_hook3();
		
		if(rt_irq_timer0_hook4 != NULL)
			rt_irq_timer0_hook4();
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位
}


