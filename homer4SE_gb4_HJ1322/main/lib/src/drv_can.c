



#include <stdio.h>
#include  <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "freertos/semphr.h"


#include "drv_can.h"
#include "drv_gpio.h"


#define CAN_PORT_BAND   250



/****************************************************/

static SemaphoreHandle_t    can_mutex;

static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG(GPIO_CAN_TX,GPIO_CAN_RX);

static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();/* 暂时不过滤 */

#if CAN_PORT_BAND == 250

static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();

#else

static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();

#endif


/********************************************************
**  CAN发送函数
*********************************************************/
uint8_t rt_can_send(id_type type,uint32_t id, uint8_t *data,uint8_t data_len)
{
    twai_message_t can_msg = {0};

    xSemaphoreTake(can_mutex,portMAX_DELAY);

    can_msg.identifier = id;
    can_msg.extd = type;
    can_msg.data_length_code = data_len;
    memcpy(can_msg.data,data,can_msg.data_length_code);

    if(ESP_OK != twai_transmit(&can_msg,pdMS_TO_TICKS(100))) 
    {
        xSemaphoreGive(can_mutex);	
        return 1;
    }

    xSemaphoreGive(can_mutex);	

    return 0;      
}



/*******************************
**  
********************************/

uint8_t rt_hw_init_can(void)
{
    if (ESP_OK != twai_driver_install(&g_config, &t_config, &f_config))
    {
        printf("-- fail installed can driver\r\n");
    }

    if (ESP_ERR_INVALID_STATE == twai_start())
    {
        printf("-- can driver is not in stopped state, or is not installed\r\n");
    }

    // log_printf(LOG_UART, "- can driver init success\r\n");

    rt_can_normal();

    can_mutex = xSemaphoreCreateMutex();
    xSemaphoreGive(can_mutex);

    printf("-- can driver is now running\r\n");
    
    return 0;
}





