

#include <rtthread.h>
#include <rtdevice.h>

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "drv_gpio.h"
#include "drv_timer.h"
#include "drv_rtc.h"

#include "common.h"

#include "app_lte.h"
#include "app_can_send.h"
#include "pro_data.h"
#include "app_shell.h"
#include "app_iap.h"
#include "app_sms.h"
#include "app_at.h"
#include "app_products.h"



/********************* 本地全局变量 ********************/

static struct lte_info        lte_info = {0};

static rt_mq_t 								lte_mq	= NULL;      //


/***********************************
**	
************************************/

rt_mq_t	get_lte_link_mq(void)     
{
	return lte_mq;
}





/***************************
**	返回网络附着状态
****************************/

uint8_t read_lte_attached_state(void)
{
	uint8_t rv;
	
	rv = lte_info.net_attach_state;  
	
	return rv;
}


/***************************
**	固定长度20
**	返回0：读取失败
**	返回20:ICCID固定长度
****************************/

uint8_t read_lte_icc_id(uint8_t *source,uint8_t buf_size)
{
	int i;
	
	if(buf_size < 20)
		return 0;

	for(i = 0;i < 20;i++)
		*(source + i) = lte_info.iccid[i];
	
	
	memcpy(source,"89860478012040602294",sizeof("89860400011740705033"));  //调试使用
	
	return 20;
}



/***************************
**	返回LTE模块IMEI号
****************************/

uint8_t read_lte_imei_id(uint8_t *buf,uint8_t buf_size)
{
	uint8_t rv;
	
	if(buf_size < 17)
		return 0;
	
	rv = rt_strlen((const char *)lte_info.imei);
	
	rt_memcpy(buf,lte_info.imei,rv);
	
	return rv;
}







/*************************
**	重新启动模块
**************************/

void reboot_lte_module(void)
{
	rt_lte_pwk_high();
	rt_thread_delay(200);
	rt_lte_pwk_low();
	rt_thread_delay(100);
	lte_info.net_attach_state = 0;
  rt_lte_power_off();
	rt_thread_delay(1000);   //关闭电源10S以上
	//rt_kprintf("-- Close LTE module......\r\n");
}



/***************************************************
**	LTE开机成功状态
*****************************************************/

uint8_t read_lte_module_state(void)
{
	uint8_t rv;
	
	rv = lte_info.lte_module_state;
	
	return rv;
}



/*******************************
**	返回网络状态
*********************************/

uint8_t read_lte_net_init_state(void)
{
	uint8_t rv;

	rv = lte_info.net_init_state;

	return rv;
}




/***************************************************
**	返回SIM卡状态
*****************************************************/
uint8_t read_lte_sim_state(void)
{
	uint8_t rv;
	
	rv = lte_info.sim_state;
	
	return rv;
}




/******************************
**	LTE信号值
*******************************/

uint8_t read_lte_csq(void)
{
	uint8_t rv;
	
	rv = lte_info.csq_value;
	
	return rv;
}




/***************************
**	起开LTE模块
****************************/

uint8_t  turn_on_lte_module(void)  
{
	uint8_t counter = 0;
	
	rt_thread_delay(10);
	rt_lte_power_on();
	rt_thread_delay(100);         //
	rt_lte_pwk_high();
	rt_thread_delay(200);
	rt_lte_pwk_low();
	
	if(at_wait_cmd_ok_syn() == 1)    //
	{
		lte_info.lte_module_state = 0;
//		set_lte_com();// 设置串口波特率为  115200
//		rt_thread_delay(10);
//		if(at_set_lte_ipr(115200) == 0)
//			rt_kprintf("-- 设置LTE模块波特率OK...\r\n");
//		if(at_save_lte_arg() == 0)
//			rt_kprintf("-- 保存LTE参数OK...\r\n");
//		rt_thread_delay(100);
//		rt_reboot_sys();
		return 1;
	}	
	else
	{
		lte_info.lte_module_state = 1;
		printf("-- AT Cmd Ready...\r\n");   //调试使用
	}

	
	if(at_ctrl_echo_syn() == 1)    //关闭LTE AT命令回显
	{
		return 1;
	}
	else
	{
		printf("-- ATE Cmd OK...\r\n");
	}

	if(at_get_imei_syn(lte_info.imei,sizeof(lte_info.imei)) == 1)  //获取IMEI
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
	
	rt_thread_delay(500);
	
	if(at_get_sim_syn(&lte_info.sim_state) > 0)     //判断SIM卡状态
	{
		return 1;
	}
	else
	{
		printf("-- Get Sim State ok... %d\r\n",lte_info.sim_state);
	}

	if(at_get_ccid_syn(lte_info.iccid,sizeof(lte_info.iccid)) == 1)   //获取ICCID
	{
		return 1;
	}
	else
	{
		printf("-- Get SIM ICCID:%s\r\n",lte_info.iccid);
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
		rt_kprintf("-- Config sms message format ok ...\r\n");
	}
	

	if(at_set_apn_syn() > 0)             //设置APN
	{
		return 1;
	}
	else
	{
		printf("-- Set apn ok ...\r\n");
	}

	counter = 0;
	lte_info.csq_value = 0;
	while(counter < 120)                              //查询CSQ值
	{
		counter++;
		rt_thread_delay(100);
		at_get_csq_syn(&lte_info.csq_value);
		rt_kprintf("-- Get CSQ Value :%d\r\n",lte_info.csq_value);
		if(lte_info.csq_value > 9 && lte_info.csq_value < 35)
			break;
		
	}
	
	rt_thread_delay(100);
	if(at_config_sms_event() > 0)
	{
		return 1;
	}
	else
	{
		printf("-- Config sms event ok ...\r\n");
	}
	
	return 0;
}



/***************************
**	关闭LTE模块
****************************/

uint8_t  turn_off_lte_module(void)  
{
	rt_lte_pwk_high();
	rt_thread_delay(200);
	rt_lte_pwk_low();
	rt_thread_delay(100);
  rt_lte_power_off();   //关闭LTE电源
	
	memset((uint8_t *)&lte_info,0,sizeof(struct lte_info));
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
		rt_kprintf("-- Lte net reg state ok......%d\r\n",lte_info.net_reg_state);
	}

	if(at_get_cgatt_syn(&lte_info.net_attach_state) > 0)
	{
		return 1;
	}
	else
	{
		rt_kprintf("-- Lte net attach state ok......%d\r\n",lte_info.net_attach_state);
	}
	
	if(at_activate_pdp() > 0)
	{
		return 1;
	}
	else
	{
		rt_kprintf("-- Activate pdp ok.......\r\n");
	}

	if(at_get_local_ip_syn(lte_info.local_ip) > 0)
	{
		return 1;
	}
	else
	{
		rt_kprintf("-- Get local ip:%d.%d.%d.%d\r\n",lte_info.local_ip[0],lte_info.local_ip[1],lte_info.local_ip[2],lte_info.local_ip[3]);
	}
	
	if(at_set_ntp_server() == 0)
	{
		
	}
	
	if(at_get_lte_cclk(&tmp_rtc) == 0)
	{
		rt_set_rtc(&tmp_rtc);
		printf("-- Get net time:20%d,%d,%d  %d-%d-%d\r\n",tmp_rtc.year,tmp_rtc.mon,tmp_rtc.day,tmp_rtc.hour,tmp_rtc.min,tmp_rtc.sec);
	}

  return 0;
}



/****************************
**	GPRS链接网关任务
**	接收其他任务发过来的队列数据
*****************************/

void thread_entry_lte(void *parameter)
{
	uint8_t 					res = 0;
	uint8_t 					step = 0;
	uint32_t 					cnt_csq = 3;
	uint32_t 					cnt_net = 0;
	struct lte_mq_t 	mq;
	
	parameter = parameter;
		
	lte_mq = rt_mq_create("lte_mq",sizeof(struct lte_mq_t),2,RT_IPC_FLAG_FIFO);    //
	
	for(;;)
  {
		if(rt_mq_recv(lte_mq ,&mq,sizeof(struct lte_mq_t),100) == RT_EOK)
		{
			switch(mq.cmd)
			{
				case 0:                               //重启网络
					rt_thread_delay(500);
					memset((uint8_t *)&lte_info,0,sizeof(lte_info));
					rt_kprintf("-- Reset Lte Model\r\n");
					step = 3;
					break;
				case 1:                               //关闭网络连接（为了测试盲区使用）
					turn_off_lte_module();
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
			case 0:
				res = turn_on_lte_module();
				if(res == 0)
				{
					rt_kprintf("-- Open Lte Model OK......\r\n");
					step++;
				}
				else
				{
					rt_kprintf("-- Open Lte Model Fail......\r\n");
					step = 3;
				}
				break;
			case 1:
				res = register_mobile_netwrok();
				if(res == 0)
				{
					lte_info.net_init_state = 1;
					step++;
					rt_kprintf("-- Register Net OK.......\r\n");
				}
				else
				{
					rt_kprintf("-- Register Net Fail.......\r\n");
					step = 3;
				}
				break;
			case 2:          //检测信号及网络附着状态
				if(read_iap_state() == 0)
				{
					if(cnt_csq++ % 5 == 0)
					{	
						if(at_get_csq_syn(&lte_info.csq_value) > 0)
						{
							//增加加测信号值判断 
							if(lte_info.csq_counter++ > 3)
							{
								lte_info.csq_counter = 0;
								lte_info.csq_value = 99;
							}
						}
					}
						//判断网络附着状态
					if(cnt_net++ % 5 == 0)
					{
						if(at_get_cgatt_syn(&lte_info.net_attach_state) > 0)
						{
							lte_info.net_attach_state = 0;
						}

						//rt_kprintf("-- the case value:%d\r\n",lte_info.csq_value);
						//rt_kprintf("-- the net attach state :%d\r\n",lte_info.net_attach_state);
					}
					if(lte_info.csq_value > 35 || lte_info.net_attach_state != 1)
					{
						memset((uint8_t *)&lte_info,0,sizeof(struct lte_info ));
						step++;
					}
					//rt_kprintf("-- the socket 1 state:%d\r\n",at_query_socket_state(1));
					//rt_kprintf("-- the socket 2 state:%d\r\n",at_query_socket_state(2));
				}
				break;
			case 3:             //
				turn_off_lte_module();    //LTE模块正常关机
				rt_thread_delay(500);
				step = 0;
				break;
			case 4:                    //可以测试盲区使用
				rt_thread_delay(100);
				//rt_kprintf("-- lte net state ....\r\n");
				break;
			default:
				step = 0;
				break;
		}
  }
}









