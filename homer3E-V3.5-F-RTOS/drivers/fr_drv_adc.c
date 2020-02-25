


#include "stm32f10x.h"




/***************本地预定义****************/

#define ADC1_DR_Address    ((unsigned int)0x4001244C)

#define	AD_CH_NUM   3

/**************本地全局变量**************/

static unsigned short int 		AdcValueBase[AD_CH_NUM];    										//ADC采集值 存放的值


/****************************
**	函数名称：
**	功能描述：初始化系统输入
*****************************/

void fr_init_adc(void)
{
	GPIO_InitTypeDef 	GPIO_InitStructure;
  DMA_InitTypeDef 	DMA_InitStructure;
  ADC_InitTypeDef 	ADC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);        //ADC1
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);          /* Enable DMA1 clock */

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
	
    /* ADC1 configuration ------------------------------------------------------------------------------------*/
  
	DMA_DeInit(DMA1_Channel1);          //DMA 
	ADC_DeInit(ADC1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;      						//
  DMA_InitStructure.DMA_MemoryBaseAddr = (unsigned int)&AdcValueBase;					//
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;               						//
  DMA_InitStructure.DMA_BufferSize = 3;                            						//
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 						//
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;          						//
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	//
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;        	//
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;              								//
  DMA_InitStructure.DMA_Priority = DMA_Priority_Low;//DMA_Priority_High;          								//
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                 								//
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);																//
  DMA_Cmd(DMA1_Channel1, ENABLE);																							//
		
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;                 					//
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;                       					//
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;                 					//
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;					//
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;             					//
  ADC_InitStructure.ADC_NbrOfChannel = AD_CH_NUM;		                          				//
  ADC_Init(ADC1, &ADC_InitStructure);
  RCC_ADCCLKConfig(RCC_PCLK2_Div8);                                 					//  
	
  ADC_RegularChannelConfig(ADC1, ADC_Channel_11,  1, ADC_SampleTime_239Cycles5);	// PC.1  电源采集 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 	2, ADC_SampleTime_239Cycles5);	// PC.2   内部电池电压采集
  ADC_RegularChannelConfig(ADC1, ADC_Channel_16,  3, ADC_SampleTime_239Cycles5);  // 单片机内部温度传感器
  /***************ADC1 Start********************/
  ADC_DMACmd(ADC1, ENABLE);                  									//
  //ADC_TempSensorVrefintCmd(DISABLE);         							//
  ADC_Cmd(ADC1, ENABLE);	                    								//
  ADC_ResetCalibration(ADC1);                									//
  while(ADC_GetResetCalibrationStatus(ADC1));									//
  ADC_StartCalibration(ADC1);	               									//
  while(ADC_GetCalibrationStatus(ADC1));     									//
	ADC_TempSensorVrefintCmd(ENABLE);														//
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);    									//
}



/**********
**
************************/

void fr_close_adc(void)
{
	
}





unsigned short int fr_read_adc_value(unsigned char ch)
{
	switch(ch)
	{
		case 0:
			return AdcValueBase[0];
		case 1:
			return AdcValueBase[1];
		case 2:
			return AdcValueBase[2];
		default:
			return 0;
	}
}
