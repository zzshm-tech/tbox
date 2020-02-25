
#include "FreeRTOS.h"

#include "stm32f10x.h"

#include "fr_drv_gpio.h"
#include "fr_drv_i2c.h"

#include "fr_drv_spi.h"
#include "fr_drv_mem.h"

#define		FRAM_PAGE_SIZE									32          //注意这个问题
#define   FRAM_DEVIDE_ADDR								0x1A0  			//EEPROM存储器地址



SemaphoreHandle_t xSemaphoreE2prom = NULL;  


/*********
**	初始化
***************/

unsigned char fr_init_flash(void)
{
	return 0;
}



/***************************************
**	函数名称：
**	功能描述：
*****************************************/

void SPI_FLASH_WriteEnable(void)
{
  SPI_FLASH_CS_LOW();
  spi_flash_sendbyte(W25X_WriteEnable);
  SPI_FLASH_CS_HIGH();
}



/****************************************
**	函数名称：
**	功能描述：
*****************************************/

void SPI_FLASH_WaitForWriteEnd(void)
{
  unsigned char	FLASH_Status = 0;

  SPI_FLASH_CS_LOW();

  /* Send "Read Status Register" instruction */
  spi_flash_sendbyte(W25X_ReadStatusReg);
	spi_flash_sendbyte(0xC1);                       //
  /* Loop as long as the memory is busy with a write cycle */
  do
  {
    /* Send a dummy byte to generate the clock needed by the FLASH
    and put the value of the status register in FLASH_Status variable */
    FLASH_Status = spi_flash_sendbyte(Dummy_Byte);

  }
  while ((FLASH_Status & WIP_Flag) == SET); /* Write in progress */

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();
}




/***************************************
**	函数名称:
**	功能描述:
****************************************/

void FlashBlockErase(unsigned short BlockNum)
{
   SPI_FLASH_WriteEnable();
   SPI_FLASH_WaitForWriteEnd();
   SPI_FLASH_CS_LOW();
   spi_flash_sendbyte(W25X_SectorErase);
	 spi_flash_sendbyte(Dummy_Byte);
	 spi_flash_sendbyte(( BlockNum & 0xFF00) >> 8);
	 spi_flash_sendbyte( BlockNum & 0xFF);
   SPI_FLASH_CS_HIGH();
   SPI_FLASH_WaitForWriteEnd();
}



/************************************
**	函数名称int32 FlashReadJEDECID(void)
**	功能描述：读取JEDECID
*************************************/

unsigned int FlashReadJEDECID(void)
{
  unsigned int Temp = 0;
	unsigned int ManufacturerID = 0;
	unsigned int MemoryType = 0;
	unsigned int MemoryCapacity = 0;

  SPI_FLASH_CS_LOW();

  spi_flash_sendbyte(W25X_JedecDeviceID);

  ManufacturerID = spi_flash_sendbyte(Dummy_Byte);
  MemoryType = spi_flash_sendbyte(Dummy_Byte);
  MemoryCapacity = spi_flash_sendbyte(Dummy_Byte);

  SPI_FLASH_CS_HIGH();

  Temp = (ManufacturerID << 16) | (MemoryType << 8) | MemoryCapacity;

  return Temp;
}





/**********************************************
**
**	
***********************************************/

unsigned int FlashReadDeviceID(void)
{
  unsigned int Temp = 0;

  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();

  /* Send "RDID " instruction */
  spi_flash_sendbyte(W25X_DeviceID);
  spi_flash_sendbyte(Dummy_Byte);
  spi_flash_sendbyte(Dummy_Byte);
  spi_flash_sendbyte(Dummy_Byte);
  
  /* Read a byte from the FLASH */
  Temp = spi_flash_sendbyte(Dummy_Byte);

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();

  return Temp;
}



/************************
**
************************/

unsigned char fr_init_fram(void)
{
	fr_power_flash_on();
	fr_init_i2c();
	
	if(xSemaphoreE2prom == NULL)
		xSemaphoreE2prom = xSemaphoreCreateMutex();
	return 0;
}




/********************************************************************************************************************
**	函数名称：void fram_page_write(unsigned char *pBuffer,unsigned short int WriteAddr, unsigned char NumByteToWrite)
**	输入参数：
**	*p_buffer:指向写数据缓冲区。
**	write_addr:开始地址。
**	num_byte:

输出参数：

*********************************************************************************************************************/

unsigned char fram_page_write(unsigned char *p_buffer,unsigned short int write_addr, unsigned char num_byte)
{ 
	unsigned char res;
	
	xSemaphoreTake(xSemaphoreE2prom,portMAX_DELAY );     //获取信号量

	res = fr_i2c_write(FRAM_DEVIDE_ADDR,p_buffer, write_addr,num_byte);   //写EERPOM    
	
	if(res == 1)
	{
		vTaskDelay(10);
		xSemaphoreGive(xSemaphoreE2prom);
		return 1;
	}
	xSemaphoreGive(xSemaphoreE2prom);
	return 0;
}


/********************************************************************************************************************
**	函数名称：void fram_buf_write(unsigned char *p_buffer, unsigned short int write_addr, unsigned short int num_byte)
**	输入参数：
**	输出参数：
**	要求有返回值 0：写入成功，1：写入失败
********************************************************************************************************************/

unsigned char fr_fram_write(unsigned short int Addr,unsigned char *Data,  unsigned short int Len)
{
	unsigned char num_of_page = 0, num_of_single = 0, count = 0;
  unsigned short int addr = 0;
	unsigned char rv;

	if((Addr + Len) > 8191)                          //首先判断地址 
		return 1;
	
		
  addr = Addr % FRAM_PAGE_SIZE;           //页内地址
  count = FRAM_PAGE_SIZE - addr;                //
  num_of_page =  Len / FRAM_PAGE_SIZE;     //要写多少页
  num_of_single = Len % FRAM_PAGE_SIZE;    //不够一页多少字节。
 
  if(addr == 0) 
  {
    if(num_of_page == 0) 
    {
      rv = fram_page_write(Data, Addr, num_of_single);
			if(rv == 0)
				return 1;
    }
    else  
    {
      while(num_of_page)
      {
        rv = fram_page_write(Data, Addr, FRAM_PAGE_SIZE); 
				if(rv == 0)
					return 1;
        Addr +=  FRAM_PAGE_SIZE;
        Data += FRAM_PAGE_SIZE;
				num_of_page--;
      }

      if(num_of_single!=0)
      {
        rv = fram_page_write(Data, Addr, num_of_single);
				if(rv == 0)
					return 1;
      }
    }
  }
  else 
  {
    if(num_of_page== 0) 
    {
      if (Len > count)
      {
        rv  = fram_page_write(Data, Addr, count);
				if(rv == 0)
					return 1;

        rv = fram_page_write((unsigned char *)(Data + count), (Addr + count), (Len - count));
				if(rv == 0)
					return 1;

      }      
      else      
      {
        rv = fram_page_write(Data, Addr, num_of_single);
				if(rv == 0)
					return 1;
      }     
    }
    else
    {
      Len -= count;
      num_of_page =  Len / FRAM_PAGE_SIZE;
      num_of_single = Len % FRAM_PAGE_SIZE;
      
      if(count != 0)
      {  
        rv = fram_page_write(Data, Addr, count);
				if(rv == 0)
					return 1;
        Addr += count;
        Data += count;
      } 
      
      while(num_of_page)
      {
        rv = fram_page_write(Data, Addr, FRAM_PAGE_SIZE);
				if(rv == 0)
					return 1;
        Addr +=  FRAM_PAGE_SIZE;
        Data += FRAM_PAGE_SIZE;  
				num_of_page--;
      }
      if(num_of_single != 0)
      {
        rv = fram_page_write(Data, Addr, num_of_single); 
				if(rv == 0)
					return 1;
      }
    }
  } 	
	
	return 0;
}


/*******************************************************************
**	函数名称：uint8 EEPROMRead(uint16 Addr, uint8 *Data, uint16 Len)
**	功能描述：读取EEPROM数据
**	输入参数：*p_buffer:读取数据缓冲区
					read_addr:开始地址
					num_byte:读取的长度
**	输出参数：0：成功；1：失败

*********************************************************************/

unsigned char fr_fram_read(unsigned short addr, unsigned char *data, unsigned short int len)
{    
	unsigned char rv;
	
	if(addr + len > 8192)
		return 1;
	
	xSemaphoreTake(xSemaphoreE2prom,portMAX_DELAY );     //获取信号量
	
	rv = fr_i2c_read(FRAM_DEVIDE_ADDR,data,addr,len);
	
	xSemaphoreGive(xSemaphoreE2prom);      							//归还信号量
	
	if(rv == 1)
		return 0;
	else
		return 1;
}









