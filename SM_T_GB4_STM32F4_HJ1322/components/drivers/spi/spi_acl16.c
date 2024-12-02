/*
 * Copyright (c)  博创联动
 *
 * 文件说明：6轴初始化    
 *
 * 修改记录：
 * Date           Author       Notes
 * 2020-04-28      WXY        创建文件
 */

/*******************************************************/
#include <stdint.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <stdio.h>
#include "drv_spi.h"
#include "spi_acl16.h"
#ifdef RT_USING_ACL16
struct rt_spi_device *acl_device;

/******************************************************
 * @desc  : acl16初始化
 * @param : 
 * @return: 
 *****************************************************/
int ACL16_Init(void)
{
	struct rt_spi_configuration acl_config;

	stm32_spi_bus_attach_device(RT_FLASH_CS2_PIN,RT_FLASH_SPI2_BUS_NAME,"aclspi");

	acl_device = (struct rt_spi_device *)rt_device_find("aclspi");
	if (RT_NULL == acl_device)
	{
		rt_kprintf("find acl_device is error\r\n");
	}
	acl_config.data_width = 8;
	acl_config.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
	acl_config.max_hz = 2 * 1000 *1000; 
	rt_spi_configure(acl_device, &acl_config);
	return 0;
}

INIT_DEVICE_EXPORT(ACL16_Init);


#endif
