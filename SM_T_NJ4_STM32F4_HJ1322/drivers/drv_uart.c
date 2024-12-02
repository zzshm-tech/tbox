


/************************
**
**
**************************/


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>


#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"


#define NUM_OF_COM		6

#define endof(array)	(array + sizeof(array))


/*************本地全局变量***************/

				
static int		 					com1_recv_size;			
static unsigned char	 	com1_recv_buf[1024];
static unsigned char		*front_com1_recv_buf = com1_recv_buf;	
static unsigned char		*rear_com1_recv_buf = com1_recv_buf;	
		
static int		 					com2_recv_size;			
static unsigned char	 	com2_recv_buf[256];
static unsigned char		*front_com2_recv_buf = com2_recv_buf;	
static unsigned char		*rear_com2_recv_buf = com2_recv_buf;	

 		
static int		 					com3_recv_size;			
static unsigned char	 	com3_recv_buf[1024];
static unsigned char		*front_com3_recv_buf = com3_recv_buf;
static unsigned char		*rear_com3_recv_buf = com3_recv_buf;

					
static int		 					com4_recv_size;			
static unsigned char	 	com4_recv_buf[1024];
static unsigned char		*front_com4_recv_buf = com4_recv_buf;
static unsigned char		*rear_com4_recv_buf = com4_recv_buf;

					
static int		 	com5_recv_size;			
static unsigned char	 	com5_recv_buf[256];
static unsigned char		*front_com5_recv_buf = com5_recv_buf;
static unsigned char		*rear_com5_recv_buf = com5_recv_buf;

					
static int		 	com6_recv_size;			
static unsigned char	 	com6_recv_buf[1024];
static unsigned char		*front_com6_recv_buf = com6_recv_buf;
static unsigned char		*rear_com6_recv_buf = com6_recv_buf;



static void (*rt_irq_uart1_hook)(uint16_t size);
static void (*rt_irq_uart2_hook)(uint16_t size);
static void (*rt_irq_uart3_hook)(uint16_t size);
static void (*rt_irq_uart4_hook)(uint16_t size);       //
static void (*rt_irq_uart5_hook)(uint16_t size);       //
static void (*rt_irq_uart6_hook)(uint16_t size);       //


void rt_irq_uart_sethook(uint8_t uart,void (*hook)(uint16_t))
{
	switch(uart)
	{
		case 1:
			if(rt_irq_uart1_hook == NULL)
				rt_irq_uart1_hook = hook;
			break;
		case 2:
			if (rt_irq_uart2_hook == NULL)
				rt_irq_uart2_hook = hook;
			break;
		case 3:
			if (rt_irq_uart3_hook == NULL)
				rt_irq_uart3_hook = hook;
			break;
		case 4:
			if (rt_irq_uart4_hook == NULL)
				rt_irq_uart4_hook = hook;
			break;
		case 5:
			if (rt_irq_uart5_hook == NULL)
				rt_irq_uart5_hook = hook;
			break;
		case 6:
			if (rt_irq_uart6_hook == NULL)
				rt_irq_uart6_hook = hook;
			break;
	}
}

/***************************
**	函数名称：
**	功能描述:
****************************/

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
    case 5:
      com5_recv_size = 0;
      front_com5_recv_buf = rear_com5_recv_buf;
      break;
    case 6:
      com6_recv_size = 0;
      front_com6_recv_buf = rear_com6_recv_buf;
      break;
   }
}



/**********************************************************************
** 函数名称：int read_com_buf(unsigned char port, unsigned char *dest, int n)
** 功能描述:
************************************************************************/

uint16_t rt_read_uart_buf(uint8_t port, uint8_t *dest, uint16_t n)
{
  uint16_t rv = 0;
  
  switch (port) 
  {
    case 1:
      if(n > com1_recv_size)
        n = com1_recv_size;
      rv = n;
      while(n > 0) 
      {
				front_com1_recv_buf++;
        if(front_com1_recv_buf == endof(com1_recv_buf))
					front_com1_recv_buf = com1_recv_buf;
        *dest++ = *front_com1_recv_buf;
        com1_recv_size--;
        n--;
        
      }
      break;
    case 2:
        if(n > com2_recv_size)
          n = com2_recv_size;
        rv = n;
        while (n > 0) 
        {
					front_com2_recv_buf++;
					if(front_com2_recv_buf == endof(com2_recv_buf))
            front_com2_recv_buf = com2_recv_buf;
          *dest++ = *front_com2_recv_buf;
					com2_recv_size--;
					n--;
        }
        break;
     case 3:
       if(n > com3_recv_size)
         n = com3_recv_size;
        rv = n;
        while(n > 0) 
        {
					front_com3_recv_buf++;
          if(front_com3_recv_buf == endof(com3_recv_buf))
            front_com3_recv_buf = com3_recv_buf;
          *dest++ = *front_com3_recv_buf;
          com3_recv_size--;
          n--;
          
        }
        break;
      case 4:
        if(n > com4_recv_size)
          n = com4_recv_size;
        rv = n;
        while (n > 0) 
        {
					front_com4_recv_buf++;
          if(front_com4_recv_buf == endof(com4_recv_buf))
            front_com4_recv_buf = com4_recv_buf;
          *dest++ = *front_com4_recv_buf;
					com4_recv_size--;
					n--; 
        }
        break;
      case 5:
        if(n > com5_recv_size)
          n = com5_recv_size;
        rv = n;
        while(n > 0)
        {
          *dest++ = *front_com5_recv_buf;
          com5_recv_size--;
	n--;
          front_com5_recv_buf++;
	if(front_com5_recv_buf == endof(com5_recv_buf))
            front_com5_recv_buf = com5_recv_buf;
	}
	break;
      case 6:
	if(n > com6_recv_size)
            n = com6_recv_size;
	rv = n;
	while(n > 0)
	{
            *dest++ = *front_com6_recv_buf;
            com6_recv_size--;
            n--;
            front_com6_recv_buf++;
            if(front_com6_recv_buf == endof(com6_recv_buf))
              front_com6_recv_buf = com6_recv_buf;
	}
	break;
        }	
      
        return rv;
}

/***************************
**	函数名称：
**	功能描述:
****************************/

uint8_t rt_write_uart_buf(uint8_t port, uint8_t *buf, uint16_t size)
{
	uint16_t i = 0;
	
  if(size == 0)
     return 1;
	
  switch(port) 
  {
    case 1:
      
      break;
    case 2:
      
      break;
    case 3:
      for(i = 0;i < size; i++)
			{
				while(USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
				USART_SendData(USART3, *buf++);
				while(USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
			}
      break;
    case 4:
      for(i = 0;i < size; i++)
			{
				while(USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET);
				USART_SendData(UART4, *buf++);
				while(USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET);
			}
      break;
    case 5:
      break;
    case 6:
      
      break;
   }
   return 0;	
}


/***************************
**	函数名称：
**	功能描述:
****************************/

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
		
	if(rt_irq_uart1_hook != NULL)
		rt_irq_uart1_hook(com1_recv_size);
}


/***************************
**	函数名称：
**	功能描述:
****************************/

void write_com2_recv_buf(void)
{
  if((rear_com2_recv_buf != front_com2_recv_buf) || com2_recv_size == 0)
  {
    if(++rear_com2_recv_buf == endof(com2_recv_buf))
      rear_com2_recv_buf = com2_recv_buf;
    *rear_com1_recv_buf = USART_ReceiveData(USART2);
    com2_recv_size++;
  }
  else 
  {
    if(++front_com2_recv_buf == endof(com2_recv_buf))
      front_com2_recv_buf = com2_recv_buf;
    com2_recv_size--;
    if(++rear_com2_recv_buf == endof(com2_recv_buf))
      rear_com2_recv_buf = com2_recv_buf;
    *rear_com1_recv_buf = USART_ReceiveData(USART2);
    com2_recv_size++;
  }	
}


/***************************
**	函数名称：
**	功能描述:
****************************/

void write_com3_recv_buf(void)
{
  if((rear_com3_recv_buf != front_com3_recv_buf) || com3_recv_size == 0)
  {
    if(++rear_com3_recv_buf == endof(com3_recv_buf))
      rear_com3_recv_buf = com3_recv_buf;
		*rear_com3_recv_buf = USART_ReceiveData(USART3);
    com3_recv_size++;
  }
  else 
  {
    if(++front_com3_recv_buf == endof(com3_recv_buf))
      front_com3_recv_buf = com3_recv_buf;
    com3_recv_size--;
    if(++rear_com3_recv_buf == endof(com3_recv_buf))
      rear_com3_recv_buf = com3_recv_buf;
		*rear_com3_recv_buf = USART_ReceiveData(USART3);
    com3_recv_size++;
  }	
	if(rt_irq_uart3_hook != NULL)
		rt_irq_uart3_hook(com3_recv_size);
}


/***************************
**	函数名称：
**	功能描述:串口4
****************************/

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
	if(rt_irq_uart4_hook != NULL)
		rt_irq_uart4_hook(com4_recv_size);	
}



/***************************
**	函数名称：
**	功能描述:
****************************/

void write_com5_recv_buf(void)
{
  if((rear_com5_recv_buf != front_com1_recv_buf) || com5_recv_size == 0)
  {
    if(++rear_com5_recv_buf == endof(com5_recv_buf))
      rear_com5_recv_buf = com5_recv_buf;
    *rear_com5_recv_buf = 0;
    com5_recv_size++;   
  }
  else 
  {
    if(++front_com5_recv_buf == endof(com5_recv_buf))
	front_com5_recv_buf = com5_recv_buf;
    com5_recv_size--;
    if(++rear_com5_recv_buf == endof(com5_recv_buf))
      rear_com5_recv_buf = com5_recv_buf;
    *rear_com5_recv_buf = 0;
    com5_recv_size++;
  }	
}



/***************************
**	函数名称：
**	功能描述:
****************************/

void write_com6_recv_buf(void)
{
  if((rear_com6_recv_buf != front_com6_recv_buf) || com6_recv_size == 0)
  {
    if(++rear_com6_recv_buf == endof(com6_recv_buf))
      rear_com6_recv_buf = com6_recv_buf;
   // HAL_UART_Receive_IT(&huart6,rear_com6_recv_buf,1);
    com6_recv_size++;
   }
   else 
  {
    if(++front_com6_recv_buf == endof(com6_recv_buf))
      front_com6_recv_buf = com6_recv_buf;
    com6_recv_size--;
    if(++rear_com6_recv_buf == endof(com6_recv_buf))
      rear_com6_recv_buf = com6_recv_buf;
    //HAL_UART_Receive_IT(&huart6,rear_com6_recv_buf,1);
    com6_recv_size++;
  }	
}


/*******************************
**
**
********************************/

void rt_hw_init_uart1(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1); //GPIOB10复用为USART3
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1); //GPIOB11复用为USART3
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_9; 		//GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;							//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;					//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 						//推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 							//上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); 										//初始化PA9，PA10
	
	USART_InitStructure.USART_BaudRate = 115200;

	USART_InitStructure.USART_WordLength = USART_WordLength_8b;

	USART_InitStructure.USART_StopBits = USART_StopBits_1;

	USART_InitStructure.USART_Parity = USART_Parity_No;

	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART1,&USART_InitStructure);

	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);

	USART_Cmd(USART1,ENABLE);
}



void rt_hw_init_uart2(void)
{
 
}



/*******************************
**	初始化串口3
********************************/

void rt_hw_init_uart3(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);
	
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource8,GPIO_AF_USART3); 	//GPIOB10复用为USART3
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource9,GPIO_AF_USART3); 	//GPIOB11复用为USART3
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9; 		//GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;							//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;					//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 						//推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 							//上拉
	GPIO_Init(GPIOD,&GPIO_InitStructure); 										//初始化PA9，PA10
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;	 							//开启GNSS模块电源
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 				//IO口速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;							//
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;						//推免复用输出
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_SetBits(GPIOE, GPIO_Pin_14);

	GPIO_Init(GPIOD, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 9600;

	USART_InitStructure.USART_WordLength = USART_WordLength_8b;

	USART_InitStructure.USART_StopBits = USART_StopBits_1;

	USART_InitStructure.USART_Parity = USART_Parity_No;

	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART3,&USART_InitStructure);

	USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);

	USART_Cmd(USART3,ENABLE);
}





/********************************
**	初始化串口4
**********************************/

void rt_hw_init_uart4(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);
	
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource0,GPIO_AF_UART4); //GPIOA-0复用为UART4
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource1,GPIO_AF_UART4); //GPIOA-1复用为UART4
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1; 		//GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;								//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;						//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 							//推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 							//上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); 										//初始化PA9，PA10
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; 		//GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;								//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;						//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 							//推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; 							//上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); 										//初始化PA9，PA10
		
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;	 								//IO口第9脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 				//IO口速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;							//
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;						//推免复用输出
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	
	
	USART_InitStructure.USART_BaudRate = 115200;

	USART_InitStructure.USART_WordLength = USART_WordLength_8b;

	USART_InitStructure.USART_StopBits = USART_StopBits_1;

	USART_InitStructure.USART_Parity = USART_Parity_No;

	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(UART4,&USART_InitStructure);

	USART_ITConfig(UART4,USART_IT_RXNE,ENABLE);

	USART_Cmd(UART4,ENABLE);
}




/********************************
**	初始化串口5
**********************************/

void rt_hw_init_uart5(void)
{
	
}




/********************************
**	初始化串口6
**      蓝夜功能
**********************************/

void rt_hw_init_uart6(void)
{
}




/******************************
**	串口1中断服务函数
**	RS323功能
*******************************/

void USART1_IRQHandler(void)
{
	uint32_t  rv = 0;
	rv = taskENTER_CRITICAL_FROM_ISR();
	if(USART_GetFlagStatus(USART1,USART_FLAG_RXNE) == SET)
	{
		write_com1_recv_buf();
	}
	
	taskEXIT_CRITICAL_FROM_ISR(rv);
}



/******************************
**	串口1中断服务函数
**	GSM 功能
*******************************/

void USART2_IRQHandler(void)
{
	
  write_com2_recv_buf();
}


/******************************
**	串口3中断服务函数
**	GNSS功能
*******************************/

void USART3_IRQHandler(void)
{
	uint32_t rv = taskENTER_CRITICAL_FROM_ISR();
	if(USART_GetFlagStatus(USART3,USART_FLAG_RXNE) == SET)
	{
		write_com3_recv_buf();	
	}
	
	taskEXIT_CRITICAL_FROM_ISR(rv);
	
}



/******************************
**	串口4中断服务函数
**	GNSS功能
*******************************/

void UART4_IRQHandler(void)
{
	uint32_t rv = taskENTER_CRITICAL_FROM_ISR();
	
	if(USART_GetFlagStatus(UART4,USART_FLAG_RXNE) == SET)
	{
		write_com4_recv_buf();
	}
	
	taskEXIT_CRITICAL_FROM_ISR(rv);
}



/******************************
**	串口5中断服务函数
**	
*******************************/

void UART5_IRQHandler(void)
{
  write_com4_recv_buf();
}


/******************************
**	串口6中断服务函数
**	蓝牙功能
*******************************/

void USART6_IRQHandler(void)
{
  write_com6_recv_buf();
}




/**********************************************
**********************************************/


int fputc(int ch, FILE *f)
{
	USART_SendData(USART1, (uint8_t)ch);

	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
	{}
	return ch;
}



/*****************************
**	打印16进制
******************************/

int printf_hex(unsigned char *buf,unsigned short int buf_size)
{
  int i;
  int j;
	
  for(j = 0,i = 0; i < buf_size;i++)
  {
    printf("%02X  ",*(buf + i));
    j++;
    if(j > 9)
    {
      j = 0;
      printf("\r\n");
    }
  }
	
  printf("\r\n");
	
  return buf_size;
}


/******************File End****************/

