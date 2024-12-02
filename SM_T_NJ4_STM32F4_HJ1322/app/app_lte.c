

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "common.h"

#include "drv_gpio.h"


#include "drv_uart.h"
#include "drv_rtc.h"

#include "app_lte.h"
#include "app_at.h"




static struct lte_info         lte_info = {0};



/*******************************
**	返回网络状态
*********************************/

uint8_t read_lte_net_init_state(void)
{
	uint8_t rv;

	rv = lte_info.net_init_state;

	return rv;
}




/***************************
**	返回SIM卡状态
****************************/

uint8_t read_lte_sim_state(void)
{
    uint8_t rv;

    rv =  lte_info.sim_state;

    return rv;
}










/***************************
**	起开LTE模块
****************************/

uint8_t  turn_on_lte_module(void)  
{
	uint8_t counter = 0;

	vTaskDelay(100);
	rt_lte_power_on();   //打开LTE模块电源
	vTaskDelay(100);
	rt_lte_switch_high();
		
	if(at_wait_cmd_ok_syn() != RES_OK)    //
	{
		return 1;
	}	
	else
	{
		printf("-- AT Cmd Ready...\r\n");   //调试使用
	}

	vTaskDelay(500);
	
	if(at_ctrl_at_syn() != 0)
	{
		return 0;
	}
	else
	{
		printf("-- Test AT OK...\r\n"); 
	}
		
	if(at_ctrl_echo_syn() != 0)    //关闭LTE AT命令回显
	{
		return 1;
	}
	else
	{
		printf("-- ATE Cmd OK...\r\n");
	}
	
	vTaskDelay(10);
	if(at_get_imei_syn(lte_info.imei,sizeof(lte_info.imei)) != 0)  //获取IMEI
	{
		return 1;
	}
	else
	{
		printf("-- The IMEI : %s\r\n",lte_info.imei);
	}

	if(at_set_cgreg_syn() > 0)                   //设置CGREG
	{
		return 1;
	}
	else
	{
		printf("-- Set CGREG Cmd OK...\r\n");
	}

	if(at_get_sim_syn(&lte_info.sim_state) > 0)     //判断SIM卡状态
	{
		return 1;
	}
	else
	{
		printf("-- Get Sim State ok... %d\r\n",lte_info.sim_state);
	}

	if(at_get_ccid_syn(lte_info.iccid,sizeof(lte_info.iccid))> 0)   //获取ICCID
	{
		return 1;
	}
	else
	{
		printf("-- Get SIM ICCID:%s\r\n",lte_info.iccid);
	}
	
	if(at_config_sms_fromat() > 0)
	{
		return 1;
	}
	else
	{
		printf("-- Config sms message format ok ...\r\n");
	}

	if(at_config_sms_event() > 0)
	{
		return 1;
	}
	else
	{
		printf("-- Config sms event ok...\r\n");
	}
	
	
	counter = 0;
	while(counter < 3)                              //查询CSQ值
	{
		counter++;
		vTaskDelay(100);
		at_get_csq_syn(&lte_info.csq_value);
		//printf("-- Get CSQ Value :%d\r\n",lte_info.csq_value);
	}
	
	return 0;
}








/***************************
**	注册网络
****************************/

uint8_t	register_mobile_netwrok(void)
{	
	struct rt_tm tmp_rtc = {0};

	if(at_get_cgreg_syn(&lte_info.net_reg_state) > 0)
	{
		return 1;
	}
	else
	{
		printf("-- Lte net reg state ok......%d\r\n",lte_info.net_reg_state);
	}

	if(at_get_cgatt_syn(&lte_info.net_attach_state) > 0)
	{
		return 1;
	}
	else
	{
		printf("-- Lte net attach state ok......%d\r\n",lte_info.net_attach_state);
	}

	if(at_set_apn_syn() > 0)             //设置APN
	{
		return 1;
	}
	else
	{
		printf("-- Set apn ok.......\r\n");
	}
	
	if(at_get_local_ip_syn(lte_info.local_ip) > 0)
	{
		return 1;
	}
	else
	{
		printf("-- Get local ip:%d.%d.%d.%d\r\n",lte_info.local_ip[0],lte_info.local_ip[1],lte_info.local_ip[2],lte_info.local_ip[3]);
	}

	if(at_get_lte_cclk(&tmp_rtc) > 0)
	{
		return 1;
	}
	else
	{
		set_rtc_time(&tmp_rtc);
		printf("-- Get net time:20%d,%d,%d  %d-%d-%d\r\n",tmp_rtc.year,tmp_rtc.mon,tmp_rtc.day,tmp_rtc.hour,tmp_rtc.min,tmp_rtc.sec);
	}
    
	return 0;
}





/***************************
**	控制LTE任务
****************************/

void thread_entry_lte(void *parameter)
{	
	uint8_t res = 0;
	uint8_t step = 0;
	uint8_t cnt = 0;

	for(;;)
	{
		switch(step)
		{
			case 0:
				res = turn_on_lte_module();
				if(res == 0)
				{
					printf("-- Open Lte Model OK......\r\n");
					step++;
				}
				break;
			case 1:
				res = register_mobile_netwrok();
				if(res == 0)
				{
					printf("-- Register Net OK.......\r\n");
					lte_info.net_init_state = 1;
					step++;
				}
				else
				{
					
				}
				break;
			case 2:
				vTaskDelay(100);
				if(cnt++ >= 5)
				{	
					if(at_get_csq_syn(&lte_info.csq_value) > 0)
					{
						//增加加测信号值判断
						lte_info.csq_value = 99;	
					}

					if(at_get_cgatt_syn(&lte_info.net_attach_state) > 0)
					{
						lte_info.net_attach_state = 0;
					}

					printf("-- the case value:%d\r\n",lte_info.csq_value);
					printf("-- the net attach state :%d\r\n",lte_info.net_attach_state);

					if(lte_info.csq_value > 35 || lte_info.net_attach_state != 1)
					{
						//printf("-- runing this is...\r\n");
						memset((uint8_t *)&lte_info,0,sizeof(struct lte_info ));
						step++;
					}

					cnt = 0;   					// 获取信号状态
				}
				break;
		}
	}
}


