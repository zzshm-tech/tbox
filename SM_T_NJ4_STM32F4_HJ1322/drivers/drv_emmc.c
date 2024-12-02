
/*********************
**      SDIO
**      CMD  双向
**      DATA0-DATA3，四线制数据项
**      CLK 主机发出的时钟信号
**      
****************************/

#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"


#include "board.h"
#include "drv_emmc.h"
#include "drv_uart.h"





/**************************
**	初始化EMMC
***************************/

uint8_t rt_hw_init_emmc(void)
{
	
	return 0;
}



/********************************
**	SDIO中断  服务
*********************************/

void SDIO_IRQHandler(void)
{

}

/****************File End*************/


