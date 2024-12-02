

#ifndef _DRV_TIMER_H
#define _DRV_TIMER_H


#include <stdint.h>

uint8_t rt_hw_init_tim3(void);
void rt_irq_tim3_sethook(void (*hook)(void));

#endif

