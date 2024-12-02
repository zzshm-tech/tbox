



#ifndef _APP_LED_H
#define _APP_LED_H


#include "stdint.h"

struct led_cnt_str
{
	uint32_t green_cnt_on;
	uint32_t green_cnt_off;
	
	uint32_t yellow_cnt_on;
	uint32_t yellow_cnt_off;
	
	uint32_t red_cnt_on;
	uint32_t red_cnt_off;

};


void thread_entry_led(void *parameter);							//Êä³öÏß³Ì

#endif





