


#include "stm32f10x.h"

#include "fr_drv_i2c.h"



#define nop() 									__nop()

#define scl_out_high()         	GPIOB->BSRR = GPIO_Pin_10
#define scl_out_low()						GPIOB->BRR  = GPIO_Pin_10
   
#define sda_out_high()         	GPIOB->BSRR = GPIO_Pin_11
#define sda_out_low()         	GPIOB->BRR  = GPIO_Pin_11

#define scl_read()      				GPIOB->IDR  & GPIO_Pin_10
#define sda_read()     					GPIOB->IDR  & GPIO_Pin_11



/********************************************************************************
**	函数名称：void i2c_delay(void)
**	功能描述：
**	说明：
********************************************************************************/

static void i2c_delay(void)
{        
  unsigned char i = 40; 
  
  while(i--) 
	{
		nop();
	}
}


/************************************
**	函数名称:uint8 CpuFlashEraseSector(uint16 Sector)
**	功能描述:读数据
**	输入参数:Addr:地址；Data：存放数据的地址；Len：要写入数据的长度;
**	0:成功，1：失败
*************************************/

void fr_init_i2c(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10 | GPIO_Pin_11;    //I2C 引脚
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}


/****************************************************************************************
函数名称：
功能描述：发送I2C开始信号
返回值：
****************************************************************************************/

static unsigned char i2c_start(void)
{
	sda_out_high();              //输出高 等于释放总线了，
	scl_out_high();
	
	i2c_delay();
	
	if(!sda_read())
		return 0; 
	
	sda_out_low();
	i2c_delay();
	
	if(sda_read())
		return 0;  

	sda_out_low();                 	//
	i2c_delay();                  	//
	
	return 1;
}


/******************************************************************
函数名称：void i2c_stop(void)
功能描述：停止I2C信号

******************************************************************/

static void i2c_stop(void)
{
	scl_out_low();
	i2c_delay();
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

static void i2c_ack(void)
{      
	scl_out_low();
	i2c_delay();
  
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
static void i2c_no_ack(void)
{    
	scl_out_low();
	i2c_delay();
	
	sda_out_high();
	
	i2c_delay();
	scl_out_high();
	
	i2c_delay();
	scl_out_low();
	i2c_delay();
}


/****************************************************************************
** 	函数名称：unsigned char i2c_send_byte(unsigned char sendbyte) 
**	功能描述：发送字节
**	输入参数：要发送的一个字节数据
**	输出参数：0--发送错误
					1--发送正确

*****************************************************************************/

unsigned char i2c_write_byte(unsigned char sendbyte) 
{
	unsigned char i;
	signed int cnt;
	
	cnt = 52000;
      
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








/**************************************************************************************************************************
**	函数名称：signed char i2c_read(unsigned short int slave_addr,unsigned char *data,unsigned short int mem_addr,unsigned short data_len)
**	功能描述：读从机数据
**	输入参数：slave_addr----从机地址
					data---接收数据缓冲区指针
					mem_addr---从机数据地址
					data_len---从机数据长度

*************************************************************************************************************************/

signed char fr_i2c_read(unsigned short int slave_addr,unsigned char *data,unsigned short int mem_addr,unsigned short data_len)
{
	unsigned short int 			i;
	
	if(i2c_start() < 1)                                   //发送开始信号
		return 0;
		
	if(i2c_write_byte((unsigned char)slave_addr & 0xFFFE) == 0)     //发送从机地址
		return 0;
		
	if(slave_addr & 0xFF00)                                        //发送数据地址
	{  
		if(i2c_write_byte(*((unsigned char *)&mem_addr + 1)) == 0)
			return 0;
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
	               								
	return 1;
}


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

unsigned char fr_i2c_write(unsigned short int slave_addr,unsigned char *data,unsigned short int mem_addr,unsigned short int data_len)
{
	unsigned int i;

	if(i2c_start() < 1)    //发送开始信号
		return 0;
	
	if(i2c_write_byte((unsigned char)slave_addr) == 0)
		return 0;
		

	if(slave_addr & 0xFF00)                                       
	{
		if(i2c_write_byte(*((unsigned char *)&mem_addr + 1)) == 0)
			return 0;
			
	}
	if(i2c_write_byte(*(unsigned char *)&mem_addr ) == 0)
		return 0;
		
	for(i = 0;i < data_len;i++)
	{
		if(i2c_write_byte(*(data + i)) == 0)
			return 0;
			
	}
		
	i2c_stop();
	
	return 1;
}









/***************File End****************/





