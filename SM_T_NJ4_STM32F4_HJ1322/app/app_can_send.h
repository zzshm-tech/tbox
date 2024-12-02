



#ifndef _APP_CAN_SEND_H
#define _APP_CAN_SEND_H

#include <stdint.h>
#include "queue.h"

struct can_send_mq_t
{
	uint32_t		 cmd;           //参数类型
	uint16_t 	 	 len;					//不同的参数类型，代表不同的解析方法
	uint8_t		 	 data[32];
};


QueueHandle_t get_can_send_queue(void);

void thread_entry_can_send(void *parameter);




#endif


