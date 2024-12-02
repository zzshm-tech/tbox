






#ifndef _APP_SMS_H
#define _APP_SMS_H

#include <stdint.h>
#include "queue.h"


struct sms_down_str
{
	uint16_t len;
	uint8_t data[512];
};


void thread_entry_sms(void *parameter);
QueueHandle_t  get_sms_queue(void);

#endif









