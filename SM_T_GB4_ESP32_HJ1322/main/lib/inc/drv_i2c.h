



#ifndef _DRV_I2C_H
#define _DRV_I2C_H

#include "drv_gpio.h"
#include "driver/i2c.h"

uint8_t rt_hw_init_i2c(void);
uint8_t  rt_i2c_write_data(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t size);
uint8_t  rt_i2c_read_data(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t size);
uint8_t  rt_i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t val);
uint8_t  rt_i2c_read_byte(uint8_t addr, uint8_t reg);

#endif 


