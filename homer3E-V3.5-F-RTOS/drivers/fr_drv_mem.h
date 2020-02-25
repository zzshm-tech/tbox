

#ifndef _FR_DRV_MEM_H
#define _FR_DRV_MEM_H


/************************************************************************************
	The W25X32A array is organized into 16,384 programmable pages of 256-bytes each. 
	Up to 256 bytes can be programmed at a time using the Page Program instruction. 
	Pages can be erased in groups of 16 (sector erase), groups of 256 (block erase) 
	or the entire chip (chip erase). 
	The W25X32A has 1,024 erasable sectors and 64 erasable blocks. 
	The small 4KB sectors allow for greater flexibility in applications that require 
	data and parameter storage.
************************************************************************************/

//0XEF13,W25Q80  
//0XEF14,W25Q16    
//0XEF15,W25Q32  
//0XEF16,W25Q64   

// 4Kbytes为一个Sector
// 16个扇区为1个Block
// W25X32
// 容量为4M字节,共有64个Block,1024个Sectors




#ifndef OK
#define OK                     1 
#endif

#ifndef ERROR
#define ERROR                  0    
#endif




/* Private typedef -----------------------------------------------------------*/


#define SPI_FLASH_PageSize      			4096
#define SPI_FLASH_PerWritePageSize    256

/* Private define ------------------------------------------------------------*/
#define W25X_WriteEnable		      0x06 
#define W25X_WriteDisable		      0x04 
#define W25X_ReadStatusReg		    0x05 
#define W25X_WriteStatusReg		    0x01 
#define W25X_ReadData			        0x03 
#define W25X_FastReadData		      0x0B 
#define W25X_FastReadDual		      0x3B 
#define W25X_PageProgram		      0x02 
#define W25X_BlockErase			      0xD8 
#define W25X_SectorErase		      0x20 
#define W25X_ChipErase			      0xC7 
#define W25X_PowerDown			      0xB9 
#define W25X_ReleasePowerDown	    0xAB 
#define W25X_DeviceID			        0xAB 
#define W25X_ManufactDeviceID   	0x90 
#define W25X_JedecDeviceID		    0x9F 

#define WIP_Flag                  0x01  /* Write In Progress (WIP) flag */

#define Dummy_Byte                0xFF

#ifndef UNINITIALIZED
#define UNINITIALIZED 0
#endif

#ifndef INITIALIZED
#define INITIALIZED 	1
#endif




// Manufacturer ID
#define WINBOND_SERIAL_FLASH   0xEF 

// device id
#define DEVICE_ID_W25X10					0x10
#define DEVICE_ID_W25X20					0x11
#define DEVICE_ID_W25X40					0x12
#define DEVICE_ID_W25X80					0x13
#define DEVICE_ID_W25X16					0x14
#define DEVICE_ID_W25X32					0x15

// Memory Type: 30
// Memory Capacity:
#define MEMORY_INFOR_W25X10			0x3011
#define MEMORY_INFOR_W25X20			0x3012
#define MEMORY_INFOR_W25X40			0x3013
#define MEMORY_INFOR_W25X80			0x3014

#define JEDEC_ID_W25X10				0xEF3011
#define JEDEC_ID_W25X20				0xEF3012
#define JEDEC_ID_W25X40				0xEF3013
#define JEDEC_ID_W25X80				0xEF3014

#define JEDEC_ID_W25X32	      0xEF3016

#define JEDEC_ID_W25Q32	      0xEF4016

#define ADDRESS_PAGE_2047     0x07FF00
#define ADDRESS_PAGE0			    0x000000

#ifndef FLASH_PAGE_SIZE
#define FLASH_PAGE_SIZE 			256
#endif


#define ADDR_SECTOR_1023			0x3FF000

#define  FLASH_WriteAddress     0x00000
#define  FLASH_ReadAddress      FLASH_WriteAddress
#define  FLASH_SectorToErase    FLASH_WriteAddress


#define SPI_FLASH_SPI                           SPI1
#define SPI_FLASH_SPI_CLK                       RCC_APB2Periph_SPI1
#define SPI_FLASH_SPI_SCK_PIN                   GPIO_Pin_5                  /* PA.05 */
#define SPI_FLASH_SPI_SCK_GPIO_PORT             GPIOA                       /* GPIOA */
#define SPI_FLASH_SPI_SCK_GPIO_CLK              RCC_APB2Periph_GPIOA
#define SPI_FLASH_SPI_MISO_PIN                  GPIO_Pin_6                  /* PA.06 */
#define SPI_FLASH_SPI_MISO_GPIO_PORT            GPIOA                       /* GPIOA */
#define SPI_FLASH_SPI_MISO_GPIO_CLK             RCC_APB2Periph_GPIOA
#define SPI_FLASH_SPI_MOSI_PIN                  GPIO_Pin_7                  /* PA.07 */
#define SPI_FLASH_SPI_MOSI_GPIO_PORT            GPIOA                       /* GPIOA */
#define SPI_FLASH_SPI_MOSI_GPIO_CLK             RCC_APB2Periph_GPIOA
#define SPI_FLASH_CS_PIN                        GPIO_Pin_4                  /* PC.04 */
#define SPI_FLASH_CS_GPIO_PORT                  GPIOA                       /* GPIOC */
#define SPI_FLASH_CS_GPIO_CLK                   RCC_APB2Periph_GPIOA

#define ADRESS_FALSH_PAGE0  0x00000000



#define ADDR_MAX		0x400000	// 定义芯片内部最大地址 

#define	SECTOR_NUMS   1024		// 扇区数目
#define SECTOR_SIZE		4096    // 扇区大小 4K

typedef enum ERTYPE
{
	Sec1,
	Sec8,
	Sec16,
	Chip
}ErType;  


typedef enum IDTYPE
{
	Manu_ID,
	Dev_ID,
	Jedec_ID
}idtype;



typedef enum 
{ 
	PASSED = 0,
	FAILED, 
}TestStatus;



/**********************************
**
**	
**********************************/


#define BOOTLOADER_SIZE     (1024 * 32)        													// 	32K  		bytes  
#define DATA_FILED_SIZE     (1024 * 10)         												// 	2K   		bytes   
#define SYSCFG_FILED_SIZE		(1024 * 70)				 													// 	70K			bytes
#define APP_DATA_SIZE       (1024 * 200)       													// 	200K 		bytes

#define ADDR_BOOTLOADER			0x08000000            							 				//	BoolLoader地址空间
#define ADDR_DATA_FILED			(ADDR_BOOTLOADER + BOOTLOADER_SIZE)  				//	系统BootLoader信息
#define ADDR_SYSCFG_FILED		(ADDR_DATA_FILED + DATA_FILED_SIZE)   			//	系统配置信息
#define ADDR_APP_RUN				(ADDR_SYSCFG_FILED + SYSCFG_FILED_SIZE)  		//	应用程序区域
#define ADDR_APP_BKP				(ADDR_APP_RUN    + APP_DATA_SIZE)      			//	应用程序备份区域



#define PAGE_SIZE           (0x800)    																	// 2 Kbytes 		每页大小
#define FLASH_SIZE          (0x80000)  																	// 512 KBytes   最大



unsigned char fr_init_fram(void);
unsigned char fr_fram_write(unsigned short int Addr,unsigned char *Data,  unsigned short int Len);
unsigned char fr_fram_read(unsigned short addr, unsigned char *data, unsigned short int len);
int storage_medium_init(void);
unsigned int w25qxx_flash_write(unsigned char *buff, unsigned int sectors, unsigned int num);
unsigned int  w25qxx_flash_read(unsigned char *buff, unsigned int sectors, unsigned int num);

#endif






