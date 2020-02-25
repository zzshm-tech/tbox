
/********************************************
**	
**	
*********************************************/

#ifndef FIFODATA_H
#define FIFODATA_H

#include "data_type.h"

unsigned short int ReadSendQueue(unsigned char *Source);
unsigned char WriteSendQueue(unsigned char *Buf,unsigned short int DataLen);
void ClearSendQueue(void);
void WriteCmdDataBuf(unsigned char *Buf,unsigned short int DataLen);
unsigned short int ReadCmdDataBuf(unsigned char *buf);
void ClearCmdData(void);
void SaveSendQueue(void);
void LoadSendQueue(void);
void ClearBindData(void);

SendQueueStr *GetSendQueueSpace(void);

void test_SendQueue(void);
void init_fifo_sema(void);


#endif



/***********************File End*********************/


