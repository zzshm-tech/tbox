


#ifndef _FR_DRV_SPI_H
#define _FR_DRV_SPI_H

#include "stm32f10x.h"

#define SPI_FLASH_CS_LOW()       GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define SPI_FLASH_CS_HIGH()      GPIO_SetBits(GPIOA, GPIO_Pin_4)

void fr_init_spi1(void);
unsigned char spi_flash_sendbyte(unsigned char  byte);
uint8_t spi_read_write_byte(SPI_TypeDef *SPIx, uint8_t t_data);

#endif


