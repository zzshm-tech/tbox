




#ifndef _DRV_CAN_H
#define _DRV_CAN_H

#include <stdint.h>


struct can_rx_msg
{
	uint32_t id;
	uint8_t data[8];
};


struct can_tx_msg
{
	uint32_t id;
	uint8_t data[8];
};



typedef enum 
{
    STD  = 0,
    EXTID 
    
}id_type;


uint8_t rt_hw_init_can(uint16_t baudrate);
uint16_t rt_read_can_rx_buf(uint8_t ch,struct can_rx_msg *p_msg,uint32_t size);
uint8_t rt_write_can_rx_buf(uint8_t ch,id_type type,uint32_t id, uint8_t *data,uint8_t len);
#endif



