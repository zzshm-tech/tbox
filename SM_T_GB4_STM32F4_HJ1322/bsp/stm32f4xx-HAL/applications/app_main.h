


#ifndef _APP_MAIN_H
#define _APP_MAIN_H

#include <rtthread.h>
#include <rtdevice.h>



/***    ****/

struct app_main_mq_t
{
	uint8_t		 	state;           //参数类型
	uint32_t 	 	len;			 //不同的参数类型，代表不同的解析方法
};




rt_mq_t get_app_main_queue(void);
uint8_t read_sys_run_state(void);



#endif




