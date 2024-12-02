

#ifndef _APP_SHELL_H
#define _APP_SHELL_H



#define SHELL_BUFF 512


#include "drv_gpio.h"



struct debug_str
{
    uint8_t gnss_nema_debug;

};


#define SHELL_CONFIG()                     \
    {                                      \
        .uart = {                          \
            .uart_port = UART_NUM_0,       \
            .rx_pin = GPIO_UART0_RXD,       \
            .tx_pin = GPIO_UART0_TXD,       \
            .baud_rate = 115200,           \
            .data_bits = UART_DATA_8_BITS, \
            .parity = UART_PARITY_DISABLE, \
            .stop_bits = UART_STOP_BITS_1, \
            .rx_buff_size = DBUGU_BUFF,    \
            .tx_buff_size = 0              \
        }                                  \
    }



void thread_entry_shell(void *parameter);


#endif


