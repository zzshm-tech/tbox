


#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"

#include "drv_i2c.h"




#define I2C_SCL_PIN                 GPIO_PIN_10
#define I2C_SCL_PORT                GPIOB
//#define I2C_SCL_PORT_CLK_ENABLE     __HAL_RCC_GPIOB_CLK_ENABLE
#define I2C_SDA_PIN                 GPIO_PIN_11
#define I2C_SDA_PORT                GPIOB
//#define I2C_SDA_PORT_CLK_ENABLE     __HAL_RCC_GPIOB_CLK_ENABLE



/*****************************
**
******************************/

uint32_t i2c_read(uint16_t dev_addr,uint32_t mem_addr,uint32_t len)
{
	return 0;
}

/*****************************
**
******************************/

uint32_t i2c_write(uint16_t dev_addr,uint32_t mem_addr,uint32_t len)
{
	return 0;
}


