



#ifndef _CAN_H
#define _CAN_H

#include "data_type.h"

void read_can_info(struct can_info_str  *pcan);
void task_can_lock(void *data);
void task_can_rx(void *data);
unsigned char read_can_connect_state(void);
unsigned char read_lock_expect_state(void);
unsigned char read_mon_expect_state(void);
unsigned short int read_can_actual_rotate(void);

#endif




