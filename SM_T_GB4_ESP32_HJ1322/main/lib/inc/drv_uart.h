

#ifndef _DRV_UART_H
#define _DRV_UART_H


#include "driver/gpio.h"
#include "driver/uart.h"


#define DBUGU_BUFF          1024
#define GNSS_BUFF_SIZE      1200
#define LTE_BUFF_SIZE       1200



typedef struct {
    struct {
        uart_port_t         uart_port;          /*!< UART port number */
        uint32_t            rx_pin;             /*!< UART Rx Pin number */
        uint32_t            tx_pin;             /*!< UART Rx Pin number */
        uint32_t            baud_rate;          /*!< UART baud rate */
        uart_word_length_t  data_bits;          /*!< UART data bits length */
        uart_parity_t       parity;             /*!< UART parity */
        uart_stop_bits_t    stop_bits;          /*!< UART stop bits length */
        uint32_t            rx_buff_size;
        uint32_t            tx_buff_size;
    } uart;                                     /*!< UART specific configuration */
} uart_config;


/*
#define DEBUG_CONFIG()                     \
    {                                      \
        .uart = {                          \
            .uart_port = UART_NUM_0,       \
            .rx_pin = UART_DEBUG_RX,       \
            .tx_pin = UART_DEBUG_TX,       \
            .baud_rate = 115200,           \
            .data_bits = UART_DATA_8_BITS, \
            .parity = UART_PARITY_DISABLE, \
            .stop_bits = UART_STOP_BITS_1, \
            .rx_buff_size = DBUGU_BUFF,    \
            .tx_buff_size = 0              \
        }                                  \
    }

#define GNSS_CONFIG()                      \
    {                                      \
        .uart = {                          \
            .uart_port = UART_NUM_1,       \
            .rx_pin = UART_GNSS_RX,        \
            .baud_rate = 9600,             \
            .data_bits = UART_DATA_8_BITS, \
            .parity = UART_PARITY_DISABLE, \
            .stop_bits = UART_STOP_BITS_1, \
            .rx_buff_size = GNSS_BUFF_SIZE,\
            .tx_buff_size = 0              \
        }                                  \
    }
#define LTE_CONFIG()                       \
    {                                      \
        .uart = {                          \
            .uart_port = UART_NUM_2,       \
            .rx_pin = UART_LTE_RX,         \
            .tx_pin = UART_LTE_TX,         \
            .baud_rate = 115200,           \
            .data_bits = UART_DATA_8_BITS, \
            .parity = UART_PARITY_DISABLE, \
            .stop_bits = UART_STOP_BITS_1, \
            .rx_buff_size = LTE_BUFF_SIZE, \
            .tx_buff_size = 0              \
        }                                  \
    }

typedef struct 
{
    uint8_t  recv_buf[DBUGU_BUFF];
    int16_t recv_len;
}debug_str;

typedef enum
{
    PRINT_HEX = 0,
    PRINT_DEC = 1,
    PRINT_CHAR = 2,
} print_mode;

*/



uint8_t rt_hw_init_uart0(void);
uint8_t rt_hw_init_uart1(uint32_t banud);
uint8_t rt_hw_init_uart2(uint32_t banud);


uint32_t read_data_from_uart0(uint8_t *buf, uint32_t size, uint32_t FrameMsTimeout, uint32_t ByteMsTimeout);
uint32_t read_data_from_uart1(uint8_t *buf, uint32_t size, uint32_t FrameMsTimeout, uint32_t ByteMsTimeout);
uint32_t read_data_from_uart2(uint8_t *buf, uint32_t size, uint32_t FrameMsTimeout, uint32_t ByteMsTimeout);
uint8_t write_data_to_uart0(uint8_t *buf, uint32_t len);
uint8_t write_data_to_uart1(uint8_t *buf, uint32_t len);
uint8_t write_data_to_uart2(uint8_t *buf, uint32_t len);



#endif

