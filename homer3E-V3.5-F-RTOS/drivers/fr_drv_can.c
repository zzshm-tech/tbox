

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "stm32f10x.h"

#include "data_type.h"


/*********回调函数***********/
static void (*fr_can1_hook)();


static CanRxMsg 									can1_rx_msg;		 				//CAN消息体接收缓冲区
static CanTxMsg										can1_tx_msg;

SemaphoreHandle_t 								can1_tx_sema = NULL;   //CAN发送互斥信号



/**********************************
**	初始化can接口
**	STM32F103只有一路CAN1
**	根据STM32F103实际运行主频
***********************************/

void fr_init_can1_sema(void)
{
	can1_tx_sema = xSemaphoreCreateMutex();   //创建信号量
}


/**********************************
**	初始化can接口
**	STM32F103只有一路CAN1
**	根据STM32F103实际运行主频
**	
***********************************/
void fr_can1_init(unsigned short int baudrate)
{
		GPIO_InitTypeDef				GPIO_InitStructure;
    CAN_InitTypeDef        	CAN_InitStructure;
    CAN_FilterInitTypeDef  	CAN_FilterInitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;     			//CAN休眠IO控制口
		GPIO_InitStructure.GPIO_Mode =  GPIO_Mode_Out_PP;		//
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;    //
		GPIO_Init(GPIOB, &GPIO_InitStructure);							//
		GPIO_ResetBits(GPIOB,GPIO_Pin_9);        						//
	
    //Configure CAN pin: RX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //Configure CAN pin: TX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //CAN register init
    CAN_DeInit(CAN1);

    // CAN1 cell init
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = DISABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;

    //CAN_SJW_1tq CAN_BS1_3tq CAN_BS2_2tq 	24 	// 250K*/
    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq; //
    CAN_InitStructure.CAN_BS1 = CAN_BS1_3tq; //
    CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq; //

    switch(baudrate)         //该段代码需要清理
    { 
#ifdef USE_HIS
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
        CAN_InitStructure.CAN_Prescaler = 12;
        break;
#else
    case  250:
        CAN_InitStructure.CAN_Prescaler = 24;
        break;
    case  500:
        CAN_InitStructure.CAN_Prescaler = 12;
        break;
    case 1000:
        CAN_InitStructure.CAN_Prescaler = 6;
        break;
    default:
        CAN_InitStructure.CAN_Prescaler = 24;
        break;
#endif
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
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);
    CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}


/********************************
**	CAN 接口中断服务函数
*********************************/

void fr_set_can1_rx_hook(void (*hook)(unsigned int can_id,unsigned char *data_buf))
{
	if(fr_can1_hook == NULL)  
		fr_can1_hook = hook;
}



/*************************
**	CAN发送
**************************/

void fr_can1_tx_msg(unsigned short int baudrate, struct can_tx_data_str *data)
{
	unsigned char i;
	
		xSemaphoreTake(can1_tx_sema,portMAX_DELAY );     //获取信号量
	
    can1_tx_msg.StdId = 0x00;
    can1_tx_msg.ExtId = data->id;
    can1_tx_msg.RTR = CAN_RTR_DATA;
    can1_tx_msg.IDE = CAN_ID_EXT;
    can1_tx_msg.DLC = 8;

    can1_tx_msg.Data[0]	= data->data0;
    can1_tx_msg.Data[1]	= data->data1;
    can1_tx_msg.Data[2]	= data->data2;
    can1_tx_msg.Data[3]	= data->data3;
    can1_tx_msg.Data[4]	= data->data4;
    can1_tx_msg.Data[5]	= data->data5;
    can1_tx_msg.Data[6]	= data->data6;
    can1_tx_msg.Data[7]	= data->data7;
	
		i = CAN_Transmit(CAN1, &can1_tx_msg);   //
		if(i == 4)
		{
			fr_can1_init(250);
		}
		vTaskDelay(1);     //
	
	xSemaphoreGive(can1_tx_sema);      //归还信号量
}




/********************************
**	CAN 接口中断服务函数
*********************************/

void USB_LP_CAN1_RX0_IRQHandler(void)
{
	CAN_ClearITPendingBit(CAN1,CAN_IT_FMP0);
	
	CAN_Receive(CAN1, CAN_FIFO0, &can1_rx_msg);
	
	//调用回调函数
	if(fr_can1_hook != NULL)
			fr_can1_hook(can1_rx_msg.ExtId,can1_rx_msg.Data);   //
}

