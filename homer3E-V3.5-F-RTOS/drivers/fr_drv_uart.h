
#ifndef _FR_DRV_UART_H
#define _FR_DRV_UART_H

#define BAUD_RATE_2400                  2400
#define BAUD_RATE_4800                  4800
#define BAUD_RATE_9600                  9600
#define BAUD_RATE_19200                 19200
#define BAUD_RATE_38400                 38400
#define BAUD_RATE_57600                 57600
#define BAUD_RATE_115200                115200
#define BAUD_RATE_230400                230400
#define BAUD_RATE_460800                460800
#define BAUD_RATE_921600                921600
#define BAUD_RATE_2000000               2000000
#define BAUD_RATE_3000000               3000000

#define DATA_BITS_5                     5
#define DATA_BITS_6                     6
#define DATA_BITS_7                     7
#define DATA_BITS_8                     8
#define DATA_BITS_9                     9

#define STOP_BITS_1                     0
#define STOP_BITS_2                     1
#define STOP_BITS_3                     2
#define STOP_BITS_4                     3


void fr_init_uart1(void);
void fr_send_uart1(unsigned char *source, unsigned int len);
void fr_set_uart1_rx_hook(void (*hook)(void));
void fr_close_uart1(void);


void fr_init_uart2(void);
void fr_send_uart2(unsigned char *source, unsigned int len);
void fr_set_uart2_rx_hook(void (*hook)(void));

void fr_init_uart4(void);
void fr_send_uart4(unsigned char *source, unsigned int len);
void fr_set_uart4_rx_hook(void (*hook)(void));
void fr_close_uart4(void);



int read_uart_pkt(unsigned char port,unsigned char *dest,int max_destsize);


#endif



