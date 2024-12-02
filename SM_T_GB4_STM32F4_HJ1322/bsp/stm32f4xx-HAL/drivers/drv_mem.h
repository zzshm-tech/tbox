


#ifndef _DRV_MEM_H
#define _DRV_MEM_H


#define FLASH_Sector_0     ((uint16_t)0x0000) /*!< Sector Number 0   */
#define FLASH_Sector_1     ((uint16_t)0x0001) /*!< Sector Number 1   */
#define FLASH_Sector_2     ((uint16_t)0x0002) /*!< Sector Number 2   */
#define FLASH_Sector_3     ((uint16_t)0x0003) /*!< Sector Number 3   */
#define FLASH_Sector_4     ((uint16_t)0x0004) /*!< Sector Number 4   */
#define FLASH_Sector_5     ((uint16_t)0x0005) /*!< Sector Number 5   */
#define FLASH_Sector_6     ((uint16_t)0x0006) /*!< Sector Number 6   */
#define FLASH_Sector_7     ((uint16_t)0x0007) /*!< Sector Number 7   */
#define FLASH_Sector_8     ((uint16_t)0x0008) /*!< Sector Number 8   */
#define FLASH_Sector_9     ((uint16_t)0x0009) /*!< Sector Number 9   */
#define FLASH_Sector_10    ((uint16_t)0x0010) /*!< Sector Number 10  */
#define FLASH_Sector_11    ((uint16_t)0x0011) /*!< Sector Number 11  */



#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */


#define BOOTLOADER_SIZE 				(1024 *  32)					/***************/
#define	DATA_FILED_SIZE					(1024 *  32) 					/***************/	
#define APP_RUN_SIZE       			(1024 *  192)        	/***************/
#define APP_BACK_SIZE						(1024 *  256)

#define ADDR_BOOTLOADER     		0x08000000
#define ADDR_DATA_FILED     		(ADDR_BOOTLOADER + BOOTLOADER_SIZE) /***************/
#define ADDR_APP_RUN        		(ADDR_DATA_FILED + DATA_FILED_SIZE) /***************/
#define	ADDR_APP_BACK						(ADDR_APP_RUN + APP_RUN_SIZE)



#define BASE_ADDR								0x08000000

#define SECTOR0_SIZE   					(1024 *  16)

#define SECTOR1_SIZE   					(1024 *  16)
#define SECTOR2_SIZE   					(1024 *  16) 
#define SECTOR3_SIZE   					(1024 *  16) 
#define SECTOR4_SIZE   					(1024 *  64) 
#define SECTOR5_SIZE   					(1024 * 128)

#define SECTOR6_SIZE   					(1024 * 128) 
#define SECTOR7_SIZE   					(1024 * 128)

#define SECTOR8_SIZE   					(1024 * 128) 
#define SECTOR9_SIZE   					(1024 * 128) 
#define SECTOR10_SIZE   				(1024 * 128) 
#define SECTOR11_SIZE   				(1024 * 128) 



#define ADDR_SECTOR0						(BASE_ADDR)
#define ADDR_SECTOR1						(ADDR_SECTOR0 + SECTOR0_SIZE)
#define ADDR_SECTOR2						(ADDR_SECTOR1 + SECTOR1_SIZE)
#define ADDR_SECTOR3						(ADDR_SECTOR2 + SECTOR2_SIZE)
#define ADDR_SECTOR4						(ADDR_SECTOR3 + SECTOR3_SIZE)
#define ADDR_SECTOR5						(ADDR_SECTOR4 + SECTOR4_SIZE)
#define ADDR_SECTOR6						(ADDR_SECTOR5 + SECTOR5_SIZE)
#define ADDR_SECTOR7						(ADDR_SECTOR6 + SECTOR6_SIZE)
#define ADDR_SECTOR8						(ADDR_SECTOR7 + SECTOR7_SIZE)
#define ADDR_SECTOR9						(ADDR_SECTOR8 + SECTOR8_SIZE)
#define ADDR_SECTOR10						(ADDR_SECTOR9 + SECTOR9_SIZE)
#define ADDR_SECTOR11						(ADDR_SECTOR10 + SECTOR10_SIZE)



#define APP_DATA_MAX_SIZE 					(1024 * 200)
#define APP_DATA_MIN_SIZE 					(1024 * 50)




#define RT_DEVICE_CTRL_INNER_MEM_ERASE   0

#include "board.h"

/*****************************
**
******************************/

struct inner_mem_device
{
	struct rt_device parent;
  rt_mutex_t lock;
};


/************************************
**
**************************************/

struct erase_mem_args
{
	rt_uint32_t block_addr;
	rt_uint32_t size;
};




int rt_hw_inner_mem_init(void);



#endif


