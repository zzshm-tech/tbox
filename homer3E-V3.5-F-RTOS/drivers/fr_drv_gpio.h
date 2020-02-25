

#ifndef _FR_DRV_GPIO_H_
#define _FR_DRV_GPIO_H_

#include "stm32f10x.h"


//LED
//led
#define PORT_GPIO_LED_RED GPIOD
#define PIN_GPIO_LED_RED GPIO_Pin_2

#define PORT_GPIO_LED_BLUE GPIOB
#define PIN_GPIO_LED_BLUE GPIO_Pin_3

#define PORT_GPIO_LED_GREEN GPIOB
#define PIN_GPIO_LED_GREEN GPIO_Pin_4

#define fr_led_red_on() GPIO_SetBits(PORT_GPIO_LED_RED, PIN_GPIO_LED_RED)
#define fr_led_red_off() GPIO_ResetBits(PORT_GPIO_LED_RED, PIN_GPIO_LED_RED)

#define fr_led_blue_on() GPIO_SetBits(PORT_GPIO_LED_BLUE, PIN_GPIO_LED_BLUE)
#define fr_led_blue_off() GPIO_ResetBits(PORT_GPIO_LED_BLUE, PIN_GPIO_LED_BLUE)

#define fr_led_green_on() GPIO_SetBits(PORT_GPIO_LED_GREEN, PIN_GPIO_LED_GREEN)
#define fr_led_green_off() GPIO_ResetBits(PORT_GPIO_LED_GREEN, PIN_GPIO_LED_GREEN)

#define fr_led_red_flip() GPIO_ReadOutputDataBit(PORT_GPIO_LED_RED, PIN_GPIO_LED_RED) < 1 ? fr_led_red_on() : fr_led_red_off();
#define fr_led_blue_flip() GPIO_ReadOutputDataBit(PORT_GPIO_LED_BLUE, PIN_GPIO_LED_BLUE) < 1 ? fr_led_blue_on() : fr_led_blue_off();
#define fr_led_green_flip() GPIO_ReadOutputDataBit(PORT_GPIO_LED_GREEN, PIN_GPIO_LED_GREEN) < 1 ? fr_led_green_on() : fr_led_green_off();

//can sleep
#define PORT_GPIO_CAN_POWER GPIOB
#define PIN_GPIO_CAN_POWER GPIO_Pin_9
#define fr_can_power_on() GPIO_ResetBits(PORT_GPIO_CAN_POWER, PIN_GPIO_CAN_POWER)
#define fr_can_power_off() GPIO_SetBits(PORT_GPIO_CAN_POWER, PIN_GPIO_CAN_POWER)

//gps电源
#define PORT_GPIO_GPS_POWER GPIOB
#define PIN_GPIO_GPS_POWER GPIO_Pin_12
#define fr_power_gps_on() GPIO_SetBits(PORT_GPIO_GPS_POWER, PIN_GPIO_GPS_POWER)
#define fr_power_gps_off() GPIO_ResetBits(PORT_GPIO_GPS_POWER, PIN_GPIO_GPS_POWER)

//gps vback
#define PORT_GPIO_GPS_VBACK GPIOB
#define PIN_GPIO_GPS_VBACK GPIO_Pin_14
#define fr_power_gps_vback_on() GPIO_SetBits(PORT_GPIO_GPS_VBACK, PIN_GPIO_GPS_POWER)
#define fr_power_gps_vback_off() GPIO_ResetBits(PORT_GPIO_GPS_VBACK, PIN_GPIO_GPS_POWER)

//gprs模块供电
#define PORT_GPIO_GPRS_POWER GPIOA
#define PIN_GPIO_GPRS_POWER GPIO_Pin_15
#define fr_power_gprs_on()                                                                              \
    {                                                                                                   \
        fr_gpio_set_mode(PORT_GPIO_GPRS_POWER, PIN_GPIO_GPRS_POWER, GPIO_Mode_Out_PP, GPIO_Speed_2MHz); \
        GPIO_ResetBits(PORT_GPIO_GPRS_POWER, PIN_GPIO_GPRS_POWER);                                      \
    }
#define fr_power_gprs_off() fr_gpio_set_mode(PORT_GPIO_GPRS_POWER, PIN_GPIO_GPRS_POWER, GPIO_Mode_IN_FLOATING, GPIO_Speed_2MHz);

//gprs 开机/关机 键
#define PORT_GPIO_GPRS_START_CTRL GPIOB
#define PIN_GPIO_GPRS_START_CTRL GPIO_Pin_5
#define fr_gprs_switch_low() GPIO_SetBits(PORT_GPIO_GPRS_START_CTRL, PIN_GPIO_GPRS_START_CTRL)
#define fr_gprs_switch_high() GPIO_ResetBits(PORT_GPIO_GPRS_START_CTRL, PIN_GPIO_GPRS_START_CTRL)

//gprs模块电源状态
#define PORT_GPIO_GPRS_STATE GPIOC
#define PIN_GPIO_GPRS_STATE GPIO_Pin_8
#define fr_power_gprs_state() GPIO_ReadInputDataBit(PORT_GPIO_GPRS_STATE, PIN_GPIO_GPRS_STATE)

////gprs 模块唤醒引脚
// #define PORT_GPIO_GPRS_WEAK_UP GPIOC
// #define PIN_GPIO_GPRS_WEAK_UP GPIO_Pin_7
// #define rt_gprs_weak_up_high() rt_gpio_set_mode(PORT_GPIO_GPRS_WEAK_UP, PIN_GPIO_GPRS_WEAK_UP, GPIO_Mode_IN_FLOATING, GPIO_Speed_2MHz);
// #define rt_gprs_weak_up_low()                                                                               \
//     {                                                                                                       \
//         rt_gpio_set_mode(PORT_GPIO_GPRS_WEAK_UP, PIN_GPIO_GPRS_WEAK_UP, GPIO_Mode_Out_PP, GPIO_Speed_2MHz); \
//         GPIO_ResetBits(PORT_GPIO_GPRS_WEAK_UP, PIN_GPIO_GPRS_WEAK_UP);                                      \
//     }

//jdqk dect:not used
#define PORT_GPIO_JDQK GPIOC
#define PIN_GPIO_JDQK GPIO_Pin_0
#define fr_power_jdqk_on() GPIO_SetBits(PORT_GPIO_JDQK, PIN_GPIO_JDQK)
#define fr_power_jdqk_off() GPIO_ResetBits(PORT_GPIO_JDQK, PIN_GPIO_JDQK)

//acc
#define PORT_GPIO_ACC GPIOA
#define PIN_GPIO_ACC GPIO_Pin_0
#define fr_gpio_state_acc() GPIO_ReadInputDataBit(PORT_GPIO_ACC, PIN_GPIO_ACC)

//2路模拟量(power_adc  bat_adc)
#define PORT_GPIO_ADC_BAD GPIOC
#define PIN_GPIO_ADC_BAD GPIO_Pin_2

#define PORT_GPIO_ADC_POW GPIOB
#define PIN_GPIO_ADC_POW GPIO_Pin_1

//flash /eeprom 电源
#define PORT_GPIO_FLASH_POWER GPIOB
#define PIN_GPIO_FLASH_POWER GPIO_Pin_13
#define fr_power_flash_on() GPIO_SetBits(PORT_GPIO_FLASH_POWER, PIN_GPIO_FLASH_POWER)
#define fr_power_flash_off() GPIO_ResetBits(PORT_GPIO_FLASH_POWER, PIN_GPIO_FLASH_POWER)

//SPI HOLD/WP
#define PORT_GPIO_SPI1_HOLD GPIOB
#define PIN_GPIO_SPI1_HOLD GPIO_Pin_0
#define fr_flash_hold_on() GPIO_SetBits(PORT_GPIO_SPI1_HOLD, PIN_GPIO_SPI1_HOLD)
#define fr_flash_hold_off() GPIO_ResetBits(PORT_GPIO_SPI1_HOLD, PIN_GPIO_SPI1_HOLD)

#define PORT_GPIO_SPI1_WP GPIOC
#define PIN_GPIO_SPI1_WP GPIO_Pin_4
#define fr_flash_wp_on() GPIO_SetBits(PORT_GPIO_SPI1_WP, PIN_GPIO_SPI1_WP)
#define fr_flash_wp_off() GPIO_ResetBits(PORT_GPIO_SPI1_WP, PIN_GPIO_SPI1_WP)

void fr_gpio_set_mode(GPIO_TypeDef *port, unsigned short int pin, GPIOMode_TypeDef mode, GPIOSpeed_TypeDef speed);
int fr_gpio_init(void);
unsigned char fr_read_gpio_state(GPIO_TypeDef* GPIOx, unsigned short int GPIO_Pin);

#endif /* end of include guard: __DRV_GPIO_H__ */
