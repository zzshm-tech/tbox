


#ifndef _BOARD_H
#define _BOARD_H

#include <stdint.h>

#define bitset(var, bitno) ((var) |= 1UL << (bitno))
#define bitclr(var, bitno) ((var) &= ~(1UL << (bitno)))


void show_board_info(void);
void rt_reboot_sys(void);
uint8_t rt_hw_init_board(void);



#endif

