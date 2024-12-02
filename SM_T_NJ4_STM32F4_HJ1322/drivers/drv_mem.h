




#ifndef _DRV_MEM_H
#define _DRV_MEM_H





#define BOOTLOADER_SIZE 				(1024 *  48)					/***************/
#define	DATA_FILED_SIZE					(1024 *  16) 					/***************/	
#define APP_DATA_SIZE       		(1024 * 192)        	        /***************/

#define ADDR_BOOTLOADER     		0x08000000
#define ADDR_DATA_FILED     		(ADDR_BOOTLOADER + BOOTLOADER_SIZE)     /***************/
#define ADDR_APP_RUN        		(ADDR_DATA_FILED + DATA_FILED_SIZE)     /***************/
#define ADDR_APP_BKP        		(ADDR_APP_RUN    + APP_DATA_SIZE)		/***************/



#ifndef OK
#define OK                     1 
#endif

#ifndef ERROR
#define ERROR                  0    
#endif

#define SPI_FLASH_CS_LOW()       GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define SPI_FLASH_CS_HIGH()      GPIO_SetBits(GPIOA, GPIO_Pin_4)


/* Private typedef -----------------------------------------------------------*/

//#define SPI_FLASH_PageSize     			 4096

#define SPI_FLASH_PageSize      				256
#define SPI_FLASH_PerWritePageSize      256

#define EX_FLASH_PAGESIZE       				256				/**页大小**/
#define EX_FLASH_SECTOR_SIZE						4096			/**扇区大小**/
#define EX_FLASH_SECTOR_NUM							2048			/**扇区**/
#define EX_FLASH_PAGE_NUM								32768     /**总页**/

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
#define FLASH_PAGE_SIZE 256
#endif


#define ADDR_SECTOR_1023		0x3FF000

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


/*******8M Ex Flash*******/
#define ADDR_MAX		0x3FFFFF	// 定义芯片内部最大地址 
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






unsigned char init_backup_sram(void);

unsigned short int read_backupsram_data(unsigned short int addr,unsigned char *p_data, unsigned short int data_len);
unsigned short int write_backupsram_data(unsigned short int addr,unsigned char *p_data, unsigned short int data_len);

void i2c_init(void);
signed char i2c_read(unsigned short int slave_addr,unsigned char *data,unsigned short int mem_addr,unsigned short data_len);
signed char i2c_write(unsigned short int slave_addr,unsigned char *data,unsigned short int mem_addr,unsigned short int data_len);
void spi1_init(void);


unsigned int ReadExFlashJEDEC(void);
void WriteExFlashPage(unsigned char *pBuffer, unsigned int WriteAddr, unsigned short int NumByteToWrite);
unsigned char EraseExFlashSector(unsigned int SectorIndex);
unsigned char ReadExFlashBuf(unsigned char *pBuffer, unsigned int ReadAddr, unsigned short int NumByteToRead);


void fram_buf_write(unsigned char *p_buffer, unsigned short int write_addr, unsigned short int num_byte);
unsigned char fram_buf_read(unsigned char *p_buffer, unsigned short int read_addr, unsigned short int num_byte);




#endif




