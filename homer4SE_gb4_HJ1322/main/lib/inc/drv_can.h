


#ifndef _DRV_CAN_H
#define _DRV_CAN_H


#include "driver/twai.h"

/* 通用设置 */
#define TWAI_GENERAL_CONFIG(tx_io_num, rx_io_num)                       \
        {.mode = TWAI_MODE_NORMAL,                                      \
         .tx_io = tx_io_num, .rx_io = rx_io_num,                        \
        .clkout_io = TWAI_IO_UNUSED, .bus_off_io = TWAI_IO_UNUSED,      \
        .tx_queue_len = 5, .rx_queue_len = 5,                           \
        .alerts_enabled = TWAI_ALERT_NONE,  .clkout_divider = 0,        }



struct rt_can_msg
{
    uint32_t id  : 29;
    uint32_t ide : 1;
    uint32_t rtr : 1;
    uint32_t rsv : 1;
    uint32_t len : 8;
    uint32_t priv : 8;
    uint32_t hdr : 8;
    uint32_t reserved : 8;
    uint8_t data[8];
};




typedef enum 
{
    STD  = 0,
    EXTID 
    
}id_type;



uint8_t rt_hw_init_can(void);
uint8_t rt_can_send(id_type type,uint32_t id, uint8_t *data,uint8_t data_len);


#endif



