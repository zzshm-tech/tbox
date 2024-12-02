



#include "board.h"
#include "drv_ticks.h"
#include "drv_uart.h"
#include "drv_gpio.h"
#include "drv_rtc.h"
#include "drv_can.h"
#include "drv_timer.h"


#include "stm32f4xx.h"





/**************** 本地全局变量 *****************/



/*************************************
**	软件延时
**	
**************************************/

void delay_ms(unsigned int delayms)
{
    delayms = delayms > 10000UL ? 10000UL : delayms;

    delayms = 8000 * delayms;
    while(delayms--)
    {
        for(; delayms > 0; delayms--)
            asm("nop");
    }
}


/********************************
**	初始化
*********************************/

uint8_t  rt_hw_init_nvic(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x8000);	// 0x08042C00        0xE000
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;

	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;

	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;

	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);
	
	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;

	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;

	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);
	
	
	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x03;

	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;

	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);
	
	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x04;

	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;

	NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn; 
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;//抢占优先级1
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;//子优先级2
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
  NVIC_Init(&NVIC_InitStructure);//配置
	
	return 0;
}

/********************************
**	函数名称：uint8 WdtInit(INT16U time_ms)
**	功能描述：初始化看门狗的CPU设置，打开看门狗uint8 WdtInit(INT16U time_ms)
*********************************/

uint8_t rt_hw_init_iwdg(uint16_t sec)
{
	float tmp;
	
	if(sec > 25)
		sec = 0;
	
	sec *= 1000;
	
	if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
      RCC_ClearFlag();/* 如果IWDG复位,清除复位标志*/
    

  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);/*写入0x5555,去除写保护*/

  IWDG_SetPrescaler(IWDG_Prescaler_256);/*设置预分频值为256 即 40KHz(LSI) / 256 = 156.25Hz, (6.4ms)*/
    /*这个寄存器的值0x000-0xfff*/
	
	tmp = (sec - 6.4) / 6.4;
	
	sec = (unsigned short int) tmp;
	
  IWDG_SetReload(sec);		//填充装载寄存器值为:4000 当前独立看门狗的溢出时间为(4000+1)*6.4 =25.6064s*/
  IWDG_ReloadCounter();		//写入0xAAAA喂狗一次*/
  IWDG_Enable();
	
  return 0;
}




//喂独立看门狗
void rt_feed_iwdg(void)
{
  IWDG_ReloadCounter();//reload
}



/************************************
**
*************************************/


void rt_enter_critical(void)
{
  __set_FAULTMASK(1);
}


/************************************
**      退出临界段
*************************************/
void rt_exit_critical(void)
{
  __set_FAULTMASK(0);
}





/***************************************
**	初始化外部晶振 8MHz-- 168Mhz
****************************************/

uint8_t rt_hw_init_hse(void)

{
	RCC_DeInit();
	RCC_HSEConfig(RCC_HSE_ON);

	if(RCC_WaitForHSEStartUp() != SUCCESS)
		return 0;
	
	RCC_HCLKConfig(RCC_SYSCLK_Div1);                       //设置 AHB总线时钟等于系统时钟
  RCC_PCLK2Config(RCC_HCLK_Div2);                        //设置 APB2时钟等于系统时钟 / 2
  RCC_PCLK1Config(RCC_HCLK_Div4);                        //设置APB1时钟等于系统时钟/2
  RCC_PLLConfig(RCC_PLLSource_HSE,8,336,2,7);    								//设置锁相环倍频值 PLLCLK = 8MHZ * 9 =72MHZ
  RCC_PLLCmd(ENABLE);                                    //开启锁相环
	
	while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)      //等待锁相环启动完毕
	{
	}
  RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);             //Select PLL as system clock
  while(RCC_GetSYSCLKSource() != 0x08);                  //这里需要设置一个 跳出函数的变量
	
	return 1;
}








/******************************
**	初始化内部晶振 16MHz
*******************************/

void rt_hw_init_hsi(void)
{
 
}







/************************
**	软件重启系统
*************************/

void rt_reboot_sys(void)
{
  
}



/***************************
** 初始化BSP
****************************/

uint8_t rt_hw_init_board(void)
{
	rt_hw_init_hse();
	rt_hw_init_nvic();
	rt_hw_gpio_init();
	rt_hw_init_rtc();
	rt_hw_init_uart1();
	rt_hw_init_uart3();
	rt_hw_init_uart4();
	rt_hw_init_can(250);
	rt_hw_set_wakeup(0);
	rt_hw_init_tim3();
	//rt_hw_init_iwdg(25);
	
	return 0;
	
}


