/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-07-29     Arda.Fu      first implementation
 */
 
 
#include <rtthread.h>
#include <board.h>

int main(void)
{
    /* user app entry */
	rt_thread_t tid;
	
	//tid = rt_thread_create("load", rt_load_data_thread_entry, RT_NULL, 1024, 19, 20);			//2048
	//if(tid != RT_NULL) rt_thread_startup(tid);
    
	return 0;
}
