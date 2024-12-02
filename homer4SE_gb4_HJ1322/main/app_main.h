


#ifndef _APP_MAIN_H
#define _APP_MAIN_H




/***    ****/

struct app_main_mq_str
{
	uint8_t		 	state;           //参数类型
	uint32_t 	 	len;			 //不同的参数类型，代表不同的解析方法
};






QueueHandle_t get_app_main_queue(void);
uint8_t read_sys_run_state(void);



#endif

