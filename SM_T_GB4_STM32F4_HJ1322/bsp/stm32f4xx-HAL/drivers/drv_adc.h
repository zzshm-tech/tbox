#ifndef __DRV_ADC_H__
#define __DRV_ADC_H__
#include <rtthread.h>
#ifdef __cplusplus
 extern "C" {
#endif
#define ADC_BUF_NUM 10
#define ADC_CONVERT_NUM (8)
#define ADC_DMA_NUM (ADC_BUF_NUM * ADC_CONVERT_NUM)
#define ADC_VREF    (3.3 * 1000)



struct ADC_RESULT
{
	rt_uint16_t ext_ai1_vol;							//ANALOG_DET_CHANNEL1     1/50
	rt_uint16_t ext_ai2_vol;							//ANALOG_DET_CHANNEL2     1/50
	rt_uint16_t battery;									//ANALOG_DET_CHANNEL3     1/3.3
	rt_uint16_t power;										//ANALOG_DET_CHANNEL4     1/50
	rt_uint16_t ant_h_vol;           			//定位天线状态
	rt_uint16_t ant_l_vol;           			//定位天线状态
	rt_uint16_t shell_vol;								//外壳
	rt_uint16_t temperature;          		//CH16
	
};

struct adc_result
{
	rt_uint16_t acc_vol;							//ANALOG_DET_CHANNEL1     1/50
	rt_uint16_t board_vol;							//ANALOG_DET_CHANNEL2     1/50
	rt_uint16_t battery;									//ANALOG_DET_CHANNEL3     1/3.3
	rt_uint16_t power;										//ANALOG_DET_CHANNEL4     1/50
	rt_uint16_t ant_h_vol;           			//定位天线状态
	rt_uint16_t ant_l_vol;           			//定位天线状态
	rt_uint16_t shell_vol;								//外壳
	rt_uint16_t temperature;          		//CH16
};

rt_err_t drv_adc_init(rt_device_t dev);
extern void Drv_ADC_Init(void);
rt_err_t drv_adc_control(rt_device_t dev,int cmd,void *args);
#ifdef __cplusplus
 }
#endif
#endif /*__CAN_H__ */

