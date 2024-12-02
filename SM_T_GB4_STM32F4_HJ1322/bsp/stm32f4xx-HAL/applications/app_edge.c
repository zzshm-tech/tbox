


#include <stdio.h>
#include <string.h>

#include <rtthread.h>
#include <rtdevice.h>

#include "pro_data.h"
#include "common.h"

#include "drv_rtc.h"
#include "app_packet.h"

#include "app_gnss.h"
#include "app_lte.h"
#include "app_fifo.h"
#include "app_mon.h"
#include "app_can_recv.h"    
#include "app_shell.h"
#include "app_gb4.h"
#include "app_files.h"
#include "app_iap.h"
#include "app_edge.h"



static rt_mq_t									  edge_cmd_mq = NULL;     //RTC接收事件

static rt_event_t									edge_event = NULL;		//			

static struct edge_data_t 				edge_data = {0};      //运行数据



/***************************************
**	
***************************************/
uint32_t get_edge_data_add(uint32_t addr)
{
	uint32_t rv = 0;
	
	rv = addr - (uint32_t)&edge_data.day_total_time;
	
	return rv;
}




/********************************************
**	
********************************************/
void modify_edge_data_handle(uint32_t index,void *data, uint32_t len)
{
	rt_device_t eeprom_dev = RT_NULL;
	rt_size_t rv;
	uint32_t offset = 0;
	
	eeprom_dev = rt_device_find("at24cxx");
	rt_device_open(eeprom_dev,RT_DEVICE_OFLAG_RDWR);
	
	switch(index)
	{
		case 4:			//修改企标网关地址
			offset = get_edge_data_add((uint32_t)&edge_data.vehicle_work_time);
			rv = rt_device_write(eeprom_dev,offset + 1536,(uint8_t *)data, len);
			if(rv == len)
			{
				rt_kprintf("-- modify edge data vehicle work time OK..... \r\n");
			}
			else
			{
				rt_kprintf("-- modify edge data vehicle work time Fail..... \r\n");
			}
			break;
	case 5:			//修改偏移地址
			offset = get_edge_data_add((uint32_t)&edge_data.vehicle_offset_time);
			rv = rt_device_write(eeprom_dev,offset + 1536,(uint8_t *)data, len);
			if(rv == len)
			{
				rt_kprintf("-- modify edge data vehicle_offset_time OK..... \r\n");
			}
			else
			{
				rt_kprintf("-- modify edge data vehicle_offset_time Fail..... \r\n");
			}
			break;				
		default:
			break;
	}
	
	rt_device_close(eeprom_dev);
}




/**********************************
**	
***********************************/

static uint8_t save_edge_data(void)
{
	rt_device_t eeprom_dev = RT_NULL;
	uint16_t result = 0;
	uint8_t tmp = 0;
  eeprom_dev = rt_device_find("at24cxx");
	rt_device_open(eeprom_dev,RT_DEVICE_OFLAG_RDWR);
	
	tmp = CalcCrc8((uint8_t *)&edge_data,sizeof(edge_data) - 1);
	edge_data.crc_value = tmp;
	result = rt_device_write(eeprom_dev,1536,(uint8_t *)&edge_data,sizeof(edge_data));
	//rt_kprintf("-- Save edge data....\r\n");
	if(result == sizeof(struct edge_data_t))
	{
		rt_device_close(eeprom_dev);           //注意这个地方的代码
		return 0;
	}
	rt_device_close(eeprom_dev);
	return 1;

}



/**********************************
**	把这些数据都保存到   EEPROM内部
***********************************/

static uint8_t load_edge_data(void)
{
	rt_device_t 					eeprom_dev = RT_NULL;
	rt_err_t 							result = RT_EOK;
	uint8_t 							tmp = 0;
	
  eeprom_dev = rt_device_find("at24cxx");
	
	rt_device_open(eeprom_dev,RT_DEVICE_OFLAG_RDWR);
	
	result = rt_device_read(eeprom_dev,1536,(uint8_t *)&edge_data,sizeof(edge_data));
	
	tmp = CalcCrc8((uint8_t *)&edge_data,sizeof(edge_data) - 1);
	
	//rt_kprintf("-- Read sys_run_data info:%u,%u\r\n",result,edge_data.vehicle_work_time,edge_data.vehicle_offset_time);
	
	if(result != RT_EOK || edge_data.crc_value != tmp)
	{
		memset((uint8_t *)&edge_data,0,sizeof(edge_data));
	}
	
	rt_device_close(eeprom_dev);
	
	return 0;
}



/**************************
**	返回当天工作时间
***************************/

uint32_t read_day_total_time(void)
{
	uint32_t rv = 0;
	
	rv = edge_data.day_total_time / 360;
	
	return rv;
}



/************************
**	返回TBOX计算的工作时间
*************************/
uint32_t read_engine_total_time(void)
{
	uint32_t rv = 0;
	
	rv = edge_data.engine_total_time / 360;
	
	return rv;
}



/***********************
**	发动机工作时间
************************/

uint32_t read_vehicle_work_time(void)
{
	uint32_t rv = 0;
	
	rv = edge_data.vehicle_work_time;
	
	return rv;
}


/***************************
**	
****************************/

int32_t read_vehicle_offset_time(void)
{
	int32_t rv = 0;
	
	rv = edge_data.vehicle_offset_time;
	
	return rv;
}



int32_t read_vehicle_fuel_offset(void)
{
	int32_t rv = 0;
	
	rv = edge_data.fuel_offset;
	
	return rv;
}



/***********************
**	发动机工作时间
************************/

uint32_t read_vehicle_total_fule(void)
{
	uint32_t rv = 0;
	
	rv = edge_data.total_fuel;
	
	return rv;
}

/*************************
**	复位日工作时间
**************************/

uint8_t reset_day_total_time(void)
{
	struct rt_tm         tm;
	time_t          t_t;
	
	rt_get_rtc(&tm);
	t_t = rt_mktime(&tm);
 
	if(tm.day != edge_data.day)
	{
		if(t_t >= edge_data.unix)
		{
			edge_data.day = tm.day;
			edge_data.unix = t_t;
			edge_data.day_total_time = 0;
			
			save_edge_data();
		}
	}

	return 0;
}


/**************************
**	彻底复位ECU数据
***************************/

uint8_t reset_edge_data(void)
{
	rt_device_t eeprom_dev = RT_NULL;
	rt_uint16_t result = 0;
	
  eeprom_dev = rt_device_find("at24cxx");
	rt_device_open(eeprom_dev,RT_DEVICE_OFLAG_RDWR);
	memset((uint8_t *)&edge_data,0,sizeof(edge_data));
	result = rt_device_write(eeprom_dev,1536,(uint8_t *)&edge_data,sizeof(edge_data));

	if(result == sizeof(edge_data))
	{
		rt_device_close(eeprom_dev);           //注意这个地方的代码
		return 0;
	}
	rt_device_close(eeprom_dev);
	return 1;
}



/*******************************
**	
*******************************/

void send_edge_event(void)
{
	if(edge_event != RT_NULL)
		rt_event_send(edge_event,1);
}



/*******************************
**	
*******************************/

rt_mq_t get_edge_cmd_mq(void)
{
	return edge_cmd_mq;
}


/**********************************
**	
***********************************/

void thread_entry_edge(void *parameter)
{
	uint8_t 					acc_state = 0;
	uint8_t 					acc_back = 0;
	uint16_t 					counter = 1;
	uint32_t 					wt_work_cnt = 0;
	uint32_t 					wt_fuel_cnt = 0;
	uint32_t 					rotate = 0;
	int32_t 					tmp = 0;

	struct edge_mq_t 	edge_mq = {0};
	
	parameter = parameter;
	
	edge_event = rt_event_create("edge", 1);
	rt_irq_wkup_sethook(send_edge_event);
	 
	edge_cmd_mq = rt_mq_create("edge_cmd_mq",sizeof(struct edge_mq_t),1,RT_IPC_FLAG_FIFO); 
	
	rt_thread_delay(10);
	load_edge_data();
				
	for(;;)
	{
    if(rt_event_recv(edge_event, 1, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &rotate) != RT_EOK)
		{
			rt_kprintf("rt_event_recv failed\r\n");   //打印错误信息
			continue;  //
		}
		
		
		if(rt_mq_recv(edge_cmd_mq,&edge_mq,sizeof(edge_mq),0) == RT_EOK)
		{
			//rt_kprintf("-- the run this is .... %d\r\n",edge_mq.cmd);
			switch(edge_mq.cmd)
			{
				case 0:    //修改整车工作时间
					//rt_kprintf("-- Modelay :%d\r\n",*(uint32_t *)edge_mq.data);
					rotate = *(uint32_t *)edge_mq.data;
					if(rotate == 0)
					{
						memset((uint8_t *)&edge_data,0,sizeof(edge_data));
						save_edge_data();
					}
					else
					{
						edge_data.vehicle_offset_time = 0;
						edge_data.vehicle_work_time = rotate;
						modify_edge_data_handle(4,(uint8_t *)&rotate, 4);
						rt_thread_delay(10);
						modify_edge_data_handle(5,(uint8_t *)&rotate, 4);
					}
					break;
				case 1:
					break;
			}
		}
		
		acc_state = read_in_acc_state();    //ACC状态
		rotate = read_engine_rotate();      //发动机转速
		
		
		if(acc_state > 0)
		{
			reset_day_total_time();
			
			if(rotate > 5600 && rotate < 28000)
			{
				edge_data.engine_total_time++;    //统计发动机运行时间
				edge_data.day_total_time++;
				counter++;
			}
			if(counter % 60 == 0)
			{
				//处理 发动机工时
				if(read_engine_wt_counter() != wt_work_cnt && read_engine_work_time() != 0xFFFFFFFF)
				{
					wt_work_cnt = read_engine_wt_counter();
					
					if(edge_data.vehicle_work_time > 0 && edge_data.vehicle_work_time != 0xFFFFFFFF)
					{
						tmp = edge_data.vehicle_work_time - read_engine_work_time() - edge_data.vehicle_offset_time;
						
						if(tmp > 500 || tmp < -500)
						{
							edge_data.vehicle_offset_time += tmp;
						}
						
						rotate = read_engine_work_time() + edge_data.vehicle_offset_time;
						
						if(edge_data.vehicle_work_time <= rotate)
						{
							edge_data.vehicle_work_time = rotate;
						}
					}
					else
					{
						edge_data.vehicle_work_time = read_engine_work_time();
					}
				}
				
				//处理累计油耗
				if(read_fuel_wt_counter() != wt_fuel_cnt && read_total_fuel() != 0xFFFFFFFF)
				{
					wt_fuel_cnt = read_fuel_wt_counter();
					if(edge_data.total_fuel > 0 && edge_data.total_fuel != 0xFFFFFFFF)
					{
						tmp = edge_data.total_fuel - read_total_fuel() - edge_data.fuel_offset;
							
						if(tmp > 500 || tmp < -500)
						{
							edge_data.fuel_offset += tmp;
						}
						
						rotate = read_total_fuel() + edge_data.fuel_offset;
							
						if(edge_data.total_fuel <= rotate)
						{
							edge_data.total_fuel = rotate;
						}
					}
					else
					{
						edge_data.total_fuel = read_total_fuel();
					}
				}
			}
		}
		
		if((counter % 600 == 0) || (acc_state != acc_back && acc_back > 0))
		{
			counter = 1;
			save_edge_data();
		}
		
		acc_back = acc_state;
		
//	rt_kprintf("\r\n-- thread_entry_edge ... %u,%d \r\n",edge_data.vehicle_work_time,edge_data.vehicle_offset_time);
//	rt_kprintf("-- the total fuel:%d,%d\r\n",edge_data.total_fuel,edge_data.fuel_offset);
	}
}


