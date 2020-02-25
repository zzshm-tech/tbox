/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      first implementation
 * 2013-11-15     bright       add RCC initial and print RCC freq function
 */


/*
ÉèÖÃÊ±ÖÓÁ÷³Ì£º
1¡¢½«RCC¼Ä´æÆ÷ÖØÐÂÉèÖÃÎªÄ¬ÈÏÖµ  RCC_DeInit
2¡¢´ò¿ªÍâ²¿¸ßËÙÊ±ÖÓ¾§ÕñHSE    RCC_HSEConfig(RCC_HSE_ON);
3¡¢µÈ´ýÍâ²¿¸ßËÙÊ±ÖÓ¾§Õñ¹¤×÷    HSEStartUpStatus = RCC_WaitForHSEStartUp();
4¡¢ÉèÖÃPLL              RCC_PLLConfig
5¡¢´ò¿ªPLL              RCC_PLLCmd(ENABLE);
6¡¢ÉèÖÃÏµÍ³Ê±ÖÓ          RCC_SYSCLKConfig
7¡¢ÉèÖÃAHBÊ±ÖÓ           RCC_HCLKConfig;
8¡¢ÉèÖÃµÍËÙËÙAHBÊ±ÖÓ      RCC_PCLK1Config
9¡¢ÉèÖÃ¸ßËÙAHBÊ±ÖÓ        RCC_PCLK2Config;
10¡¢µÈ´ýPLL¹¤×÷          while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) 
11¡¢ÅÐ¶ÏÊÇ·ñPLLÊÇÏµÍ³Ê±ÖÓ  while(RCC_GetSYSCLKSource() != 0x08)
12¡¢´ò¿ªÒªÊ¹ÓÃµÄÍâÉèÊ±ÖÓ   RCC_APB2PeriphClockCmd()/RCC_APB1PeriphClockCmd£¨£©
¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª
°æÈ¨ÉùÃ÷£º±¾ÎÄÎªCSDN²©Ö÷¡¸Ocarina_123¡¹µÄÔ­´´ÎÄÕÂ£¬×ñÑ­ CC 4.0 BY-SA °æÈ¨Ð­Òé£¬×ªÔØÇë¸½ÉÏÔ­ÎÄ³ö´¦Á´½Ó¼°±¾ÉùÃ÷¡£
Ô­ÎÄÁ´½Ó£ºhttps://blog.csdn.net/u010659754/article/details/88933141

*/



/*

void HSE_sysclock_config( uint32_t  RCC_PLLMul_x )
{
    RCC_DeInit();    //ÏÈ¸´Î»RCC¼Ä´æÆ÷
    RCC_HSEConfig( RCC_HSE_ON );    //Ê¹ÄÜHSE
    //¼ì²âHSEÊÇ·ñÆô¶¯³É¹¦
    if ( SUCCESS == RCC_WaitForHSEStartUp() )
    {
        //Ê¹ÄÜÔ¤È¡Ö¸£¬ÕâÊÇFLASH¹Ì¼þÖÐµÄº¯Êý
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
        //ÉèÖÃFLASHµÈ´ýÖÜÆÚ¡£  ÒòÎª±¶Æµ³É72M ËùÒÔµÈ´ýÁ½¸öÖÜÆÚ¡£
        FLASH_SetLatency(FLASH_Latency_2);  
        //ÅäÖÃÈý¸ö×ÜÏßµÄ±¶ÆµÒò×Ó
        //HCLK --> AHB ×î´óÎª72M£¬ËùÒÔÖ»ÐèÒª1·ÖÆµ
        RCC_HCLKConfig(RCC_SYSCLK_Div1);
        //PCLK1 --> APB1 ×î´óÎª36M£¬ËùÒÔÒª2·ÖÆµ
		RCC_PCLK1Config(RCC_HCLK_Div2);
        //PCLK2 --> APB2 ×î´óÎª72M£¬ËùÒÔÖ»ÐèÒª1·ÖÆµ
		RCC_PCLK2Config(RCC_HCLK_Div1);
        //ÏÈÅäÖÃËøÏà»· PLLCLK = HSE * ±¶ÆµÒò×Ó
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_x);
        RCC_PLLCmd(ENABLE);        //Ê¹ÄÜPLL
        while ( RESET == RCC_GetFlagStatus(RCC_FLAG_PLLRDY) );  //µÈ´ýPLLÎÈ¶¨
        //Ñ¡ÔñÏµÍ³Ê±ÖÓ£¨Ñ¡ÔñËøÏà»·Êä³ö£©
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        //µÈ´ýÑ¡ÔñÎÈ¶¨
        while ( 0x08 != RCC_GetSYSCLKSource() );
    }
    else
    {
        //HSEÆô¶¯Ê§°Ü
    }

}
¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª
°æÈ¨ÉùÃ÷£º±¾ÎÄÎªCSDN²©Ö÷¡¸Ocarina_123¡¹µÄÔ­´´ÎÄÕÂ£¬×ñÑ­ CC 4.0 BY-SA °æÈ¨Ð­Òé£¬×ªÔØÇë¸½ÉÏÔ­ÎÄ³ö´¦Á´½Ó¼°±¾ÉùÃ÷¡£
Ô­ÎÄÁ´½Ó£ºhttps://blog.csdn.net/u010659754/article/details/88933141


*/



#define STM32_ALARM_TIMEOUT 20



#include "FreeRTOS.h"           //×¢Òâ
#include "stm32f10x.h"
#include "board.h"
#include "fr_drv_uart.h"
#include "fr_drv_gpio.h"
#include "fr_drv_timer.h"
#include "fr_drv_rtc.h"
#include "fr_drv_mem.h"

/**
 * @addtogroup STM32
 */

/*@{*/


/*************************************
**	Èí¼þÑÓÊ±
**************************************/

void delay_ms(unsigned int delayms)
{
    delayms = delayms > 10000UL ? 10000UL : delayms;

    delayms = 8000 * delayms;
    while(delayms--)
    {
        for(; delayms > 0; delayms--)
            __nop();
    }
}



/********************************
**	º¯ÊýÃû³Æ£ºuint8 WdtInit(INT16U time_ms)
**	¹¦ÄÜÃèÊö£º³õÊ¼»¯¿´ÃÅ¹·µÄCPUÉèÖÃ£¬´ò¿ª¿´ÃÅ¹·uint8 WdtInit(INT16U time_ms)
*********************************/

unsigned char fr_init_wdt(unsigned short int sec)
{
	float tmp;
	
	if(sec > 25)
		sec = 0;
	
	sec *= 1000;
	
	if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
      RCC_ClearFlag();/* Èç¹ûIWDG¸´Î»,Çå³ý¸´Î»±êÖ¾*/
    

  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);/*Ð´Èë0x5555,È¥³ýÐ´±£»¤*/

  IWDG_SetPrescaler(IWDG_Prescaler_256);/*ÉèÖÃÔ¤·ÖÆµÖµÎª256 ¼´ 40KHz(LSI) / 256 = 156.25Hz, (6.4ms)*/
    /*Õâ¸ö¼Ä´æÆ÷µÄÖµ0x000-0xfff*/
	
	tmp = (sec - 6.4) / 6.4;
	
	sec = (unsigned short int) tmp;
	
  IWDG_SetReload(sec);/*Ìî³ä×°ÔØ¼Ä´æÆ÷ÖµÎª:4000 µ±Ç°¶ÀÁ¢¿´ÃÅ¹·µÄÒç³öÊ±¼äÎª(4000+1)*6.4 =25.6064s*/
  IWDG_ReloadCounter();/*Ð´Èë0xAAAAÎ¹¹·Ò»´Î*/
  IWDG_Enable();	/*Ð´Èë0xCCCC¿ªÆô¶ÀÁ¢¹·*/
		
	return 0;
}





/********************
**	ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½:
**	ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½:
*********************/
void fr_init_hse_72mhz(void)
{
	RCC_DeInit();                                          //ï¿½ï¿½Ê¼ï¿½ï¿½ÎªÈ±Ê¡Öµ
  RCC_HSEConfig(RCC_HSE_ON);                             //Ê¹ï¿½ï¿½ï¿½â²¿ï¿½Ä¸ï¿½ï¿½ï¿½Ê±ï¿½ï¿½
  while(RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET) {}; //ï¿½È´ï¿½ï¿½â²¿ï¿½ï¿½ï¿½ï¿½Ê±ï¿½ï¿½Ê¹ï¿½Ü¾ï¿½ï¿½ï¿½
	FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);  //Ê¹ï¿½ï¿½Ô¤È¡Ö¸ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½FLASHï¿½Ì¼ï¿½ï¿½ÐµÄºï¿½ï¿½ï¿½
	FLASH_SetLatency(FLASH_Latency_2);    //ï¿½ï¿½ï¿½ï¿½FLASHï¿½È´ï¿½ï¿½ï¿½ï¿½Ú¡ï¿½  ï¿½ï¿½Îªï¿½ï¿½Æµï¿½ï¿½72M ï¿½ï¿½ï¿½ÔµÈ´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ú¡ï¿½
		// 8M/2*16=64M RCC_PLLMul_x = RCC_PLLMul_16
  RCC_HCLKConfig(RCC_SYSCLK_Div1);                       //ï¿½ï¿½ï¿½ï¿½ AHBï¿½ï¿½ï¿½ï¿½Ê±ï¿½Óµï¿½ï¿½ï¿½ÏµÍ³Ê±ï¿½ï¿½
  RCC_PCLK2Config(RCC_HCLK_Div1);                        //ï¿½ï¿½ï¿½ï¿½ APB2Ê±ï¿½Óµï¿½ï¿½ï¿½ÏµÍ³Ê±ï¿½ï¿½
  RCC_PCLK1Config(RCC_HCLK_Div2);                        //ï¿½ï¿½ï¿½ï¿½APB1Ê±ï¿½Óµï¿½ï¿½ï¿½ÏµÍ³Ê±ï¿½ï¿½/2
  RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);   //ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½à»·ï¿½ï¿½ÆµÖµ PLLCLK = 8MHZ * 9 =72MHZ
  RCC_PLLCmd(ENABLE);                                    //ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½à»·

  while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) {}; //ï¿½È´ï¿½ï¿½ï¿½ï¿½à»·ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?  RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);             //Select PLL as system clock
  RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);             //Select PLL as system clock
	while(RCC_GetSYSCLKSource() != 0x08);                  //Wait till PLL is used as system clock source	
}




/***********************
**	ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½:
**	ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½:
************************/
void fr_init_hse_8mhz(void)
{
    RCC_DeInit();                                          			//ï¿½ï¿½Ê¼ï¿½ï¿½ÎªÈ±Ê¡Öµ
    RCC_HSEConfig(RCC_HSE_ON);                             			//Ê¹ï¿½ï¿½ï¿½â²¿ï¿½Ä¸ï¿½ï¿½ï¿½Ê±ï¿½ï¿½
    while(RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET) {}; 			//ï¿½È´ï¿½ï¿½â²¿ï¿½ï¿½ï¿½ï¿½Ê±ï¿½ï¿½Ê¹ï¿½Ü¾ï¿½ï¿½ï¿½
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);  //Ê¹ï¿½ï¿½Ô¤È¡Ö¸ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½FLASHï¿½Ì¼ï¿½ï¿½ÐµÄºï¿½ï¿½ï¿½
		FLASH_SetLatency(FLASH_Latency_0);    //ï¿½ï¿½ï¿½ï¿½FLASHï¿½È´ï¿½ï¿½ï¿½ï¿½Ú¡ï¿½  ï¿½ï¿½Îªï¿½ï¿½Æµï¿½ï¿½72M ï¿½ï¿½ï¿½ÔµÈ´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ú¡ï¿½	
    RCC_HCLKConfig(RCC_SYSCLK_Div1);                       			//ï¿½ï¿½ï¿½ï¿½ AHBï¿½ï¿½ï¿½ï¿½Ê±ï¿½Óµï¿½ï¿½ï¿½ÏµÍ³Ê±ï¿½ï¿½
    RCC_PCLK2Config(RCC_HCLK_Div1);                        			//ï¿½ï¿½ï¿½ï¿½ APB2Ê±ï¿½Óµï¿½ï¿½ï¿½ÏµÍ³Ê±ï¿½ï¿½
    RCC_PCLK1Config(RCC_HCLK_Div2);                        			//ï¿½ï¿½ï¿½ï¿½APB1Ê±ï¿½Óµï¿½ï¿½ï¿½ÏµÍ³Ê±ï¿½ï¿½/2
		RCC_SYSCLKConfig(RCC_SYSCLKSource_HSE);  	
			
    while(RCC_GetSYSCLKSource() != 0x04);                  			//Wait till PLL is used as system clock source 
		
}



/******************************
**
*********************************/

void fr_init_hsi_64mhz(void)
{
	__IO uint32_t HSIStatus = 0;
    
  RCC_DeInit();    //ï¿½È¸ï¿½Î»RCCï¿½Ä´ï¿½ï¿½ï¿½
    
  RCC_HSICmd(ENABLE);   //Ê¹ï¿½ï¿½HSI
    
  HSIStatus = RCC->CR & RCC_CR_HSIRDY;    //ï¿½ï¿½ï¿½HSEï¿½Ç·ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½É¹ï¿½
    
	if ( RCC_CR_HSIRDY == HSIStatus )
  {
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);  //Ê¹ï¿½ï¿½Ô¤È¡Ö¸ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½FLASHï¿½Ì¼ï¿½ï¿½ÐµÄºï¿½ï¿½ï¿½
		FLASH_SetLatency(FLASH_Latency_2);    //ï¿½ï¿½ï¿½ï¿½FLASHï¿½È´ï¿½ï¿½ï¿½ï¿½Ú¡ï¿½  ï¿½ï¿½Îªï¿½ï¿½Æµï¿½ï¿½72M ï¿½ï¿½ï¿½ÔµÈ´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ú¡ï¿½
		// 8M/2*16=64M RCC_PLLMul_x = RCC_PLLMul_16
		RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_16); //ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½à»· PLLCLK = HSI * ï¿½ï¿½Æµï¿½ï¿½ï¿½ï¿½
		//ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ßµÄ±ï¿½Æµï¿½ï¿½ï¿½ï¿½
		// HCLK = SYSCLK AHB 36M
		RCC_HCLKConfig(RCC_SYSCLK_Div1);//HCLK --> AHB ï¿½ï¿½ï¿½Î?2Mï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö»ï¿½ï¿½Òª1ï¿½ï¿½Æµ
		RCC_PCLK1Config(RCC_HCLK_Div2); //PCLK1 --> APB1 ï¿½ï¿½ï¿½Î?6Mï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Òª2ï¿½ï¿½Æµ	
		RCC_PCLK2Config(RCC_HCLK_Div1); //PCLK2 --> APB2 ï¿½ï¿½ï¿½Î?2Mï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö»ï¿½ï¿½Òª1ï¿½ï¿½Æµ
		RCC_PLLCmd(ENABLE); //Ê¹ï¿½ï¿½PLL
		// Wait till PLL is ready
		while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET); //ï¿½È´ï¿½PLLï¿½È¶ï¿½
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK); //Ñ¡ï¿½ï¿½ÏµÍ³Ê±ï¿½Ó£ï¿½Ñ¡ï¿½ï¿½ï¿½ï¿½ï¿½à»·ï¿½ï¿½ï¿½ï¿½ï¿?		// Wait till PLL is used as system clock source
		while (RCC_GetSYSCLKSource() != 0x08); //ï¿½È´ï¿½Ñ¡ï¿½ï¿½ï¿½È¶ï¿½
		//RCC_GetClocksFreq(&RCC_ClockFreq);//ï¿½é¿´Æµï¿½ï¿½Öµ
	}
	else
	{
			//HSIï¿½ï¿½ï¿½ï¿½Ê§ï¿½ï¿½
	}
}



/******************************
**	Ê¹ï¿½ï¿½ï¿½Ú²ï¿½8MHzï¿½ï¿½ï¿½ï¿½
*********************************/

void fr_init_hsi_8mhz(void)
{
	__IO uint32_t HSIStatus = 0;
    
  RCC_DeInit();    //ï¿½È¸ï¿½Î»RCCï¿½Ä´ï¿½ï¿½ï¿½
    
  RCC_HSICmd(ENABLE);  //Ê¹ï¿½ï¿½HSI
    //ï¿½ï¿½ï¿½HSEï¿½Ç·ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½É¹ï¿½
  HSIStatus = RCC->CR & RCC_CR_HSIRDY;
  
	if (RCC_CR_HSIRDY == HSIStatus )
  {
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);  //Ê¹ï¿½ï¿½Ô¤È¡Ö¸ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½FLASHï¿½Ì¼ï¿½ï¿½ÐµÄºï¿½ï¿½ï¿½
		FLASH_SetLatency(FLASH_Latency_0);    //ï¿½ï¿½ï¿½ï¿½FLASHï¿½È´ï¿½ï¿½ï¿½ï¿½Ú¡ï¿½  ï¿½ï¿½Îªï¿½ï¿½Æµï¿½ï¿½72M ï¿½ï¿½ï¿½ÔµÈ´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ú¡ï¿½
		
		RCC_HCLKConfig(RCC_SYSCLK_Div1);//HCLK --> AHB ï¿½ï¿½ï¿½Î?2Mï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö»ï¿½ï¿½Òª1ï¿½ï¿½Æµ
		RCC_PCLK1Config(RCC_HCLK_Div2); //PCLK1 --> APB1 ï¿½ï¿½ï¿½Î?6Mï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Òª2ï¿½ï¿½Æµ	
		RCC_PCLK2Config(RCC_HCLK_Div1); //PCLK2 --> APB2 ï¿½ï¿½ï¿½Î?2Mï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö»ï¿½ï¿½Òª1ï¿½ï¿½Æµ
		//RCC_PLLCmd(ENABLE); //Ê¹ï¿½ï¿½PLL
		// Wait till PLL is ready
		while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET); //ï¿½È´ï¿½PLLï¿½È¶ï¿½
		RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI); //Ñ¡ï¿½ï¿½ÏµÍ³Ê±ï¿½Ó£ï¿½Ñ¡ï¿½ï¿½ï¿½ï¿½ï¿½à»·ï¿½ï¿½ï¿½ï¿½ï¿?		// Wait till PLL is used as system clock source
		while (RCC_GetSYSCLKSource() != 0x00); //ï¿½È´ï¿½Ñ¡ï¿½ï¿½ï¿½È¶ï¿½
		//RCC_GetClocksFreq(&RCC_ClockFreq);//ï¿½é¿´Æµï¿½ï¿½Öµ
	}
	else
	{
		//HSIï¿½ï¿½ï¿½ï¿½Ê§ï¿½ï¿½
	}
}










/***************************
**	³õÊ¼»¯PVD
*****************************/

void fr_init_pvd(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;

    /* Enable PWR and BKP clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

    /* Configure EXTI Line to generate an interrupt on falling edge */
    /* Configure EXTI Line16(PVD Output) to generate an interrupt on rising and falling edges */

	EXTI_DeInit();                                            /*½«EXIT¼Ä´æÆ÷ÖØÉèÖÃÎªÈ±Ê¡Öµ*/
	
  EXTI_InitStructure.EXTI_Line = EXTI_Line16;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

    /* Configure the PVD Level to 2.9V */
  PWR_PVDLevelConfig(PWR_PVDLevel_2V9);
    /* Enable the PVD Output */
  PWR_PVDCmd(ENABLE);
  EXTI_ClearITPendingBit(EXTI_Line16);
}



/***************************
**	³õÊ¼»¯WKUP
****************************/

void fr_init_wkup(void)
{
    EXTI_InitTypeDef  EXTI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    EXTI_InitStructure.EXTI_Line = EXTI_Line0 ;		            /*ÖÐ¶ÏÏß*/
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt ;      /*ÖÐ¶ÏÄ£Ê½*/
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising ;   /*ÉÏÉýÑØ´¥·¢*/
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
	
    EXTI_InitStructure.EXTI_Line = EXTI_Line17;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

		EXTI_ClearITPendingBit(EXTI_Line17);

    RTC_WaitForLastTask();
    //RTC_ITConfig(RTC_IT_ALR, ENABLE);
		RTC_ITConfig(RTC_IT_SEC, ENABLE); //Ê¹ÄÜ RTC ÃëÖÐ¶Ï
    RTC_WaitForLastTask();	   				//¿ªÆôÄÖÖÓÖÐ¶Ï
}



/*******************************************************************************
* Function Name  : NVIC_Configuration
* Description    : Configures Vector Table base location.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void fr_nvic_config(void)
{
NVIC_InitTypeDef NVIC_InitStructure;

   //NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0xE000);	// 0x08042C00        0xE000
   NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);/* 4 bits for Preemption Priority*/

   NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);

   NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);
	
	 NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);
	
	 NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);

   NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);

   NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);

	 NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);
	 
	 	NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);
}

/**
* @brief  Inserts a delay time.
* @param  nCount: specifies the delay time length.
* @retval None
*/
void Delay(__IO uint32_t nCount)
{
	/* Decrement nCount value */
	while (nCount != 0)
	{
		nCount--;
	}
}


/**
 * print RCC freq information
 *
 * for example:
 *
 * SYSCLK_Frequency is 48000000HZ
 * PCLK_Frequency is 48000000HZ
 * HCLK_Frequency is 48000000HZ
 * CECCLK_Frequency is 32786HZ
 * ADCCLK_Frequency is 14000000HZ
 * USART1CLK_Frequency is 48000000HZ
 * I2C1CLK_Frequency is 8000000HZ
 * SystemCoreClock is 48000000HZ
 *
 */

/**
 * This is the timer interrupt service routine.
 *
 */


/************************
**
**************************/

void fr_reset_sys(void)
{
	NVIC_SystemReset();
}




/******************************
 ** This function will initial STM32 board.
 ******************************/
void fr_init_board(void)
{
	fr_init_pvd();
	
	if(PWR_GetFlagStatus(PWR_FLAG_PVDO) == 0)
	{
		fr_set_console("uart1");
		fr_gpio_init();
		fr_nvic_config();
		fr_timer3_init();
		fr_init_fram(); 
			
	}
}









/******************************
**	
*******************************/
void fr_wkup_idle(void)
{
	EXTI_InitTypeDef  EXTI_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
  PWR_BackupAccessCmd(ENABLE);

  EXTI_ClearITPendingBit(EXTI_Line17);
  EXTI_InitStructure.EXTI_Line = EXTI_Line17;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = DISABLE;
  EXTI_Init(&EXTI_InitStructure);

  EXTI_ClearITPendingBit(EXTI_Line17);

  RTC_WaitForLastTask();
  RTC_ITConfig(RTC_IT_ALR, DISABLE);
  RTC_WaitForLastTask();	    /*--¿ªÆôÄÖÖÓÖÐ¶Ï--*/
}


/******************************
 ** ¹¤×÷Ä£Ê½½øÈëÐÝÃßÄ£Ê½
******************************/
void fr_enter_sleep(void)
{
	unsigned int tmp;

	fr_init_wkup();
	tmp = RTC_GetCounter();				//
  fr_set_rtc_alarm(tmp + STM32_ALARM_TIMEOUT);		//¶ÀÁ¢¿´ÃÅ¹·µÄ×î´óÎ¹¹·¼ä¸ô25s*    //   STM32_ALARM_TIMEOUT
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;   //¹Ø±ÕµÎ´ðÊ±ÖÓ
  PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI); 			//½øÈëÍ£Ö¹Ä£Ê½
}


/******************************
**	Ë¯ÃßÄ£Ê½½øÈëÐÝÃßÄ£Ê½
*******************************/

void fr_exit_sleep(void)
{
	fr_init_hse_72mhz();
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; 						 // Ê¹ÄÜµÎ´ð¶¨Ê±Æ÷ 
  fr_wkup_idle();
}



/******************************
**	Î¹¹·
*******************************/

void fr_reset_iwdg(void)
{
	IWDG_ReloadCounter();
}


/*@}*/
