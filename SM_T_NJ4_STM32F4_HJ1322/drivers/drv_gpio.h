
#ifndef __DRV_GPIO_H__
#define __DRV_GPIO_H__


#include <stdint.h>


//LED灯部分

#define PORT_GPIO_LED_RED GPIOD
#define PIN_GPIO_LED_RED GPIO_Pin_4

#define PORT_GPIO_LED_BLUE GPIOC
#define PIN_GPIO_LED_BLUE GPIO_Pin_5

#define PORT_GPIO_LED_YELLOW GPIOB
#define PIN_GPIO_LED_YELLOW GPIO_Pin_4

#define PORT_GPIO_LED_GREEN GPIOB
#define PIN_GPIO_LED_GREEN GPIO_Pin_3

//RS232通讯电源
#define PORT_GPIO_RS232_POWER GPIOA
#define PIN_GPIO_RS232_POWER GPIO_Pin_8

#define PORT_GPIO_CAN_POWER GPIOA
#define PIN_GPIO_CAN_POWER GPIO_Pin_15

//can sleep
#define PORT_GPIO_CAN1_STB GPIOE
#define PIN_GPIO_CAN1_STB GPIO_Pin_3

#define PORT_GPIO_CAN2_STB GPIOE
#define PIN_GPIO_CAN2_STB GPIO_Pin_5


//GNSS电源
#define PORT_GPIO_GNSS_POWER GPIOB
#define PIN_GPIO_GNSS_POWER GPIO_Pin_4


//GNSS模块vback引脚
#define PORT_GPIO_GNSS_VBACK GPIOE
#define PIN_GPIO_GNSS_VBACK GPIO_Pin_13

//LTE模块供电
#define PORT_GPIO_LTE_POWER GPIOE
#define PIN_GPIO_LTE_POWER GPIO_Pin_10

//LTE模块开机按钮
#define PORT_GPIO_LTE_PWK GPIOE
#define PIN_GPIO_LTE_PWK GPIO_Pin_4

//LTE模块复位按钮
#define PORT_GPIO_LTE_RESET GPIOA
#define PIN_GPIO_LTE_RESET GPIO_Pin_2

//jdqk dect:not used
#define PORT_GPIO_JDQK GPIOC
#define PIN_GPIO_JDQK GPIO_Pin_0

//acc
#define PORT_GPIO_ACC GPIOA
#define PIN_GPIO_ACC GPIO_Pin_0


//2路模拟量(power_adc  bat_adc)
#define PORT_GPIO_ADC_BAD GPIOC
#define PIN_GPIO_ADC_BAD GPIO_Pin_2

#define PORT_GPIO_ADC_POW GPIOB
#define PIN_GPIO_ADC_POW GPIO_Pin_1

//flash /eeprom 电源
#define PORT_GPIO_FLASH_POWER GPIOD
#define PIN_GPIO_FLASH_POWER GPIO_Pin_12


//SPI HOLD/WP
#define PORT_GPIO_SPI1_HOLD GPIOB
#define PIN_GPIO_SPI1_HOLD GPIO_Pin_0
#define rt_flash_hold_on() GPIO_SetBits(PORT_GPIO_SPI1_HOLD, PIN_GPIO_SPI1_HOLD)
#define rt_flash_hold_off() GPIO_ResetBits(PORT_GPIO_SPI1_HOLD, PIN_GPIO_SPI1_HOLD)

#define PORT_GPIO_SPI1_WP GPIOC
#define PIN_GPIO_SPI1_WP GPIO_Pin_4
#define rt_flash_wp_on() GPIO_SetBits(PORT_GPIO_SPI1_WP, PIN_GPIO_SPI1_WP)
#define rt_flash_wp_off() GPIO_ResetBits(PORT_GPIO_SPI1_WP, PIN_GPIO_SPI1_WP)

//蓝牙部分
#define PORT_GPIO_BLUE_TOOTH_POWER GPIOE
#define PIN_GPIO_BLUE_TOOTH_POWER GPIO_Pin_15
//蓝牙链接状态
#define PORT_GPIO_BLUE_TOOTH_CONNECT GPIOE
#define PIN_GPIO_BLUE_TOOTH_CONNECT GPIO_Pin_12
//蓝牙工作模式
#define PORT_GPIO_BLUE_TOOTH_CMD GPIOE
#define PIN_GPIO_BLUE_TOOTH_CMD GPIO_Pin_11

#define PORT_GPIO_PWM_ONE GPIOE
#define PIN_GPIO_PWM_ONE GPIO_Pin_0

#define PORT_GPIO_PWM_TWO GPIOE
#define PIN_GPIO_PWM_TWO GPIO_Pin_1


int rt_hw_gpio_init(void);



void rt_led_yellow_on(void);
void rt_led_yellow_off(void);
void rt_led_blue_on(void);
void rt_led_blue_off(void);
void rt_led_green_on(void);
void rt_led_green_off(void);
void rt_led_red_on(void);
void rt_led_red_off(void);

void rt_lte_power_on(void);
void rt_lte_power_off(void);

void rt_lte_switch_low(void);
void rt_lte_switch_high(void);

void rt_lte_reset_low(void);
void rt_lte_reset_high(void);

void rt_can1_stb_on(void);
void rt_can1_stb_off(void);


void rt_gnss_power_on(void);
void rt_gnss_power_off(void);
void rt_gnss_back_low(void);
void rt_gnss_back_high(void);



/*************************************
**      
**************************************/

void rt_power_bt_on(void);
void rt_power_bt_off(void);
void rt_bt_mode_nor_at(void);
void rt_bt_mode_not_at(void);
uint8_t rt_bt_read_connect(void);

void rt_pwm_one_on(void);
void rt_pwm_two_on(void);
void rt_pwm_one_off(void);
void rt_pwm_two_off(void);



#endif /* end of include guard: __DRV_GPIO_H__ */

