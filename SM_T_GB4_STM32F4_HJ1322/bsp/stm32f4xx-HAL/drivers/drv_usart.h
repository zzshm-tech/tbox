/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 * 2015-08-01     xiaonong     the first version for stm32f7xx
 * 2016-01-15     ArdaFu       the first version for stm32f4xx with STM32 HAL
 */
#ifndef __USART_H__
#define __USART_H__


#define UART1_BUFF_SIZE 2176   

#define UART2_BUFF_SIZE 1024

#define UART3_BUFF_SIZE 1024

extern int hw_usart_init(void);


#endif
