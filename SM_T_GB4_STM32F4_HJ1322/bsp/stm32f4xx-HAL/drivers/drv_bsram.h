

#ifndef _DRV_BSRAM_H
#define _DRV_BSRAM_H




#include "board.h"




#define BACKUP_SRAM_SIZE		(4*1024)		//备份SRAM大小


#define RT_DEVICE_CTRL_BACK_SRAM_WRITE   0       /**д**/
#define RT_DEVICE_CTRL_BACK_SRAM_READ   1        /****/



struct back_sram_device
{
	struct rt_device parent;
  rt_mutex_t lock;
};


#endif


