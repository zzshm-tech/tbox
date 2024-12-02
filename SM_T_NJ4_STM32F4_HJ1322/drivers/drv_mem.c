
#define BACKUP_SRAM_SIZE		(4*1024)


#include <stdio.h>
#include <string.h>



#include "stm32F4xx.h"
#include "stm32f4xx_hal.h"
#include "drv_mem.h"



#define		FRAM_PAGE_SIZE									32   			/**FRAM每页字节数**/
#define  	FRAM_PAGE_NUM										256       /**FRAM分页总数量**/
#define   FRAM_DEVIDE_ADDR								0x1A0  		/**FRAM从机地址**/

/*********************************************************************************

**********************************************************************************/

#define nop() 									__nop()

#define scl_out_high()         	GPIOB->BSRRL = GPIO_Pin_10 
#define scl_out_low()						GPIOB->BSRRH = GPIO_Pin_10 
   
#define sda_out_high()         	GPIOB->BSRRL = GPIO_Pin_11
#define sda_out_low()         	GPIOB->BSRRH = GPIO_Pin_11

#define scl_read()      				GPIOB->IDR  & GPIO_Pin_10
#define sda_read()     					GPIOB->IDR  & GPIO_Pin_11


/********************************************************************************
函数名称：void i2c_delay(void)
功能描述：
说明：

	// 20		- 330K 
	// 40	 	- 200K 波形不好
	// 50	 	- 132K
	// 80 	- 100K 波形良好
	// 100	- 80k  波形良好

********************************************************************************/

static void i2c_delay(void)
{        
  unsigned char i = 200; 
  
  while(i--) 
	{
		nop();
	}
}


/********************************************************************************
函数名称：
功能描述：初始化模拟I2C引脚


********************************************************************************/

void i2c_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; 						//EEPROM写保护引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 		//IO口速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;					//复用功能
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;				//推免复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;					//上拉
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB,GPIO_Pin_1);
	
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;				//推免复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;					//上拉
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}




void sda_out_mode(void)
{
    GPIO_InitTypeDef  GPIO_Struct;
    GPIO_Struct.GPIO_Pin = GPIO_Pin_11;
    GPIO_Struct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_Struct.GPIO_OType = GPIO_OType_OD;
    GPIO_Struct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_Struct);
}
void sda_in_mode(void)
{
    GPIO_InitTypeDef  GPIO_Struct;
    GPIO_Struct.GPIO_Pin = GPIO_Pin_11;
    GPIO_Struct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_Struct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_Struct);
}


/****************************************************************************************
函数名称：
功能描述：发送I2C开始信号
返回值：
****************************************************************************************/

unsigned char i2c_start(void)
{
	sda_out_mode();
	sda_out_high();
	scl_out_high();
	i2c_delay();
	sda_out_low();
	i2c_delay();
	scl_out_low();                  	//
	
	return 1;
}


/******************************************************************
函数名称：void i2c_stop(void)
功能描述：停止I2C信号

******************************************************************/

void i2c_stop(void)
{
	sda_out_mode();
	scl_out_low();
	i2c_delay();        //
	sda_out_low();
	i2c_delay();
	scl_out_high();
	i2c_delay();
	sda_out_high();
	i2c_delay();                   
}


/*************************************************************************
函数名称：void i2c_ack(void)
功能描述：发送ACK信号，ACK：应答信号
*************************************************************************/

void i2c_ack(void)
{      
	scl_out_low();
	i2c_delay();
  
	sda_out_mode();
	sda_out_low();
	i2c_delay();
	scl_out_high();
	
	i2c_delay();
	scl_out_low();
	i2c_delay();
}


/**********************************************************************************
函数名称；void i2c_no_ack(void)
功能描述：
输入参数：无
输出参数：无
***********************************************************************************/
void i2c_no_ack(void)
{    
	scl_out_low();
	i2c_delay();
	sda_out_mode();
	sda_out_high();
	
	i2c_delay();
	scl_out_high();
	
	i2c_delay();
	scl_out_low();
	i2c_delay();
}


/****************************************************************************
函数名称：unsigned char i2c_send_byte(unsigned char sendbyte) 
功能描述：发送字节
输入参数：要发送的一个字节数据
输出参数：0--发送错误
					1--发送正确

*****************************************************************************/

unsigned char i2c_write_byte(unsigned char sendbyte) 
{
	unsigned char i;
	signed int cnt;
	
	cnt = 156000;
      
	sda_out_mode();
	for(i = 0;i < 8;i++)
	{	
		scl_out_low();
		i2c_delay();
		
		if(sendbyte & 0x80) 
			sda_out_high();
		else
			sda_out_low(); 
		
		sendbyte <<= 1;		
		i2c_delay();
		scl_out_high();
		i2c_delay();
	}
	
	scl_out_low();
	i2c_delay();
	sda_out_high();
	i2c_delay();
	scl_out_high();
	sda_in_mode();
	while(sda_read() && ((cnt--)) > 0)
	{
		nop();
	}
	
	if(cnt <= 0)
		return 0;
	
	scl_out_low();
	
	return 1;
}


/***************************************************************************************
函数名称：unsigned char i2c_receive_byte(unsigned char *byte)  
功能描述：接收来自从机发送的一个字节数据
输入参数：指向接收字节数据存放的位置。
***************************************************************************************/

unsigned char i2c_read_byte(unsigned char *byte)  
{ 
    unsigned char i;
    unsigned char receive_byte;

		receive_byte = 0;
		sda_out_high();
		
		sda_in_mode();
    for(i = 0;i < 8;i++)
    {
			scl_out_low();
      i2c_delay();
      scl_out_high();
			i2c_delay();
			
      if(sda_read())
      {
        receive_byte = (receive_byte << 1) | 0x01;
      }
			else
			{
				receive_byte = receive_byte << 1;
			}
			
			i2c_delay();
    }
		
    scl_out_low();
		*byte = receive_byte;
		
    return 1;
}








/*****************************************************************************
预定义系统互斥信号量
******************************************************************************/



/*****************************************************************************
函数名称：

******************************************************************************/


/***************************************************************************************************************************************************
函数名称：unsigned char i2c_write(unsigned short int slave_addr,unsigned char *data,unsigned short int mem_addr,unsigned short int data_len)
功能描述：写一个串字节数据
**输入参数 ：slave_addr—————————从机设备地址	
						 data----指针类型数据，指向要写的字节缓冲区
						 mem_add----从机地址空间
						 data_len-----数据长度
**输出参数 ：0----写数据错误
						 1----写数据正确
说明：salve_addr ,这个参数是从机地址，之所以使用两个字节，把高字节作为区分从机数据空间地址的长度，
									有些I2C从机设备数据空间地址是两个字节，例如AT24C256EEPROM存储器，在访问这类设备时，
									需要发送两个字节的数据空间地址。
									还有些I2C从机设备数据空间地址只有一个字节，例如外部RTC时钟设备，在访问这类设备时只需要发送
									一个字节数据空间地址即可。
									这样就是从机地址的高八位来区分数据空间地址是8位的还是16位的，高八位0，访问的是8地址数据空间，
									高8位大于0，说明访问的数据地址空间是16位的。
*****************************************************************************************************************************************************/

signed char i2c_write(unsigned short int slave_addr,unsigned char *data,unsigned short int mem_addr,unsigned short int data_len)
{
	unsigned int i;
	unsigned char res;
	
	
  res = 1;                  //等待信号量
	if(res == 1)
	{
		if(i2c_start() < 1)    //发送开始信号
		{
			                     //释放互斥信号
			return 0;
		}

		if(i2c_write_byte((unsigned char)slave_addr) == 0)
		{
			                    //释放互斥信号
			return 0;
		}

		if(slave_addr & 0xFF00)                                       
		{
			if(i2c_write_byte(*((unsigned char *)&mem_addr + 1)) == 0)
			{
				                    //释放互斥信号
				return 0;
			}
		}
		if(i2c_write_byte(*(unsigned char *)&mem_addr ) == 0)
		{
			                     //释放互斥信号
			return 0;
		}
		
		for(i = 0;i < data_len;i++)
		{
			if(i2c_write_byte(*(data + i)) == 0)
			{
				                     //释放互斥信号
				return 0;
			}
		}
		
		i2c_stop();
		                    //释放互斥信号
	}
	
	return 1;
}


/**************************************************************************************************************************
函数名称：signed char i2c_read(unsigned short int slave_addr,unsigned char *data,unsigned short int mem_addr,unsigned short data_len)
功能描述：读从机数据
输入参数：slave_addr----从机地址
					data---接收数据缓冲区指针
					mem_addr---从机数据地址
					data_len---从机数据长度

*************************************************************************************************************************/

signed char i2c_read(unsigned short int slave_addr,unsigned char *data,unsigned short int mem_addr,unsigned short data_len)
{
	unsigned short int 			i;
	unsigned char 					res;
	
	                      //等待信号量
	res = 1;
	if(res == 1)
	{
		if(i2c_start() < 1)                                   //发送开始信号
		{
			                   		//释放互斥信号
			return 0;
		}
		
		if(i2c_write_byte((unsigned char)slave_addr & 0xFFFE) == 0)     //发送从机地址
		{
			                     //释放互斥信号
			return 0;
		}
		
		if(slave_addr & 0xFF00)                                        //发送数据地址
		{  
			if(i2c_write_byte(*((unsigned char *)&mem_addr + 1)) == 0)
			{
				                    //释放互斥信号
				return 0;
			}
		}
		
		if(i2c_write_byte(*(unsigned char *)&mem_addr ) == 0)
			return 0;	
		
		if(i2c_start() < 1)                                         		//发送开始信号
			return 0;

		if(i2c_write_byte((unsigned char)(slave_addr | 0x0001)) == 0)   	//发送从机地址及读标志位
			return 0;

		for(i = 0; i < data_len;i++)                               			//循环接收从机数据
		{
			if(i2c_read_byte(data + i) == 0)
				return 0;
			if(i == (data_len - 1))
			{
				i2c_no_ack();                                       				//最好一个数据不发送ACK信号
			}
			else
			{
				i2c_ack();
			}
		}
		i2c_stop();                       //停止I2C信号
		  		//释放互斥信号
	}
	else
	{
		         //释放资源
		return 0;
	}
	                  								
	return 1;
}




unsigned char fram_page_write(unsigned char *p_buffer,unsigned short int write_addr, unsigned char num_byte)
{ 
	unsigned char res;
	
	
	res = i2c_write(FRAM_DEVIDE_ADDR,p_buffer, write_addr,num_byte);   //写EERPOM  
	if(res == 1)
	{
		delay_ms(20);
		return 1;
	}
	return 0;
}




void fram_buf_write(unsigned char *p_buffer, unsigned short int write_addr, unsigned short int num_byte)
{
	unsigned char num_of_page = 0, num_of_single = 0, count = 0;
  unsigned short int addr = 0;

	//xSemaphoreTake(xSemaEEPROM,portMAX_DELAY );     //获取信号量
  addr = write_addr % FRAM_PAGE_SIZE;           //计算要写多少写
  count = FRAM_PAGE_SIZE - addr;                //
  num_of_page =  num_byte / FRAM_PAGE_SIZE;     //要写多少页
  num_of_single = num_byte % FRAM_PAGE_SIZE;    //不够一页多少字节。
 
  if(addr == 0) 
  {
    if(num_of_page == 0) 
    {
      fram_page_write(p_buffer, write_addr, num_of_single);
    }
    else  
    {
      while(num_of_page)
      {
        fram_page_write(p_buffer, write_addr, FRAM_PAGE_SIZE); 
        write_addr +=  FRAM_PAGE_SIZE;
        p_buffer += FRAM_PAGE_SIZE;
				num_of_page--;
      }

      if(num_of_single!=0)
      {
        fram_page_write(p_buffer, write_addr, num_of_single);
      }
    }
  }
  else 
  {
    if(num_of_page== 0) 
    {
      if(num_byte > count)
      {
        fram_page_write(p_buffer, write_addr, count);

        fram_page_write((unsigned char *)(p_buffer + count), (write_addr + count), (num_byte - count));

      }      
      else      
      {
        fram_page_write(p_buffer, write_addr, num_of_single);
      }     
    }
    else
    {
      num_byte -= count;
      num_of_page =  num_byte / FRAM_PAGE_SIZE;
      num_of_single = num_byte % FRAM_PAGE_SIZE;
      
      if(count != 0)
      {  
        fram_page_write(p_buffer, write_addr, count);

        write_addr += count;
        p_buffer += count;
      } 
      
      while(num_of_page)
      {
        fram_page_write(p_buffer, write_addr, FRAM_PAGE_SIZE);

        write_addr +=  FRAM_PAGE_SIZE;
        p_buffer += FRAM_PAGE_SIZE;  
				num_of_page--;
      }
      if(num_of_single != 0)
      {
        fram_page_write(p_buffer, write_addr, num_of_single); 
      }
    }
  } 	
	//xSemaphoreGive(xSemaEEPROM);      //归还信号量
}



/**************************************************************************************************
函数名称：unsigned char fram_buf_read(unsigned char *p_buffer, unsigned short int read_addr, unsigned short int num_byte)
功能描述：读取fram数据
输入参数：*p_buffer:读取数据缓冲区
					read_addr:开始地址
					num_byte:读取的长度
输出参数：

**************************************************************************************************/

unsigned char fram_buf_read(unsigned char *p_buffer, unsigned short int read_addr, unsigned short int num_byte)
{    
	unsigned char state;
	
	//xSemaphoreTake(xSemaEEPROM,portMAX_DELAY );     //获取信号量
	state = i2c_read(FRAM_DEVIDE_ADDR,p_buffer,read_addr,num_byte);           //有延时
	//xSemaphoreGive(xSemaEEPROM);      //归还信号量
	if(state == 0)
		return 0;
	
	return 1;
}





/**********************************************************
函数名称：
功能描述：初始化SPI1
************************************************************/

void spi1_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
  /*!< Enable the SPI clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

  /*!< Enable GPIO clocks */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	/********************************
	PA5 - SCK
	PA6 - MISO
	PA7 - MOSI
	PA4 - CS
	PA3 - WP
	*********************************/
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource5,GPIO_AF_SPI1);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource6,GPIO_AF_SPI1);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource7,GPIO_AF_SPI1);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
        
  /*!< SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /*!< SPI MOSI pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /*!< SPI MISO pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; 
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler=SPI_BaudRatePrescaler_256;
	SPI_InitStructure.SPI_FirstBit=SPI_FirstBit_MSB;SPI_InitStructure.
	SPI_CRCPolynomial = 7;
	SPI_Init(SPI1, &SPI_InitStructure);   
	SPI_Cmd(SPI1, ENABLE); 

  /*!< Configure sFLASH Card CS pin in output pushpull mode ********************/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;          //SPI-CS引脚
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;             //电源引脚
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_SetBits(GPIOD, GPIO_Pin_12);
}



/***********************************************************
**	函数信息：
**	功能描述： 发送一个字节
**	输入参数：
**	输出参数：
************************************************************/

unsigned char SPI_FLASH_SendByte(unsigned char byte)
{
  /* Loop while DR register in not emplty */
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);

  /* Send byte through the SPI1 peripheral */
  SPI_I2S_SendData(SPI1, byte);

  /* Wait to receive a byte */
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

  /* Return the byte read from the SPI bus */
  return SPI_I2S_ReceiveData(SPI1);
}




/*******************************************************************************
函数信息：
功能描述：
输入参数：
输出参数：

*******************************************************************************/

void SPI_FLASH_WriteEnable(void)
{
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();

  /* Send "Write Enable" instruction */
  SPI_FLASH_SendByte(W25X_WriteEnable);

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();
}


/*******************************************************************************
函数信息：
功能描述： 
输入参数：
输出参数：

*******************************************************************************/

static void SPI_FLASH_WaitForWriteEnd(void)
{
  unsigned char	FLASH_Status = 0;

  SPI_FLASH_CS_LOW();

  /* Send "Read Status Register" instruction */
  SPI_FLASH_SendByte(W25X_ReadStatusReg);

  /* Loop as long as the memory is busy with a write cycle */
  do
  {
    /* Send a dummy byte to generate the clock needed by the FLASH
    and put the value of the status register in FLASH_Status variable */
    FLASH_Status = SPI_FLASH_SendByte(Dummy_Byte);

  }
  while ((FLASH_Status & WIP_Flag) == SET); /* Write in progress */

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();
}





/*******************************************************************************
功能描述：闪存擦除
*******************************************************************************/
unsigned char EraseExFlashSector(unsigned int SectorIndex)
//unsigned char SPI_FLASH_SectorErase(unsigned int SectorNum)
{
	uint32_t SectorAddr = 0;
	
	/*  检查入口参数 */
	if ((SectorIndex > SECTOR_NUMS))	
		return ERROR;	
   	
	
	SectorAddr = SectorIndex * SECTOR_SIZE;
	
  /* Send write enable instruction */
  SPI_FLASH_WriteEnable();

  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();	
	
  /* Sector Erase */
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();
  /* Send Sector Erase instruction */
  SPI_FLASH_SendByte(W25X_SectorErase);
  /* Send SectorAddr high nibble address byte */
  SPI_FLASH_SendByte((SectorAddr & 0xFF0000) >> 16);
  /* Send SectorAddr medium nibble address byte */
  SPI_FLASH_SendByte((SectorAddr & 0xFF00) >> 8);
  /* Send SectorAddr low nibble address byte */
  SPI_FLASH_SendByte(SectorAddr & 0xFF);
  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();
  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();
	
	return OK;
	
}




/*******************************************************************************
函数信息：
功能描述：闪存也写
输入参数：
输出参数：
*******************************************************************************/

void WriteExFlashPage(unsigned char *pBuffer, unsigned int WriteAddr, unsigned short int NumByteToWrite)
{
  /* Send write enable instruction */
  SPI_FLASH_WriteEnable();
	
  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();

  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();
  /* Send "Write to Memory " instruction */
  SPI_FLASH_SendByte(W25X_PageProgram);
  /* Send WriteAddr high nibble address byte to write to */
  SPI_FLASH_SendByte((WriteAddr & 0xFF0000) >> 16);
  /* Send WriteAddr medium nibble address byte to write to */
  SPI_FLASH_SendByte((WriteAddr & 0xFF00) >> 8);
  /* Send WriteAddr low nibble address byte to write to */
  SPI_FLASH_SendByte(WriteAddr & 0xFF);

  if(NumByteToWrite > SPI_FLASH_PerWritePageSize)
  {
     NumByteToWrite = SPI_FLASH_PerWritePageSize;
     //printf("\n\r Err: SPI_FLASH_PageWrite too large!");
  }

  /* while there is data to be written on the FLASH */
  while (NumByteToWrite--)
  {
    /* Send the current byte */
    SPI_FLASH_SendByte(*pBuffer);
    /* Point on the next byte to be written */
    pBuffer++;
  }

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();

  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();
}





/*******************************************************************************
函数信息：
功能描述： 
输入参数：
输出参数：

*******************************************************************************/

unsigned char ReadExFlashBuf(unsigned char *pBuffer, unsigned int ReadAddr, unsigned short int NumByteToRead)
{

	if ((ReadAddr + NumByteToRead > ADDR_MAX)||(NumByteToRead == 0))	
		return ERROR;	 //	检查入口参数
	
	
	/* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();
	
  /* Send "Read from Memory " instruction */
  SPI_FLASH_SendByte(W25X_ReadData);

  /* Send ReadAddr high nibble address byte to read from */
  SPI_FLASH_SendByte((ReadAddr & 0xFF0000) >> 16);
  /* Send ReadAddr medium nibble address byte to read from */
  SPI_FLASH_SendByte((ReadAddr& 0xFF00) >> 8);
  /* Send ReadAddr low nibble address byte to read from */
  SPI_FLASH_SendByte(ReadAddr & 0xFF);

  while (NumByteToRead--) /* while there is data to be read */
  {
    /* Read a byte from the FLASH */
    *pBuffer = SPI_FLASH_SendByte(Dummy_Byte);
    /* Point to the next location where the byte read will be saved */
    pBuffer++;
  }

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();
	
  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();	
	
	return OK;
}






/*******************************************************************************
函数信息：
功能描述： 
输入参数：
输出参数：

*******************************************************************************/

unsigned int ReadExFlashJEDEC(void)
//unsigned int SPI_FLASH_Read_JEDEC_ID(void)
{
  unsigned int Temp = 0;
	unsigned int ManufacturerID = 0;
	unsigned int MemoryType = 0;
	unsigned int MemoryCapacity = 0;

  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();

  /* Send "RDID " instruction */
  SPI_FLASH_SendByte(W25X_JedecDeviceID);

  /* Read a byte from the FLASH */
  ManufacturerID = SPI_FLASH_SendByte(Dummy_Byte);

  /* Read a byte from the FLASH */
  MemoryType = SPI_FLASH_SendByte(Dummy_Byte);

  /* Read a byte from the FLASH */
  MemoryCapacity = SPI_FLASH_SendByte(Dummy_Byte);

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();

  Temp = (ManufacturerID << 16) | (MemoryType << 8) | MemoryCapacity;

  return Temp;
}


/*******************************************************************************
函数信息：
功能描述： 读取闪存ID号
输入参数：
输出参数：
*******************************************************************************/

unsigned int ReadExFlashDeviceID(void)
{
  unsigned int Temp = 0;

  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();

  /* Send "RDID " instruction */
  SPI_FLASH_SendByte(W25X_DeviceID);
  SPI_FLASH_SendByte(Dummy_Byte);
  SPI_FLASH_SendByte(Dummy_Byte);
  SPI_FLASH_SendByte(Dummy_Byte);
  
  /* Read a byte from the FLASH */
  Temp = SPI_FLASH_SendByte(Dummy_Byte);

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();

  return Temp;
}






/*******************************
**	备份寄存器
*******************************/

unsigned char init_backup_sram(void)
{ 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE); 			//电源接口时钟使能 (Power interface clock enable)
	PWR_BackupAccessCmd(ENABLE);  														//DBP 位置 1，使能对备份域的访问														
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_BKPSRAM, ENABLE);   //通过将 RCC AHB1 外设时钟使能寄存器
	PWR_BackupRegulatorCmd(ENABLE);														//应用程序必须等待备份调压器就绪标志
	
	while(PWR_GetFlagStatus(PWR_FLAG_BRR) != SET);
	
	return 1;
}


/*************************
**	
*****************************/

unsigned short int write_backupsram_data(unsigned short int addr,unsigned char *p_data, unsigned short int data_len)
{ 
	unsigned int len;
	
	if(p_data == NULL) 
		return 0;											//无效的地址
	if(data_len == 0) 
		return 0;											//无效的数量
	if(addr >= BACKUP_SRAM_SIZE) 
		return 0;						//起始地址有误
	len = addr + data_len;	
	if(len > BACKUP_SRAM_SIZE) 
		len = BACKUP_SRAM_SIZE;					//限制范围，只有4KB
	len -= addr;													//计算要写入的数据长度
	memcpy((unsigned char *)BKPSRAM_BASE + addr, p_data, data_len);
	
	return len;
}
 


/*************************
**	读取备份SRAM
**	
*****************************/

unsigned short int read_backupsram_data(unsigned short int addr,unsigned char *p_data, unsigned short int data_len)
{ 
	unsigned int len;
	
	if(p_data==NULL) 
		return 0;											//无效的地址
	if(data_len == 0) 
		return 0;											//无效的数量
	if(addr >= BACKUP_SRAM_SIZE) 
		return 0;											//
	len = addr + data_len;	
	if(len > BACKUP_SRAM_SIZE) 
		len = BACKUP_SRAM_SIZE;					//限制范围，只有4KB
	len -= addr;													//
	memcpy(p_data, (u8 *)BKPSRAM_BASE + addr, data_len);
	
	return len;
}



/******************File End********************/


