

#include "board.h"

#include "stm32f4xx.h"

#include "drv_gpio.h"


/*********************************************
**	初始化IO口
**********************************************/


void rt_gpio_set_mode(GPIO_TypeDef *port, uint16_t pin,GPIOMode_TypeDef mode,GPIOPuPd_TypeDef pull, GPIOSpeed_TypeDef speed)
{
	GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin = pin;
  GPIO_InitStructure.GPIO_Mode = mode;
	GPIO_InitStructure.GPIO_PuPd = pull;
  GPIO_InitStructure.GPIO_Speed = speed;
  GPIO_Init(port, &GPIO_InitStructure);
}




/****************************
**	点亮黄色LED灯
*****************************/

void rt_led_yellow_on(void)
{
  
}


/****************************
**        熄灭黄色LED灯
*****************************/

void rt_led_yellow_off(void) 
{
}


/****************************
** 点亮蓝色LED灯
*****************************/

void rt_led_blue_on(void) 
{
  
}



/****************************
** 熄灭蓝色LED灯
*****************************/

void rt_led_blue_off(void) 
{
}


/***********************************
** 点亮绿色LED灯
************************************/

void rt_led_green_on(void) 
{
	GPIO_SetBits(PORT_GPIO_LED_GREEN, PIN_GPIO_LED_GREEN);
}
	

/***********************************
** 熄灭绿色LED灯
************************************/
	
void rt_led_green_off(void) 
{
	GPIO_ResetBits(PORT_GPIO_LED_GREEN, PIN_GPIO_LED_GREEN);
}



/***********************************
** 点亮红色LED灯
************************************/
void rt_led_red_on(void)
{
  
}


/***********************************
** 熄灭红色LED灯
************************************/
void rt_led_red_off(void)
{
  
}


/******************************
**	关闭RS232通讯电源
********************************/

void rt_power_rs232_on(void) 
{
  GPIO_SetBits(PORT_GPIO_RS232_POWER, PIN_GPIO_RS232_POWER);
}



/******************************
**	关闭RS232通讯电源
********************************/
void rt_power_rs232_off(void) 
{
	GPIO_ResetBits(PORT_GPIO_RS232_POWER, PIN_GPIO_RS232_POWER);
}


/******************************
** 打开LTE电源
********************************/

void rt_lte_power_on(void)
{
	GPIO_SetBits(PORT_GPIO_LTE_POWER, PIN_GPIO_LTE_POWER);
}


/******************************
** 关闭LTE电源
********************************/
void rt_lte_power_off(void)
{
	GPIO_ResetBits(PORT_GPIO_LTE_POWER, PIN_GPIO_LTE_POWER);
}


/******************************
** LTE PWK引脚拉低
********************************/

void rt_lte_switch_low(void)
{
	GPIO_ResetBits(PORT_GPIO_LTE_PWK, PIN_GPIO_LTE_PWK);
}

/******************************
** LTE PWK引脚拉高
********************************/
void rt_lte_switch_high(void)
{
	GPIO_SetBits(PORT_GPIO_LTE_PWK, PIN_GPIO_LTE_PWK);
}

/******************************
** LTE RESET引脚 拉低
********************************/

void rt_lte_reset_low(void)
{
}

/******************************
** LTE RESET引脚 拉高
********************************/
void rt_lte_reset_high(void)
{
}


/****************************
** 外部Flash电源
****************************/

void rt_power_flash_on(void)
{
}


/****************************
** 关闭外部Flash电源
****************************/

void rt_power_flash_off(void)
{
}




/****************************
**      打开GNSS模块电源
****************************/
void rt_gnss_power_on(void)
{
	GPIO_SetBits(PORT_GPIO_GNSS_POWER, PIN_GPIO_GNSS_POWER);
}


/********************************
**      关闭GNSS模块电源
********************************/

void rt_gnss_power_off(void)
{
	GPIO_ResetBits(PORT_GPIO_GNSS_POWER, PIN_GPIO_GNSS_POWER);
}


/********************************
**
********************************/

void rt_gnss_back_low(void)
{
}


/********************************
**
********************************/

void rt_gnss_back_high(void)
{
}





void rt_power_can_on(void)
{
	
}


void rt_power_can_off(void)
{
  
}

/**********************************
**
***********************************/

void rt_can1_stb_on(void)
{
	GPIO_SetBits(PORT_GPIO_CAN1_STB,PIN_GPIO_CAN1_STB);
}



/**********************************
**
***********************************/

void rt_can2_stb_on(void)
{
	GPIO_SetBits(PORT_GPIO_CAN2_STB,PIN_GPIO_CAN2_STB);
}


/*************************************
**      
**************************************/

void rt_can1_stb_off(void)
{
  GPIO_ResetBits(PORT_GPIO_CAN1_STB,PIN_GPIO_CAN1_STB);
}


/*************************************
**      
**************************************/

void rt_can2_stb_off(void)
{
  GPIO_ResetBits(PORT_GPIO_CAN2_STB,PIN_GPIO_CAN2_STB);
}


/*************************************
**      
**************************************/

void rt_power_bt_on(void)
{
  
}


/*************************************
**     蓝牙部分 
**************************************/

void rt_power_bt_off(void)
{
 
}


void rt_bt_mode_nor_at(void)
{
   
}


void rt_bt_mode_not_at(void)
{
  
}


/************************
**      返回蓝牙连接状态
****************************/

uint8_t rt_bt_read_connect(void)
{
  uint8_t rv;
  
  rv = 0;
  
  return rv;
}



/********************************
**
********************************/

void rt_pwm_one_on(void)
{
}

/********************************
**
********************************/

void rt_pwm_two_on(void)
{
}


/********************************
**
********************************/

void rt_pwm_one_off(void)
{
}

/********************************
**      
********************************/

void rt_pwm_two_off(void)
{
}






/*****************************
**	初始化
*******************************/

int rt_hw_gpio_init(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE,ENABLE);
		
  rt_gpio_set_mode(PORT_GPIO_LED_YELLOW,PIN_GPIO_LED_YELLOW,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);	
  rt_gpio_set_mode(PORT_GPIO_LED_GREEN,PIN_GPIO_LED_GREEN,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);	
  rt_gpio_set_mode(PORT_GPIO_LED_BLUE,PIN_GPIO_LED_BLUE,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);	
  
  rt_led_yellow_on();         //黄色LED
  rt_led_green_on();          //
  rt_led_blue_on();           //
      
          //RS232通讯电源
  rt_gpio_set_mode(PORT_GPIO_RS232_POWER,PIN_GPIO_RS232_POWER,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);	
  rt_power_rs232_on();

  rt_gpio_set_mode(PORT_GPIO_LTE_POWER, PIN_GPIO_LTE_POWER,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);	
  rt_gpio_set_mode(PORT_GPIO_LTE_PWK,PIN_GPIO_LTE_PWK,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);	
  rt_gpio_set_mode(PORT_GPIO_LTE_RESET, PIN_GPIO_LTE_RESET,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);	
	
  rt_lte_power_off();
  rt_lte_switch_low();
  rt_lte_reset_low();
	
	//flash /eeprom 电源
  rt_gpio_set_mode(PORT_GPIO_FLASH_POWER,PIN_GPIO_FLASH_POWER,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);	
  rt_power_flash_off();
          
          //GNSS电源及BACK引脚供电
  rt_gpio_set_mode(PORT_GPIO_GNSS_POWER,PIN_GPIO_GNSS_POWER,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);	
  rt_gpio_set_mode(PORT_GPIO_GNSS_VBACK,PIN_GPIO_GNSS_VBACK,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);	
  rt_gnss_power_off();
  rt_gnss_back_low();
  
  rt_gpio_set_mode(PORT_GPIO_CAN1_STB,PIN_GPIO_CAN1_STB,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);
  rt_can1_stb_on();
  
  rt_gpio_set_mode(PORT_GPIO_CAN2_STB,PIN_GPIO_CAN2_STB,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);
  rt_can2_stb_on();

  rt_gpio_set_mode(PORT_GPIO_BLUE_TOOTH_POWER,PIN_GPIO_BLUE_TOOTH_POWER,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);
  rt_power_bt_off();
  
  //蓝牙链接状态-下拉输入
  rt_gpio_set_mode(PORT_GPIO_BLUE_TOOTH_CONNECT,PIN_GPIO_BLUE_TOOTH_CONNECT,GPIO_Mode_IN,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);  //
  
  rt_gpio_set_mode(PORT_GPIO_BLUE_TOOTH_CMD,PIN_GPIO_BLUE_TOOTH_CMD,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);
  //rt_bt_mode_nor_at();
  rt_bt_mode_not_at();
  
  rt_gpio_set_mode(PORT_GPIO_PWM_ONE, PIN_GPIO_PWM_ONE,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);
  rt_gpio_set_mode(PORT_GPIO_PWM_TWO, PIN_GPIO_PWM_TWO,GPIO_Mode_OUT,GPIO_PuPd_NOPULL,GPIO_Speed_2MHz);
  
  return 0;
}


/**************************
** 关闭串口
***************************/

int rt_hw_gpio_close(void)
{
  return 0;
}


