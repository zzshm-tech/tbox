#ifndef __DRV_TIMER_H__
#define __DRV_TIMER_H__

#include "board.h"

void rt_irq_timer3_sethook(void (*hook)(void));
void rt_irq_timer3_resethook(void);
#endif /* end of include guard: __DRV_TIMER_H__ */
