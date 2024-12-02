

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

#include "board.h"
#include "drv_uart.h"
#include "drv_rtc.h"
#include "drv_gpio.h"
#include "drv_uart.h"


#include "common.h"

#include "app_lte.h"
#include "ringbuffer.h"
#include "app_at.h"




/********************  *********************/

static struct lte_info         			lte_info = {0};

static QueueHandle_t                    lte_qu = NULL; 	    // 



/**************************************
**
***************************************/

QueueHandle_t  get_lte_qu(void)
{
	return lte_qu;
}




/**************************************
**
***************************************/

void view_lte_info(void)
{
	printf("\r\n-------------------- 本地网络信息 -----------------------\r\n");   //本地ID
	printf("-- 4G模组开机状态 : %d\r\n",lte_info.lte_state);              //
    printf("-- SIM卡初始化状态 : %d\r\n",lte_info.sim_state);              //
    printf("-- SIM卡ICCID信息 : %s\r\n",lte_info.iccid);              //ICCID
	printf("-- 4G模组IMEI信息 : %s\r\n",lte_info.imei);		              //
   	printf("-- 4G网络信号值 : %d\r\n",lte_info.csq_value);              //
    printf("-- 4G网络注册状态 : %d\r\n",lte_info.net_reg_state);          //
    printf("-- GPRS网络附着状态 : %d\r\n",lte_info.net_attach_state);       //
    printf("-- 整体网络初始完成状态 : %d\r\n",lte_info.net_init_state);
	printf("-- 获取到的本地IP地址 : %d.%d.%d.%d\r\n",lte_info.local_ip[0],lte_info.local_ip[1],lte_info.local_ip[2],lte_info.local_ip[3]);   //本地ID 
	printf("----------------------------------------------------------\r\n");                        
}


/******************************************
**
*****************************************/

// uint8_t read_lte_sokcet_state(uint8_t index)
// {
// 	uint8_t rv;

// 	if(index > 4)
// 		return 0;

// 	rv = lte_info.socket_state[index];

// 	return rv;
// }



/******************************************
**
*****************************************/

uint8_t read_lte_init_state(void)
{
	uint8_t rv;

	rv = lte_info.lte_state;


	return rv;
}



/***************************
**	返回SIM卡状�?
****************************/

uint8_t read_lte_sim_state(void)
{
    uint8_t rv;

    rv =  lte_info.sim_state;

    return rv;
}




/***************************
**	固定长度20
**	返回0：读取失�?
**	返回20:ICCID固定长度
****************************/

uint8_t read_lte_icc_id(uint8_t *buf,uint8_t buf_size)
{
	int i;
	
	if(buf == NULL || buf_size < 20)
		return 0;
	
	//memcpy(&lte_info.iccid[0],"89860492192490278946",sizeof("89860492192490282477"));
	
	for(i = 0;i < 20;i++)
		*(buf + i) = lte_info.iccid[i];
	
	return 20;
}



/***************************
**	返回LTE模块IMEI�?
****************************/

uint8_t read_lte_imei_id(uint8_t *buf,uint8_t size)
{
    uint8_t rv;
	
	if(buf == NULL || size < 15)
		return 0;
	

	rv = strlen((const char *)lte_info.imei);
	//printf("-- lte info len:%d\r\n",rv);
	//memcpy(buf,"865061052263148",sizeof("865061052263148"));
	memcpy(buf,lte_info.imei,rv);

	return rv;
}



/******************************************
**
*****************************************/

uint8_t read_lte_net_reg_state(void)
{
	uint8_t rv;

	rv = lte_info.net_reg_state;

	return rv;
}



/*******************************
**	返回网络状�?
*********************************/

uint8_t read_lte_net_init_state(void)
{
	uint8_t rv;

	rv = lte_info.net_init_state;

	return rv;
}



/***************************
**	返回网络附着状�?
****************************/

uint8_t read_lte_attached_state(void)
{
	uint8_t rv;
	
	rv = lte_info.net_attach_state;
	
	return rv;
}


/***************************
**	返回LTE网络信号�?
****************************/

uint8_t read_lte_csq(void)
{
	uint8_t rv;

	rv = lte_info.csq_value;
	//rv = 28;
	return rv;
}




/***************************
** LTE关机
****************************/
uint8_t turn_off_lte_module(void) 
{
	
	vTaskDelay(10);
	rt_lte_pwrkey_high();
	vTaskDelay(150);
	rt_lte_pwrkey_low();
	memset((uint8_t *)&lte_info,0,sizeof(lte_info));
	vTaskDelay(100);   // 10S之后在进行重�?

	rt_lte_power_off();   //打开LTE模块电源
	vTaskDelay(200);   // 10S之后在进行重�?
	printf("-- turn off lte module ..... \r\n");
	return 0;
}





/***************************
**	起开LTE模块
****************************/

uint8_t  turn_on_lte_module(void)  
{
	uint8_t counter = 0;

	rt_lte_power_on();   //打开LTE模块电源
	vTaskDelay(10);
	rt_lte_pwrkey_high();
	vTaskDelay(200);
	rt_lte_pwrkey_low();

	if(at_wait_cmd_ok_syn() > 0)    //
	{
		return 1;
	}	
	else
	{
		printf("-- AT Cmd Ready...\r\n");   //调试使用
	}

	
	if(at_ctrl_echo_syn() > 0)    //关闭LTE AT命令回显
	{
		return 1;
	}
	else
	{
		printf("-- ATE Cmd OK...\r\n");
	}

	vTaskDelay(10);
	if(at_get_imei_syn(lte_info.imei,sizeof(lte_info.imei)) == 1)  //获取IMEI
	{
		return 1;
	}
	else
	{
		printf("-- The IMEI : %s\r\n",lte_info.imei);
	}

	if(at_set_cereg_syn() > 0)                   //设置CGREG
	{
		return 1;
	}
	else
	{
		printf("-- Set CGREG Cmd OK...\r\n");
	}

	if(at_get_sim_syn(&lte_info.sim_state) > 0)     //判断SIM卡状�?
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
		printf("-- Get SIM ICCID:%s\r\n\r\n",lte_info.iccid);
	}

	if(at_query_urc_port_state() > 0)
	{
		if(at_config_sms_port() > 0)
		{
			return 1;
		}
		else
		{
			printf("-- Config urc prot ok ...\r\n");
		}
	}
	else
	{
		printf("-- Config urc prot have finished ...\r\n");
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
		printf("-- Config sms event ok ...\r\n");
	}
	
	
	if(at_config_hex_recv() > 0)
	{
		return 1;
	}
	else
	{
		printf("-- Config hex recv sys format ok ... \r\n");
	}	


	counter = 0;
	while(counter++ < 3)                              //查询CSQ�?
	{
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

	if(at_get_cereg_syn(&lte_info.net_reg_state) > 0)
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
		printf("-- Set apn ok ...\r\n");
	}

	if(at_activate_pdp() > 0)
	{
		return 1;
	}
	else
	{
		printf("-- Activate pdp ok.......\r\n");
	}

	if(at_get_local_ip_syn(lte_info.local_ip) > 0)
	{
		return 1;
	}
	else
	{
		printf("-- Get local ip:%d.%d.%d.%d\r\n",lte_info.local_ip[0],lte_info.local_ip[1],lte_info.local_ip[2],lte_info.local_ip[3]);
	}

	if(at_set_ntp_server() == 0)
	{
		printf("-- Set ntp server ok .........\r\n");
	}

	if(at_get_lte_cclk(&tmp_rtc) == 0)
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
	uint16_t cnt = 0;
	struct lte_mq_t 	mq;

	parameter = parameter;

	lte_qu = xQueueCreate(2,sizeof(struct lte_mq_t));

	memset((uint8_t *)&lte_info,0,sizeof(lte_info));

	for(;;)
	{
		if(xQueueReceive(lte_qu,&mq,100) == pdTRUE)
        {
			switch(mq.cmd)
			{
				case 0:
					vTaskDelay(100);
					memset((uint8_t *)&lte_info,0,sizeof(lte_info));
					printf("-- Reset Lte Net ....... \r\n");
					break;
				case 1:
					turn_off_lte_module();
					printf("-- Close Lte Net ....... OK\r\n");
					step = 4;	
					break;
				case 2:
					if(step == 4)
					{
						step = 0;
						printf("-- Open Lte Net ....... \r\n");
					}	
					
					break;
			}
		}	
		
		switch(step)
		{
			case 0:              //LTE开		
				res = turn_on_lte_module();
				if(res == 0)
				{
					printf("-- Open Lte Model OK......\r\n");
					step++;
				}
				else
				{
					step = 3;
				}
				break;
			case 1:        		//LTE注册网络
				res = register_mobile_netwrok();
				if(res == 0)
				{
					printf("-- Register Net OK.......\r\n");
					lte_info.net_init_state = 1;
					step++;
					//step = 4;
				}
				else
				{
					step = 3;
				}
				break;
			case 2:					//  获取信号�? 、获取网络状态、获取网络附着状�?
				//vTaskDelay(100);
				if(cnt++ >= 5)
				{	
					if(at_get_csq_syn(&lte_info.csq_value) > 0)
					{
						//增加加测信号值判�?
						lte_info.csq_value = 99;	
					}

					if(at_get_cgatt_syn(&lte_info.net_attach_state) > 0)
					{
						lte_info.net_attach_state = 0;
					}

					// printf("-- the case value:%d\r\n",lte_info.csq_value);
					// printf("-- the net attach state :%d\r\n",lte_info.net_attach_state);

					if(lte_info.csq_value > 35 || lte_info.net_attach_state != 1)
					{
						memset((uint8_t *)&lte_info,0,sizeof(struct lte_info ));
						step++;
					}

					cnt = 0;   					// 获取信号状�?
				}
				break;
			case 3:                
				res = turn_off_lte_module();     //关闭LTE模块
				if(res == 0)
				{
					step = 0;                //
					vTaskDelay(2000);
					//printf("-- Close Lte module ok.....\r\n");
				}
				break;
			case 4:
				vTaskDelay(100);
				break;
			default:
				vTaskDelay(100);
				step = 0;
				break;
		}
	}
}






