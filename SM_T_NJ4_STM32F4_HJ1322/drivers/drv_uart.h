


#ifndef _DRV_UART_H
#define _DRV_UART_H


#include <stdint.h>


#define DBUGU_BUFF          1024
#define GNSS_BUFF_SIZE      1200
#define LTE_BUFF_SIZE       1200



void rt_hw_init_uart1(void);
void rt_hw_init_uart2(void);
void rt_hw_init_uart3(void);
void rt_hw_init_uart4(void);
void rt_hw_init_uart5(void);
void rt_hw_init_uart6(void);

void to_com_buf(unsigned char port, unsigned char *source, unsigned int USARTx_Send_Counter);
int read_com_pkt(unsigned char port, unsigned char *dest, int max_destsize);
int rt_printf(const char *format, ...);
int rt_printf_hex(unsigned char *buf,unsigned short int buf_size);



uint16_t rt_read_uart_buf(uint8_t port, uint8_t *dest, uint16_t n);
void rt_irq_uart_sethook(uint8_t uart,void (*hook)(uint16_t));
uint8_t rt_write_uart_buf(uint8_t port, uint8_t *buf, uint16_t size);

#endif








