



#ifndef _APP_SHELL_H_
#define _APP_SHELL_H_


#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>





/*********************************
**	自定义UART_CMD参数
*************************************/



struct shell_info_t
{
    struct rt_semaphore 	sign; 				//信号量对象 (接收数据信号量)
    uint32_t 							ticks;        //接收数据tick
    uint16_t 							len;					//
};






struct shell_debug_t
{
    uint8_t 							nema_state;        //接收数据tick
};



uint16_t send_data_to_shell_dev(uint8_t *data,uint16_t len);
void close_shell_module(void);
void thread_entry_shell(void *parameter);



#endif


