





#include <stdint.h>
#include <string.h>
#include <stdio.h>



#include "FreeRTOS.h"
#include "task.h"



/**********************************
**  ·µ»ØÉý¼¶×´Ì¬
***********************************/

uint8_t read_iap_state(void)
{
    uint8_t rv = 0;

    rv = 0;//iap_state;

    return rv;
}




/****************************
**	
****************************/

void thread_entry_iap(void *parameter)
{
	parameter = parameter;

	for(;;)
	{
		vTaskDelay(100);
	}
}


