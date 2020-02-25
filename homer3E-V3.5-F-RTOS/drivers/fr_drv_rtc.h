
/***************************
**
**
****************************/


#ifndef _FR_DRV_RTC_H
#define _FR_DRV_RTC_H

void fr_init_rtc(void);
unsigned int fr_read_rtc(void);
void fr_set_rtc(unsigned int n);
void fr_set_rtc_alarm(unsigned int sec);


#endif








