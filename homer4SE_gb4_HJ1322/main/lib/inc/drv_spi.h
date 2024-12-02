


#ifndef _DRV_SPI_H
#define _DRV_SPI_H


#include "driver/spi_master.h"

uint8_t  rt_hw_init_spi(void);

esp_err_t spi_write_data(spi_device_handle_t spi, const uint8_t *data, uint32_t len);
esp_err_t spi_read_data(spi_device_handle_t spi, uint8_t *buf, uint32_t len);


#endif



