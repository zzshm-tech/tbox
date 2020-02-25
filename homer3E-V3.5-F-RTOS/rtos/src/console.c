


#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "fr_drv_uart.h"


/*******************全局变量********************/

SemaphoreHandle_t xSemaConsole = NULL;  

static portCHAR console_buf[256];          //console缓冲区，静态变量



/*****************************
**
******************************/

int fr_printf(const char *format, ...)
{
  unsigned short int datalen = 0;
	
	xSemaphoreTake(xSemaConsole,portMAX_DELAY );     //获取信号量
	
	va_list args;
  va_start(args, format);
  datalen = vsnprintf((portCHAR *)console_buf, sizeof(console_buf), (portCHAR *)format, args);
  va_end(args);
	
	fr_send_uart2((unsigned char *)console_buf, datalen);
	xSemaphoreGive(xSemaConsole);      					//归还信号量
	
  return datalen;
}


/**************************
**	
***************************/

int fr_send_console(const char *format,unsigned short int str_len)
{
	if(str_len >1024)
		return 0;
	
	xSemaphoreTake(xSemaConsole,portMAX_DELAY );     //获取信号量
	memcpy(console_buf,format,str_len);
	
	fr_send_uart2((unsigned char *)console_buf,str_len);
	xSemaphoreGive(xSemaConsole);      					//归还信号量
	
	return 1;
}

/*************************
**
*************************/

void fr_set_console(const char *name)
{
	name = name;
	
	fr_init_uart2();
	if(xSemaConsole == NULL)
		xSemaConsole = xSemaphoreCreateMutex();   //创建信号量
	
}


