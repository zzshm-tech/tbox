

/****************************
**
*****************************/

#ifndef _DRV_EMMC_H
#define _DRV_EMMC_H

#include <stdint.h>


#define SDIO_TIMEOUT ((uint32_t)0x100000)




struct rt_device_blk_geometry
{
    uint32_t sector_count;                           /**< count of sectors */
    uint32_t bytes_per_sector;                       /**< number of bytes per sector */
    uint32_t block_size;                             /**< number of bytes to erase one block */
};




uint8_t rt_hw_init_emmc(void);

#endif



/**********************File End*****************/


