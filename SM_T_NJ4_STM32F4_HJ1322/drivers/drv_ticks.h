

#ifndef _DRV_TICKS_H
#define _DRV_TICKS_H

#include <stdint.h>
#include "data_type.h"



void stopwatch16_init(struct stopwatch16 *sw);
void stopwatch32_init(struct stopwatch32 *sw);
uint16_t stopwatch16_read_value(struct stopwatch16 *sw);
uint32_t stopwatch32_read_value(struct stopwatch32 *sw);
void fr_init_stk(void);


#endif



