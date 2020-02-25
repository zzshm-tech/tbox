

#include "stm32f10x.h"
#include "fr_drv_gpio.h"



/**********************
**	设置IO口模式
************************/

void fr_gpio_set_mode(GPIO_TypeDef *port, unsigned short int pin,
                      GPIOMode_TypeDef mode, GPIOSpeed_TypeDef speed)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = pin;
    GPIO_InitStructure.GPIO_Mode = mode;
    GPIO_InitStructure.GPIO_Speed = speed;
    GPIO_Init(port, &GPIO_InitStructure);
}



/**********************
**	GSM
************************/

int fr_gpio_init(void)
{
    /*Start GPIO.(A-F) Prot*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC |
                           RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO,
                           ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    //led
    fr_gpio_set_mode(PORT_GPIO_LED_RED, PIN_GPIO_LED_RED, GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    fr_gpio_set_mode(PORT_GPIO_LED_BLUE, PIN_GPIO_LED_BLUE, GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    fr_gpio_set_mode(PORT_GPIO_LED_GREEN, PIN_GPIO_LED_GREEN, GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
	
    fr_led_red_off();
    fr_led_blue_off();
    fr_led_green_off();

    //can sleep
    fr_gpio_set_mode(PORT_GPIO_CAN_POWER, PIN_GPIO_CAN_POWER, GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    fr_can_power_off();

    //gps 电源
    fr_gpio_set_mode(PORT_GPIO_GPS_POWER, PIN_GPIO_GPS_POWER, GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    fr_power_gps_off();

    //gps vback
    fr_gpio_set_mode(PORT_GPIO_GPS_VBACK, PIN_GPIO_GPS_VBACK, GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    fr_power_gps_vback_off();

    //acc
    fr_gpio_set_mode(PORT_GPIO_ACC, PIN_GPIO_ACC, GPIO_Mode_IPD, GPIO_Speed_2MHz);

    //gprs 电源控制
    fr_power_gprs_off();

    //gprs 开关机控制
    fr_gpio_set_mode(PORT_GPIO_GPRS_START_CTRL, PIN_GPIO_GPRS_START_CTRL, GPIO_Mode_Out_OD, GPIO_Speed_2MHz);
    fr_gprs_switch_low();

    //gprs 唤醒
    // rt_gprs_weak_up_high();

    //gprs 电源状态
    fr_gpio_set_mode(PORT_GPIO_GPRS_STATE, PIN_GPIO_GPRS_STATE, GPIO_Mode_IPD, GPIO_Speed_2MHz);

    //spi power / hold / wp
    fr_gpio_set_mode(PORT_GPIO_FLASH_POWER, PIN_GPIO_FLASH_POWER, GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    fr_power_flash_on();

    fr_gpio_set_mode(PORT_GPIO_SPI1_HOLD, PIN_GPIO_SPI1_HOLD, GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    fr_flash_hold_on();

    fr_gpio_set_mode(PORT_GPIO_SPI1_WP, PIN_GPIO_SPI1_WP, GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    fr_flash_wp_on();

    //将空闲的IO 设置成模拟输入状态
    fr_gpio_set_mode(GPIOA, GPIO_Pin_1 | GPIO_Pin_8, GPIO_Mode_AIN, GPIO_Speed_2MHz);
    fr_gpio_set_mode(GPIOB, GPIO_Pin_1 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_15, GPIO_Mode_AIN, GPIO_Speed_2MHz);
    fr_gpio_set_mode(GPIOC, GPIO_Pin_0 | GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_9 | GPIO_Pin_12 | GPIO_Pin_13, GPIO_Mode_AIN, GPIO_Speed_2MHz);
    
		return 0;
}




/***********************
**	读取IO口状态
*************************/

unsigned char fr_read_gpio_state(GPIO_TypeDef* GPIOx, unsigned short int GPIO_Pin)
{
	return GPIO_ReadInputDataBit(GPIOx,GPIO_Pin);  //Acc
}

/******************************
**	
******************************/

void fr_close_gpio(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 |
                                  GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_7 | GPIO_Pin_8 |
                                  GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 |  GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7 | 
                              GPIO_Pin_9 |GPIO_Pin_10 | GPIO_Pin_11 |GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;           //GNSS备份引脚供电
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
		GPIO_SetBits(GPIOC,GPIO_Pin_8);  
		
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOD, &GPIO_InitStructure);	
}



/************************************
**	使用PA0：外部事件输入中断
**	
*************************************/

void EXTI0_IRQHandler(void)
{
	EXTI_ClearFlag(EXTI_Line0);
	EXTI_ClearITPendingBit(EXTI_Line0);
}



