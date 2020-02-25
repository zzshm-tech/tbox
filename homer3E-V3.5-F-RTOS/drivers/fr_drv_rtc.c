

#include <stdio.h>
#include <string.h>

#include "stm32f10x.h"

#include "board.h"




static void (*fr_rtc_hook)();


/*************************************
**	软件延时
**************************************/

static void fr_rtc_delay_ms(unsigned int delayms)
{
    delayms = delayms > 10000UL ? 10000UL : delayms;

    delayms = 8000 * delayms;
    while(delayms--)
    {
        for(; delayms > 0; delayms--)
            __nop();
    }
}


/**************************************
**	初始化RTC内部晶振
**************************************/

unsigned char fr_init_rtc_lsi(void)
{
	unsigned short int temp=0;
	
	BKP_DeInit();
	RCC_LSICmd(ENABLE);
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
	//等待稳定
  while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET && temp < 1000)
	{
		temp++;
		delay_ms(10);
	}
	if(temp >= 1000)
		return 1;//初始化时钟失败,晶振有问
   //RTC开启
   RCC_RTCCLKCmd(ENABLE);
		//开启后需要等待APB1时钟与RTC时钟同步，才能读写寄存器
   RTC_WaitForSynchro();
    //读写寄存器前，要确定上一个操作已经结束
   RTC_WaitForLastTask();
    //设置RTC分频器，使RTC时钟为1Hz
    //RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1)
   RTC_SetPrescaler(39999);
   //等待寄存器写入完成
   RTC_WaitForLastTask();
	 RTC_ExitConfigMode(); 							//退出配置模式
   //使能秒中断
   //RTC_ITConfig(RTC_IT_SEC, ENABLE);
   RTC_ITConfig(RTC_IT_ALR, ENABLE);
   //等待写入完成
   RTC_WaitForLastTask();
	 
	 return 0;
}







/***********************************
**	初始化RTC外部晶振
************************************/

unsigned char fr_init_rtc_lse(void)
{
	unsigned short int temp = 0;
	
	BKP_DeInit(); 
	RCC_LSEConfig(RCC_LSE_ON); //设置外部低速晶振(LSE)
	while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET && temp < 1000)
	{
		temp++;
		delay_ms(10);
	}
	if(temp >= 1000)
	{
		return 1;//初始化时钟失败,晶振有问题 
	}
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE); //设置 RTC 时钟
		
	RCC_RTCCLKCmd(ENABLE); 							//使能 RTC 时钟
	RTC_WaitForLastTask(); 							//等待最近一次对 RTC 寄存器的写操作完成
	RTC_WaitForSynchro(); 							//等待 RTC 寄存器同步
	RTC_ITConfig(RTC_IT_SEC, ENABLE); 	//使能 RTC 秒中断
	RTC_WaitForLastTask(); 							//等待最近一次对 RTC 寄存器的写操作完成
	RTC_EnterConfigMode(); 							// 允许配置
	RTC_SetPrescaler(32767); 						//设置 RTC 预分频的值
	RTC_WaitForLastTask(); 							//等待最近一次对 RTC 寄存器的写操作完成
	RTC_ExitConfigMode(); 							//退出配置模式
	//RTC_ITConfig(RTC_IT_SEC, ENABLE);
  RTC_ITConfig(RTC_IT_ALR, ENABLE);
   //等待写入完成
  RTC_WaitForLastTask();
	
	return 0; //ok
	
}


/*************************************
**	函数名称:
**	功能描述:
**************************************/
void fr_init_rtc(void)
{
    unsigned short int temp = 0;

		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE); 	//启用PWR和BKP的时钟(from APB1)
    PWR_BackupAccessCmd(ENABLE);            																	//后备域解锁
		
    if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
    {
			if(fr_init_rtc_lse() == 0)
			{
				//DebugPrintf(">RTC LSE OSC Init OK......\r\n");
			}
			
			else if(fr_init_rtc_lsi() == 0)
			{
				//DebugPrintf(">RTC LSI RC Init OK......\r\n");
			}
			else
			{
				//DebugPrintf("RTC Init Fail............\r\n");
			}
      BKP_WriteBackupRegister(BKP_DR1, 0xA5A5); //配置完成后，向后备寄存器中写特殊字符0xA5A5
    }
    else 			//若后备寄存器没有掉电，则无需重新配置RTC
    {
        if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)  //*这里我们可以利用RCC_GetFlagStatus()函数查看本次复位类型*/
        {
           //DebugPrintf("> Power On Reset occurred....\r\n");
        }
        else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
        {
            //DebugPrintf("> External Reset occurred....\r\n");
        }
				//DebugPrintf("> No Need To configure Rtc....\r\n");
        RCC_ClearFlag();		                      //清除RCC中复位标志*/
        temp = 0;
        while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
        {
            temp++;
            //DelayMS(10);
						fr_rtc_delay_ms(10);
            if(temp >= 300)
            {
                //DebugPrintf(">RTC_LSE Not Available \r\n");//	等待稳定
                return;
            }
        }
				
				RTC_WaitForSynchro(); //等待最近一次对 RTC 寄存器的写操作完成
				RTC_ITConfig(RTC_IT_SEC,DISABLE); //使能 RTC 秒中断
				RTC_WaitForLastTask(); //等待最近一次对 RTC 寄存器的写操作完成
		}
}




/***********************
**	设置RTC
************************/

void fr_set_rtc(unsigned int n)
{
	RTC_WaitForLastTask();
	RTC_SetCounter(n);
	RTC_WaitForLastTask();	
}


/***********************
**	读取RTC
************************/

unsigned int fr_read_rtc(void)
{
	unsigned int rv;
	
	rv =  RTC_GetCounter();	
	
	return rv;
}



/*************************
**	设置休眠时间
*************************/

void fr_set_rtc_alarm(unsigned int sec)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);        //注意这两行代码

    RTC_WaitForLastTask();  /*等待上一次的写完*/
    RTC_EnterConfigMode();  /*进入配置模式*/
    RTC_SetAlarm(sec); /*设置闹钟时间:RTC_ALR = RTC_CNT+sec */
    RTC_WaitForLastTask();  /*等待本次的写完*/
    RTC_ExitConfigMode();   /*退出配置模式*/
}



/***********************
**	定时器唤醒中断
************************/
void RTCAlarm_IRQHandler(void)
{
	if(RTC_GetITStatus(RTC_IT_ALR) != RESET)
  {
		EXTI_ClearITPendingBit(EXTI_Line17);						//待机模式唤醒不需要设置line17
        
		if(PWR_GetFlagStatus(PWR_FLAG_WU) != RESET) 			//Check if the Wake-Up flag is set 
    {
			PWR_ClearFlag(PWR_FLAG_WU);									//Clear Wake Up flag
    }
    RTC_WaitForLastTask();
    RTC_ClearITPendingBit(RTC_IT_ALR);							//Clear RTC Alarm interrupt pending bit
    RTC_WaitForLastTask();
  }
}


/********************
**
**
**********************/


void fr_set_rtc_hook(void (*hook)(void))
{
	if(hook == NULL)
		return;
	
	if(fr_rtc_hook == NULL)
		fr_rtc_hook = hook;
		
}

/*******************************
**
**
**	RTC中断服务程序
**
**	
********************************/


void RTC_IRQHandler(void)
{
		if(RTC_GetITStatus(RTC_IT_SEC) != RESET)
		{
			RTC_ClearITPendingBit(RTC_IT_SEC);
			if(fr_rtc_hook == NULL)
				fr_rtc_hook();
		}            
}


