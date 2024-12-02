
#include <stdio.h>
#include <string.h>


#include "stm32f4xx.h"


#include "drv_nor_flash.h"
#include "drv_uart.h"


struct flash_info_str flash_info;       //闪存信息



/**********************************
**      SPI CS引脚选择
***********************************/

void rt_flash_cs_low(void)
{
	
}


/**********************************
**      SPI CS引脚选择
***********************************/

void rt_flash_cs_high(void)
{
  
}


/**********************************
**      写一个字节
***********************************/

uint8_t rt_flash_read_write_byte(uint8_t tx_byte)
{
  uint8_t rx_byte;
  
  rx_byte = 0;
  
  return rx_byte;
}


/**********************************
**      写使能
***********************************/

void rt_flash_write_enable(void)
{
  rt_flash_cs_low();
  
  rt_flash_read_write_byte(sFLASH_CMD_NOR_WREN);
  
  rt_flash_cs_high();
}


/**********************************
**      写使能
***********************************/

void rt_flash_write_disenable(void)
{
  rt_flash_cs_low();
  
  rt_flash_read_write_byte(sFLASH_CMD_NOR_WDIS);
  
  rt_flash_cs_high();
}

/**********************************
**      
***********************************/
void rt_flash_wait_write_end(void)
{
  
}


/***********************************
**
************************************/

void rt_flash_sector_erase(uint32_t sector_addr)
{
  rt_flash_write_enable();
  
  rt_flash_cs_low();
  
  rt_flash_read_write_byte(sFLASH_CMD_NOR_SE);              /* 发送扇区擦除指令*/
  rt_flash_read_write_byte((sector_addr & 0xFF0000) >> 16); /* 擦除地址高字节*/
  rt_flash_read_write_byte((sector_addr & 0xFF00) >> 8);    /* 擦除地址中字节*/
  rt_flash_read_write_byte(sector_addr & 0xFF);             /* 擦除地址低字节*/
  rt_flash_cs_high();
  
  rt_flash_wait_write_end();
}


void rt_flash_wait_busy(void)
{
  uint32_t      timeout = 0;
  uint8_t       data = 0;
  
  rt_flash_cs_low();
  
  while(1)
  {
    rt_flash_read_write_byte(sFLASH_CMD_NOR_RDSR);
    data = rt_flash_read_write_byte(sFLASH_DUMMY_BYTE);
    if(timeout >= 0xFFFF || (data & 0x01) == 0)
      break;
  }
  
  rt_flash_cs_high();
}




/*****************************
**      读取Flash ID
*******************************/

uint32_t rt_flash_read_id(void)
{
  uint8_t data1, data2, data3;
  uint32_t data;
  
  rt_flash_cs_low();
  rt_flash_read_write_byte(sFLASH_CMD_NOR_RDID);
  data1 = rt_flash_read_write_byte(sFLASH_DUMMY_BYTE);
  data2 = rt_flash_read_write_byte(sFLASH_DUMMY_BYTE);
  data3 = rt_flash_read_write_byte(sFLASH_DUMMY_BYTE);
  
  rt_flash_cs_high();
  data = (data1 << 16) | (data2 << 8) | data3;
  
  return data;
}

/**************************
**      初始化SPI
****************************/
int rt_hw_init_spi1(void)
{
  return 0;
}



/********************************
**      初始化nor flash
********************************/

int rt_hw_flash_init(void)
{
  uint32_t flash_id;
  
  flash_id = rt_flash_read_id();
  
  if((flash_id >> 16 & 0xff) != sFlash_MF_ID)
  {
    printf("-- manufacturers ID error!\r\n");
    return 1;
  }

  flash_info.bytes_per_sector = 4096;
  flash_info.block_size = 4096; /* block erase: 4k */

  /* get memory type and capacity */
  if ((flash_id & 0xffff) == sFlash_MTC_GD25Q128)
  {
    flash_info.sector_count = 2048;
    printf("-- W25Q64BV detection\r\n");
  }
  else
  {
     printf("-- Memory Capacity error!\r\n");
     return 1;
  }
    
  return 0;
}



int storage_medium_init(void)
{
  return 0;
}

