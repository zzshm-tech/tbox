
#include <stdio.h>
#include <string.h>

#include "fr_drv_spi.h"


#include "stm32f10x.h"
#include "board.h"
#include "spi_flash_gd.h"

#include "FreeRTOS.h"


struct flash_geometry flash_info;

//#define FLASH_DEBUG
#ifdef FLASH_DEBUG
    #define flash_debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
    #define flash_debug(fmt, ...)
#endif

#define flash_cs_low() GPIO_ResetBits(SPI_PRO, SPI_CS_PIN)
#define flash_cs_high() GPIO_SetBits(SPI_PRO, SPI_CS_PIN)

static uint8_t flash_spi_read_write_byte(uint8_t t_data)
{
    return spi_read_write_byte(SPI_DEV, t_data);
}

static void flash_write_enable(void)
{
    flash_cs_low();

    /* Send "Write Enable" instruction*/
    flash_spi_read_write_byte(sFLASH_CMD_NOR_WREN);
    flash_cs_high();
}

//static void flash_write_disable(void)
//{
//    flash_cs_low();

//    /* Send "Write Enable" instruction*/
//    flash_spi_read_write_byte(sFLASH_CMD_NOR_WDIS);
//    flash_cs_high();
//}

static void flash_nor_wait_write_end(void)
{
    uint8_t flashstatus = 0;
    uint32_t timeout = 0;

    flash_cs_low();
    flash_spi_read_write_byte(sFLASH_CMD_NOR_RDSR); /* 读状态寄存器*/

    do
    {
        flashstatus = flash_spi_read_write_byte(sFLASH_CMD_NOR_RDSR);
        if ((timeout++) >= 0xffff)
            break;
    }
    while ((flashstatus & sFLASH_WIP_FLAG) == SET);   /* flash写结束寄存器状态为0*/
    flash_cs_high();
}

static uint32_t flash_nor_read_id(void)
{
    uint8_t data1, data2, data3;
    uint32_t data;

    flash_cs_low();
    flash_spi_read_write_byte(sFLASH_CMD_NOR_RDID);
    data1 = flash_spi_read_write_byte(sFLASH_DUMMY_BYTE);
    data2 = flash_spi_read_write_byte(sFLASH_DUMMY_BYTE);
    data3 = flash_spi_read_write_byte(sFLASH_DUMMY_BYTE);
    flash_cs_high();
    data = (data1 << 16) | (data2 << 8) | data3;

    return data;
}

static void flash_nor_sector_erase(uint32_t sector_addr)
{
    /* 发送写开启指令*/
    flash_write_enable();

    flash_cs_low();
    flash_spi_read_write_byte(sFLASH_CMD_NOR_SE);              /* 发送扇区擦除指令*/
    flash_spi_read_write_byte((sector_addr & 0xFF0000) >> 16); /* 擦除地址高字节*/
    flash_spi_read_write_byte((sector_addr & 0xFF00) >> 8);    /* 擦除地址中字节*/
    flash_spi_read_write_byte(sector_addr & 0xFF);             /* 擦除地址低字节*/
    flash_cs_high();

    /* 等待指令结束*/
    flash_nor_wait_write_end();
}

static void flash_nor_page_program(uint8_t *buff, uint32_t write_addr, uint16_t write_len)
{
    /* 发送写开启指令*/
    flash_write_enable();

    flash_cs_low();
    flash_spi_read_write_byte(sFLASH_CMD_NOR_WRITE);          /* 发送写入内存指令*/
    flash_spi_read_write_byte((write_addr & 0xFF0000) >> 16); /* 写入地址高字节*/
    flash_spi_read_write_byte((write_addr & 0xFF00) >> 8);    /* 写入地址中字节*/
    flash_spi_read_write_byte(write_addr & 0xFF);             /* 写入地址低字节*/

    while (write_len--)
    {
        flash_spi_read_write_byte(*buff);
        buff++;
    }
    flash_cs_high();

    /* 等待指令结束*/
    flash_nor_wait_write_end();
}

static void flash_nor_read(uint8_t *buff, uint32_t read_addr, uint16_t read_len)
{
    flash_cs_low();
    flash_spi_read_write_byte(sFLASH_CMD_NOR_READ);          /* 发送读数据指令*/
    flash_spi_read_write_byte((read_addr & 0xFF0000) >> 16); /* 读出地址高字节*/
    flash_spi_read_write_byte((read_addr & 0xFF00) >> 8);    /* 读出数据中字节*/
    flash_spi_read_write_byte(read_addr & 0xFF);             /* 读出数据低字节*/

    while (read_len--)
    {
        *buff = flash_spi_read_write_byte(sFLASH_DUMMY_BYTE);
        buff++;
    }
    flash_cs_high();
}

static uint8_t flash_nor_read_low_sr(void)
{
    uint8_t data = 0;
    flash_cs_low();
    flash_spi_read_write_byte(sFLASH_CMD_NOR_RDSR);
    data = flash_spi_read_write_byte(sFLASH_DUMMY_BYTE);
    flash_cs_high();
    return data;
}

static void w25qxx_wait_busy(void)
{
    uint32_t timeout = 0;
    while (flash_nor_read_low_sr() & (0x01))
    {
        if (timeout >= 0xffff)
            break;
    }
}

/**
 * @desc  : flash 写入一页数据
 * @param : page_add:页地址;
 * @return: 页大小
 * @date  : 2018-7-15 19:13:27
 */
static uint32_t w25qxx_page_write(uint32_t page_addr, uint8_t *buff)
{
    uint32_t index;

    flash_nor_sector_erase(page_addr); /* erase 4k*/
    w25qxx_wait_busy();                /* wait erase done.*/

    /* nor flash 的页缓存为256byte 超过256字节的部分将被丢弃,一页分四次写入*/
    for (index = 0; index < (sFlash_PAGE_SIZE / 256); index++)
    {
        flash_nor_page_program(buff, page_addr, 256);
        buff += 256;
        page_addr += 256;
        w25qxx_wait_busy();
    }
    return sFlash_PAGE_SIZE;
}
/**
 * @desc  : nor flash 读出数据
 * @param : buff* 数据地址; sectors 扇区地址; num: 扇区的个数
 * @return: 0:读出成功; 1:读出失败
 * @date  : 2018-7-15 20:22:12
 */
unsigned int  w25qxx_flash_read(unsigned char *buff, unsigned int sectors, unsigned int num)
{
    flash_nor_read(buff, sectors * flash_info.bytes_per_sector, num * flash_info.bytes_per_sector);
    return 0;
}
/**
 * @desc  : nor flash 写入数据
 * @param : buff* 数据地址; sectors 扇区地址; num 扇区个数
 * @return: 0:写入成功; 1写入失败
 * @date  : 2018-7-15 20:30:23
 */
unsigned int w25qxx_flash_write(unsigned char *buff, unsigned int sectors, unsigned int num)
{
    uint32_t i = 0;
    uint32_t block = num;
    uint8_t *ptr = buff;

    while (block--)
    {
        w25qxx_page_write((sectors + i) * flash_info.bytes_per_sector, ptr);
        ptr += sFlash_PAGE_SIZE;
        i++;
    }

    return 0;
}
/**
 * @desc  : 获取flash 信息
 * @param : cmd
 * @return: 1:获取失败; 0: 获取成功
 * @date  :
 */
uint32_t w25qxx_flash_control(struct flash_geometry *args)
{
    if (args == NULL)
        return 1;
    args->bytes_per_sector = flash_info.bytes_per_sector;
    args->sector_count = flash_info.sector_count;
    args->block_size = flash_info.block_size;
    return 0;
}
/**
 * @desc  : gd nor flash 初始化
 * @param : None
 * @return: 0:初始化成功; 1: 初始化失败
 * @date  :
 */
int nor_flash_init(void)
{
    unsigned int id_recv;

    /* read flash id */
    id_recv = flash_nor_read_id();

    if ((id_recv >> 16 & 0xff) != sFlash_MF_ID)
    {
        fr_printf("- manufacturers ID error!\r\n");
        fr_printf("- JEDEC Read-ID Data : 0x%06X\r\n", id_recv);
        return 1;
    }
    fr_printf("- nor flash id:0x%x\r\n", id_recv);

    flash_info.bytes_per_sector = 4096;
    flash_info.block_size = 4096; /* block erase: 4k */

    /* get memory type and capacity */
    if ((id_recv & 0xffff) == sFlash_MTC_GD25Q128)
    {
        fr_printf("GD128 detection\r\n");
        flash_info.sector_count = 4096;
    }
    else
    {
				fr_printf("Memory Capacity error!\r\n");
        return 1;
    }
    return 0;
}

