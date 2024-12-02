



#ifndef _DRV_GPIO_H
#define _DRV_GPIO_H


#include "driver/gpio.h"
#include "driver/adc.h"

#include "drv_aw9523b.h"


#define  GPIO_LED_R				AW9523B_P0_0
#define  GPIO_LED_G				AW9523B_P0_1
#define  GPIO_LED_B				AW9523B_P0_2
#define  GPIO_LDR				GPIO_NUM_35
#define  GPIO_MOTO_OUT  		GPIO_NUM_5

#define	 GPIO_GNSS_POWER    	AW9523B_P0_3
#define	 GPIO_LTE_POWER    		AW9523B_P1_7
#define	 GPIO_LTE_PWRKEY    	GPIO_NUM_21

#define	 GPIO_CAN_RX    		GPIO_NUM_4
#define	 GPIO_CAN_TX    		GPIO_NUM_17
#define	 GPIO_CAN_STB    		AW9523B_P0_7 

#define  GPIO_I2C_SCL    		GPIO_NUM_16           
#define  GPIO_I2C_SDA    		GPIO_NUM_0
#define  GPIO_I2C_RST    		GPIO_NUM_13

#define  GPIO_SPI_MISO 			GPIO_NUM_33
#define  GPIO_SPI_MOSI 			GPIO_NUM_32
#define  GPIO_SPI_CLK 			GPIO_NUM_26
#define  GPIO_SPI_CS1 			GPIO_NUM_25

#define  GPIO_SDIO_CLK 			GPIO_NUM_14
#define  GPIO_SDIO_CMD 			GPIO_NUM_15
#define  GPIO_SDIO_D0 			GPIO_NUM_2

#define  GPIO_UART0_TXD  		GPIO_NUM_1
#define  GPIO_UART0_RXD  		GPIO_NUM_3

#define  GPIO_UART1_TXD  		GPIO_NUM_22
#define  GPIO_UART1_RXD  		GPIO_NUM_19

#define  GPIO_UART2_TXD  		GPIO_NUM_27
#define  GPIO_UART2_RXD  		GPIO_NUM_12

#define	 ADC_VCC				ADC1_CHANNEL_0
#define	 ADC_ACC     			ADC1_CHANNEL_1
#define	 ADC_BAT				ADC1_CHANNEL_6
#define	 ADC_IN1     			ADC1_CHANNEL_2
#define	 ADC_IN2     			ADC1_CHANNEL_3
#define  ADC_SHELL				ADC1_CHANNEL_7 

#define  GPIO_RS485_DEN  		GPIO_NUM_23
#define  GPIO_VCP_EN   			AW9523B_P1_4
#define  GPIO_MCU_RX_RS232_EN   AW9523B_P1_6
#define  GPIO_MCU_RX_RS485_EN   AW9523B_P1_5

#define  GPIO_CHRG_EN			AW9523B_P0_4
#define  GPIO_CHRG_DONE			AW9523B_P0_5
#define  GPIO_CHRG_STANDBY		AW9523B_P0_6
#define	 GPIO_ACL16_POWER    	AW9523B_P0_3

#define	 rt_gnss_power_on()		aw9523b_gpio_Write(GPIO_GNSS_POWER, 1)
#define	 rt_gnss_power_off()	aw9523b_gpio_Write(GPIO_GNSS_POWER, 0)
#define	 rt_lte_power_on()		aw9523b_gpio_Write(GPIO_LTE_POWER, 1)
#define	 rt_lte_power_off()		aw9523b_gpio_Write(GPIO_LTE_POWER, 0)
#define	 rt_lte_pwrkey_low()	gpio_set_level(GPIO_LTE_PWRKEY, 1)
#define	 rt_lte_pwrkey_high()		gpio_set_level(GPIO_LTE_PWRKEY, 0)

#define	 rt_i2c_rst_high()			gpio_set_level(GPIO_I2C_RST, 1)
#define	 rt_i2c_rst_low()			gpio_set_level(GPIO_I2C_RST, 0)

#define	 rt_can_normal()			aw9523b_gpio_Write(GPIO_CAN_STB, 0)
#define	 rt_can_standby()			aw9523b_gpio_Write(GPIO_CAN_STB, 1)

#define	 rt_get_ldr_status()		gpio_get_level(GPIO_LDR)

#define	 rt_set_moto_out_high()		gpio_set_level(GPIO_MOTO_OUT, 1)
#define	 rt_set_moto_out_low()		gpio_set_level(GPIO_MOTO_OUT, 0)

#define	 rt_acl16_power_on()		aw9523b_gpio_Write(GPIO_ACL16_POWER, 1)
#define	 rt_acl16_power_off()		aw9523b_gpio_Write(GPIO_ACL16_POWER, 0)

#define	 rt_rs232_485_power_on()	aw9523b_gpio_Write(GPIO_VCP_EN, 1)
#define	 rt_rs232_485_power_off()	aw9523b_gpio_Write(GPIO_VCP_EN, 0)

#define	 rt_rx_rs232_switch_on()	aw9523b_gpio_Write(GPIO_MCU_RX_RS232_EN, 0)
#define	 rt_rx_rs232_switch_off()	aw9523b_gpio_Write(GPIO_MCU_RX_RS232_EN, 1)

#define	 rt_rx_rs485_switch_on()	aw9523b_gpio_Write(GPIO_MCU_RX_RS485_EN, 0)
#define	 rt_rx_rs485_switch_off()	aw9523b_gpio_Write(GPIO_MCU_RX_RS485_EN, 1)

#define	 rt_rx_rs485_enable()		gpio_set_level(GPIO_RS485_DEN, 0)
#define	 rt_tx_rs485_enable()		gpio_set_level(GPIO_RS485_DEN, 1)

#define	 rt_set_chrg_en_on()			aw9523b_gpio_Write(GPIO_CHRG_EN, 1)
#define	 rt_set_chrg_en_off()			aw9523b_gpio_Write(GPIO_CHRG_EN, 0)
#define	 rt_get_chrg_done_status()		aw9523b_gpio_read(GPIO_CHRG_DONE)
#define	 rt_get_chrg_standby_status()	aw9523b_gpio_read(GPIO_CHRG_STANDBY)




typedef enum
{
	LED_R,
	LED_G,
	LED_B,
	LED_ALL
} led_list_e;



typedef enum 
{
    LED_ON, 
    LED_OFF,
	LED_FLIP
} led_status_e;




uint8_t rt_hw_init_gpio(void);
void bsp_i2c_rst_gpio_init(void);
void bsp_lte_pwrkey_control(void);
void bsp_lte_power_reset(void);
void bsp_gnss_power_reset(void);
void rt_led_control(led_list_e led, led_status_e status);
void rt_i2c_gpio_init(void);



void com_rx_control(uint32_t value);

void rt_led_off(led_list_e led);




uint8_t  rt_hw_init_i2c_rst(void);

#endif


/********************************File End*********************************/