


#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board.h"
#include "stm32f4xx.h"

#include "drv_can.h"



#define CAN_RX_BUF_NUM 20
#define can_rx_buf_endof(array)	(array + CAN_RX_BUF_NUM)


static struct can_rx_msg          can1_msg_rx_buf[CAN_RX_BUF_NUM] = {0};    //CAN1路接收缓冲区
static uint16_t          					can1_msg_rx_num = 0;
static struct can_rx_msg       	 	*front_can1_rx_buf = can1_msg_rx_buf;	
static struct can_rx_msg			 		*rear_can1_rx_buf = can1_msg_rx_buf;





/***************************
**	函数名称：
**	功能描述:
****************************/

void write_can1_rx_buf(CanRxMsg *p_msg)
{
	struct can_rx_msg tmp = {0};
	
	tmp.id = p_msg->ExtId;
	memcpy(tmp.data,p_msg->Data,8);
  //rt_printf("%d   %X %X\r\n",can_one_msg_rx_num,rear_can_one_rx_buf,can_rx_buf_endof(can_one_msg_rx_buf));
  if((rear_can1_rx_buf != front_can1_rx_buf) || can1_msg_rx_num == 0)
  {
    if(++rear_can1_rx_buf == can_rx_buf_endof(can1_msg_rx_buf))
      rear_can1_rx_buf = can1_msg_rx_buf;
    
    memcpy((uint8_t *)rear_can1_rx_buf,(uint8_t *)&tmp,sizeof(struct can_rx_msg));
    can1_msg_rx_num++;
  }
  else 
  {
    if(++front_can1_rx_buf == can_rx_buf_endof(can1_msg_rx_buf))
      front_can1_rx_buf = can1_msg_rx_buf;
    can1_msg_rx_num--;
    if(++rear_can1_rx_buf == can_rx_buf_endof(can1_msg_rx_buf))
      rear_can1_rx_buf = can1_msg_rx_buf;
    
    memcpy((uint8_t *)rear_can1_rx_buf,(uint8_t *)&tmp,sizeof(struct can_rx_msg));
    can1_msg_rx_num++;
  }	
}



/*******************************
**	CAN1配置
********************************/
uint8_t rt_hw_init_can(uint16_t baudrate)
{
  GPIO_InitTypeDef				GPIO_InitStructure;
  CAN_InitTypeDef        	CAN_InitStructure;
  CAN_FilterInitTypeDef  	CAN_FilterInitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
	
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_CAN1);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource1,  GPIO_AF_CAN1); 	
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	
  CAN_DeInit(CAN1);
	
	
			
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOE, GPIO_Pin_3);
	
  //
  CAN_InitStructure.CAN_TTCM = DISABLE;
  CAN_InitStructure.CAN_ABOM = DISABLE;
  CAN_InitStructure.CAN_AWUM = DISABLE;
  CAN_InitStructure.CAN_NART = ENABLE;
  CAN_InitStructure.CAN_RFLM = DISABLE;
  CAN_InitStructure.CAN_TXFP = DISABLE;
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
  CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
	
	CAN_InitStructure.CAN_SJW=CAN_SJW_1tq;
	CAN_InitStructure.CAN_BS1=CAN_BS1_6tq;
	CAN_InitStructure.CAN_BS2=CAN_BS2_7tq;

  switch(baudrate)         //
  { 
		case  250:
				CAN_InitStructure.CAN_Prescaler = 12;
				break;
		case  500:
				CAN_InitStructure.CAN_Prescaler = 6;
				break;
		case 1000:
				CAN_InitStructure.CAN_Prescaler = 3;
				break;
		default:
				CAN_InitStructure.CAN_Prescaler = 24;
				break;
  }
  CAN_Init(CAN1, &CAN_InitStructure);
    /* CAN filter init */
  CAN_FilterInitStructure.CAN_FilterNumber = 0;
  CAN_FilterInitStructure.CAN_FilterMode  = CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
  CAN_FilterInitStructure.CAN_FilterIdHigh = 0;
  CAN_FilterInitStructure.CAN_FilterIdLow  = 0;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow  = 0;
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
  CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
  CAN_FilterInit(&CAN_FilterInitStructure);
  CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
	
	return 0;
}



/**********************************************************************
** 函数名称：int read_com_buf(unsigned char port, unsigned char *dest, int n)
** 功能描述:
************************************************************************/

uint16_t rt_read_can_rx_buf(uint8_t ch,struct can_rx_msg *p_msg,uint32_t size)
{
  uint32_t rv = 0;
  
  if(size < 50)
    return 0;
  
	switch(ch)
	{
		case 1:
			rv = can1_msg_rx_num;
			while(can1_msg_rx_num > 0) 
			{
				front_can1_rx_buf++;
				if(front_can1_rx_buf == can_rx_buf_endof(can1_msg_rx_buf))
					front_can1_rx_buf = can1_msg_rx_buf;
				memcpy(p_msg,front_can1_rx_buf,sizeof(CanRxMsg));
				can1_msg_rx_num--;
			 }
			break;
		case 2:
			rv = can1_msg_rx_num;
			while(can1_msg_rx_num > 0) 
			{
				memcpy(p_msg,front_can1_rx_buf,sizeof(CanRxMsg));
				can1_msg_rx_num--;
				front_can1_rx_buf++;
				if(front_can1_rx_buf == can_rx_buf_endof(can1_msg_rx_buf))
					front_can1_rx_buf = can1_msg_rx_buf;
			 }
			break;
	}
  
  return rv;
}



/**********************
**	
**********************/

uint8_t rt_write_can_rx_buf(uint8_t ch,id_type type,uint32_t id, uint8_t *data,uint8_t len)
{
	CanTxMsg 			tx_msg;    				//CAN消息体发送缓冲区
	
	switch(ch)
	{
		case 1:
			
			tx_msg.StdId = 0x00;
			tx_msg.ExtId = id;
			tx_msg.RTR = CAN_RTR_DATA;
			tx_msg.IDE = CAN_ID_EXT;
			tx_msg.DLC = 8;
			tx_msg.Data[0]	= *(data + 0);
			tx_msg.Data[1]	= *(data + 1);
			tx_msg.Data[2]	= *(data + 2);
			tx_msg.Data[3]	= *(data + 3);
			tx_msg.Data[4]	= *(data + 4);
			tx_msg.Data[5]	= *(data + 5);
			tx_msg.Data[6]	= *(data + 6);
			tx_msg.Data[7]	= *(data + 7);
			CAN_Transmit(CAN1, &tx_msg);
		
			break;
		case 2:
			break;
	}
	return 0;
}


/*****************************
**	CAN1接收中断
*******************************/

void CAN1_RX0_IRQHandler(void)
{
	CanRxMsg 						rx_msg = {0};
	
	uint32_t rv = taskENTER_CRITICAL_FROM_ISR();
	
	CAN_Receive(CAN1,0,&rx_msg);
	write_can1_rx_buf(&rx_msg);
	
	taskEXIT_CRITICAL_FROM_ISR(rv);
	
}


