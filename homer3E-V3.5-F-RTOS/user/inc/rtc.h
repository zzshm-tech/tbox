

#ifndef _RTC_H
#define _RTC_H

#include "data_type.h"

void task_rtc(void *data);
	
void read_time(struct time_str *ptime);

unsigned int calendar_to_timestamp(struct time_str *t);
struct time_str *timestamp_to_calendar(unsigned int t);
unsigned int read_timestamp(void);

#endif


