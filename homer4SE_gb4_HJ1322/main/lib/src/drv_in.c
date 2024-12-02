


#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "drv_in.h"
#include "drv_gpio.h"





#define DEFAULT_VREF 1100               //Per design the ADC reference voltage is 1100 mV, however the true reference voltage can range from 1000 mV to 1200 mV amongst different ESP32s.
#define SAMPLE_NUM 50                   //Multisampling






static const adc_atten_t adc_atten = ADC_ATTEN_DB_11;
static const adc_unit_t adc_unit = ADC_UNIT_1;
static const adc_bits_width_t adc_bits_width = ADC_WIDTH_BIT_12;
static esp_adc_cal_characteristics_t *adc_char;




/********************************************************
**  ADC
*********************************************************/
uint8_t  rt_hw_init_adc(void)
{
    adc1_config_width(adc_bits_width);
    adc1_config_channel_atten(ADC_VCC, adc_atten);
    adc1_config_channel_atten(ADC_ACC, adc_atten);
    adc1_config_channel_atten(ADC_BAT, adc_atten);
    adc1_config_channel_atten(ADC_IN1, adc_atten);
	adc1_config_channel_atten(ADC_IN2, adc_atten);
    adc1_config_channel_atten(ADC_SHELL,adc_atten);

    adc_char = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    if (adc_char == NULL)
    {
        printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
        return 1;
    }
    
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(adc_unit, adc_atten, adc_bits_width, DEFAULT_VREF, adc_char);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        printf("-- Characterized using Two Point Value\r\n");
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        printf("-- Characterized using eFuse Vref\r\n");
    }
    else
    {
        printf("-- Characterized using Default Vref\r\n");
    }

    return 0;
}




/********************************************************
 * @desc: 读取
 * @param:
 * @return:
 * @date:2021/10/11
*********************************************************/
uint8_t get_adc_value(struct adc_value_t *data)
{
    struct adc_value_t adc_value = {0};
    uint32_t adc_reading[ADC_NUM] = {0};

    for (uint32_t i = 0; i < SAMPLE_NUM; i++)
    {
        adc_reading[0] += adc1_get_raw((adc1_channel_t)ADC_VCC);
        adc_reading[1] += adc1_get_raw((adc1_channel_t)ADC_ACC);
        adc_reading[2] += adc1_get_raw((adc1_channel_t)ADC_BAT);
        adc_reading[3] += adc1_get_raw((adc1_channel_t)ADC_IN1);
        adc_reading[4] += adc1_get_raw((adc1_channel_t)ADC_IN2);
        adc_reading[5] += adc1_get_raw((adc1_channel_t)ADC_SHELL);
    }

    for (uint32_t i = 0; i < ADC_NUM; i++)
    {
        adc_reading[i] /= SAMPLE_NUM;
    }

    //Convert adc_reading to voltage in mV
    if (adc_reading[0] > 0)
    {
        adc_value.vcc = esp_adc_cal_raw_to_voltage(adc_reading[0], adc_char) * 11;
    }

    if (adc_reading[1] > 0)
    {
        adc_value.acc = esp_adc_cal_raw_to_voltage(adc_reading[1], adc_char) * 11;
    }
    
    if (adc_reading[2] > 0)
    {
        adc_value.bat = esp_adc_cal_raw_to_voltage(adc_reading[2], adc_char) * 3/2;
    }

    if (adc_reading[3] > 0)
    {
        adc_value.in1 = esp_adc_cal_raw_to_voltage(adc_reading[3], adc_char) * 11;
    }
    
    if (adc_reading[4] > 0)
    {
        adc_value.in2 = esp_adc_cal_raw_to_voltage(adc_reading[4], adc_char) * 11;
    }

    if(adc_reading[5] > 0)
    {
        adc_value.shell = esp_adc_cal_raw_to_voltage(adc_reading[5], adc_char);
    }
 
    adc_value.vcc += 300;
    
    memcpy((uint8_t *)data,(uint8_t *)&adc_value,sizeof(struct adc_value_t ));

    return 0;
}
