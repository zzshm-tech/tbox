


#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "drv_fram.h"


/** FRAM-AT24C **/

#define  FRAM_PAGE_SIZE		32   	  /**   FRAM每页字节数       **/
#define  FRAM_PAGE_NUM		256       /**   FRAM分页总数量       **/
#define  FRAM_DEVIDE_ADDR	0x1A0  	  /**   FRAM从机地址         **/




/**********************************
**
***********************************/

uint32_t  rt_fram_write(uint32_t addr,uint8_t *buf,uint32_t data_len)
{
  return 1;
  
}


/********************************
**
*********************************/

uint32_t rt_fram_read(uint32_t addr,uint8_t *buf,uint32_t data_len)
{
  return 1;
}





