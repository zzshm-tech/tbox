

#ifndef _MON_H
#define _MON_H

unsigned short int read_power_vol(void);
unsigned short int read_batter_vol(void);
void task_in(void *data);
unsigned char read_acc_state(void);
unsigned char read_shell_state(void);
unsigned char read_ant_state(void);
void process_in(void);

#endif

