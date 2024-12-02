



#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_task_wdt.h"

#include "pro_data.h"
#include "common.h"
#include "ringbuffer.h"

#include "drv_can.h"

#include "app_can_recv.h"
#include "app_can_send.h"
#include "app_products.h"
#include "app_main.h"




#define FILTER_CODE_NUM 		63



/********************************** 
** 定义过滤故障码
***********************************/

static const struct dm1_filter_str	ecu_dm1_filter[FILTER_CODE_NUM] =       //DM1 需要过滤故障码
 {
	{3,     110},       //0 水温传感器开路     1
    {4,     110},       //1 水温传感器短路     2
    {2,     519097},    //2 尿素品质传感器CAN线开路（尿素浓度报文超时）     3 
    {11,	7351},      //3 下游氮氧传感器CAN线开路（下游氮氧浓度报文超时）    4 
    {2,     7354},      //4 尿素品质传感器CAN线开路（尿素温度报文超时）     5
    {2,     4766},      //5 DPF上游温度传感器信号不可信           6
    {3,     171},       //6 环境温度传感器开路               7
    {4,     171},       //7 环境温度传感器短路               8
    {17,	3233},      //8 下游氮氧传感器未在规定时间内加热到设定值    9
    {17,	3235},      //9 下游氮氧浓度不准                10
    {5,     3226},      //10 下游氮氧传感器开路             11
    {7,     3226},      //11 下游氮氧传感器短路故障（传感器内部电路）
    {6,     3226},      //12 下游氮氧传感器供电电源故障（传感器内部电路）
    {0,     5245},      //13 非加热系统不能供给尿素严重驾驶限制故障
    {3,     108},       //14 环境压力过高
    {4,     108},       //15 环境压力过低
    {11,	4795},      //16 DPF 移除
    {15,	4781},      //17 DPF 过载Level1
    {20,	3251},      //18 DPF 压差传感器信号不可信
    {0,     3251},      //19 DPF压差传感器开路
    {1,     3251},      //20 DPF压差传感器短路
    {21,	4364},      //21 SCR效率诊断2
    {16,	518150},    //22 尿素回流管压力过高
    {2,     518150},    //23 尿素压力管压力过高
    {0,     518150},    //24 尿素压力不稳定
    {16,	518153},    //25 尿素在喷射过程中压力过高
    {18,	518153},    //26 喷射过程中尿素压力过低
    {15,	518153},    //27 喷射过程中尿素压力过高
    {11,	518155},    //28 尿素泵建压失败
    {1,     4360},      //29 后处理上游（SCR）排气温度偏低
    {2,     4360},      //30 后处理（SCR）上游排气温度过高
    {11,	4360},      //31 冷启动前后处理上游（SCR）排气温度传感器偏差大
    {0,     5394},      //32 尿素喷嘴的针阀被卡住
    {16,	3364},      //33 尿素浓度偏低
    {3,     3242},      //34 DPF上游温度传感器开路
    {4,     3242},      //35 DPF上游温度传感器短路
    {3,     4360},      //36 后处理上游温度（SCR）传感器开路
    {4,     4360},      //37 后处理上游温度（SCR）传感器短路
    {14,	5425},      //38 尿素箱空
    {18,	5245},      //39 尿素箱液位低于5%
    {5,     5394},      //40 尿素喷嘴开路
    {3,     5394},      //41 尿素喷嘴对电源短路
    {4,     5394},      //42 尿素喷嘴对地短路
    {2,     5394},      //43 尿素喷嘴高低边短路
    {11,	519700},    //44 后处理加热系统部件故障导致加热系统强制关闭清空
    {22,	4375},      //45 尿素泵无法驱动
    {5,     4375},      //46 尿素泵开路
    {3,     4375},      //47 尿素泵对电源短路
    {4,     4375},      //48 尿素泵对地短路
    {15,	1387},      //49 尿素泵压力比环境压力差值高50百帕
    {17,	1387},      //50 尿素泵压力比环境压力差值低500百帕
    {3,     1387},      //51 尿素泵压力传感器开路
    {4,     1387},      //52 尿素泵压力传感器短路
    {5,     4376},      //53 反向阀继电器开路
    {3,     4376},      //54 反向阀继电器对电源短路
    {4,     4376},      //55 反向阀继电器对地短路
	{1,		5245},      //56 尿素箱液位低于10%
	{2,		5245},		//57 非加热尿素系统不能供给尿素
	{1,		5764},      //58 EGR阀电机开路故障（HBrgOpnLd）
    {1,		5831},      //59 EGR阀卡滞（JamVlvClsd）
    {0,		5831},      //60 EGR阀卡滞（JamVlvOpn）
    {3,		5263},      //61 EGR阀传感器开路或短路
    {4,		5263}      	//62 EGR阀传感器短路或开路
 };



/********************** 本地全局变量 *********************/

static struct can_msg_str 				can_recvmsg = {0};		//CAN数据接收缓冲区

static struct md1                       dm1 = {0};              //DM故障数据(包括发动机，整车系统，变速箱，仪表)

static struct ecu_data_str				ecu_data = {0};         //发动机ECU数据

static struct gb27145_dm_str    		gb27145_dm = {0};		//用作国标故障码(27145传送）		

/********************* 休眠保持数据 ******************/

static RTC_DATA_ATTR struct back_data_t 						back_data = {0};

static RTC_DATA_ATTR struct gb1939_dm_str						ecu_dm1_qb4 = {0};	 	//DM1 接收到的故障码 把总线上的 所有DM1 全部解析出来，

static RTC_DATA_ATTR struct gb1939_dm_str						ecu_dm1_gb4 = {0};	 	//DM1 接收到的故障码 把总线上的 所有DM1 全部解析出来，



/********************* CAN  转 COM口 *******************************/

static struct rt_ringbuffer             cc_rb = {0};

static uint8_t							local_buff[512];






/******************************
**	发动机燃料流量
*******************************/

uint16_t read_engine_fuel_flow(void)
{
	uint16_t rv = 0;
	
	rv = ecu_data.engine_fuel_flow;
	
	return rv;
}







/**************************
**
****************************/

uint8_t read_friction_torque(void)
{
	uint8_t rv = 0xFF;
	
	if(ecu_data.friction_torque >= 125)
		rv = ecu_data.friction_torque - 125;
	//rv = 9;
	return rv;
}




/**************************
**
****************************/

uint8_t read_engine_torque(void)
{
	uint8_t rv = 0xFF;
	
	if(ecu_data.engine_torque >= 125)
		rv = ecu_data.engine_torque - 125;
	//rv  =19;
	return rv;
}




/****************************
**	发动机转速
*****************************/

uint16_t read_engine_rotate(void)
{
	uint16_t rv;
	
	rv = ecu_data.engine_rotate;
	//rv = 750;
	return rv;
}






/*****************************
**
******************************/

uint16_t  read_max_ref_torque(void)
{
	uint16_t rv = 0;
	
	rv = ecu_data.max_ref_torque;
	//rv = 10000;
	return rv;
}






/*******************************
**	返回发动机工作时间（发动机的累计工作）
********************************/

uint32_t read_engine_work_time(void)
{
	uint32_t rv = 0;

	if(back_data.flag == 0x5AA55AA5)
		rv = back_data.engine_work_time;
	else
		rv = 0xFFFFFFFF;

	return rv;
}


/*******************************
**	返回累计油耗
********************************/

uint32_t read_total_fuel(void)
{
	uint32_t rv = 0;

	if(back_data.flag == 0x5AA55AA5)
		rv = back_data.total_fuel;
	else
		rv = 0xFFFFFFFF;
	//rv = 12654;
	return rv;
}


/***********************
**	潍柴发动机实际锁车状态
************************/

static void analysis_wc_lock_state(void)
{
	uint8_t i = 0;
	
	for(i = 0;i < DM1_1939_NUM;i++)
	{
		if(ecu_dm1_qb4.spn[i] == 7728)   //国四锁车
		{
			back_data.wc_lock_bank = ecu_dm1_qb4.fmi[i] + 1;
			return;
		}
		
		if(ecu_dm1_qb4.spn[i] == 522014) //国三锁车
		{
			back_data.wc_lock_bank = ecu_dm1_qb4.fmi[i];
			return;
		}
	}


}

/******************************
**	函数名称:
**	功能描述:
*******************************/
static uint8_t dm1_can_id(unsigned char id)
{
	 if(id == 0x00) 
		 return 0;
	 if(id == 0xf0) 
		 return 1;
	 if(id == 0x03) 
		 return 2;
	 if(id == 0x17) 
		 return 3;
		
	 return 0xff;
}



/*****************************
**	调试使用
******************************/

void view_gb27145_dm(void)
{
	uint8_t i;

	printf("\r\n************ GB27145环保故障码 ***********\r\n");
	printf("-- 环保故障灯状态：%d\r\n",gb27145_dm.light_state);
	printf("-- 环保故障码数量：%d\r\n",gb27145_dm.num);

	for(i = 0;i < gb27145_dm.num;i++)
	{
		printf("-- 环保故障码【%d】 P%04X-%02X,    %d\r\n",i,gb27145_dm.dtco[i],gb27145_dm.ftp[i],gb27145_dm.status[i]);
	}

	printf("\r\n**************************************\r\n");
	
}


/*******************************************************
**	
*******************************************************/

uint16_t  read_gb27145_dm_serial(uint8_t *buf,uint16_t size)
{
	uint16_t len = 0;
	uint16_t i = 0;

	if(buf == NULL)
		return 0;
	
	if(gb27145_dm.num == 0)
		return 0;
	
	for(i = 0;i < gb27145_dm.num;i++)        //故障码不能超过20
	{
		//*(buf + len++) = gb27145_dm.cls[i];   //故障等级
		*((uint16_t *)(buf + len)) = swap_uint16_t(gb27145_dm.dtco[i]);
		len += 2;
		*(buf + len++) = gb27145_dm.ftp[i];
		*(buf + len++) = gb27145_dm.status[i];
	}

	//printf("-- read 27145 dfc...\r\n");
	//mem_printf(LOG_ERROR,PRINT_HEX,buf,len);

	return len;
}






/*******************************************************
**	OBD  所有故障
*******************************************************/

uint16_t  read_qb4_dm_serial(uint8_t *buf,uint16_t size)
{
	uint16_t len = 0;

	if(buf == NULL || size < 160)
		return 0;
	
	if(ecu_dm1_qb4.num == 0 || ecu_dm1_qb4.num > DM1_1939_NUM)
		return 0;
	
	len = ecu_dm1_qb4.num * 4;

	memcpy(buf,ecu_dm1_qb4.serial_code,len);

	//printf("-- read 27145 dfc...\r\n");
	return len;
}




/*************************
**	解析最大参考扭矩
**************************/

void analysis_reference_torque(uint32_t can_id,uint8_t *data)
{
	static uint8_t 		cnt = 0;
	static uint8_t 		step = 0;
	
	cnt++;
	if(cnt > 10)
	{
		step = 0;
		cnt = 0;
	}
	
	switch(step)
	{
		case 0:
			if(can_id == 0x18ECFF00 && *(data + 5) == 0xE3 && *(data + 6) == 0xFE && *(data + 7) == 0x00)
			{
				cnt = 0;
				step++;
				//rt_kprintf("-- the (1) \r\n");
			}
			break;
		case 1:
			//rt_kprintf("-- the (2) %d\r\n",*(data + 0));
			if(can_id == 0x18EBFF00 && *(data + 0) == 3)
			{
				
				ecu_data.max_ref_torque = *(uint16_t *)(data + 6);
				if(ecu_data.max_ref_torque != 0xFFFF)
				{
					ecu_data.max_ref_torque  *= 20;
				}
				step = 0;
			}
			
			break;
	}
}




/**********************************
**
***********************************/

uint8_t read_gb27145_light_state(void)
{
	uint8_t rv;

	rv = gb27145_dm.light_state;

	return rv;
}


/*******************************************************
**	OBD
*******************************************************/

uint16_t  read_gb4_dm_serial(uint8_t *buf,uint16_t size)
{
	uint16_t len = 0;

	if(buf == NULL || size < 160)
		return 0;
	
	if(ecu_dm1_gb4.num == 0 || ecu_dm1_gb4.num > DM1_1939_NUM)
		return 0;
	
	len = ecu_dm1_gb4.num * 4;

	memcpy(buf,ecu_dm1_gb4.serial_code,len);

	//printf("-- read 27145 dfc...\r\n");
	return len;
}



/********************************
**		
*********************************/

uint8_t check_fault_code(struct dm1_filter_str arg)
{
    int i;

    for(i = 0;i < FILTER_CODE_NUM;i++)
    {
       if(arg.fmi == ecu_dm1_filter[i].fmi && arg.spn == ecu_dm1_filter[i].spn)
			return 1;
    }

    return 0;
}


/********************************
**	解析1939故障码
*********************************/
uint8_t analysis_gb1939_dmc(uint32_t can_id,uint8_t *data)
{
	uint8_t   	    	dm1_msg_index;				//多包消息起始位置
	uint8_t   	    	dm1_id = 0;
	uint32_t  			dm1_id_cache = 0;
	uint32_to_byte 		m_tmp;
	uint32_t    		i = 0;

	dm1_id = dm1_can_id(can_id & 0x000000FF);               					//关联相应的数组
	if(dm1_id > 0) 
		return 1;
	dm1_id_cache = can_id >> 8;
	if(dm1_id_cache == 0)
		return 1;
		
	//printf("-- the SPN FMI :%x\r\n",dm1_id_cache);
	if(dm1_id_cache  == 0x18FECA)                     				   								//DM1-单个故障 
	{
		if(*(uint32_t *)(data + 2) != 0)                            //故障指示灯都为0时认为故障消失
		{
        	memcpy(&dm1.data[dm1_id][0], (data + 2), 4);     			//存储数据，单个故障码时
			dm1.data_len[dm1_id] = 4;                                   		//单个故障时，数据长度固定为4
		}
		else
		{
			dm1.bam_flag[dm1_id] = 0;					                                //收到相应数据单包，多包标志清零
			dm1.data_len[dm1_id] = 0;
			memset((uint8_t *)data,0,8);
			
		}
	} 
	else if(dm1_id_cache == 0x18ECFF)													//DM1-多包消息声明信息
	{
		if(*data != 0x20)											//固定控制字节 声明数据首字节0x20
			return 1;
		if((*(data + 5) != 0xCA) || (*(data + 6) != 0xFE) || (*(data + 7) != 0x00))           //不是DM1故障码
		{
			dm1.bam_flag[dm1_id] = 0;
			dm1.current_index[dm1_id] = 0;
			return 1;
		}
		dm1.current_index[dm1_id] = 1;
		dm1.bam_flag[dm1_id] = 1;                                          //相应标志置一，以便于接收相应的多包数据包
		dm1.msg_num[dm1_id] = *(data + 3);
		dm1.data_len[dm1_id] = (*(data + 1) - 2) / 4 * 4;            //本控制单元故障总字节数，	
		if(dm1.data_len[dm1_id] > DM1_1939_NUM * 4)
			dm1.data_len[dm1_id] = DM1_1939_NUM * 4;

		//printf("-- the dm1 len:%d\r\n",dm1.data_len[dm1_id]);
		memset(data,0,8);
	}
	else if((dm1_id_cache == 0x18EBFF) && (dm1.bam_flag[dm1_id] == 1))          //已经收到多包声明数据
	{
		if(dm1.current_index[dm1_id] > ((DM1_1939_NUM * 4 - 5) / 7 + 1) + (((DM1_1939_NUM * 4 - 5) % 7) > 0 ? 1 : 0))
			return 1;

		if((dm1.current_index[dm1_id] != *data) ||(dm1.current_index[dm1_id] > dm1.msg_num[dm1_id]))
		{
			dm1.bam_flag[dm1_id] = 0;
			dm1.current_index[dm1_id] = 0;
			memset(data,0,8);
			return 1;
		}

		if(dm1.current_index[dm1_id] == 1)               //接收首包数据
		{
			memcpy(&dm1.data[dm1_id][0],(data + 3), 5);
		}
		else                                         //接收其余数据
		{
			dm1_msg_index =  (dm1.current_index[dm1_id] - 2) * 7 + 5;
			memcpy(&dm1.data[dm1_id][dm1_msg_index], data + 1, 7);
		}
		
		dm1.current_index[dm1_id]++;
		memset((uint8_t *)data,0,0);
	}
					
	dm1_id = 0;
					
	//printf("-- the run is......%d\r\n",dm1.data_len[0]);
	memset((uint8_t *)&ecu_dm1_qb4,0,sizeof(struct gb1939_dm_str));
	ecu_dm1_qb4.num  = dm1.data_len[0] / 4;

	memset((uint8_t *)&ecu_dm1_gb4,0,sizeof(struct gb1939_dm_str));

	for(i = 0; i < dm1.data_len[0] && dm1_id < DM1_1939_NUM;)
	{
		struct dm1_filter_str fc;
		uint32_t tmp = 0;

		m_tmp.byte[0] = dm1.data[0][i + 0];
		m_tmp.byte[1] = dm1.data[0][i + 1];
		m_tmp.byte[2] = (dm1.data[0][i + 2] >> 5) & 0x07;
		m_tmp.byte[3] = 0;

		ecu_dm1_qb4.spn[dm1_id] = m_tmp.value;
						
		ecu_dm1_qb4.fmi[dm1_id] = (dm1.data[0][i + 2] & 0x1F);  
		

		tmp = *(uint32_t *)&dm1.data[0][i + 0];
		tmp &= 0x00FFFFFF;
		memcpy((uint8_t *)&ecu_dm1_qb4.serial_code + i,(uint8_t *)&tmp,4);

		fc.spn = ecu_dm1_qb4.spn[dm1_id];
		fc.fmi = ecu_dm1_qb4.fmi[dm1_id];

		if(check_fault_code(fc) > 0)
		{
			ecu_dm1_gb4.spn[ecu_dm1_gb4.num] = fc.spn;
			ecu_dm1_gb4.fmi[ecu_dm1_gb4.num] = fc.fmi;
			tmp = *(uint32_t *)&dm1.data[0][i + 0];
			tmp &= 0x00FFFFFF;
			memcpy((uint8_t *)&ecu_dm1_gb4.serial_code + ecu_dm1_gb4.num * 4,(uint8_t *)&tmp,4);
			ecu_dm1_gb4.num++;
		}

		i += 4;
		dm1_id++;
	}

	return 0;
}


/********************************
**	解析27145故障码
*********************************/

uint8_t analysis_gb27145_dm(uint8_t *data)
{
	uint8_t i = 0;

	///mem_printf(LOG_ERROR, PRINT_HEX,data,8);
	if((*(data + 1) == 0x62) && (*(uint16_t *)(data + 2) == 0x01F4))
	{
		gb27145_dm.light_state = (*(data + 4) >> 7) & 0x01;

		if(gb27145_dm.light_state == 0)
		{
			memset((uint8_t *)&gb27145_dm,0,sizeof(gb27145_dm));
		}
		else
		{
			struct can_send_mq_t tmp_mq;
			QueueHandle_t t_q = NULL;

			tmp_mq.cmd = 2;
			tmp_mq.len = 0;
			t_q = get_can_send_queue();
        	if(t_q != NULL)
            	xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));
			gb27145_dm.num = *(data + 4) & 0x3F;
		}
	}
	else if(gb27145_dm.num == ((*(data + 1) - 6) / 5) && (*(data + 2) == 0x59))
	{
			struct can_send_mq_t tmp_mq;
			QueueHandle_t t_q = NULL;

			tmp_mq.cmd = 3;
			tmp_mq.len = 0;
			t_q = get_can_send_queue();
        	if(t_q != NULL)
            	xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));
	}	
	else 
	{
		i = (*data) - 0x21;

		if(i <= 8)
			memcpy(&gb27145_dm.serial[i * 7],data + 1,7);
	}

	for(i = 0;i < gb27145_dm.num;i++)
	{
		gb27145_dm.cls[i] = gb27145_dm.serial[i * 5];      	//故障等级
		gb27145_dm.dtco[i] = swap_uint16_t(*(uint16_t *)&gb27145_dm.serial[i * 5 + 1]);         	//暂时定义成 SPN 
		gb27145_dm.ftp[i] = gb27145_dm.serial[i * 5 + 3];         	//暂时定义成FMI
		gb27145_dm.status[i] = gb27145_dm.serial[i * 5 + 4] & 0x01;
		//gb27145_dm.serial[100];        //
	}
		
	return 0;
}


/********************************
**	返回CAN链接状态
*********************************/

uint8_t read_can_connect_state(void)
{
	uint8_t rv;

	rv = can_recvmsg.can_status;
	//printf("-- can connect state:%d\r\n",rv);

	return rv;
}



/*********************************
 **	
 *********************************/

uint8_t read_ecu_mon_state(void)
{
	uint8_t rv  = 0;

	if(read_ecu_manu_type() == 4)
		rv = get_yuc_mon_state();    //玉柴锁车装填
	else
		rv = back_data.mon_status;   //潍柴锁车状态
	
	return rv;
}



/******************************************************
**
*******************************************************/

uint8_t read_ecu_lock_state(void)
{
	uint8_t rv = 0;

	if(read_ecu_manu_type() == 4)
		rv = get_yuc_lock_state();
	else
	{
		if(back_data.lock_preparative_status == 1)
		{
			if(back_data.lock_status == 0)
				rv = 9;
			else
				rv = back_data.wc_lock_bank;  
		}
	}
		
	

	return rv;
}


/***************************************
** 
****************************************/

uint8_t read_ecu_key_state(void)     					
{
	uint8_t rv;
	
	if(read_ecu_manu_type() == 4)
		rv = get_yuc_key_state();   //玉柴校验状态
	else
		rv = back_data.key_status;

	return rv;

}




/**************************************
** 
***************************************/

uint8_t read_ecu_id_state(void)	
{
	uint8_t rv = 0;
	
	if(read_ecu_manu_type() == 4)
		rv = get_yuc_gps_state();
	else
		rv = back_data.id_status;

	return rv;
}						




/******************************
** OBD1939
********************************/

uint8_t read_mil_light_state(void)
{
	uint8_t rv = 0;
	
	
 if(ecu_data.mil_light_state == 1)
		rv = 1;
	else if(ecu_data.mil_light_state == 4)
		rv = 2;
	
	
	return rv;
}




uint8_t read_hmi_mon_state(void)	
{
	uint8_t rv = 0;

	rv = ecu_data.hmi_mon_state;

	return rv;
}

/*********************************
**	返回ECU数据
**********************************/

uint8_t read_ecu_data(struct ecu_data_str *data)
{
	if(data == NULL)
		return 1;
	
	memcpy((uint8_t *)data,(uint8_t *)&ecu_data,sizeof(struct ecu_data_str));

	return 0;
}




/*************************************
**	返回ECU类型
**************************************/

uint8_t read_ecu_manu_type(void)
{
	uint8_t rv  = 0;

	rv = back_data.manu_type;

	return rv;
}



/****************************
**
******************************/

uint8_t read_wc_mon_state(void)
{
	uint8_t rv = 0;

	rv = back_data.mon_status;

	return rv;
}



/****************************
**
******************************/

uint8_t read_wc_pre_lock_state(void)
{
	uint8_t rv = 0;

	rv = back_data.lock_preparative_status;

	return rv;
}


/*************************************
**	返回ECU类型
**************************************/

uint8_t read_ecu_res_state(void)
{
	uint8_t rv  = 0;

	rv = ecu_data.res_state;

	return rv;
}



/*************************************
**
**************************************/


void read_ecu_dm1_data(struct gb1939_dm_str *data)
{
	if(data == NULL)
		return;
	
	memcpy((uint8_t *)data,(uint8_t *)&ecu_dm1_qb4,sizeof(struct gb1939_dm_str));
}



/****************************************
**	复位ECU数据  复位为无效数据
**	
*****************************************/

static void reset_ecu_data(void)
{
	// ecu_data.travel_speed = 0xFFFF;      					//车速
	// ecu_data.air_pressure = 0xFF;     					//大气压力
	// ecu_data.engine_actual_torque_percent = 0xFF;						//柴油机净输出扭矩/柴油机实际扭矩/指示扭矩
	// ecu_data.friction_torque = 0xFF;					//摩擦扭矩
	// ecu_data.engine_actual_rotate = 0xFFFF;  					//柴油机转速
	// ecu_data.engine_fuel_flow = 0xFFFF;  				//发动机燃料流量
	// ecu_data.scr_upstream_nox = 0xFFFF;					//SCR上游NOx传感器输出值
	// ecu_data.scr_downstream_nox = 0xFFFF;				//SCR下游NOx传感器输出值
	// ecu_data.reactant_allowance = 0xFF; 				//反应剂余量
	// ecu_data.enter_volume = 0xFFFF;						//进气量
	// ecu_data.scr_entrance_temp = 0xFFFF; 				//SCR入口温度
	// ecu_data.scr_exit_temp = 0xFFFF;					//SCR出口温度
	// ecu_data.dpf_diffPressure = 0xFFFF;					//DPF压差
	// ecu_data.coolant_temp = 0xFF;						//冷却液温度
	// ecu_data.fuel_percent = 0xFF;						//
	// ecu_data.egr_opening = 0xFFFF;						//EGR阀开度
	// ecu_data.egr_setting = 0xFFFF;						//EGR设定值

	memset((uint8_t *)&ecu_data,0xFF,sizeof(struct ecu_data_str));
	
}




/******************************
**	测试使用
********************************/

static void analysis_can_recv_data(twai_message_t *msg,uint16_t num)
{
	int16_t             i;
	uint32_t            can_id;
	uint8_t             *data;
	
	if(msg == NULL || num > 100)
		return;
	
	for(i = 0;i < num;i++)
	{
		can_id = (msg + i)->identifier;
		data = (msg + i)->data;
		switch(can_id)
		{
			case 0x18FEEE00:
				ecu_data.fuel_temp = (*(data + 1) == 0xFF) ? 40 : (*(data + 1));			//燃油温度    （叉车燃油温度）
				ecu_data.oil_temp = *(uint16_t *)(data + 2);         				//机油温度
				ecu_data.coolant_temp = (*(data + 0) == 0xFF) ? 40 : (*(data + 0));							//冷却液温度
				break;
			case 0x18FEEF00:
				ecu_data.oil_pressure = *(data + 3);//机油压力
				ecu_data.water_position = *(data + 7);//冷却液液位
				ecu_data.relative_oil_pressure = ((*(data + 3) == 0xFF) ? 0 : *(data + 3)) * 4;		//相对机油压力
				ecu_data.absolute_oil_pressure = ((*(data + 3) == 0xFF) ? 0 : *(data + 3)) * 4;		//绝对机油压力
				ecu_data.oil_position = ((*(data + 2) == 0xFF) ? 0 : *(data + 2)) * 0.4;							//机油液位
				ecu_data.crank_pressure = 0;   				//曲轴箱压力(预留)
				ecu_data.cool_pressure = 0;  					//冷却液压力(预留)
				ecu_data.cool_position = ((*(data + 7) == 0xFF) ? 0 : *(data + 2)) * 0.4;						//冷却液位置
				ecu_data.fuel_pressure = (*(data + 0)) * 4;      //燃油压力
				break;
			case 0x0CF00400:
				ecu_data.engine_torque_mode = *(data + 0) & 0x0F;		  //发动机扭矩模式
				ecu_data.engine_rotate = *(uint16_t *)(data + 3);		  //发动机转速
				ecu_data.engine_torque = *(data + 2);	  //发动机实际扭矩	
				ecu_data.engine_need_torque_percent =	*(data + 7);	  //发动机需求扭矩百分比
				ecu_data.driver_cmd_torque_percent = *(data + 1);         //驾驶员需求扭矩百分比
				break;
			case 0x0CF00300:
				ecu_data.accelerator_percent = *(data + 1) * 0.4;							//加速踏板行程值
				ecu_data.engine_load_percent = *(data + 2);  						//发动机负荷   当前转速下负荷百分比
				ecu_data.speed_status = (*(data + 0) >> 4) & 0x03;							//速度限制状态
				ecu_data.acc_switch_status = (*(data + 0) >> 2) & 0x03;     	  //踏板开关
				ecu_data.acc_idling_status = (*(data + 0)) & 0x03;   		  //加速踏板怠速开关状态
				break;
			case 0x18FEE400:   //(暂时不使用)
				break;
			case 0x18FD7C00:
				ecu_data.dpf_surplus_light = *(data + 0) & 0x07;		//DPF再生提醒灯
				ecu_data.dfp_build_light = (*(data + 1) >> 2) & 0x03;	//DPF主动再生状态指示灯
				ecu_data.dpf_forbid_light = (*(data + 2) >> 2) & 0x03;	//DPF的再生禁止指示灯
				ecu_data.dpf_build_fault = (*(data + 4) >> 4) & 0x03;	//系统故障抑制DPF的活性再生判断灯
				ecu_data.dpf_temp_light	= (*(data + 6) >> 2) & 0x07;	//排气系统高温灯

				break;
			case 0x18FEF200:
				ecu_data.engine_fuel_flow = *(uint16_t *)(data + 0);		//发动机燃料流量 （国四数据流）  发动机燃油消耗率
				ecu_data.engine_fuel_instant = *(uint16_t *)(data + 2);		//瞬时油耗-燃油经济性	 Km/L
				ecu_data.engine_fuel_average = *(uint16_t *)(data + 4);		//平均油耗				Km/L
				break;
			case 0x18FEFF00:
				ecu_data.oil_water_pilot = *(data + 0) & 0x03;  	//水中有油指示  0:正常；1：报警
				break;
			case 0x18FEF700:
				ecu_data.accumulator_vol = *(uint32_t *)(data + 4) * 0.5;   //电平电压
				break;
			case 0x18FFF400:  //(????)
				break;
			case 0x18FEDF00:
				ecu_data.engine_set_rotate = *(uint16_t *)(data + 1) * 0.125;     		//发动机设定转速
				ecu_data.friction_torque = (*(data + 0) == 0xFF) ? 0 : (*(data + 0));    //摩擦扭矩（发动机最大基准扭矩百分比）
					//发动机工作速度异步调节
				ecu_data.engine_affiliated_torque_percent = *(data + 4);	// 发动机附件扭矩百分比
					//废气质量流量 (LSB)
					//废气质量流量 (MSB)
				break;
			case 0x18FF3500:    //(?????)
				//写 K−Epsilon常数
				//读取传感器序列号
				//读取硬件和软件版本
				//读取K−Epsilon常数
				//读取温标系数
				//读取大量空气流量测量
				//读取差压测量
				// 读取差压传感器温度
				// 读取绝对压力测量
				// 读取外壳温度
				// 读取介质温度
				// 读取单片机温度

				break;
			case 0x18FEF600:
				ecu_data.relative_add_pressure = *(data + 1);								//相对增压压力
				ecu_data.absolute_add_pressure = *(data + 1);								//绝对增压压力
				ecu_data.dfp_inlet_pressure = *(data + 0);						// 微粒捕集器入口压力  (颗粒捕捉器上游压力)
				ecu_data.entered_air_intake_temp = *(data + 2);						// 进气歧管空气温度  （进气温度）
				ecu_data.air_intake_prossure = 	*(data + 3);					// 进气压力
				ecu_data.exhaust_gas_temp = *(uint16_t *)(data + 4);					// 排气温度

				break;
			case 0x18FEF500:
				ecu_data.air_pressure = *(data + 0);         		//大气压力
				//ecu_data.engine_nacelle_temp = 0;			//发动机舱内温度 (未使用)
				ecu_data.air_temp = *(uint16_t *)(data + 3);//大气温度
				ecu_data.entered_air_temp = *(data + 5);					//发动机进气温度
				ecu_data.way_temp = 0;									//路面温度
				break;
			case 0x18FE6900:
				ecu_data.air_booster_out_temp = *(uint16_t *)(data + 6);//发动机增压空气冷却器出口温度
				break;
			case 0x18FFAA00:  //（？？？？？？）
						// 发送DPF再生请求
						// 发送灰超载检测状态
						// 发送停止RGN状态
						// 显示DPF再生状态
				break;
			case 0x18FEFC17:
				ecu_data.fuel_percent = *(data + 1);
				break;

			/********** 法规CAN报文开始 ****************/

			case 0x0CF00A00:
			case 0x18F00A00:
				ecu_data.enter_volume = *(uint16_t *)(data + 2);            //进气量
				break;
			case 0x18F00E51:
				ecu_data.scr_upstream_nox = *(uint16_t *)(data + 0);        //SCR上游NOx传感器输出值
				break;
			case 0x18F00F52:
				ecu_data.scr_downstream_nox = *(uint16_t *)(data + 0);      //SCR下游NOx传感器输出值
				break;
//			case 0x0CF0D200:  //EEC18
//				ecu_data.egr_opening = *(uint32_t *)(data + 4);							//EGR阀开度
//				break;
			case 0x18FD0700:      //
				ecu_data.mil_light_state = *(data + 0) & 0x03;  //排放控制报警灯状态
				ecu_data.brake_status_light = *(data + 1) & 0x03;  //发动机制动灯状态
				break;
			case 0x14FD3E00:
			case 0x18FDB43D:
				ecu_data.scr_entrance_temp = *(uint16_t *)(data + 0); 				//后处理SCR入口温度
				ecu_data.scr_exit_temp = *(uint16_t *)(data + 3);						//后处理SCR出口温度
				break;
			case 0x0CF0D200:
			case 0x0CFD9400:
			case 0x18FD9400:
				ecu_data.egr_opening = *(uint16_t *)(data + 0);		//EGR阀实际开度
				break;
			case 0x18FDD500:
				ecu_data.egr_setting = *(uint16_t *)(data + 4);		//EGR阀设定开度
				break;
			case 0x18FDB200:
				ecu_data.dpf_diffPressure = *(uint16_t *)(data + 4);			//后处理DPF压差
				break;
			case 0x18FE5600:  
				ecu_data.reactant_allowance = *(data + 0);   // 后处理反应剂罐液位（尿素液位） //反应剂余量
				break;	
			case 0x18FEE500:						           /** 发动机运行时间 **/
				if(ecu_data.engine_rotate > 4800 && ecu_data.engine_rotate != 0xFFFF)
				{
					ecu_data.engine_work_time = *(uint32_t *)(data + 0);					//发动机工作时间
					ecu_data.engine_total_revolutions = *(uint32_t *)(data + 4);
					back_data.engine_work_time = ecu_data.engine_work_time;
					back_data.flag = 0x5AA55AA5;
				}
				break;
			case 0x18FEF100:  //CCVS (巡航控制)
				ecu_data.travel_speed = *(uint16_t *)(data + 1) ;							//行驶速度
				ecu_data.clutch_status = (*(data + 0) >> 6) & 0x03;						//离合开关状态
				ecu_data.brake_status = (*(data + 0) >> 2) & 0x03;							//刹车开关状态
				ecu_data.cruise_speed = *(data + 6);							//巡航控制巡航速度
				ecu_data.cruise_control_status = (*(data + 3) >> 5) & 0x07;;    //巡航控制状态
				ecu_data.cruise_enable_status = (*(data + 3) >> 2) & 0x03;;			//巡航使能
				ecu_data.cruise_acitve_status = (*(data + 3)) & 0x03;;			//巡航激活状态
				break;
			case 0x18FEE000:
				ecu_data.once_travel = *(uint32_t *)(data + 0) * 0.125 * 10;							//单次行驶距离
				ecu_data.total_travel = *(uint32_t *)(data + 4) * 0.125 * 10;  					//总里程
				break;
			
			case 0x18ff0800:     //潍柴ECU锁车
				ecu_data.cold_boot_status = ((*(data + 2)) >> 4) & 0x0F;
				
				back_data.manu_type = 2;
				back_data.mon_status = (*(data + 2) & 0x01);			//激活状态
				back_data.lock_status =  (*(data + 2) & 0x02) ? 1: 0;    //实际锁车状态
				back_data.key_status = (*(data + 2) & 0x04) ? 1: 0;
				back_data.id_status = (*(data + 2) & 0x08) ? 1: 0;            //GPSID匹配状态
				back_data.lock_preparative_status = (*(data + 1) & 0x01);

				break;
			case 0x18FEE900:
				if(ecu_data.engine_rotate > 4800 && ecu_data.engine_rotate != 0xFFFF)
				{
					ecu_data.once_fuel = (*(uint32_t *)(data + 0)) * 5;      					//单次油耗
					ecu_data.total_fuel = (*(uint32_t *)(data + 4)) * 5;  						//累计油耗
					back_data.total_fuel = ecu_data.total_fuel;
				}
				break;	
			case 0x18FEF803:  //(??????)
				ecu_data.cluth_pressure = (*(data + 0) == 0xFF) ? 0 : *(data + 0);							  							//离合器压力
				ecu_data.gearbox_oil_level = (*(data + 1) == 0xFF) ? 0 : *(data + 1);					  								//变速箱齿轮油液位
				ecu_data.gearbox_oil_diff_pressure = (*(data + 2) == 0xFF) ? 0 : *(data + 2);	  								//变速箱齿轮油滤压差
				ecu_data.gearbox_oil_pressure = (*(data + 3) == 0xFF) ? 0 : *(data + 3);				  							//变速箱齿轮油压力
				ecu_data.gearbox_oil_temp = (*(uint16_t *)(data + 4) == 0xFFFF) ? 0 : *(uint16_t *)(data + 4);	//传动系机油温度
				ecu_data.gearbox_oil_level_status = ((*(data + 7) == 0xFF) ? 0 : *(data + 7) >> 4) & 0x0F;		  //变数变速箱齿轮油液位测量状态
				break;
			case 0x18FD9503:  //(?????)
				ecu_data.gearbox_oil_level_switch = (*(data + 0) >> 2) & 0x03;     																					//变速箱齿轮油液位开关状态
				ecu_data.gearbox_oil_filter_switch = (*(data + 0)) & 0x03;    																							//变速箱油过滤器限位开关
				ecu_data.gearbox_oil_out_temp = (*(uint16_t *)(data + 1) == 0xFFFF) ? 0 : *(uint16_t *)(data + 1);         //变速器变矩器油出口温度
				break;
			case  0x18FD9BA3:    //
				ecu_data.urea_box_temp = *(data + 0);//尿素温度
				ecu_data.urea_concentration = *(data + 1);		//尿素浓度
				break;
			case 0x18FE563D:
				ecu_data.urea_percent = *(data + 0);   //尿素液位
				break;
			case 0x18FFA100:
				ecu_data.dpf_dustiness = *(data + 2);//DPF积尘量
				break;
			case 0x18FDC700:
				ecu_data.energy_saving_state = *(data + 3);//节能状态
				break;
			case 0x18F0010B:
				ecu_data.assist_stalling_sign = *(data + 3);//辅助熄火
				break;
			case 0x0C000003:
				ecu_data.intervene_engine_mode = *(data + 0) & 0x03;  //发动机控制
				break;
			case 0x18FEE300:
				//
				//参考扭矩   
				break;
			case 0x18F0000F:
				ecu_data.electronic_buff_torque = *(data + 1);//缓冲器扭矩百分比
				ecu_data.buffer_torque = *(data + 1);//缓冲器扭矩百分比
				break;
			case 0x18FEDC00:

				break;
			case 0x18FEF000:

				break;
			case 0x18FCBD00:
			
				break;
			case 0x18FE56A3:   //东方红拖拉机 ？？？？？
				ecu_data.reactant_allowance = (*(data + 0) == 0xFF) ? 0 : (*(data + 0));   // 后处理反应剂罐液位（尿素液位） //反应剂余量
				break;
			case 0x1CECFF00:    //TPCM
				break; 
			case 0x1CEBFF00:    //TPDT
				break;
			case 0x10FAF1AA:      //仪表解锁报文			
				break;
			case 0x18FFE523:
				ecu_data.fuel_percent = *(data + 0);
				break;
			case 0x18DAF100:          //ISO27145  (国标环保故障)
				analysis_gb27145_dm(data);
				break;
			case 0x18feca00:	 	//DM1故障（以潍柴的为例）
			case 0x18ecff00:   		//
			case 0x18ebff00:   		//
				analysis_gb1939_dmc(can_id,data);
				analysis_wc_lock_state();    //解析潍柴锁车级别
				analysis_reference_torque(can_id,data);		//解析发动机最大进准扭矩
				break;
			case 0x18FEB017:
				ecu_data.hmi_mon_state = (*(data + 0)) & 0x03;						//仪表防拆激活状态
				ecu_data.hmi_hbstate = (*(data + 1)) & 0x03;						//仪表心跳校验状态
				break;
			case 0x18EB0100:
				{
					struct can_send_mq_t tmp_mq;
					QueueHandle_t t_q = NULL;

					tmp_mq.cmd = 5;
					tmp_mq.len = 8;
					memcpy(tmp_mq.data,data,8);
					t_q = get_can_send_queue();
					if(t_q != NULL)
						xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));
					//ecu_data.res_heart++;
				}
				break;
			case 0x18EB0700:            //云内激活锁车应答
				{
					struct can_send_mq_t tmp_mq;
					QueueHandle_t t_q = NULL;

					tmp_mq.cmd = 6;
					tmp_mq.len = 8;
					memcpy(tmp_mq.data,data,8);
					t_q = get_can_send_queue();
					if(t_q != NULL)
						xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));
					//ecu_data.res_lock++;
				}
				
				break;
			case 0x18EB0800:    //
				{
					ecu_data.res_state = *(data + 2);               //ECU锁车应答状态
					ecu_data.mon_status = *(data + 2) & 0x01;			//激活状态
					ecu_data.lock_status = (*(data + 2) >> 1) & 0x01;    //实际锁车状态
					ecu_data.key_status = (*(data + 2) >> 2) & 0x01;
					ecu_data.id_status = (*(data + 2) >> 3) & 0x01;            //GPSID匹配状态
					ecu_data.lock_preparative_status = (*(data + 2) >> 4) & 0x01;
					//if(ecu_data.res_counter < 100)
					//	ecu_data.res_counter++;

					//printf("-- res:%d,%d,%d,%d,%d\r\n",ecu_data.mon_status,ecu_data.lock_status,ecu_data.key_status,ecu_data.id_status,ecu_data.lock_preparative_status);
				}
				break;
			case 0x18FD0100:  // 玉柴  潍柴  的握手
				{
					struct can_send_mq_t tmp_mq;
					QueueHandle_t t_q = NULL;

					// ecu_data.res_state = *(data + 5);               //ECU锁车应答状态
					// ecu_data.mon_status = (*(data + 5) >> 6) & 0x03;			//激活状态
					// ecu_data.lock_status = (*(data + 2) >> 1) & 0x01;    //实际锁车状态
					// ecu_data.key_status = (*(data + 5) >> 4) & 0x01;   //KEY状态
					// ecu_data.id_status = (*(data + 5) >> 4) & 0x01;            //GPSID匹配状态
					// ecu_data.seed = *(uint32_t *)(data + 0);
					// ecu_data.lock_preparative_status = (*(data + 2) >> 4) & 0x01;
					//ecu_data.res_heart++;

					tmp_mq.cmd = 7;   //如果潍柴 直接应答
					tmp_mq.len = 8;
					memcpy(tmp_mq.data,data,8);
					t_q = get_can_send_queue();
					if(t_q != NULL)
						xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));
				}
				break;
			case 0x18f3c100:
			case 0x18F2E300:
				back_data.manu_type = 4;   //玉柴ECU
				break;
			case 0x18EB1000:    //Seed
				{
					struct can_send_mq_t tmp_mq;
					QueueHandle_t t_q = NULL;

					tmp_mq.cmd = 9;   //如果潍柴 直接应答
					tmp_mq.len = 8;
					memcpy(tmp_mq.data,data,8);
					t_q = get_can_send_queue();
					if(t_q != NULL)
						xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));
				}
				break; 
			case 0x18EB2000:   //状态
				{
					struct can_send_mq_t tmp_mq;
					QueueHandle_t t_q = NULL;

					tmp_mq.cmd = 8;   //如果潍柴 直接应答
					tmp_mq.len = 8;
					memcpy(tmp_mq.data,data,8);
					t_q = get_can_send_queue();
					if(t_q != NULL)
						xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));
				}
				break;
			case 0x18FC2A00:   // 暂时用做却分
				break;
			default:
				break;
			
		}
		
		msg++;
	}
}



/***********************************************
**	CAN生产接口
***********************************************/

void analysis_can_data_product(twai_message_t *msg,uint16_t num)
{
	int16_t             i;
	uint32_t            can_id;
	uint8_t             *data;
	
	if(msg == NULL || num > CAN_RX_MSG_NUM)
		return;
	
	for(i = 0;i < num;i++)
	{
		can_id = (msg + i)->identifier;
		data = (msg + i)->data;

		switch(can_id)
		{
			case 0x18FF35F3:                    //判断仪表或者VCU的防拆应答状态
				
				break;
			case 0x1FFFFFAA:										//测试
				{
					QueueHandle_t 				t_q = NULL;
					struct products_mq_str		t_m = {0};

					t_m.cmd = 5;
					memcpy(t_m.data,data,8);
					t_m.len = 8;
					t_q = get_products_queue();
					if(t_q != NULL)
						xQueueSend(t_q,&t_m,sizeof(struct products_mq_str));
				}
				break;
			case 0x18BC0A59:          //CAN数据发送内容
				{
					rt_ringbuffer_put(&cc_rb,data,8);
				}
				break;
			case 0x18BC0B59:         //CAN数据发送结束
				{
					//把接收到到的数据，发到生产处理任务里面
					QueueHandle_t 				t_q = NULL;
					struct products_mq_str		t_m = {0};
					uint16_t                	rb_return_len = 0;
					uint16_t                	w_len = 0;
					char 						*p;
					
					rb_return_len = rt_ringbuffer_data_len(&cc_rb);
        			w_len = rt_ringbuffer_get(&cc_rb, t_m.data, rb_return_len);
					if(w_len == 0 || w_len > 320)
						break;
					//printf("-- can to com:%d,%s\r\n",w_len,t_m.data);
					p = strstr((char *)t_m.data, "AT+Test");           //进入配置模式  （手动进入配置模式）
					if(p != NULL)
					{
						//printf("-- can to com:%d\r\n",w_len);
						t_m.cmd = 4;
						t_q = get_products_queue();
						if(t_q != NULL)
							xQueueSend(t_q,&t_m,sizeof(struct products_mq_str));
						break;
					}

					p = strstr((char *)t_m.data, "HOMER3ETESTOVER!");           //进入配置模式  （手动进入配置模式）
					if(p != NULL)
					{
						//printf("-- can to com:%d\r\n",w_len);
						t_m.cmd = 1;
						t_q = get_products_queue();
						if(t_q != NULL)
							xQueueSend(t_q,&t_m,sizeof(struct products_mq_str));
						break;
					}


					p = strstr((char *)t_m.data, "BDWMODIF:");           //进入配置模式  （手动进入配置模式）
					if(p != NULL)
					{	
						uint16_t i = 0;

						t_m.cmd = 0;

						for(i = 0;i < w_len;i++)
						{
							t_m.data[i] = *(p + i + 8);
						}

						t_m.len = w_len - 8;
						t_q = get_products_queue();
						if(t_q != NULL)
							xQueueSend(t_q,&t_m,sizeof(struct products_mq_str));
						break;
					}

					p = strstr((char *)t_m.data, "AT+RESET");           //进入配置模式  （手动进入配置模式）
					if(p != NULL)
					{
						 QueueHandle_t  	            qu_t = NULL;          //生产队列
        				struct app_main_mq_str      a_mq = {0};

       				 	//printf("-- Reset be from can port ...\r\n");
						a_mq.state = 1;

						qu_t =  get_app_main_queue();
						if(qu_t != NULL)
						{
							xQueueSend(qu_t,&a_mq,sizeof(struct app_main_mq_str));
						}
						vTaskDelay(100);
					}
				}
				break;
		}
	}
}




/**********************
**  CAN接收
************************/

void thread_entry_can_recv(void *parameter)
{
	twai_status_info_t can_status_info;

    parameter = parameter;

	rt_ringbuffer_init(&cc_rb, local_buff, sizeof(local_buff)); 
	rt_hw_init_can();
	reset_ecu_data();    //复位ECU数据
	
	back_data.manu_type = 4;
	
	for (;;)
    {
	   if(ESP_OK == twai_receive(&can_recvmsg.rx_msg[0], 1))
        {
			analysis_can_recv_data(can_recvmsg.rx_msg,CAN_RX_MSG_NUM);
			analysis_can_data_product(can_recvmsg.rx_msg,CAN_RX_MSG_NUM);
            memset((uint8_t *)&can_recvmsg,0,sizeof(struct can_msg_str));
			twai_clear_receive_queue();
			can_recvmsg.cnt = 1;
			//printf("-- run can recv  1 ...  %d\r\n",can_recvmsg.cnt);
			continue;
        }

		//printf("-- run can recv  2 ...  %d\r\n",can_recvmsg.cnt);
		if(can_recvmsg.cnt++  % 500 == 0)
		{
			//printf("-- can recv msg ..... fail......%d\r\n",can_recvmsg.cnt);
			can_recvmsg.can_status = 1;
			memset(can_recvmsg.rx_msg,0,sizeof(can_recvmsg.rx_msg));

			if(ESP_OK == twai_get_status_info(&can_status_info))
			{
				if(can_status_info.state != TWAI_STATE_RUNNING)
				{
					printf("-- can_status_info error %d\r\n",can_status_info.state);
        			twai_stop();
            		twai_driver_uninstall();
           			rt_hw_init_can();
					printf("-- Reset Init CAN OK>>>>>>> %d\r\n",can_recvmsg.cnt);
				}
			}
			else
			{
				printf("-- can_status_info return error \\r\n");
        		twai_stop();
            	twai_driver_uninstall();
           		rt_hw_init_can();
			}
		}

		if(can_recvmsg.cnt % 3200 == 0)   //接收不到CAn
		{
			//printf("-- reset can data .........\r\n");
			reset_ecu_data();    //复位ECU数据
		}  
    }
    
}




