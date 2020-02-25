


#ifndef _GSM_H
#define _GSM_H

/********************************
**	GNSS定位模块任务：泰斗N303
**********************************/

void task_gsm(void *data);

unsigned char read_icc_id(unsigned char *source,unsigned char buf_size);
unsigned char read_gsm_signal(void);
unsigned char read_net_state(void);
void gsm_modle_close(void);
unsigned char gsm_send_data(unsigned char socket_num,unsigned char *data,unsigned short int data_size);


#endif


