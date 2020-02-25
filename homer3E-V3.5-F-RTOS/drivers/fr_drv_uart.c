
/*************************
*	
***************************/

#include <stdio.h>
#include <string.h>

#include "stm32f10x.h"
#include "board.h"



#define endof(array)									(array + sizeof(array))  
#define NUM_OF_COM										4



static void (*fr_uart1_hook)();

static void (*fr_uart2_hook)();

static void (*fr_uart4_hook)();
			
static int		 					com1_recv_size;			
static unsigned char	 	com1_recv_buf[1000];
static unsigned char		*front_com1_recv_buf = com1_recv_buf;	
static unsigned char		*rear_com1_recv_buf = com1_recv_buf;	

			
static int		 					com2_recv_size;			
static unsigned char	 	com2_recv_buf[255];
static unsigned char		*front_com2_recv_buf = com2_recv_buf;	
static unsigned char		*rear_com2_recv_buf = com2_recv_buf;	


		
static int		 					com3_recv_size;			
static unsigned char	 	com3_recv_buf[1];
static unsigned char		*front_com3_recv_buf = com3_recv_buf;
static unsigned char		*rear_com3_recv_buf = com3_recv_buf;
				
static int		 					com4_recv_size;			
static unsigned char	 	com4_recv_buf[1024];
static unsigned char		*front_com4_recv_buf = com4_recv_buf;
static unsigned char		*rear_com4_recv_buf = com4_recv_buf;

/**************************************************
**	函数名称:
**	功能描述:
****************************************************/

void write_com1_recv_buf(void)
{
	if((rear_com1_recv_buf != front_com1_recv_buf) || com1_recv_size == 0)
	{
		if(++rear_com1_recv_buf == endof(com1_recv_buf))
			rear_com1_recv_buf = com1_recv_buf;
		*rear_com1_recv_buf = USART_ReceiveData(USART1);
		com1_recv_size++;
	}
	else 
	{
		if(++front_com1_recv_buf == endof(com1_recv_buf))
			front_com1_recv_buf = com1_recv_buf;
		com1_recv_size--;
			if(++rear_com1_recv_buf == endof(com1_recv_buf))
				rear_com1_recv_buf = com1_recv_buf;
			*rear_com1_recv_buf = USART_ReceiveData(USART1);
			com1_recv_size++;
	}	
}


/**************************************************
**	函数名称:
**	功能描述:
****************************************************/

void clear_com(unsigned char port)
{
	switch (port) 
	{
		case 1:
			com1_recv_size = 0;
			front_com1_recv_buf = rear_com1_recv_buf;
			break;
		case 2:
			com2_recv_size = 0;
			front_com2_recv_buf = rear_com2_recv_buf;
			break;
		case 3:
			com3_recv_size = 0;
			front_com3_recv_buf = rear_com3_recv_buf;
			break;
		case 4:
			com4_recv_size = 0;
			front_com4_recv_buf = rear_com4_recv_buf;
			break;
	}
}



/**************************************************
**	函数名称:
**	功能描述:
****************************************************/

int datasize_in_com(unsigned char port)
{
	int	 rv;
	
	switch (port) 
	{
		case 1:
			rv = com1_recv_size;
			break;
		case 2:
			rv = com2_recv_size;
			break;
		case 3:
			rv = com3_recv_size;
			break;
		case 4:
			rv = com4_recv_size;
			break;
		default:
			rv = 0;
	}	
	return rv;
}


/**************************************************
**	函数名称:
**	功能描述:
****************************************************/

int read_com_buf(unsigned char port, unsigned char *dest, int n)
{
	int	 rv = 0;
	
	switch (port) 
	{
		case 1:
			if (n > com1_recv_size)
				n = com1_recv_size;
			rv = n;
			while (n > 0) 
			{
				front_com1_recv_buf++;
				if (front_com1_recv_buf == endof(com1_recv_buf))
					front_com1_recv_buf = com1_recv_buf;
				*dest++ = *front_com1_recv_buf;
				com1_recv_size--;
				n--;
			}
			break;
		case 2:
			if (n > com2_recv_size)
				n = com2_recv_size;
			rv = n;
			while (n > 0) 
			{
				front_com2_recv_buf++;
				if (front_com2_recv_buf == endof(com2_recv_buf))
					front_com2_recv_buf = com2_recv_buf;
				*dest++ = *front_com2_recv_buf;
				com2_recv_size--;
				n--;
			}
			break;
		case 3:
			if (n > com3_recv_size)
				n = com3_recv_size;
			rv = n;
			while (n > 0) 
			{
				front_com3_recv_buf++;
				if (front_com3_recv_buf == endof(com3_recv_buf))
					front_com3_recv_buf = com3_recv_buf;
				*dest++ = *front_com3_recv_buf;
				com3_recv_size--;
				n--;
			}
			break;
		case 4:
			if (n > com4_recv_size)
				n = com4_recv_size;
			rv = n;
			while (n > 0) 
			{
				front_com4_recv_buf++;
				if (front_com4_recv_buf == endof(com4_recv_buf))
					front_com4_recv_buf = com4_recv_buf;
				*dest++ = *front_com4_recv_buf;
				com4_recv_size--;
				n--;
			}
			break;
	}	
	return rv;
}


/**************************************************
**	函数名称:
**	功能描述:
****************************************************/

int read_uart_pkt(unsigned char port,unsigned char *dest,int max_destsize)
{
	int				 recv_size;
	
	if (port > NUM_OF_COM)
		return 0;
	
		recv_size = datasize_in_com(port);
	
		if(recv_size > max_destsize) 
		{
			clear_com(port);
			return 0;
		}
		
		return read_com_buf(port, dest, recv_size);
}



/************************************************************
**	函数信息：void write_com3_recv_buf(void)
**	功能描述：
**************************************************************/

void write_com2_recv_buf(void)
{
	if((rear_com2_recv_buf != front_com2_recv_buf) || com2_recv_size == 0)
	{
		if(++rear_com2_recv_buf == endof(com2_recv_buf))
			rear_com2_recv_buf = com2_recv_buf;
		*rear_com2_recv_buf = USART_ReceiveData(USART2);
		com2_recv_size++;
	}
	else 
	{
		if(++front_com2_recv_buf == endof(com2_recv_buf))
			front_com2_recv_buf = com2_recv_buf;
		com2_recv_size--;
			if(++rear_com2_recv_buf == endof(com2_recv_buf))
				rear_com2_recv_buf = com2_recv_buf;
			*rear_com2_recv_buf = USART_ReceiveData(USART2);
			com2_recv_size++;
	}	
}


/**********************************************************
**	函数信息：void write_com3_recv_buf(void)
**	功能描述：
**********************************************************/

void write_com4_recv_buf(void)
{
	if((rear_com4_recv_buf != front_com4_recv_buf) || com4_recv_size == 0)
	{
		if(++rear_com4_recv_buf == endof(com4_recv_buf))
			rear_com4_recv_buf = com4_recv_buf;
		*rear_com4_recv_buf = USART_ReceiveData(UART4);
		com4_recv_size++;
	}
	else 
	{
		if(++front_com4_recv_buf == endof(com4_recv_buf))
			front_com4_recv_buf = com4_recv_buf;
		com4_recv_size--;
			if(++rear_com4_recv_buf == endof(com4_recv_buf))
				rear_com4_recv_buf = com4_recv_buf;
			*rear_com4_recv_buf = USART_ReceiveData(UART4);
			com4_recv_size++;
	}	
}


/*************************************
**	USART1中断服务函数
**************************************/

void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
			USART_ClearITPendingBit(USART1, USART_IT_RXNE);
      write_com1_recv_buf();
			fr_uart1_hook();
    }
}


/*************************************
**	USART1中断服务函数
**************************************/

void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
    {
			USART_ClearITPendingBit(USART2, USART_IT_RXNE);
      write_com2_recv_buf();
			fr_uart2_hook();
    }
}

/*************************

**	USART1----GNSS(N303定位模块模块)
**	PA10  ---   RX
**	PA11	---		TX

***************************/

void fr_init_uart1(void)
{
    GPIO_InitTypeDef         GPIO_InitStructure;
    USART_InitTypeDef        USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    
    /* Configure USART3 Tx (PB.10) as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure USART3 Rx (PB.11) as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_DeInit(USART1);
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);												//Enable USART3
    USART_ClearITPendingBit(USART1, USART_IT_RXNE);
}


/***********************
**	关闭：
*************************/
void fr_close_uart1(void)
{
	GPIO_InitTypeDef         GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
	USART_DeInit(USART1);
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
  USART_Cmd(USART1, DISABLE);												//Enable USART3
}


void fr_send_uart1(unsigned char *source, unsigned int len)
{
	unsigned int k;

	if (len == 0)
		return;

	for(k = 0;k < len; k++)
	{
				while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
				USART_SendData(USART1, *source++);
				while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
	}
}



void fr_set_uart1_rx_hook(void (*hook)(void))
{
	if(fr_uart1_hook == NULL)  
		fr_uart1_hook = hook;
}


void fr_set_uart2_rx_hook(void (*hook)(void))
{
	if(fr_uart2_hook == NULL)  
		fr_uart2_hook = hook;
}

/******************************
**	初始化串口2
*******************************/

void fr_init_uart2(void)
{
    GPIO_InitTypeDef         GPIO_InitStructure;
    USART_InitTypeDef        USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    
    /* Configure USART3 Tx (PB.10) as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure USART3 Rx (PB.11) as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_DeInit(USART2);
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);												//Enable USART3
    USART_ClearITPendingBit(USART2, USART_IT_RXNE);
}


/***********************
**	关闭串口2
*************************/
void fr_close_uart2(void)
{
	
}



/*************************
**	串口2发送数据
**************************/

void fr_send_uart2(unsigned char *source, unsigned int len)
{
	unsigned int k;

	if (len == 0)
		return;
	
	for(k = 0;k < len; k++)
	{
			while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
			USART_SendData(USART2, *source++);
			while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
	}
}



/******************************
**	初始化串口4
*******************************/

void fr_init_uart4(void)
{
    GPIO_InitTypeDef         GPIO_InitStructure;
    USART_InitTypeDef        USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
    
    /* Configure USART3 Tx (PB.10) as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* Configure USART3 Rx (PB.11) as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    USART_DeInit(UART4);
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(UART4, &USART_InitStructure);

    USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
    USART_Cmd(UART4, ENABLE);												//
    USART_ClearITPendingBit(UART4, USART_IT_RXNE);
}



/******************************
**	初始化串口接收回调函数
*******************************/

void fr_set_uart4_rx_hook(void (*hook)(void))
{
	if(fr_uart4_hook == NULL)  
		fr_uart4_hook = hook;
}

/***********************
**	关闭串口4
*************************/
void fr_close_uart4(void)
{
	GPIO_InitTypeDef         GPIO_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,DISABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
	USART_DeInit(UART4);
	USART_ITConfig(UART4, USART_IT_RXNE, DISABLE);
  USART_Cmd(UART4, DISABLE);												//Enable
}


/*****************************
**	使用串口4发送数据
**	
**********************************/

void fr_send_uart4(unsigned char *source, unsigned int len)
{
	unsigned int k;

	if (len == 0)
		return;

	for(k = 0;k < len; k++)
	{
			while(USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET);
			USART_SendData(UART4, *source++);
			while(USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET);
	}
}




/*************************************
**	USART1中断服务函数
**************************************/

void UART4_IRQHandler(void)
{
    if(USART_GetITStatus(UART4, USART_IT_RXNE) == SET)
    {
			USART_ClearITPendingBit(UART4, USART_IT_RXNE);
      write_com4_recv_buf();
			fr_uart4_hook();
    }
}



/******************End File*****************/
