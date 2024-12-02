/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-07     aozima       the first version
 * 2018-11-16     Ernest Chen  add finsh command and update adc function
 */

#ifndef __ADC_H__
#define __ADC_H__
#include <rtthread.h>


#define ADC_NUM_OF_CONV 11


#define ADC_SWITCH_ON 1
#define ADC_SWITCH_OFF 2
#define ADC_GET_RESULT 3

struct rt_adc_device;
struct rt_adc_ops
{
//    rt_err_t (*enabled)(struct rt_adc_device *device, rt_uint32_t channel, rt_bool_t enabled);
//    rt_err_t (*convert)(struct rt_adc_device *device, rt_uint32_t channel, rt_uint32_t *value);
	rt_err_t (*rt_adc_init)(rt_device_t dev);
	rt_err_t (*rt_adc_control)(rt_device_t dev,int cmd,void *args);
};

struct rt_adc_device
{
    struct rt_device parent;
    const struct rt_adc_ops *ops;
};
typedef struct rt_adc_device *rt_adc_device_t;

typedef enum
{
    RT_ADC_CMD_ENABLE,
    RT_ADC_CMD_DISABLE,
} rt_adc_cmd_t;

//rt_err_t rt_hw_adc_register(rt_adc_device_t adc,const char *name, const struct rt_adc_ops *ops, const void *user_data);


extern rt_err_t rt_hw_adc_register(struct rt_adc_device *adc_device,const char	*name,const struct rt_adc_ops *ops,void *data);






rt_uint32_t rt_adc_read(rt_adc_device_t dev, rt_uint32_t channel);
rt_err_t rt_adc_enable(rt_adc_device_t dev, rt_uint32_t channel);
rt_err_t rt_adc_disable(rt_adc_device_t dev, rt_uint32_t channel);

#endif /* __ADC_H__ */
