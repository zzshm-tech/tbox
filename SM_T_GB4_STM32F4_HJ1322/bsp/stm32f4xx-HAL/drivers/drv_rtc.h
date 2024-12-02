/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-10-25     ZYH      first implementation
 */
 
#ifndef __DRV_RTC_H__
#define __DRV_RTC_H__


#include <stdint.h>

typedef unsigned int time_t;     /* date/time in unix secs past 1-Jan-70 */


int rt_hw_rtc_init(void);



#pragma pack(1)

struct rt_tm
{
	uint32_t year;
	uint32_t mon;
	uint32_t day;
	uint32_t hour;
	uint32_t min;
	uint32_t sec;
};

#pragma pack()


void rt_irq_wkup_sethook(void (*hook)(void));
void rt_irq_alarm_sethook(void (*hook)(void));

time_t rt_time(time_t *t);
struct rt_tm *rt_localtime(const time_t *time);
time_t rt_mktime(struct rt_tm *t);
uint8_t rt_set_rtc(struct rt_tm *tm);
uint8_t rt_get_rtc(struct rt_tm *tm);
uint32_t get_rtc_timestamp(void);
uint8_t rt_set_rtc_wkup(uint16_t n);


#endif



