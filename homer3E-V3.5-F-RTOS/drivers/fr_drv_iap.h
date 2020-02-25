



#ifndef	_FR_DRV_IAP_H_
#define _FR_DRV_IAP_H_

#include "stm32f10x.h"

FLASH_Status BackupEraseHandle(void);
unsigned int Receive_Packet(unsigned char *data, unsigned int length);
void UpgradeFailReset(void);
void UpgradeOkReset(void);

#endif



