


#ifndef _DRV_IN_H
#define _DRV_IN_H


#include "driver/adc.h"
#include "esp_adc_cal.h"



#define ADC_NUM     6


struct adc_value_t
{
    uint32_t acc;       /* acc电压*/
    uint32_t vcc;       /* vcc电压*/
    uint32_t bat;       /* 电池电压*/
    uint32_t in1;       /* 外部输入电压1*/
    uint32_t in2;       /* 外部输入电压2*/
    uint32_t shell;     /* 光敏输入 */
};




uint8_t  rt_hw_init_adc(void);

uint8_t get_adc_value(struct adc_value_t *data);

#endif


