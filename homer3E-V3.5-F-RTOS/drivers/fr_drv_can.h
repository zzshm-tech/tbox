


#ifndef _FR_DRV_CAN_H
#define _FR_DRV_CAN_H


#include "data_type.h"

void fr_can1_init(unsigned short int baudrate);
void fr_set_can1_rx_hook(void (*hook)(unsigned int can_id,unsigned char *data_buf));
void fr_init_can1_sema(void);
void fr_can1_tx_msg(unsigned short int baudrate, struct can_tx_data_str *data);

#endif




