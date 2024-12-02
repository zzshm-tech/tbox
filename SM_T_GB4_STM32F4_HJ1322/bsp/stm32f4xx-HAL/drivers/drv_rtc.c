/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-10-25     ZYH      first implementation
 */
#include "drv_rtc.h"
#include <board.h>
#include <rtdevice.h>
#include <string.h>
#include <time.h>



static RTC_HandleTypeDef 					hrtc;

static struct rt_tm 							tm;

/*****************************/

static void (*rt_irq_wkup_hook1)();

static void (*rt_irq_wkup_hook2)();

static void (*rt_irq_alarm_hook)();
				


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
            if(tm->day > 31 || tm->day == 0)
                return 1;
            
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            if(tm->day > 30 || tm->day == 0)
                return 1;

            break;
        case 2:
            tmp = tm->year + 2000;
            if(tmp % 400==0 || (tmp % 4==0 && tmp % 100 != 0))
            {
                if(tm->day > 29 || tm->day == 0)
                   return 1;
            }
						else
						{
               if(tm->day > 28 || tm->day == 0)
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




/*****************************
**	RTC init function 
*****************************/

void MX_RTC_Init(void)
{
	RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;
  /**Initialize RTC Only*/
	
	hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  RT_ASSERT(HAL_RTC_Init(&hrtc) == HAL_OK);
	
  if(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x32F2)
  {
    sTime.Hours = 0;
    sTime.Minutes = 0;
    sTime.Seconds = 0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    RT_ASSERT(HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK);
    sDate.Year = 22;
    sDate.Month = 1;
    sDate.Date = 1;
		sDate.WeekDay = RTC_WEEKDAY_THURSDAY;
    RT_ASSERT(HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK);
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x32F2);
  }
}




/*********************************
**
**********************************/

void HAL_RTC_MspInit(RTC_HandleTypeDef *rtcHandle)
{
    if (rtcHandle->Instance == RTC)
    {
        /* USER CODE BEGIN RTC_MspInit 0 */
        /* USER CODE END RTC_MspInit 0 */
        /* RTC clock enable */
        __HAL_RCC_RTC_ENABLE();
				HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
				HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
				HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
				HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
        /* USER CODE BEGIN RTC_MspInit 1 */
        /* USER CODE END RTC_MspInit 1 */
    }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef *rtcHandle)
{
    if (rtcHandle->Instance == RTC)
    {
        /* USER CODE BEGIN RTC_MspDeInit 0 */
        /* USER CODE END RTC_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_RTC_DISABLE();
				HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);
        /* USER CODE BEGIN RTC_MspDeInit 1 */
        /* USER CODE END RTC_MspDeInit 1 */
    }
}

static rt_err_t stm32_rtc_control(struct rt_device *dev,
                                  int              cmd,
                                  void             *args)
{
		struct rt_tm		now;
		struct rt_tm		*pt;
		
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

   
    switch (cmd)
    {
    case RT_DEVICE_CTRL_RTC_GET_TIME:                           //获取时间-日历日期
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
        now.hour = sTime.Hours;
        now.min = sTime.Minutes;
        now.sec = sTime.Seconds;
        now.year = sDate.Year;
        now.mon = sDate.Month;
        now.day = sDate.Date;
				*((struct rt_tm *)args) = now;
        break;
    case RT_DEVICE_CTRL_RTC_SET_TIME:                           //设置时间-日历时间
				rt_enter_critical();
				pt = rt_localtime((const time_t *)args);
				rt_exit_critical();
        sTime.Hours = pt->hour;
        sTime.Minutes = pt->min;
        sTime.Seconds = pt->sec;
				
				sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
				sTime.StoreOperation = RTC_STOREOPERATION_RESET;
				RT_ASSERT(HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK);
				sDate.WeekDay = 0;
        sDate.Year = pt->year;
        sDate.Month = pt->mon ;
        sDate.Date = pt->day;
        RT_ASSERT(HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK);
        break;
		case RT_DEVICE_CTRL_RTC_GET_ALARM:           /**< get alarm */
				break;
		case RT_DEVICE_CTRL_RTC_SET_ALARM:           /**< set alarm */
				break;
		case RT_DEVICE_CTRL_RTC_GET_WKUP:            /**< get alarm */
				break;
		case RT_DEVICE_CTRL_RTC_SET_WKUP:            /**< set alarm */
				
				__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);//清除RTC WAKE UP的标志
				HAL_RTCEx_SetWakeUpTimer_IT(&hrtc,*(rt_uint16_t *)args,RTC_WAKEUPCLOCK_CK_SPRE_16BITS);            //设置重装载值和时钟 
				HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
				HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
				break;
		}
    return RT_EOK;
}

static rt_err_t stm32_rtc_init(struct rt_device *dev)
{
    return RT_EOK;
}

static rt_err_t stm32_rtc_open(struct rt_device *dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t stm32_rtc_close(struct rt_device *dev)
{
    return RT_EOK;
}

static rt_size_t stm32_rtc_read(struct rt_device *dev,
                                rt_off_t          pos,
                                void             *buffer,
                                rt_size_t         size)
{
    stm32_rtc_control(dev, RT_DEVICE_CTRL_RTC_GET_TIME, buffer);
    return size;
}

static rt_size_t stm32_rtc_write(struct rt_device *dev,
                                 rt_off_t          pos,
                                 const void       *buffer,
                                 rt_size_t         size)
{
    stm32_rtc_control(dev, RT_DEVICE_CTRL_RTC_SET_TIME, (void *)buffer);
    return size;
}

struct rt_device rtc_device;
int rt_hw_rtc_init(void)
{
    MX_RTC_Init();
    rtc_device.type        = RT_Device_Class_RTC;
    rtc_device.rx_indicate = RT_NULL;
    rtc_device.tx_complete = RT_NULL;
    rtc_device.init        = stm32_rtc_init;
    rtc_device.open        = stm32_rtc_open;
    rtc_device.close       = stm32_rtc_close;
    rtc_device.read        = stm32_rtc_read;
    rtc_device.write       = stm32_rtc_write;
    rtc_device.control     = stm32_rtc_control;
    rtc_device.user_data   = RT_NULL;
    /* register a character device */
    return rt_device_register(&rtc_device, "rtc", RT_DEVICE_FLAG_DEACTIVATE);
}
INIT_BOARD_EXPORT(rt_hw_rtc_init);




/***********************
**	wkup中断回调函数
**********************/

void rt_irq_wkup_sethook(void (*hook)(void))
{
	if(rt_irq_wkup_hook1 == NULL)
    rt_irq_wkup_hook1 = hook;
	else if(rt_irq_wkup_hook2 == NULL)
		rt_irq_wkup_hook2 = hook;
}





/***********************
**	RTC WKUP中断
**********************/

void RTC_WKUP_IRQHandler(void)
{
	HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
	
	if(rt_irq_wkup_hook1 != NULL)
    rt_irq_wkup_hook1();
	if(rt_irq_wkup_hook2 != NULL)
		rt_irq_wkup_hook2();
}



/***********************
**	Alarm中断回调函数定时器中断服务函数
**********************/

void rt_irq_alarm_sethook(void (*hook)(void))
{
	if(rt_irq_alarm_hook == RT_NULL)
     rt_irq_alarm_hook = hook;
}


/***********************
**	RTC Alarm-A中断
**********************/

void RTC_Alarm_IRQHandler(void)
{
  HAL_RTC_AlarmIRQHandler(&hrtc);
	
	if(rt_irq_alarm_hook != RT_NULL)
     rt_irq_alarm_hook();
}




extern uint32_t get_rtc_timestamp(void);

/***********************
**	设置时间
************************/
uint8_t rt_set_rtc(struct rt_tm *tm)
{
	uint32_t 				now_timestamp;
	int32_t	 				tmp;
	uint32_t				tmp_timestamp;
	rt_device_t 		rtc_dev = RT_NULL;       //
			

	if(tm == NULL)
		return 1;
		
	if(check_rtc_time(tm) > 0)
		return 1;
	

	now_timestamp = get_rtc_timestamp() + 28800;                //读取系统时间戳
	tmp_timestamp = rt_mktime(tm);     //要检验的时间戳
	
	//rt_kprintf("-- the verfy time:%u,%u\r\n",tmp_timestamp,now_timestamp);
	tmp = now_timestamp - tmp_timestamp;
	if(tmp > 0 || tmp <= -3)                          //
	{
		
		now_timestamp = tmp_timestamp;
		rtc_dev = rt_device_find("rtc");
		if(rtc_dev != RT_NULL)
		{
				if(rt_device_open(rtc_dev, 0) == RT_EOK)
				{
					rt_device_write(rtc_dev, RT_DEVICE_CTRL_RTC_SET_TIME, &now_timestamp, sizeof(now_timestamp));
					//rt_kprintf("-- verfy time...%u,%u\r\n",tmp_timestamp,now_timestamp);
					rt_device_close(rtc_dev);
				}
				else
				{
					return 1;
				}
		}
		else
		{
			return 1;
		}
	}
	
	return 0;
}



/***************************
**	获取系统时间
****************************/

uint8_t rt_get_rtc(struct rt_tm *tm)
{
  rt_device_t device = RT_NULL;

    /* optimization: find rtc device only first. */
	
	device = rt_device_find("rtc");

    /* read timestamp from RTC device. */
  if(device != RT_NULL)
  {
		if(rt_device_open(device, 0) == RT_EOK)
    {
			rt_device_control(device, RT_DEVICE_CTRL_RTC_GET_TIME, tm);
      rt_device_close(device);
    }
		else
		{
			return 1;
		}
  }
	else
	{
		return 1;
	}
  
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
	
  rt_get_rtc(&tm);
	
	tmp.tm_year = tm.year + 100;
	tmp.tm_mon = tm.mon - 1;
	tmp.tm_mday = tm.day;
	tmp.tm_hour = tm.hour;
	tmp.tm_min	=  tm.min;
	tmp.tm_sec = tm.sec;
	
	now = mktime(&tmp) - 28800;
	
  return now;
}





/***************************
**
****************************/

uint8_t rt_set_rtc_wkup(uint16_t n)
{
	rt_device_t 					rtc_dev = NULL;
	uint32_t 							tmp = 0;
	
	tmp = n;
	if(tmp > 20)
		tmp = 20;
	
	rtc_dev = rt_device_find("rtc");
	rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_WKUP, &tmp); 
	return 0;
}


