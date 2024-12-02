


/***************************
**	
**	CreateTime:
*****************************/

#ifndef _BSP_RTC_H
#define _BSP_RTC_H

#include <stdint.h>
#include "data_type.h"

#include "drv_rtc.h"




/******************************************************************************
                             参数寄存器地址宏定义
******************************************************************************/
 
#define PCF8563_Address_Control_Status_1         (unsigned char)0x00  //控制/状态寄存器1
#define PCF8563_Address_Control_Status_2         (unsigned char)0x01  //控制/状态寄存器2
 
#define PCF8563_Address_CLKOUT                   (unsigned char)0x0d  //CLKOUT频率寄存器
#define PCF8563_Address_Timer                    (unsigned char)0x0e  //定时器控制寄存器
#define PCF8563_Address_Timer_VAL                (unsigned char)0x0f  //定时器倒计数寄存器
 
#define PCF8563_Address_Years                    (unsigned char)0x08  //年
#define PCF8563_Address_Months                   (unsigned char)0x07  //月
#define PCF8563_Address_Days                     (unsigned char)0x05  //日
#define PCF8563_Address_WeekDays                 (unsigned char)0x06  //星期
#define PCF8563_Address_Hours                    (unsigned char)0x04  //小时
#define PCF8563_Address_Minutes                  (unsigned char)0x03  //分钟
#define PCF8563_Address_Seconds                  (unsigned char)0x02  //秒
 
#define PCF8563_Alarm_Minutes                    (unsigned char)0x09  //分钟报警
#define PCF8563_Alarm_Hours                      (unsigned char)0x0a  //小时报警
#define PCF8563_Alarm_Days                       (unsigned char)0x0b  //日报警
#define PCF8563_Alarm_WeekDays                   (unsigned char)0x0c  //星期报警


typedef unsigned int time_t;




struct rt_tm
{
  uint32_t year;
  uint32_t mon;
  uint32_t day;
  uint32_t hour;
  uint32_t min;
  uint32_t sec;
};



uint8_t rt_hw_init_rtc(void);
uint8_t rt_set_time(struct rt_tm *t);
time_t rt_time(time_t *t);
struct rt_tm *rt_localtime(const time_t *time);
time_t rt_mktime(struct rt_tm *t);
void rt_irq_wkup_sethook(void (*hook)(void));




uint8_t get_rtc_time(struct rt_tm *tm);
uint32_t get_rtc_timestamp(void);
uint8_t rt_hw_set_wakeup(uint32_t cnt);
uint8_t set_rtc_time(struct rt_tm *tm);


#endif

/****************File End***************/


