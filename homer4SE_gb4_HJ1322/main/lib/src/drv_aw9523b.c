

#include <stdio.h>
#include <string.h>
#include <stdint.h>


#include "drv_aw9523b.h"
#include "drv_i2c.h"
#include "drv_gpio.h"

uint8_t pinModeP0 = 0xff; /* P0管脚默认为GPIO模式 */
uint8_t pinModeP1 = 0xff; /* P1管脚默认为GPIO模式 */

uint8_t gpioModeP0 = 0x00; /* P0 GPIO默认为输出模式 */
uint8_t gpioModeP1 = 0x00; /* P1 GPIO默认为输出模式 */

uint8_t gpioLevelP0 = 0x00; /* P0 GPIO电平默认为0 */
uint8_t gpioLevelP1 = 0x00; /* P1 GPIO电平默认为0 */

int aw9523b_modify_bit(int currentByte, int position, int bit);






/************************************
**  
*************************************/ 

int aw9523b_modify_bit(int currentByte, int position, int bit) // bit field, change position, change value
{
    int mask = 1 << position;
    return (currentByte & ~mask) | ((bit << position) & mask);
}

void aw9523b_pin_mode_config(aw9523b_pin_num_e pin, aw9523b_pin_mode_e mode)
{
    if (pin < 8)
    {
        pinModeP0 = aw9523b_modify_bit(pinModeP0, pin, mode);
        rt_i2c_write_byte(AW9523B_I2C_ADDRESS, AW9523B_P0_LED_MODE, pinModeP0);
    }
    else
    {
        pinModeP1 = aw9523b_modify_bit(pinModeP1, (pin - 8), mode);
        rt_i2c_write_byte(AW9523B_I2C_ADDRESS, AW9523B_P1_LED_MODE, pinModeP1);
    }
}

void aw9523b_gpio_mode_config(aw9523b_pin_num_e pin, aw9523b_gpio_mode_e mode)
{
    if (pin < 8)
    {
        gpioModeP0 = aw9523b_modify_bit(gpioModeP0, pin, mode);
        rt_i2c_write_byte(AW9523B_I2C_ADDRESS, AW9523B_P0_CONF_STATE, gpioModeP0);
    }
    else
    {
        gpioModeP1 = aw9523b_modify_bit(gpioModeP1, (pin - 8), mode);
        rt_i2c_write_byte(AW9523B_I2C_ADDRESS, AW9523B_P1_CONF_STATE, gpioModeP1);
    }
}

void aw9523b_p0_output_mode_config(aw9523b_p0_mode_e mode)
{
   rt_i2c_write_byte(AW9523B_I2C_ADDRESS, AW9523B_REG_GLOB_CTR, mode);
}

void aw9523b_gpio_Write(aw9523b_pin_num_e pin, uint8_t value)
{
    if (pin < 8)
    {
        gpioLevelP0 = aw9523b_modify_bit(gpioLevelP0, pin, value);
        rt_i2c_write_byte(AW9523B_I2C_ADDRESS, AW9523B_P0_OUT_STATE, gpioLevelP0);
    }
    else
    {
        gpioLevelP1 = aw9523b_modify_bit(gpioLevelP1, pin - 8, value);
        rt_i2c_write_byte(AW9523B_I2C_ADDRESS, AW9523B_P1_OUT_STATE, gpioLevelP1);
    }
}

int8_t aw9523b_gpio_read(aw9523b_pin_num_e pin)
{
    uint8_t reg_data = 0;

    if (pin < 8)
    {
        reg_data = rt_i2c_read_byte(AW9523B_I2C_ADDRESS, AW9523B_P0_IN_STATE);
        return (reg_data >> pin) & 0x01;
    }
    else
    {
        reg_data = rt_i2c_read_byte(AW9523B_I2C_ADDRESS, AW9523B_P1_IN_STATE);
        return (reg_data >> (pin - 8)) & 0x01;
    }

    return -1;
}

void aw9523b_led_Write(aw9523b_pin_num_e pin, uint8_t value)
{
    uint8_t reg = 0;
    uint8_t data = 0;

    switch (pin)
    {
    case AW9523B_P0_0:
        reg = AW9523B_REG_LED_P0_0;
        break;
    case AW9523B_P0_1:
        reg = AW9523B_REG_LED_P0_1;
        break;
    case AW9523B_P0_2:
        reg = AW9523B_REG_LED_P0_2;
        break;
    case AW9523B_P0_3:
        reg = AW9523B_REG_LED_P0_3;
        break;
    case AW9523B_P0_4:
        reg = AW9523B_REG_LED_P0_4;
        break;
    case AW9523B_P0_5:
        reg = AW9523B_REG_LED_P0_5;
        break;
    case AW9523B_P0_6:
        reg = AW9523B_REG_LED_P0_6;
        break;
    case AW9523B_P0_7:
        reg = AW9523B_REG_LED_P0_7;
        break;
    case AW9523B_P1_0:
        reg = AW9523B_REG_LED_P1_0;
        break;
    case AW9523B_P1_1:
        reg = AW9523B_REG_LED_P1_1;
        break;
    case AW9523B_P1_2:
        reg = AW9523B_REG_LED_P1_2;
        break;
    case AW9523B_P1_3:
        reg = AW9523B_REG_LED_P1_3;
        break;
    case AW9523B_P1_4:
        reg = AW9523B_REG_LED_P1_4;
        break;
    case AW9523B_P1_5:
        reg = AW9523B_REG_LED_P1_5;
        break;
    case AW9523B_P1_6:
        reg = AW9523B_REG_LED_P1_6;
        break;
    case AW9523B_P1_7:
        reg = AW9523B_REG_LED_P1_7;
        break;
	default:
		printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
		return;
    }

    if (value == 0) 
    {
        data = 0x5;
    }
    else 
    {
        data = 0x00;
    }

    rt_i2c_write_byte(AW9523B_I2C_ADDRESS, reg, data);
}

void aw9523b_reset(void)
{
    rt_hw_init_i2c_rst();
}

uint16_t get_aw9523b_gpio_status(void)
{
    uint16_t status = 0;

    status = (gpioLevelP0 << 8) | gpioLevelP1;
    // printf("aw9523b_gpio_status = %04x\r\n", status);

    return status;
}





/************************************
**  初始化AW9523B
*************************************/ 

uint8_t rt_hw_init_aw9523b(void)
{
    aw9523b_reset();

    vTaskDelay(10 / portTICK_RATE_MS);

    /* pin默认为gpio模式，把相应管教配置为led模式 */
    aw9523b_pin_mode_config(GPIO_LED_B, AW9523B_LED_MODE);
    aw9523b_pin_mode_config(GPIO_LED_G, AW9523B_LED_MODE);
    aw9523b_pin_mode_config(GPIO_LED_R, AW9523B_LED_MODE);

    /* gpio默认为输出模式，把相应管教配置为输入模式 */
    aw9523b_gpio_mode_config(GPIO_CHRG_DONE, W9523B_GPIO_MODE_INPUT);
    aw9523b_gpio_mode_config(GPIO_CHRG_STANDBY, W9523B_GPIO_MODE_INPUT);

    /* p0 gpio默认为开漏输出模式，配置为推挽输出模式 */
    aw9523b_p0_output_mode_config(AW9523B_MODE_PUSH_PULL);

    rt_set_chrg_en_on();

    return 0;
}



