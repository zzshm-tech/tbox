

#include "board.h"

#include "FreeRTOS.h"
#include "task.h"

extern int $Super$$main(void);
extern int main(void);

/*****************************
**
*****************************/
void main_thread_entry(void *parameter)
{
	parameter = parameter;
	
	$Super$$main(); /* for ARMCC. */
}


/*****************************
**
*****************************/

int $Sub$$main(void)
{
	rt_hw_init_board();
	
	xTaskCreate(main_thread_entry,"main",512,       NULL,  30,  NULL);  //创建SHELL任务
  
	vTaskStartScheduler();   /* 启动任务，开启调度 */
	
	return 0;
}

