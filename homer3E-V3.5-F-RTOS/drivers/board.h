/*
 * 
 */

#ifndef _BOARD_H_
#define _BOARD_H_





#define NOR_FLASH          \
    {                      \
        SD_CARD, NOR_FLASH \
    }


#define DC_MEM_PROT GPIOB
#define DC_MEM_PIN GPIO_Pin_13
#if defined(NOR_FLASH)
    #include "spi_flash_gd.h"
    #define SPI_DEV SPI1
    #define SPI_PRO GPIOA

    #define SPI_CS_PIN GPIO_Pin_4
    #define SPI_MOSI_PIN GPIO_Pin_7
    #define SPI_MISO_PIN GPIO_Pin_6
    #define SPI_SCK_PIN GPIO_Pin_5
    #define PORT_GPIO_FLASH_HOLD GPIOB
    #define PIN_GPIO_FLASH_HOLD GPIO_Pin_0
    #define PORT_GPIO_FLASH_WP GPIOC
    #define PIN_GPIO_FLASH_WP GPIO_Pin_4
	
			
		unsigned char spi_flash_sendbyte(unsigned char  byte);
		
		
#elif defined(SD_CARD)
    #include "dev_sd.h"
    #define SPI_DEV SPI2
    #define SPI_PRO GPIOB

    #define SPI_CS_PIN GPIO_Pin_12
    #define SPI_MOSI_PIN GPIO_Pin_15
    #define SPI_MISO_PIN GPIO_Pin_14
    #define SPI_SCK_PIN GPIO_Pin_13
#else
    #error "SPI chioce error."
#endif



void fr_init_board(void);
void fr_reset_sys(void);
void work_to_sleep(void);
void sleep_to_work(void);
void delay_ms(unsigned int delayms);
void fr_reset_iwdg(void);
unsigned char fr_init_wdt(unsigned short int sec);
void fr_enter_sleep(void);
void fr_exit_sleep(void);
void fr_init_hsi_8mhz(void);
#endif

// <<< Use Configuration Wizard in Context Menu >>>
