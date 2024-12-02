
/**********************************
**  UART0:调试串口
**  UART1:
**  UART2:
**********************************/

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "esp_system.h"
#include "esp_spi_flash.h"



#include "drv_uart.h"
#include "drv_gpio.h"




#define RX_BUF_SIZE (4224)    /** 接收缓冲区 **/



/*******************  *******************/

static QueueHandle_t uart0_queue;    //DEBUG
static QueueHandle_t uart1_queue;    //
static QueueHandle_t uart2_queue;    //




/***************************
**  
****************************/

uint8_t rt_hw_init_uart0(void)
{ 
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, GPIO_UART0_TXD, GPIO_UART0_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_0, RX_BUF_SIZE, 0, 1, &uart0_queue, 0);
    uart_pattern_queue_reset(UART_NUM_0, 1);
    //printf("-- init uart0 ...... \r\n");
    return 0;
}



/***************************
**  初始化UART1
****************************/

uint8_t rt_hw_init_uart1(uint32_t banud)
{ 
    uart_config_t uart_config = {
        .baud_rate = banud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };


    if(uart_is_driver_installed(UART_NUM_1) == false)
    {
        uart_param_config(UART_NUM_1, &uart_config);
        uart_set_pin(UART_NUM_1, GPIO_UART1_TXD, GPIO_UART1_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        uart_driver_install(UART_NUM_1, RX_BUF_SIZE, 0, 1, &uart1_queue, 0);
        uart_pattern_queue_reset(UART_NUM_1, 1);
    }
    else
    {
        uart_driver_delete(UART_NUM_1);
        uart_param_config(UART_NUM_1, &uart_config);
        uart_set_pin(UART_NUM_1, GPIO_UART1_TXD, GPIO_UART1_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        uart_driver_install(UART_NUM_1, RX_BUF_SIZE, 0, 1, &uart1_queue, 0);
    }
        
    //uart_pattern_queue_reset(UART_NUM_1, 1);
    printf("-- Init Uart1 OK .... \r\n");
    return 0;
}




/***************************
**  初始化UART2
****************************/

uint8_t rt_hw_init_uart2(uint32_t banud)
{ 
    uart_config_t uart_config = {
        .baud_rate = banud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };



    
    
    if(uart_is_driver_installed(UART_NUM_2) == false)
    {
        uart_param_config(UART_NUM_2, &uart_config);
        uart_set_pin(UART_NUM_2, GPIO_UART2_TXD, GPIO_UART2_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        uart_driver_install(UART_NUM_2, RX_BUF_SIZE, 0, 1, &uart2_queue, 0);
        uart_pattern_queue_reset(UART_NUM_2, 1);
    }
    else
    {
        uart_driver_delete(UART_NUM_2);
        uart_param_config(UART_NUM_2, &uart_config);
        uart_set_pin(UART_NUM_2, GPIO_UART2_TXD, GPIO_UART2_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        uart_driver_install(UART_NUM_2, RX_BUF_SIZE, 0, 1, &uart2_queue, 0);
    }

    return 0;
}








/*********************************
**
**********************************/

uint32_t GetUartQueueEvent(uart_port_t uart_num, uint32_t xTicksToWait)
{
    uart_event_t event;
    QueueHandle_t queue;

    if (uart_num == UART_NUM_0)
    {
        queue = uart0_queue;
    }
    else if (uart_num == UART_NUM_1)
    {
        queue = uart1_queue;
    }
    else if (uart_num == UART_NUM_2)
    {
        queue = uart2_queue;
    }
    else
    {
        printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
        return 0;
    }
    //Waiting for UART event.
    if (xQueueReceive(queue, (void *)&event, xTicksToWait))
    {
        switch (event.type)
        {
        case UART_DATA:
            return event.size;

        case UART_FIFO_OVF:
            uart_flush_input(uart_num);
            xQueueReset(queue);
            break;

        case UART_BUFFER_FULL:     //
            uart_flush_input(uart_num);
            xQueueReset(queue);
            break;

        default:
            break;
        }
    }

    return 0;
}

/********************************
**  从UART0读取数据
*********************************/

uint32_t read_data_from_uart0(uint8_t *buf, uint32_t size, uint32_t FrameMsTimeout, uint32_t ByteMsTimeout)
{
    int  len = 0;

    if(buf == NULL)
        return 0;
    //printf("-- Read Uart0  Data....\r\n");
    GetUartQueueEvent(UART_NUM_0, pdMS_TO_TICKS(FrameMsTimeout));
    len = uart_read_bytes(UART_NUM_0, buf, size, pdMS_TO_TICKS(ByteMsTimeout));

    //printf("-- Read Uart0 Data....%d,%d\r\n",len,size);
    if(len == -1)
        return 0;

    return len;
}



/********************************
**  获取UART1数据
*********************************/

uint32_t read_data_from_uart1(uint8_t *buf, uint32_t size, uint32_t FrameMsTimeout, uint32_t ByteMsTimeout)
{
    int  len = 0;

    if(buf == NULL)
        return 0;

    GetUartQueueEvent(UART_NUM_1, pdMS_TO_TICKS(FrameMsTimeout));
    len = uart_read_bytes(UART_NUM_1, buf, size, pdMS_TO_TICKS(ByteMsTimeout));
    
    if(len == -1)
        return 0;

    return len;
}



/********************************
**  从UART0读取数据
*********************************/

uint32_t read_data_from_uart2(uint8_t *buf, uint32_t size, uint32_t FrameMsTimeout, uint32_t ByteMsTimeout)
{
    int  len = 0;

    if(buf == NULL)
        return 0;

    GetUartQueueEvent(UART_NUM_2, pdMS_TO_TICKS(FrameMsTimeout));
    len = uart_read_bytes(UART_NUM_2, buf, size, pdMS_TO_TICKS(ByteMsTimeout));

    if(len == -1)
        return 0;

    return len;
}


/******************************
**  
*********************************/

uint8_t write_data_to_uart0(uint8_t *buf, uint32_t len)
{
    if (len == 0 || buf == NULL)
    {
        return 1;
    }
    
    uart_write_bytes(UART_NUM_0, (const char *)buf, len);

    return 0;
}



uint8_t write_data_to_uart1(uint8_t *buf, uint32_t len)
{
    if(len == 0 || buf == NULL)
    {
        return 1;
    }
    
    uart_write_bytes(UART_NUM_1, (const char *)buf, len);

    return 0;
}




uint8_t  write_data_to_uart2(uint8_t *buf, uint32_t len)
{
    if(len == 0 || buf == NULL)
    {
        return 1;
    }

    uart_write_bytes(UART_NUM_2, (const char *)buf, len);

    return 0;
}








