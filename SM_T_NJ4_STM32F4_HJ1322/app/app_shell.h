


#ifndef _APP_SHELL_H
#define _APP_SHELL_H


#include <stdint.h>


#include "semphr.h"


struct shell_info
{
  SemaphoreHandle_t 	semaphore; //信号量
  uint32_t 						ticks;         //接收数据ticks
	uint16_t 						len;
};


void thread_entry_shell(void *parameter);


#endif



