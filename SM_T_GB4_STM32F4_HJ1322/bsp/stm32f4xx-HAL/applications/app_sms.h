



#ifndef _APP_SMS_H
#define _APP_SMS_H

#include <stdint.h>
#include <rtthread.h>
#include <rtdevice.h>


struct sms_down_str
{
	uint8_t 	cmd;
	uint16_t 	len;
	uint8_t 	data[512];
};


void thread_entry_sms(void *parameter);
rt_mq_t   get_sms_queue(void);

#endif






