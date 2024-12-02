


#include "drv_spi.h"
#include "drv_acl16.h"



#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>


#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_system.h"

#include "drv_spi.h"
#include "drv_acl16.h"
#include "drv_uart.h"
#include "drv_gpio.h"




/***************** ACL 关机命令 ************************/




static spi_device_handle_t 				spi_acl16_handler;



/********************************************************
**	
*********************************************************/
uint8_t rt_hw_init_acl16(void)
{
    esp_err_t  res;

	spi_device_interface_config_t devcfg =
	{
        .clock_speed_hz = 2 * 1000 * 1000,           //Clock out at 2 MHz
        .mode = 0,                                //SPI mode 0
        .spics_io_num = GPIO_SPI_CS1,               //CS pin
        .queue_size = 7,                          //We want to be able to queue 7 transactions at a time  queued using spi_device_queue_trans but not yet finished using spi_device_get_trans_result

    };

    //res = rt_spi_init();
	rt_hw_init_spi();
	// Attach the dev to the SPI bus
	res = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_acl16_handler);
	if (res != ESP_OK)
	{
		printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
		return 1;
	}

	return 0;
}




/******************************************************
 * @desc  : acl16 写数据
 * @param : buf  数据
			len 数据长度
 * @return: 成功写入的长度
 *****************************************************/
uint16_t rt_write_data_to_acl16(uint8_t *buf, uint16_t len)
{
	if(buf == NULL)
		return 0;
	if (ESP_OK != spi_write_data(spi_acl16_handler,buf,len)) 
	{
		printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
		
		return 0;
	}
	
	return len;
}

/******************************************************
 * @desc  : acl16 读数据
 * @param : buf  数据缓冲区
			len 数据缓冲区长度
 * @return: 读到的数据长度
 *****************************************************/
uint16_t rt_read_data_from_acl16(uint8_t *buf, uint16_t len)
{
	if(buf == NULL)
		return 0;

	if (ESP_OK != spi_read_data(spi_acl16_handler, buf, len))
	{
		printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
		return 0;
	}

	return len;
}















