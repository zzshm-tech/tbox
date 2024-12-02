


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>


#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

#include "drv_rtc.h"

#include "common.h"

#include "app_products.h"
#include "app_gb4.h"
#include "app_in.h"
#include "app_main.h"
#include "app_can_recv.h"
#include "app_gnss.h"
#include "app_can_recv.h"
#include "app_packet.h"
#include "app_iap.h"



/***************************** 排放处理 ******************************/

static struct exhaust_data_t						exhaust_data = {0};

static struct exhaust_buf_t							exhaust_buff = {0};


static uint8_t data_buf[512];




/*****************************
** 	dpf_scr_real.scr_out_temp = 0xFFFF;							//SCR出口温度
		dpf_scr_real.scr_up_nox = 0xFFFF;				//SCR上游NOx传感器输出值
******************************/

static uint8_t exhaust_handle(void)
{
	uint16_t   i;
	
	exhaust_buff.cycle++;
	//printf("-- 统计数据周期  %d\r\n",exhaust_buff.cycle);
	if(exhaust_buff.index > 599)
		exhaust_buff.index = 599;
	
	
	if(read_friction_torque() != 0xFF && read_max_ref_torque() != 0xFFFF && read_engine_torque() != 0xFF)
	{
		if(read_engine_torque() - read_friction_torque() >= 0)
			exhaust_buff.array[0][exhaust_buff.index] = 3.14 * (read_engine_torque() - read_friction_torque()) / 10.0 * read_max_ref_torque() * 0.05 *  read_engine_rotate() * 0.125 / 30000.0;				// 发动机平均功率
		else
			exhaust_buff.array[0][exhaust_buff.index] = 0;
	}
	else
	{
		exhaust_buff.array[0][exhaust_buff.index] = 0xFFFF;
	}
	
	exhaust_buff.array[1][exhaust_buff.index] = 5;				// SCR上游NOx平均浓度   //无效值
	
	exhaust_buff.array[2][exhaust_buff.index] = 5;  //read_scr_downstream_nox(); 				// SCR下游NOx平均浓度
	
	exhaust_buff.array[3][exhaust_buff.index] = 5;				// SCR上游NOx平均质量流量   //无效值
	
	
	// if(read_enter_volume() != 0xFFFF && read_scr_downstream_nox() != 0xFFFF)
	// {
	// 	exhaust_buff.array[4][exhaust_buff.index] = 0.001587 * (read_enter_volume() / 3600.0 + 0.84 * read_engine_fuel_flow() / 3600.0) * read_scr_downstream_nox();	// SCR下游NOx平均质量流量
	// }
	// else
	// {
	// 	exhaust_buff.array[4][exhaust_buff.index] = 0xFFFF;
	// }

	exhaust_buff.array[4][exhaust_buff.index] = 5;
	
	exhaust_buff.array[5][exhaust_buff.index] = 5;  //0xFFFF;  //read_scr_entrance_temp();				// SCR入口平均温度
	
	exhaust_buff.array[6][exhaust_buff.index] = 5;		  	// SCR出口平均温度	  //无效值
	
	exhaust_buff.array[7][exhaust_buff.index] = read_engine_fuel_flow();		  	// 发动机燃料流量平均值	
	
	for(i = 0;i < 8;i++)
	{
		if(exhaust_buff.array[i][exhaust_buff.index] == 0xFFFF)
				return 0;
	}
	
	exhaust_buff.index++;   //有效数据周期
	//printf("-- 有效数据周期  %d\r\n",exhaust_buff.index);
	
	return 0;
}



/*****************************
**
******************************/

uint8_t calculate_exhaust_data(void)
{
	uint16_t i;
	uint32_t 							m_tmp[8];
	
	
	memset((uint8_t *)m_tmp,0,sizeof(m_tmp));
	
	if(exhaust_buff.index == 0)
		return 0;
	
	for(i = 0;i < exhaust_buff.index;i++)
	{
		m_tmp[0]  += exhaust_buff.array[0][i];   		  // 发动机平均功率			
		m_tmp[1]  += exhaust_buff.array[1][i];					// SCR上游NOx平均浓度   //无效值
		m_tmp[2]  += exhaust_buff.array[2][i];				// SCR下游NOx平均浓度
		m_tmp[3]  += exhaust_buff.array[3][i];					// SCR上游NOx平均质量流量
		m_tmp[4]  += exhaust_buff.array[4][i];     	// SCR下游NOx平均质量流量
		m_tmp[5]  += exhaust_buff.array[5][i];					// SCR入口平均温度
		m_tmp[6]  += exhaust_buff.array[6][i];  			// SCR出口平均温度
		m_tmp[7]  += exhaust_buff.array[7][i];						// 发动机燃料流量平均值
	}
	
	exhaust_data.engine_power = m_tmp[0] / (exhaust_buff.index);   		  // 发动机平均功率			
	
	exhaust_data.scr_up_nox = 0xFFFF;					// SCR上游NOx平均浓度   //无效值
	exhaust_data.scr_down_nox = 0xFFFF;  //m_tmp[2] / (exhaust_buff.index);				// SCR下游NOx平均浓度
	exhaust_data.scr_up_flow = 0xFFFF;					// SCR上游NOx平均质量流量
	exhaust_data.scr_down_flow = 0xFFFF;  //m_tmp[4] / (exhaust_buff.index) / 0.04;     	// SCR下游NOx平均质量流量
	exhaust_data.src_in_temp = 0xFFFF;  //m_tmp[5] / (exhaust_buff.index);					// SCR入口平均温度
	exhaust_data.src_out_temp = 0xFFFF;  			// SCR出口平均温度
	exhaust_data.fuel_flow = m_tmp[7] / (exhaust_buff.index);						// 发动机燃料流量平均值
	
	//printf("-- 统计周期 (1) %d,  %d\r\n",exhaust_data.index,exhaust_buff.cycle);
	exhaust_data.cycle_pwm = exhaust_buff.index * 1.0 / exhaust_buff.cycle * 100 + 125;
	exhaust_data.index = exhaust_buff.cycle * 100;	

	//printf("-- 统计周期 (2) %d\r\n",exhaust_data.cycle_pwm);
	//可以从这里打印出来
	
	memset((uint8_t *)&exhaust_buff,0,sizeof(exhaust_buff));
	
	return 0;
}





/*****************************
**
******************************/

uint8_t read_exhaust_data(struct exhaust_data_t *source)
{
	
	if(source == NULL)
		return 1;
	
	memcpy((uint8_t *)source,(uint8_t *)&exhaust_data,sizeof(exhaust_data));  //更新平均数据缓冲区

	return 0;
}





/****************************
 ** 处理秒任务
 ****************************/

void thread_entry_data_packet(void *parameter)
{
    uint32_t                now_tt = 0;    //当前时间戳
    uint32_t                tmp = 0;
    uint32_t                thread_cnt = 0;
    uint32_t 				gb4_cnt = 0;
    uint8_t                 acc_now = 0;     //当前的ACC状态
    uint8_t                 gb4_step = 0;
	
	uint32_to_byte 			back = {0};
	uint32_to_byte  		data = {0};
    uint16_t                recved = 0;


	uint32_t 				alarm_back = 0;

	

    parameter = parameter;

	vTaskDelay(500);            //5s之后再开始组报文
	
    for (;;)
    {
        vTaskDelay(10);                      //100ms执行

        tmp = get_rtc_timestamp();

        if(now_tt == tmp)
		{
			continue; 
		}

        now_tt = tmp;

        thread_cnt++;
        acc_now = (read_in_acc_state() || (read_sys_run_state() > 0));
		
		 //ACC开启或者关闭
		data.byte[0] = acc_now;     					//ACC状态
		data.byte[1] = read_ecu_lock_state();   		//锁车状态
		data.byte[2] = read_ecu_mon_state();			//锁车监控状态

		if(thread_cnt % read_config_qb4_upload_cycle() == 0 || (data.value != back.value))	//企标数据
		{	
			memset(data_buf,0,sizeof(data_buf));
			recved = build_qb4_platform_data(data_buf,sizeof(data_buf));
				
			if(recved > 0)
			{
				struct rt_tm tm;
				get_rtc_time(&tm);
				printf("-- Build QB4 Data :20%d,%d,%d %d-%d-%d   %d\r\n",tm.year,tm.mon,tm.day,tm.hour,tm.min,tm.sec,recved);
				write_qb4_fifo_buff(data_buf,recved);
			}		
		}

		back.value = data.value;

		//acc_now = 0;
		//printf("-- packse ....this : %d,%x,%d\r\n",gb4_step,read_config_vin_state(),dt_plat_state);
        switch(gb4_step)
		{
			case 0:                    //
				if(acc_now == 0 || read_config_vin_state() != 0x5A)
				//if(acc_now == 0)
					break;
				gb4_step++;
				memset((uint8_t *)&exhaust_buff,0x00,sizeof(exhaust_buff));
				memset((uint8_t *)&exhaust_data,0xFF,sizeof(exhaust_data));
				gb4_cnt = 0;
				break;
			case 1:
				tmp = read_dismantle_state();
				if(tmp != alarm_back)
				{
					alarm_back = tmp;
						
					recved = vehicle_dismantle_alarm(data_buf,sizeof(data_buf));
					if(recved > 0)
					{
						printf("-- vehicle dismantle alarm:%d,%d\r\n",tmp,recved);
						write_gb4_fifo_buff(data_buf,recved);
						//mem_printf(LOG_ERROR, PRINT_HEX,data_buf,recved);
					}
				}	
				//printf("-- build gb4 cycle:%d,%d\r\n",gb4_cnt,read_config_gb4_upload_cycle());

				exhaust_handle();
				if((++gb4_cnt % read_config_gb4_upload_cycle() == 0 && read_iap_state() == 0) || acc_now == 0)
				{
					calculate_exhaust_data();
					memset(data_buf,0,sizeof(data_buf));
					recved = build_gb4_platform_data(data_buf,sizeof(data_buf));   //实时数据
					if(recved > 0)
					{
						struct rt_tm tm;

						get_rtc_time(&tm);
						printf("-- Build GB4 Data ACC ON:20%d,%d,%d %d-%d-%d   %d\r\n",tm.year,tm.mon,tm.day,tm.hour,tm.min,tm.sec,recved);
						//mem_printf(LOG_ERROR, PRINT_HEX,data_buf,recved);
						write_gb4_fifo_buff(data_buf,recved);   //把数据放入缓冲区  GB4
					}
				}
				if(acc_now == 0)
				{
					gb4_step = 0;
				}
				break;
			default:
				gb4_step = 0;
				break;
		}
    }
}
    



