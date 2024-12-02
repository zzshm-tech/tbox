/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-01-05     Bernard      the first version
 */
#ifndef __DRV_GPIO_H__
#define __DRV_GPIO_H__

#include <stm32f4xx.h>
#include <stm32f4xx_hal.h>

#include <drivers/pin.h>

/****2020.3.22****/
#define LED_YELLOW_PIN		      90

#define LED_GREEN_PIN 				  88

#define LED_BLUE_PIN					  89		

#define LED_RED_PIN							85			


/***LED控制引脚**/


#define rt_led_yellow_on() rt_pin_write(LED_YELLOW_PIN,PIN_HIGH)
#define rt_led_yellow_off() rt_pin_write(LED_YELLOW_PIN,PIN_LOW)

#define rt_led_blue_on() rt_pin_write(LED_BLUE_PIN,PIN_HIGH)
#define rt_led_blue_off() rt_pin_write(LED_BLUE_PIN,PIN_LOW)

#define rt_led_green_on() rt_pin_write(LED_GREEN_PIN,PIN_HIGH)
#define rt_led_green_off() rt_pin_write(LED_GREEN_PIN,PIN_LOW)

#define rt_led_red_on() rt_pin_write(LED_RED_PIN,PIN_HIGH)
#define rt_led_red_off() rt_pin_write(LED_RED_PIN,PIN_LOW)

#define rt_led_yellow_flip() rt_pin_read(LED_YELLOW_PIN) == 1 ? rt_led_yellow_off() : rt_led_yellow_on()
#define rt_led_blue_flip() rt_pin_read(LED_BLUE_PIN) == 1 ? rt_led_blue_off() : rt_led_blue_on()
#define rt_led_green_flip() rt_pin_read(LED_GREEN_PIN) == 1 ? rt_led_green_off() : rt_led_green_on()
#define rt_led_red_flip() rt_pin_read(LED_RED_PIN) == 1 ? rt_led_red_off() : rt_led_red_on()




/***RS232 电源控制接口**/
#define PIN_RS232_POWER		67


#define rt_rs232_power_on() rt_pin_write(PIN_RS232_POWER,	PIN_HIGH)
#define rt_rs232_power_off() rt_pin_write(PIN_RS232_POWER,	PIN_LOW)


/***CAN 控制接口**/
#define PIN_CAN_POWER 77

#define PIN_CAN1_STB	2
#define PIN_CAN2_STB  4


#define rt_can_power_on() rt_pin_write(PIN_CAN_POWER,PIN_HIGH)
#define rt_can_power_off() rt_pin_write(PIN_CAN_POWER,PIN_LOW)

#define rt_can1_stb_on() rt_pin_write(PIN_CAN1_STB,PIN_HIGH)
#define rt_can1_stb_off() rt_pin_write(PIN_CAN1_STB,PIN_LOW)

#define rt_can2_stb_on() rt_pin_write(PIN_CAN2_STB,PIN_HIGH)
#define rt_can2_stb_off() rt_pin_write(PIN_CAN2_STB,PIN_LOW)


/***GNSS 控制接口**/

#define PIN_GNSS_POWER	45
#define PIN_GNSS_VBACK  44

#define rt_gnss_power_on() rt_pin_write(PIN_GNSS_POWER,PIN_HIGH)
#define rt_gnss_power_off() rt_pin_write(PIN_GNSS_POWER,PIN_LOW)

#define rt_gnss_vback_on() rt_pin_write(PIN_GNSS_VBACK,PIN_HIGH)
#define rt_gnss_vback_off() rt_pin_write(PIN_GNSS_VBACK,PIN_LOW)


/***LTE 控制接口**/

#define PIN_LTE_POWER  	41
#define PIN_LTE_PWK   	26

#define rt_lte_power_on() rt_pin_write(PIN_LTE_POWER,PIN_HIGH)
#define rt_lte_power_off() rt_pin_write(PIN_LTE_POWER,PIN_LOW)

#define rt_lte_pwk_high() rt_pin_write(PIN_LTE_PWK,PIN_HIGH)
#define rt_lte_pwk_low() rt_pin_write(PIN_LTE_PWK,PIN_LOW)


/**Flash Mem And EMMC 电源接口**/
#define PIN_MEM_POWER		59

#define rt_mem_power_on() rt_pin_write(PIN_MEM_POWER,PIN_HIGH)
#define rt_mem_power_off() rt_pin_write(PIN_MEM_POWER,PIN_LOW)


#define PIN_BT_POWER  46
#define rt_bt_power_on() rt_pin_write(PIN_BT_POWER,PIN_HIGH)
#define rt_bt_power_off() rt_pin_write(PIN_BT_POWER,PIN_LOW)


/**Flash Mem 控制接口**/

#define PIN_BAT_STDBY						36       /****/

#define PIN_BAT_CHRG     				26      /****/

#define PIN_BAT_CHARG_CE   			98      /****/

#define rt_read_bat_stdby_state() rt_pin_read(PIN_BAT_STDBY)

#define rt_read_bat_chrg_state() rt_pin_read(PIN_BAT_CHRG)

#define PIN_MOTO_STATE 					7       /** DIG_DET1 **/
#define PIN_ACC_STATE	        	5       /** DIG_DET2 **/
#define PIN_DI_1_STATE         	3     	/** DIG_DET3 **/
#define PIN_DI_2_STATE        	1    		/** DIG_DET4 **/


#define rt_read_moto_state() rt_pin_read(PIN_MOTO_STATE)
#define rt_read_acc_state() rt_pin_read(PIN_ACC_STATE)
#define rt_read_di_1_state() rt_pin_read(PIN_DI_1_STATE)
#define rt_read_di_2_state() rt_pin_read(PIN_DI_2_STATE)

#define PIN_EEP_WP 39
#define rt_eep_wp_low() rt_pin_write(PIN_EEP_WP,PIN_LOW)



//蓝牙部分
#define PIN_GPIO_BLUE_TOOTH_POWER 46
//蓝牙链接状态
#define PIN_GPIO_BLUE_TOOTH_CONNECT 43
//蓝牙工作模式
#define PIN_GPIO_BLUE_TOOTH_CMD 42



/*************************************
**      
**************************************/
#define  rt_power_bt_on() 	rt_pin_write(PIN_GPIO_BLUE_TOOTH_POWER,PIN_HIGH)
#define  rt_power_bt_off()  rt_pin_write(PIN_GPIO_BLUE_TOOTH_POWER,PIN_LOW)


#define rt_bt_mode_nor_at()  rt_pin_write(PIN_GPIO_BLUE_TOOTH_CMD,PIN_HIGH)
#define rt_bt_mode_not_at()  rt_pin_write(PIN_GPIO_BLUE_TOOTH_CMD,PIN_LOW)
#define rt_bt_read_connect() rt_pin_read(PIN_GPIO_BLUE_TOOTH_CONNECT)



#define PIN_ACL_WAK     		    57
#define rt_acl_wak_high()	 	    rt_pin_write(PIN_ACL_WAK, PIN_HIGH)
#define rt_acl_wak_low()	 			rt_pin_write(PIN_ACL_WAK, PIN_LOW)


#define PIN_ACL_RST    		        58
#define rt_acl_reset()	 	      rt_pin_write(PIN_ACL_RST, PIN_LOW)
#define rt_acl_unreset()	 	    rt_pin_write(PIN_ACL_RST, PIN_HIGH)



int rt_hw_gpio_init(void);

int rt_hw_gpio_close(void);


#endif
