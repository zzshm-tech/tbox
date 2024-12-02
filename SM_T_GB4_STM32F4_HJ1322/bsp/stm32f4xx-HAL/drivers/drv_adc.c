#include "drv_adc.h"
#include "board.h"
#include <rtdevice.h>
#include <rthw.h>
#include <rtthread.h>
#include "stm32f4xx_hal_adc.h"


struct rt_adc_device 		drv_adc;

volatile struct ADC_RESULT drv_adc_result;

volatile struct ADC_RESULT drv_adc_result_ret;

struct ADC_RESULT drv_adc_buf[ADC_BUF_NUM];

DMA_HandleTypeDef 			DMA2_Handler;

ADC_HandleTypeDef 			ADC1_Handler;

const struct rt_adc_ops drv_adc_ops = 
{
	drv_adc_init,
	drv_adc_control,
};
static void Drv_ADC_DMA_Init(void)
{
	__HAL_RCC_DMA2_CLK_ENABLE();
	DMA2_Handler.Instance = DMA2_Stream0;//数据流0
	DMA2_Handler.Init.Channel = DMA_CHANNEL_0;//通道0
	DMA2_Handler.Init.Direction = DMA_PERIPH_TO_MEMORY;//数据传输方向：外设->内存
	DMA2_Handler.Init.PeriphInc = DMA_PINC_DISABLE;//外设地址不变
	DMA2_Handler.Init.MemInc = DMA_MINC_ENABLE;//内存地址递增
	DMA2_Handler.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;//数据半字16位
	DMA2_Handler.Init.MemDataAlignment = DMA_PDATAALIGN_HALFWORD;//数据半字16位
	DMA2_Handler.Init.Mode = DMA_CIRCULAR;//循环模式
	DMA2_Handler.Init.Priority = DMA_PRIORITY_MEDIUM;//DMA传输方式有关数据格式几次传输
	DMA2_Handler.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	DMA2_Handler.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
	DMA2_Handler.Init.MemBurst = DMA_MBURST_SINGLE;
	DMA2_Handler.Init.PeriphBurst = DMA_PBURST_SINGLE;
	HAL_DMA_Init(&DMA2_Handler);
	//HAL_DMA_Start_IT(&DMA2_Handler,ADC1->DR,(unsigned int)&drv_adc_result,ADC_CONVERT_NUM);
	//HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}
static void drv_adc_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
 
	HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0);
	GPIO_InitStruct.Pin = GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
}

void Drv_ADC_Init(void)
{
	ADC_ChannelConfTypeDef ADC1_ChanConf;
	Drv_ADC_DMA_Init();
	rt_memset(&ADC1_ChanConf, 0, sizeof(ADC1_ChanConf));
  __HAL_RCC_ADC1_CLK_ENABLE();
	
	drv_adc_gpio_init();
	ADC1_Handler.Instance = ADC1;
	ADC1_Handler.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;				//4分频，
	ADC1_Handler.Init.Resolution = ADC_RESOLUTION_12B;									//12位模式
	ADC1_Handler.Init.DataAlign = ADC_DATAALIGN_RIGHT;									//右对齐
	ADC1_Handler.Init.ScanConvMode = ENABLE;														//扫描模式
	ADC1_Handler.Init.EOCSelection = DISABLE;//关闭EOC中断
	ADC1_Handler.Init.ContinuousConvMode = ENABLE;//开启连续转换
	ADC1_Handler.Init.NbrOfConversion = ADC_CONVERT_NUM;//2个转换在规则序列中也就是只转换规则序列2
	ADC1_Handler.Init.DiscontinuousConvMode = DISABLE;//禁止不连续采样模式
	ADC1_Handler.Init.NbrOfDiscConversion = 0;//不连续采样通道数为0
	ADC1_Handler.Init.ExternalTrigConv = ADC_SOFTWARE_START;//软件触发
	ADC1_Handler.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;//使用软件触发
	ADC1_Handler.Init.DMAContinuousRequests = ENABLE;//DMA请求
	ADC1_Handler.DMA_Handle = &DMA2_Handler;
	
	ADC1_ChanConf.Channel = ADC_CHANNEL_10;//通道temperature
	ADC1_ChanConf.Rank = 1;//第1个序列
	ADC1_ChanConf.SamplingTime=ADC_SAMPLETIME_480CYCLES;//采样时间
	HAL_ADC_ConfigChannel(&ADC1_Handler,&ADC1_ChanConf);//通道配置
	
	ADC1_ChanConf.Channel = ADC_CHANNEL_11;//PC2 BAT
	ADC1_ChanConf.Rank = 2;//第2个序列
	ADC1_ChanConf.SamplingTime=ADC_SAMPLETIME_480CYCLES;//采样时间
	HAL_ADC_ConfigChannel(&ADC1_Handler,&ADC1_ChanConf);//通道配置
	
	ADC1_ChanConf.Channel = ADC_CHANNEL_12;//PC3 POWER
	ADC1_ChanConf.Rank = 3;//第3个序列
	ADC1_ChanConf.SamplingTime=ADC_SAMPLETIME_480CYCLES;//采样时间
	HAL_ADC_ConfigChannel(&ADC1_Handler,&ADC1_ChanConf);
	
	ADC1_ChanConf.Channel = ADC_CHANNEL_13;//PC3 POWER
	ADC1_ChanConf.Rank = 4;//第3个序列
	ADC1_ChanConf.SamplingTime=ADC_SAMPLETIME_480CYCLES;//采样时间
	HAL_ADC_ConfigChannel(&ADC1_Handler,&ADC1_ChanConf);
	
	ADC1_ChanConf.Channel = ADC_CHANNEL_14;//ANT_DET_H
	ADC1_ChanConf.Rank = 5;//第3个序列
	ADC1_ChanConf.SamplingTime=ADC_SAMPLETIME_480CYCLES;//采样时间
	HAL_ADC_ConfigChannel(&ADC1_Handler,&ADC1_ChanConf);

	ADC1_ChanConf.Channel = ADC_CHANNEL_15;//ANT_DET_L
	ADC1_ChanConf.Rank = 6;//第6个序列
	ADC1_ChanConf.SamplingTime=ADC_SAMPLETIME_480CYCLES;//采样时间
	HAL_ADC_ConfigChannel(&ADC1_Handler,&ADC1_ChanConf);
	
	ADC1_ChanConf.Channel = ADC_CHANNEL_8;//通道外壳
	ADC1_ChanConf.Rank = 7;//第8个序列
	ADC1_ChanConf.SamplingTime = ADC_SAMPLETIME_480CYCLES;//采样时间
	HAL_ADC_ConfigChannel(&ADC1_Handler,&ADC1_ChanConf);
	
	ADC1_ChanConf.Channel = ADC_CHANNEL_16;//通道temperature
	ADC1_ChanConf.Rank = 8;//第1个序列
	ADC1_ChanConf.SamplingTime=ADC_SAMPLETIME_480CYCLES;//采样时间
	HAL_ADC_ConfigChannel(&ADC1_Handler,&ADC1_ChanConf);//通道配置
	
	HAL_ADC_Init(&ADC1_Handler);//初始化
	HAL_ADC_Start_DMA(&ADC1_Handler,(unsigned int*)drv_adc_buf,ADC_DMA_NUM);
}



/***********************************
**
************************************/

static void Drv_ADC_Result(struct adc_result  *adc_result)
{
	unsigned char i = 0;
	rt_uint32_t temperature = 0;
	rt_uint32_t power = 0;
	rt_uint32_t battery = 0;
	rt_uint32_t acc_vol = 0;
	rt_uint32_t shell_vol = 0;
	rt_uint32_t board_vol = 0;
	rt_uint32_t ant_h_vol = 0;
	rt_uint32_t ant_l_vol = 0;
	
	for(i = 0; i < ADC_BUF_NUM; i++)
	{
		acc_vol += drv_adc_buf[i].ext_ai1_vol;
		board_vol += drv_adc_buf[i].ext_ai2_vol;
		battery += drv_adc_buf[i].battery;
		power += drv_adc_buf[i].power;              //电源电压
		ant_h_vol += drv_adc_buf[i].ant_h_vol;
		ant_l_vol += drv_adc_buf[i].ant_l_vol;
		shell_vol += drv_adc_buf[i].shell_vol;
		temperature += drv_adc_buf[i].temperature;
	}
	
	drv_adc_result.ext_ai1_vol = acc_vol / ADC_BUF_NUM;
	drv_adc_result.ext_ai2_vol = board_vol /ADC_BUF_NUM;
	drv_adc_result.battery = battery / ADC_BUF_NUM;
	drv_adc_result.power = power / ADC_BUF_NUM;
	drv_adc_result.shell_vol = shell_vol / ADC_BUF_NUM;
	drv_adc_result.temperature = temperature / ADC_BUF_NUM;
	drv_adc_result.ant_h_vol = ant_h_vol / ADC_BUF_NUM;
	drv_adc_result.ant_l_vol = ant_l_vol / ADC_BUF_NUM;
	
	adc_result->acc_vol = 3.3 * (drv_adc_result.ext_ai1_vol) / 4096.0 * 51 * 10 + 6.0;
	adc_result->board_vol = 3.3 * (drv_adc_result.ext_ai2_vol) / 4096.0 * 4.3 * 10;
	adc_result->battery = 3.3 * (drv_adc_result.battery) / 4096.0 * 4.3 * 10;
	adc_result->power =   3.3 * (drv_adc_result.power) / 4096.0 * 51 * 10 + 7.0;
	adc_result->ant_h_vol = drv_adc_result.ant_h_vol;
	adc_result->ant_l_vol = drv_adc_result.ant_l_vol;
	
	adc_result->shell_vol = drv_adc_result.shell_vol; 
	adc_result->temperature = (drv_adc_result.temperature * 3.3 / 4096 - 0.76) / 2.5 + 25;
}



/************************
**
*************************/

void close_adc(void)
{
	__HAL_DMA_DISABLE(&DMA2_Handler);
	CLEAR_BIT(ADC1_Handler.Instance->CR2, ADC_CR2_DMA);
	HAL_ADC_Stop(&ADC1_Handler);
	rt_memset((void*)&drv_adc_result,0,sizeof(drv_adc_result));
}




/************************
**
*************************/
rt_err_t drv_adc_control(rt_device_t dev,int cmd,void *args)
{
	struct adc_result *adc_result = (struct adc_result*)args;
	switch(cmd)
	{
		case ADC_SWITCH_ON:
		{
			rt_memset((void*)&drv_adc_result,0,sizeof(drv_adc_result));
			drv_adc_init(dev);
			break;
		}
		case ADC_SWITCH_OFF:
		{
			close_adc();
			break;
		}
		case ADC_GET_RESULT:
		{
			Drv_ADC_Result(adc_result);
//			struct adc_result *args = (struct adc_result *)adc_result;
//			args = args ;
			break;
		}
	}
	return RT_EOK;
}



rt_err_t drv_adc_init(rt_device_t dev)
{
	rt_err_t result=0;
	Drv_ADC_Init();
	return result;
}



int rt_hw_init_adc(void)
{
	//Drv_ADC_Init();
	
	return rt_hw_adc_register(&drv_adc,"adc",&drv_adc_ops,RT_NULL);
}

INIT_BOARD_EXPORT(rt_hw_init_adc);


//void DMA2_Stream0_IRQHandler(void)
//{
//	if(__HAL_DMA_GET_IT_SOURCE(&DMA2_Handler, DMA_IT_TC) != RESET)
//	{
//		
//	}
//	__HAL_DMA_CLEAR_FLAG(&DMA2_Handler, DMA_IT_TC);
//	__HAL_DMA_CLEAR_FLAG(&DMA2_Handler, DMA_IT_HT);
//	__HAL_DMA_CLEAR_FLAG(&DMA2_Handler, DMA_IT_TE);
//	__HAL_DMA_CLEAR_FLAG(&DMA2_Handler, DMA_IT_DME);
//	__HAL_DMA_CLEAR_FLAG(&DMA2_Handler, DMA_IT_FE);
//}
