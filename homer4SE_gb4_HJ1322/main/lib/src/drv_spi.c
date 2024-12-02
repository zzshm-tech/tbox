



#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "drv_gpio.h"
#include "drv_spi.h"




esp_err_t spi_device_bus_transmit(spi_device_handle_t handle, spi_transaction_t *trans_desc)
{
    esp_err_t err;

    err = spi_device_acquire_bus(handle, portMAX_DELAY);
    if (err != ESP_OK) 
    {
        printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
        return err;
    }

    err = spi_device_transmit(handle, trans_desc);
    if (err != ESP_OK) 
    {
        printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
        return err;
    }

    spi_device_release_bus(handle);

    return ESP_OK;
}




/***************************************
**  
****************************************/

esp_err_t spi_write_data(spi_device_handle_t spi, const uint8_t *data, uint32_t len)
{
    spi_transaction_t spi_packet;

    memset(&spi_packet, 0, sizeof(spi_packet));

    if (len <= 0)
    {
        return ESP_FAIL;
    }

    spi_packet.length = len * 8;                         //Len is in bytes, transaction length is in bits.
    spi_packet.tx_buffer = data;                         //Data

    esp_err_t ret = spi_device_bus_transmit(spi, &spi_packet); 
    return ret;
}




esp_err_t spi_read_data(spi_device_handle_t spi, uint8_t *buf, uint32_t len)
{
    spi_transaction_t spi_packet;
    
    memset(&spi_packet, 0, sizeof(spi_packet));

    spi_packet.tx_buffer = buf;         
    spi_packet.length = 8 * len;            

    spi_packet.rx_buffer = buf;
    spi_packet.rxlength = 8 * len;                      
                        
    esp_err_t ret = spi_device_bus_transmit(spi, &spi_packet);

    return ret;
}




/******************************
**  初始化SPI
*******************************/

uint8_t  rt_hw_init_spi(void)
{
    esp_err_t ret;

    spi_bus_config_t buscfg = {
        .miso_io_num = GPIO_SPI_MISO,
        .mosi_io_num = GPIO_SPI_MOSI,
        .sclk_io_num = GPIO_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    ret = spi_bus_initialize(SPI2_HOST, &buscfg,SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) 
    {
        printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
        return 1;       
    }

    return 0;
}








