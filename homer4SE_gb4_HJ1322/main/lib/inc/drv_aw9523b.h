


#ifndef _DRV_AW9523B_H
#define _DRV_AW9523B_H

#include <stdint.h>
#include <stdlib.h>

#include "driver/i2c.h"


/** Registers */
#define AW9523B_I2C_ADDRESS     0x5B    //< I2C base address for AW9523B
#define AW9523B_REG_ID          0x10    //< id register
#define AW9523B_ID              0x23    //< id value
#define AW9523B_P0_IN_STATE     0x00    //< P0 port input state
#define AW9523B_P1_IN_STATE     0x01    //< P1 port input state
#define AW9523B_P0_OUT_STATE    0x02    //< P0 port output state
#define AW9523B_P1_OUT_STATE    0x03    //< P1 port output state
#define AW9523B_P0_CONF_STATE   0x04    //< P0 port config state
#define AW9523B_P1_CONF_STATE   0x05    //< P1 port config state
#define AW9523B_REG_GLOB_CTR    0x11    //< Global control register
#define AW9523B_P0_LED_MODE     0x12    //< P0 port led mode switch register
#define AW9523B_P1_LED_MODE     0x13    //< P1 port led mode switch register
#define AW9523B_REG_SOFT_RST    0x7F    //< Soft reset register

#define AW9523B_REG_LED_P1_0      0x20   // P1_0 口 LED 驱动电流配置
#define AW9523B_REG_LED_P1_1      0x21   // P1_1 口 LED 驱动电流配置
#define AW9523B_REG_LED_P1_2      0x22   // P1_2 口 LED 驱动电流配置
#define AW9523B_REG_LED_P1_3      0x23   // P1_3 口 LED 驱动电流配置
#define AW9523B_REG_LED_P0_0      0x24   // P0_0 口 LED 驱动电流配置
#define AW9523B_REG_LED_P0_1      0x25   // P0_1 口 LED 驱动电流配置
#define AW9523B_REG_LED_P0_2      0x26   // P0_2 口 LED 驱动电流配置
#define AW9523B_REG_LED_P0_3      0x27   // P0_3 口 LED 驱动电流配置
#define AW9523B_REG_LED_P0_4      0x28   // P0_4 口 LED 驱动电流配置
#define AW9523B_REG_LED_P0_5      0x29   // P0_5 口 LED 驱动电流配置
#define AW9523B_REG_LED_P0_6      0x2a   // P0_6 口 LED 驱动电流配置
#define AW9523B_REG_LED_P0_7      0x2b   // P0_7 口 LED 驱动电流配置
#define AW9523B_REG_LED_P1_4      0x2c   // P1_4 口 LED 驱动电流配置
#define AW9523B_REG_LED_P1_5      0x2d   // P1_5 口 LED 驱动电流配置
#define AW9523B_REG_LED_P1_6      0x2e   // P1_6 口 LED 驱动电流配置
#define AW9523B_REG_LED_P1_7      0x2f   // P1_7 口 LED 驱动电流配置

typedef enum
{
    AW9523B_LED_MODE = 0,           // 0：LED 模式
    AW9523B_GPIO_MODE = 1,          // 1：GPIO 模式
} aw9523b_pin_mode_e;

typedef enum
{
    W9523B_GPIO_MODE_OUTPUT = 0,    // 0-输出模式
    W9523B_GPIO_MODE_INPUT = 1,     // 1-输入模式
} aw9523b_gpio_mode_e;

typedef enum 
{
    AW9523B_MODE_OPEN_DRAIN = 0x00, // Port 0 open drain mode
    AW9523B_MODE_PUSH_PULL = 1 << 4 // Port 0 push pull mode
} aw9523b_p0_mode_e;

typedef enum 
{
    AW9523B_P0_0 = 0x00,
    AW9523B_P0_1 = 0x01,
    AW9523B_P0_2 = 0x02,
    AW9523B_P0_3 = 0x03,
    AW9523B_P0_4 = 0x04,
    AW9523B_P0_5 = 0x05,
    AW9523B_P0_6 = 0x06,
    AW9523B_P0_7 = 0x07,
    AW9523B_P1_0 = 0x08,
    AW9523B_P1_1 = 0x09,
    AW9523B_P1_2 = 0x0a,
    AW9523B_P1_3 = 0x0b,
    AW9523B_P1_4 = 0x0c,
    AW9523B_P1_5 = 0x0d,
    AW9523B_P1_6 = 0x0e,
    AW9523B_P1_7 = 0x0f,
} aw9523b_pin_num_e;  

void aw9523b_init(void);
void aw9523b_p0_output_mode_config(aw9523b_p0_mode_e mode);
void aw9523b_pin_mode_config(aw9523b_pin_num_e pin, aw9523b_pin_mode_e mode);
void aw9523b_gpio_mode_config(aw9523b_pin_num_e pin, aw9523b_gpio_mode_e mode);

void aw9523b_gpio_Write(aw9523b_pin_num_e pin, uint8_t value);
int8_t aw9523b_gpio_read(aw9523b_pin_num_e pin);
void aw9523b_led_Write(aw9523b_pin_num_e pin, uint8_t value);

void aw9523b_reset(void);
uint16_t get_aw9523b_gpio_status(void);


uint8_t rt_hw_init_aw9523b(void);

#endif


