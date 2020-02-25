#ifndef SPI_FLASH_GD_H_
#define SPI_FLASH_GD_H_

#include "board.h"

#define sFlash_PAGE_SIZE 4096
#define sFlash_MF_ID (0xC8)
#define sFlash_MTC_GD25Q128 (0x4018)

#define sFLASH_DUMMY_BYTE 0x55

#define sFLASH_CMD_NOR_WRITE 0x02 /* 写入数据*/
#define sFLASH_CMD_NOR_WRSR 0x01  /*!< Write Status Register instruction */
#define sFLASH_CMD_NOR_WREN 0x06  /* 开启允许写命令*/
#define sFLASH_CMD_NOR_WDIS 0x04  /* 关闭允许写命令*/
#define sFLASH_CMD_NOR_READ 0x03  /* 读出数据*/
#define sFLASH_CMD_NOR_SFDP 0x5A  /*!< Read from Memory instruction */
#define sFLASH_CMD_NOR_RDSR 0x05  /* 读状态寄存器*/
#define sFLASH_CMD_NOR_MISR 0x35  /*!< Read Status Register instruction Midde Byte */
#define sFLASH_CMD_NOR_HISR 0x15  /*!< Read Status Register instruction Hige Byte */
#define sFLASH_CMD_NOR_RDID 0x9F  /* 读器件id*/
#define sFLASH_CMD_NOR_SE 0x20    /* 扇区擦除指令*/
#define sFLASH_CMD_NOR_BE 0xD8    /*!< Block Erase instruction */
#define sFLASH_CMD_NOR_CE 0xC7    /*!< Chip Erase instruction */

#define sFLASH_WIP_FLAG 0x01 /*!< Write In Progress (WIP) flag */

struct flash_geometry
{
    unsigned int sector_count;     /* 扇区个数*/
    unsigned int bytes_per_sector; /* 每个扇区的字节数 */
    unsigned int block_size;       /* 块大小(擦除的最小单位大小) */
};

extern int nor_flash_init(void);
extern unsigned int w25qxx_flash_read(unsigned char *buff,unsigned int sectors, unsigned int num);
extern unsigned int w25qxx_flash_write(unsigned char *buff, unsigned int sectors, unsigned int num);
extern unsigned int w25qxx_flash_control(struct flash_geometry *args);
#endif /* SPI_FLASH_GD_H_ */
