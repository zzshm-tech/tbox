
/*********************
**	File Name:
**	Time:
**********************/

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"


#include "stm32f4xx.h"

#include "drv_uart.h"
#include "data_type.h"    
#include "drv_rtc.h"

/**********************************/

 	

static void (*rt_irq_wkup_hook)();
static void (*rt_irq_alarm_hook)();
					
					

static struct rt_tm 	tm;




/******************* 本地全局变量 **********************/


/*****************************
 ** 检测RTC时间
 *****************************/

static uint8_t check_rtc_time(struct rt_tm *tm)
{
    uint16_t tmp;

    if(tm == NULL)
        return 1;
    if(tm->year > 99)
        return 1;

    
    if(tm->mon > 12 || tm->mon == 0)
        return 1;

    switch(tm->mon)
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            if(tm->day < 31 || tm->day == 0)
                return 1;
            
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            if(tm->day < 30 || tm->day == 0)
                return 1;

            break;
        case 2:
            tmp = tm->year + 2000;
            if(tmp % 400==0 || (tmp % 4==0 && tmp % 100 != 0))
            {
                if(tm->day > 29 || tm->day == 0)
                   return 1;
            }
            break;
    }
    
    if(tm->hour > 23)
        return 1;
    if(tm->min > 59)
        return 1;
    if(tm->sec > 59)
        return 1;

    return 0;
}

/**************************
 ** 读取RTC
 *****************************/

uint8_t get_rtc_time(struct rt_tm *tm)
{
	RTC_TimeTypeDef  	RTC_TimeStructure;
	RTC_DateTypeDef		RTC_DateStructure;

  if(tm == NULL)
     return 1;

  RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);   //
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
	
	tm->year 	= RTC_DateStructure.RTC_Year;
	tm->mon  	= RTC_DateStructure.RTC_Month;			//
	tm->day		= RTC_DateStructure.RTC_Date;				//
	tm->hour  = RTC_TimeStructure.RTC_Hours;			//
	tm->min		= RTC_TimeStructure.RTC_Minutes;		//
	tm->sec		= RTC_TimeStructure.RTC_Seconds;		//
	
  return 0;
}



/**************************
 ** 设置RTC
 *****************************/

uint8_t set_rtc_time(struct rt_tm *tm)
{
	RTC_DateTypeDef 	RTC_DateStructure;
	RTC_TimeTypeDef  	RTC_TimeStructure;
    
	if(check_rtc_time(tm) > 1)
		return 1;
    
  RTC_DateStructure.RTC_Year = tm->year;
  RTC_DateStructure.RTC_Month = tm->mon;
  RTC_DateStructure.RTC_Date = tm->day;
  RTC_DateStructure.RTC_WeekDay = 1;   //？
  RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure);
  
  /* Set the time to 05h 20mn 00s AM */
	if(tm->hour <= 12)
		RTC_TimeStructure.RTC_H12     = RTC_H12_AM;
	
  RTC_TimeStructure.RTC_Hours   = tm->hour;
  RTC_TimeStructure.RTC_Minutes = tm->min;
  RTC_TimeStructure.RTC_Seconds = tm->sec; 
  
  RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure);

  return 0;
}




/*******************************
*** 返回系统时间戳
*******************************/

uint32_t get_rtc_timestamp(void)
{
	struct rt_tm tm;
	struct tm tmp;
	
	uint32_t now;
	
  get_rtc_time(&tm);
	
	tmp.tm_year = tm.year + 100;
	tmp.tm_mon = tm.mon - 1;
	tmp.tm_mday = tm.day;
	tmp.tm_hour = tm.hour;
	tmp.tm_min	=  tm.min;
	tmp.tm_sec = tm.sec;
	
	now = mktime(&tmp) - 28800;
	
  return now;
}




/***********************************
**	函数名称:
**	功能描述:时间戳转日历
**	返回一个指向
***********************************/

struct rt_tm *rt_localtime(const uint32_t *time)
{
    struct tm *p_t = NULL;

    p_t = localtime((const time_t *)time);
    
    if(p_t->tm_year < 100)
        return NULL;

    tm.year = p_t->tm_year - 100;
    tm.mon = p_t->tm_mon + 1;
    tm.day = p_t->tm_mday;
    tm.hour = p_t->tm_hour;
    tm.min = p_t->tm_min;
    tm.sec = p_t->tm_sec;

    return &tm;
}



/**********************************************************************
**	函数名称:
**	功能描述:时间转时间戳
**	输入参数:
**	输出参数:
**********************************************************************/


time_t rt_mktime(struct rt_tm *t)
{
    time_t rv;
    struct tm tmp;

    tmp.tm_year = t->year + 100;
    tmp.tm_mon = t->mon - 1;
    tmp.tm_mday = t->day;
    tmp.tm_hour = t->hour;
    tmp.tm_min = t->min;
    tmp.tm_sec = t->sec;

    rv = mktime(&tmp);

	return rv;               //
}




/**************************
**
***************************/


uint8_t rt_hw_set_wakeup(uint32_t cnt)
{ 
	EXTI_InitTypeDef   EXTI_InitStructure;
	
	RTC_WakeUpCmd(DISABLE);//关闭WAKE UP
	
	RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);//唤醒时钟选择
	
	RTC_SetWakeUpCounter(cnt);//设置WAKE UP自动重装载寄存器
	
	
	RTC_ClearITPendingBit(RTC_IT_WUT); //清除RTC WAKE UP的标志
  EXTI_ClearITPendingBit(EXTI_Line22);//清除LINE22上的中断标志位 
	 
	RTC_ITConfig(RTC_IT_WUT,ENABLE);//开启WAKE UP 定时器中断
	RTC_WakeUpCmd( ENABLE);//开启WAKE UP 定时器　
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line22;//LINE22
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//中断事件
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //上升沿触发 
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;//使能LINE22
  EXTI_Init(&EXTI_InitStructure);//配置
 
 return 0;
	
}



/*************************************
**	函数名称:
**	功能描述:初始化系统RTC
**	使用外部32.768晶振
**************************************/
uint8_t rt_hw_init_rtc(void)
{	
  RTC_TimeTypeDef  RTC_TimeStructure;
	RTC_InitTypeDef  RTC_InitStructure;
	RTC_DateTypeDef		RTC_DateStructure;
	
	__IO uint32_t uwAsynchPrediv = 0;
	__IO uint32_t uwSynchPrediv = 0;
	
	if(RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x3232)
  {  
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
		/* Allow access to RTC */
		PWR_BackupAccessCmd(ENABLE);
		RCC_LSEConfig(RCC_LSE_ON);
		//RCC_LSICmd(ENABLE);
		/* Wait till LSE is ready */  
		while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
		{
		}

		/* Select the RTC Clock Source */
		//RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
		/* ck_spre(1Hz) = RTCCLK(LSE) /(uwAsynchPrediv + 1)*(uwSynchPrediv + 1)*/
		uwSynchPrediv = 0xFF;
		uwAsynchPrediv = 0x7F;
		/* Enable the RTC Clock */
		RCC_RTCCLKCmd(ENABLE);

		/* Wait for RTC APB registers synchronisation */
		RTC_WaitForSynchro();
  
		/* Configure the RTC data register and RTC prescaler */
		RTC_InitStructure.RTC_AsynchPrediv = uwAsynchPrediv;
		RTC_InitStructure.RTC_SynchPrediv = uwSynchPrediv;
		RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
		RTC_Init(&RTC_InitStructure);
		
		RTC_TimeStructure.RTC_Hours = 18;
		RTC_TimeStructure.RTC_Minutes = 8;
		RTC_TimeStructure.RTC_Seconds = 0;
		
		RTC_DateStructure.RTC_Year = 22;
		RTC_DateStructure.RTC_Month = 11;
		RTC_DateStructure.RTC_Date = 5;
		
    RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure); 
		RTC_SetDate(RTC_Format_BIN,&RTC_DateStructure); 
		RTC_WriteBackupRegister(RTC_BKP_DR0, 0x3232);
  }
  else
  {
		PWR_BackupAccessCmd(ENABLE);
		RTC_WriteBackupRegister(RTC_BKP_DR0, 0x3232);
    /* Check if the Power On Reset flag is set */
    if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
    {
    }
    /* Check if the Pin Reset flag is set */
    else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
    {
    }
    
    /* Enable the PWR clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

    /* Allow access to RTC */
    PWR_BackupAccessCmd(ENABLE);

    /* Wait for RTC APB registers synchronisation */
    RTC_WaitForSynchro();
  }
	
	return 0;
}



/***********************
**	wkup中断回调函数
**********************/

void rt_irq_wkup_sethook(void (*hook)(void))
{
  if(rt_irq_wkup_hook == NULL)
     rt_irq_wkup_hook = hook;
}


/***********************
**	RTC WKUP中断
**********************/

void RTC_WKUP_IRQHandler(void)
{
	uint32_t  rv = 0;
	rv = taskENTER_CRITICAL_FROM_ISR();
	if(RTC_GetFlagStatus(RTC_FLAG_WUTF)==SET)//WK_UP中断?
	{ 
		RTC_ClearFlag(RTC_FLAG_WUTF);	//清除中断标志
		if(rt_irq_wkup_hook != NULL)
			rt_irq_wkup_hook();
	}   
	EXTI_ClearITPendingBit(EXTI_Line22);//清除中断线22的中断标志 
	taskEXIT_CRITICAL_FROM_ISR(rv);
}


/***************************************
**	Alarm中断回调函数
*****************************************/

void rt_irq_alarm_sethook(void (*hook)(void))
{
  if(rt_irq_alarm_hook == NULL)
     rt_irq_alarm_hook = hook;
}


/**************************
**	RTC Alarm-A中断
***************************/

void RTC_Alarm_IRQHandler(void)
{
  if(rt_irq_alarm_hook != NULL)
     rt_irq_alarm_hook();
}








/***********************File End********************/


