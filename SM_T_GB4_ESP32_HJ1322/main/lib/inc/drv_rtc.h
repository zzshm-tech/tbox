

#ifndef _DRV_RTC_H
#define _DRV_RTC_H


#include "driver/timer.h"
#include <sys/time.h>

struct rt_tm
{
	uint32_t year;
	uint32_t mon;
	uint32_t day;
	uint32_t hour;
	uint32_t min;
	uint32_t sec;
};



struct rtc_t
{
	uint16_t flag;
};




struct rt_tm *rt_localtime(const uint32_t *time);
time_t rt_mktime(struct rt_tm *t);



uint8_t set_rtc_time(struct rt_tm *tm);
uint8_t get_rtc_time(struct rt_tm *tm);
uint32_t get_rtc_timestamp(void);
uint8_t rt_hw_init_rtc(void);
uint8_t read_rtc_state(void);

#endif
