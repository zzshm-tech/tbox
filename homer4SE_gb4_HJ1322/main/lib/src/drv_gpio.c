

#include <drv_gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"


/***************************
**  初始化 I2C复位引脚
****************************/

uint8_t  rt_hw_init_i2c_rst(void)
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = 1ULL << GPIO_I2C_RST;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//enable pull-up mode
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	rt_i2c_rst_low();
	vTaskDelay(100);
	rt_i2c_rst_high();

    return 0;
}



/*********************************
**
**********************************/

void rt_led_on(led_list_e led)
{
	if (led == LED_R)
	{
		aw9523b_led_Write(GPIO_LED_R, 0);
	}
	else if (led == LED_G)
	{
		aw9523b_led_Write(GPIO_LED_G, 0);
	}
	else if (led == LED_B)
	{
		aw9523b_led_Write(GPIO_LED_B, 0);
	}
	else if (led == LED_ALL)
	{
		aw9523b_led_Write(GPIO_LED_R, 0);
		aw9523b_led_Write(GPIO_LED_G, 0);
		aw9523b_led_Write(GPIO_LED_B, 0);
	}
}



/*********************************
**
**********************************/


void rt_led_off(led_list_e led)
{
	if (led == LED_R)
	{
		aw9523b_led_Write(GPIO_LED_R, 1);
	}
	else if (led == LED_G)
	{
		aw9523b_led_Write(GPIO_LED_G, 1);
	}
	else if (led == LED_B)
	{
		aw9523b_led_Write(GPIO_LED_B, 1);
	}
	else if (led == LED_ALL)
	{
		aw9523b_led_Write(GPIO_LED_R, 1);
		aw9523b_led_Write(GPIO_LED_G, 1);
		aw9523b_led_Write(GPIO_LED_B, 1);
	}
}



/*********************************
**
**********************************/

void rt_led_flip(led_list_e led)
{
	static uint8_t cnt_r = 0, cnt_g = 0, cnt_b = 0, cnt_all = 0;

	if (led == LED_R)
	{
		if (cnt_r == 0)
		{
			cnt_r = 1;
			rt_led_on(LED_R);
		}
		else
		{
			cnt_r = 0;
			rt_led_off(LED_R);
		}
	}
	else if (led == LED_G)
	{
		if (cnt_g == 0)
		{
			cnt_g = 1;
			rt_led_on(LED_G);
		}
		else
		{
			cnt_g = 0;
			rt_led_off(LED_G);
		}
	}
	else if (led == LED_B)
	{
		if (cnt_b == 0)
		{
			cnt_b = 1;
			rt_led_on(LED_B);
		}
		else
		{
			cnt_b = 0;
			rt_led_off(LED_B);
		}
	}
	else if (led == LED_ALL)
	{
		if (cnt_all == 0)
		{
			cnt_all = 1;
			rt_led_on(LED_ALL);
		}
		else
		{
			cnt_all = 0;
			rt_led_off(LED_ALL);
		}
	}
}


/********************************
**  
********************************/

void rt_led_control(led_list_e led, led_status_e status)
{
	switch (status)
	{
	case LED_ON:
		rt_led_on(led);
		break;
	case LED_OFF:
		rt_led_off(led);
		break;
	case LED_FLIP:
		rt_led_flip(led);
		break;

	default:
		printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
		break;
	}
}



void rt_ldr_gpio_init(void)
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_INPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = 1ULL << GPIO_LDR;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//enable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);
}


void rt_moto_out_gpio_init(void)
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = 1ULL << GPIO_MOTO_OUT;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//enable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);
}

void rt_rs485_den_gpio_init(void)
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = (1ULL << GPIO_RS485_DEN);
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//enable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	rt_tx_rs485_enable();
}



void rt_lte_gpio_init(void)
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = 1ULL << GPIO_LTE_PWRKEY;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//enable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	// gpio_set_level(GPIO_LTE_PWRKEY, 0);
}





/*********************************************
#define  GPIO_I2C_SCL    		GPIO_NUM_16           
#define  GPIO_I2C_SDA    		GPIO_NUM_0
**********************************************/

void rt_i2c_gpio_init(void)
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_DEF_DISABLE;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = 1ULL << GPIO_NUM_0;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//enable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	// gpio_set_level(GPIO_LTE_PWRKEY, 0);
}


/***************************
**	
****************************/

uint8_t rt_hw_init_gpio(void)
{
	rt_lte_gpio_init();
	rt_ldr_gpio_init();
	rt_moto_out_gpio_init();
	rt_rs485_den_gpio_init();

	rt_rx_rs232_switch_off();
	rt_rx_rs485_switch_off();
	rt_rs232_485_power_on();

	return 0;
}

