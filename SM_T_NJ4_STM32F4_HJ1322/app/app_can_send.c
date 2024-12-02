

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "version.h"

#include "drv_can.h"



#include "app_can_send.h"
#include "app_lte.h"
#include "app_gnss.h"
#include "app_in.h"



/************* 全局变量 ****************/

static QueueHandle_t  			can_send_queue = NULL;   //生产队列

static uint8_t 							local_buff[512] = {0};




/********************************************
**
**********************************************/

QueueHandle_t get_can_send_queue(void)
{
	return can_send_queue;
}





/****************************
**	循环发送
**	CAN-ID:
**	周期发送 50ms
****************************/

 void can_cycle_send_handler(void)
{
	static uint32_t 		cnt = 0;
	static uint8_t			flag1 = 0;
	static uint8_t			step = 0;
	static uint8_t 			heart_cnt = 0;
    
	uint8_t					buf[8];
	
	cnt++;
	if(cnt % 200 == 0)     //
	{
		memset(buf,0,8);
		if(flag1 == 0)
		{
			buf[0] = 0x55; 
			flag1 = 1;
		}			
		else
		{
			buf[0] = 0xAA;
			flag1 = 0;
		}

		buf[1] = read_in_batter_vol();    //TBOX内部电池电压值
		*(uint16_t *)&buf[2] = read_in_power_vol();    //TBOX设备外部输入电源电压值
		
		rt_write_can_rx_buf(1,EXTID, 0x18BC1058,buf,8);

		memset(buf,0,8);
		buf[0] = 0;// read_lte_net_state();      //LTE网络状态
		buf[1] = read_lte_sim_state();      //SIM卡状态
		
		
		rt_write_can_rx_buf(1,EXTID,0x18BC2058,buf,8);
		
		memset(buf,0,8);
		buf[0] =  1 ;    //定位状态
		buf[1] = 12;      //SIM卡状态
		rt_write_can_rx_buf(1,EXTID,0x18BC3058,buf,8);

		memset(buf,0,8);
		*(uint32_t *)(buf + 0) = 0;
		*(uint32_t *)(buf + 4) = 0;
		rt_write_can_rx_buf(1,EXTID, 0x18BC3158, buf,8);       //定位信息广播（经纬度）
		
		memset(buf,0,8);
		
		rt_write_can_rx_buf(1,EXTID,0x18BC3258,buf,8);

		memset(buf,0,8);
		rt_write_can_rx_buf(1,EXTID,0x18BC4058,buf,8);

		buf[0] = 0x00;									//客户定义供应商代码-低字节
		buf[1] = 0x00;									//客户定义供应商代码-高字节
		buf[2] = 0x00;									//用户代码  博创编辑的用户代码
		buf[3] = USER_NUMBER_YEAR;                      //编译时间-年
		buf[4] = USER_NUMBER_MON;			    	    //编译时间-月
		buf[5] = USER_NUMBER_DAY;				        //编译时间-日		
		buf[6] = USER_MAJOR_NUMBER;                     //用户定义主版本号
		buf[7] = USER_MINOR_NUMBER;                     //用户定义次版本号
		rt_write_can_rx_buf(1,EXTID,0x18BC5058,buf,8);
	}
	
	if(cnt % 200 == 0)  //10S周期
	{
		memset(buf,0,8);
		switch(step)
		{
			case 0:
				buf[0] = 0xE5;
				buf[1] = 0xFE;
				rt_write_can_rx_buf(1,EXTID,0x18EA0021,buf,8);
				step++;
				break;
			case 1:
				buf[0] = 0xE9;
				buf[1] = 0xFE;
				rt_write_can_rx_buf(1,EXTID,0x18EA0021,buf,8);
				step++;
				break;
			case 2:
				buf[0] = 0xD5;
				buf[1] = 0xFD;
				rt_write_can_rx_buf(1,EXTID,0x18EA0021,buf,8);
				step = 0;
				break;
		}	
	}

	if(cnt % 500 == 0)
	{
		memset(buf,0,8);
		buf[0] = heart_cnt++;
		buf[1] = 0x01;
		
		if(read_lte_net_init_state() == 0)
			buf[2] = 0x02;
		else
			buf[2] = 0x01;
		
		if(read_gnss_positing_state() == 'V')
			buf[3] = 0x02;
		else
			buf[3] = 0x01;
		rt_write_can_rx_buf(1,EXTID,0x18FF36F9,buf,8);
	}
}




/****************************
**
*****************************/

void thread_entry_can_send(void *parameter)
{
	struct can_send_mq_t cs_mq = {0};
	
	parameter = parameter;
	
	can_send_queue = xQueueCreate(1,sizeof(struct can_send_mq_t));
	vTaskDelay(100);
	
	for(;;)
	{
		vTaskDelay(1);
		if(xQueueReceive(can_send_queue,&cs_mq,0) == pdTRUE)
		{
			switch(cs_mq.cmd)
			{
				case 0:       //读取设备编号		
					//read_config_terminal_id(local_buff,16);
					//write_data_to_can(local_buff,16);
					break;
				case 1:
					break;
			}
		}
		can_cycle_send_handler();
	}
}









