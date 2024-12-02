
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "ulp_common.h"

#include "board.h"
#include "drv_i2c.h"
#include "drv_aw9523b.h"
#include "drv_uart.h"
#include "drv_spi.h"
#include "drv_in.h"
#include "drv_rtc.h"




/*******************************
 ** 显示芯片信息
*******************************/


void show_board_info(void)
{
    esp_chip_info_t chip_info;

    esp_chip_info(&chip_info);   //获取芯片信息
        
    printf("\r\n\r\n");
    printf("-- This is %s chip with %d CPU core(s), WiFi%s%s \r\n",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("-- silicon revision %d  ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("-- Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    printf("\r\n");
}



/**************************
 **     重启系统
***************************/

void rt_reboot_sys(void)
{
        fflush(stdout);
        esp_restart();
}



/***************************
**      
*****************************/

uint8_t rt_hw_init_board(void)
{
        rt_i2c_gpio_init();
        rt_hw_init_i2c();        //GPIO infoace
        rt_hw_init_aw9523b();    //GPIO extern
        rt_hw_init_gpio();       //GPIO
        rt_hw_init_uart0();      //DEBUG
        rt_hw_init_uart1(460800);      //LTE
        //rt_hw_init_uart1(115200);      //LTE
        rt_hw_init_uart2(115200);      //GNSS
        rt_hw_init_adc();
        rt_hw_init_rtc();
        rt_set_moto_out_high();

        return 0;
}

