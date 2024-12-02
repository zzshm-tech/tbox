



#include <stdio.h>
#include <string.h>

#include <rtthread.h>
#include <rtdevice.h>


#include "md5.h"
#include "pro_data.h"
#include "common.h"

#include "app_can_recv.h"
#include "board.h"
#include "drv_can.h"
#include "drv_timer.h"
#include "app_lte.h"
#include "app_gnss.h"
#include "app_mon.h"
#include "app_can_send.h"
#include "app_shell.h"
#include "app_iap.h"
#include "drv_gpio.h"
#include "app_gb4.h"
#include "app_products.h"



#define FAULT_CODE_NUM 63


struct faultcode_str const faultcode[FAULT_CODE_NUM] =
{
    {3,     110},       //0 水温传感器开路
    {4,     110},       //1 水温传感器短路
    {2,     519097},    //2 尿素品质传感器CAN线开路（尿素浓度报文超时）
    {11,		7351},      //3 下游氮氧传感器CAN线开路（下游氮氧浓度报文超时）
    {2,     7354},      //4 尿素品质传感器CAN线开路（尿素温度报文超时）
    {2,     4766},      //5 DPF上游温度传感器信号不可信
    {3,     171},       //6 环境温度传感器开路
    {4,     171},       //7 环境温度传感器短路
    {17,		3233},      //8 下游氮氧传感器未在规定时间内加热到设定值
    {17,		3235},      //9 下游氮氧浓度不准
    {5,     3226},      //10 下游氮氧传感器开路
    {7,     3226},      //11 下游氮氧传感器短路故障（传感器内部电路）
    {6,     3226},      //12 下游氮氧传感器供电电源故障（传感器内部电路）
    {0,     5245},      //13 非加热系统不能供给尿素严重驾驶限制故障
    {3,     108},       //14 环境压力过高
    {4,     108},       //15 环境压力过低
    {11,		4795},      //16 DPF 移除
    {15,		4781},      //17 DPF 过载Level1
    {20,		3251},      //18 DPF 压差传感器信号不可信
    {0,     3251},      //19 DPF压差传感器开路
    {1,     3251},      //20 DPF压差传感器短路
    {21,		4364},      //21 SCR效率诊断2
    {16,		518150},    //22 尿素回流管压力过高
    {2,     518150},    //23 尿素压力管压力过高
    {0,     518150},    //24 尿素压力不稳定
    {16,		518153},    //25 尿素在喷射过程中压力过高
    {18,		518153},    //26 喷射过程中尿素压力过低
    {15,		518153},    //27 喷射过程中尿素压力过高
    {11,		518155},    //28 尿素泵建压失败
    {1,     4360},      //29 后处理上游（SCR）排气温度偏低
    {2,     4360},      //30 后处理（SCR）上游排气温度过高
    {11,		4360},      //31 冷启动前后处理上游（SCR）排气温度传感器偏差大
    {0,     5394},      //32 尿素喷嘴的针阀被卡住
    {16,		3364},      //33 尿素浓度偏低
    {3,     3242},      //34 DPF上游温度传感器开路
    {4,     3242},      //35 DPF上游温度传感器短路
    {3,     4360},      //36 后处理上游温度（SCR）传感器开路
    {4,     4360},      //37 后处理上游温度（SCR）传感器短路
    {14,		5425},      //38 尿素箱空
    {18,		5245},      //39 尿素箱液位低于5%
    {5,     5394},      //40 尿素喷嘴开路
    {3,     5394},      //41 尿素喷嘴对电源短路
    {4,     5394},      //42 尿素喷嘴对地短路
    {2,     5394},      //43 尿素喷嘴高低边短路
    {11,		519700},    //44 后处理加热系统部件故障导致加热系统强制关闭清空
    {22,		4375},      //45 尿素泵无法驱动
    {5,     4375},      //46 尿素泵开路
    {3,     4375},      //47 尿素泵对电源短路
    {4,     4375},      //48 尿素泵对地短路
    {15,		1387},      //49 尿素泵压力比环境压力差值高50百帕
    {17,		1387},      //50 尿素泵压力比环境压力差值低500百帕
    {3,     1387},      //51 尿素泵压力传感器开路
    {4,     1387},      //52 尿素泵压力传感器短路
    {5,     4376},      //53 反向阀继电器开路
    {3,     4376},      //54 反向阀继电器对电源短路
    {4,     4376},      //55 反向阀继电器对地短路
		{1,			5245},      //56 尿素箱液位低于10%
		{2,			5245},			//57 非加热尿素系统不能供给尿素
		//
		{1,			5764},      //58 EGR阀电机开路故障（HBrgOpnLd）
    {1,			5831},      //59 EGR阀卡滞（JamVlvClsd）
    {0,			5831},      //60 EGR阀卡滞（JamVlvOpn）
    {3,			5263},      //61 EGR阀传感器开路或短路
    {4,			5263}      	//62 EGR阀传感器短路或开路
};




/*************		*****************/

static rt_device_t 			can1_dev = NULL;

static rt_device_t 			can2_dev = NULL;


static struct rt_can_msg 							can_rx_msg_buff[30] = {0}; 
static struct md1                  		dm1 = {0};              	//DM故障数据(包括发动机，整车系统，变速箱，仪表)

  	

/************* 本地区全局变量 *****************/

static struct ecu_hold_t							ecu_hold = {0};		  //CAN需要断电保存的数据

static struct can_info_str 						can_info = {0};						//CAN接收线程运行信息

static struct ecu_data_str						ecu_data = {0xFF};     			//发动机ECU数据


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


/*******************************************************
**	OBD  所有故障
*******************************************************/

uint16_t  read_qb4_dm_serial(uint8_t *buf,uint16_t size)
{
	uint16_t len = 0;

	if(buf == NULL || size < 160)
		return 0;
	
	if(ecu_hold.ecu_dm1_qb4.num == 0 || ecu_hold.ecu_dm1_qb4.num > 40)
		return 0;
	
	len = ecu_hold.ecu_dm1_qb4.num * 4;

	memcpy(buf,ecu_hold.ecu_dm1_qb4.serial_code,len);

	//printf("-- read 27145 dfc...\r\n");
	return len;
}





/*******************************************************
**	OBD  过滤之后的故障
*******************************************************/

uint16_t  read_gb4_dm_serial(uint8_t *buf,uint16_t size)
{
	uint16_t len = 0;

	if(buf == NULL || size < 160)
		return 0;
	
	if(ecu_hold.ecu_dm1_gb4.num == 0 || ecu_hold.ecu_dm1_gb4.num > 40)
		return 0;
	
	len = ecu_hold.ecu_dm1_gb4.num * 4;

	memcpy(buf,ecu_hold.ecu_dm1_gb4.serial_code,len);

	//printf("-- read 27145 dfc...\r\n");
	return len;
}



/*******************************************************
**	OBD  所有故障
*******************************************************/

uint16_t  read_tcu_dm_serial(uint8_t *buf,uint16_t size)
{
	uint16_t len = 0;

	if(buf == NULL || size < 160)
		return 0;
	
	if(ecu_hold.tcu_dm1_gb4.num == 0 || ecu_hold.tcu_dm1_gb4.num > 40)
		return 0;
	
	len = ecu_hold.tcu_dm1_gb4.num * 4;

	memcpy(buf,ecu_hold.tcu_dm1_gb4.serial_code,len);

	//printf("-- read 27145 dfc...\r\n");
	return len;
}



/********************************
**	
*********************************/

uint8_t check_fault_code(struct faultcode_str arg)
{
	int i;

  for(i = 0;i < FAULT_CODE_NUM;i++)
  {
		if(arg.fmi == faultcode[i].fmi && arg.spn == faultcode[i].spn)
			return 1;
  }

  return 0;
}




/********************************
**	解析1939故障码
*********************************/
static uint8_t analysis_gb1939_dmc(uint32_t can_id,uint8_t *data)
{
	uint8_t   	    	dm1_msg_index;				//多包消息起始位置
	uint8_t   	    	dm1_id = 0;
	uint32_t  				dm1_id_cache = 0;
	uint32_to_byte 		m_tmp;
	uint32_t    			i = 0;

	dm1_id = dm1_can_id(can_id & 0x000000FF);               					//关联相应的数组
	if(dm1_id > 0) 
		return 1;
	dm1_id_cache = can_id >> 8;
	if(dm1_id_cache == 0)
		return 1;

	if(dm1_id_cache  == 0x18FECA || dm1_id_cache  == 0x18FED4)                     				   								//DM1-单个故障 
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
		if((*(data + 5) == 0xCA) && (*(data + 6) == 0xFE) && (*(data + 7) == 0x00))           //不是DM1故障码
		{
			dm1.current_index[dm1_id] = 1;
			dm1.bam_flag[dm1_id] = 1;                                          //相应标志置一，以便于接收相应的多包数据包
			dm1.msg_num[dm1_id] = *(data + 3);
			dm1.data_len[dm1_id] = (*(data + 1) - 2) / 4 * 4;            //本控制单元故障总字节数，

			if(dm1.data_len[dm1_id] > DM1_1939_NUM * 4)
				dm1.data_len[dm1_id] = DM1_1939_NUM * 4;
			memset(data,0,8);
			return 1;
		}
		
		if((*(data + 5) == 0xD4) && (*(data + 6) == 0xFE) && (*(data + 7) == 0x00))           //不是DM1故障码
		{
			dm1.current_index[dm1_id] = 1;
			dm1.bam_flag[dm1_id] = 1;                                          //相应标志置一，以便于接收相应的多包数据包
			dm1.msg_num[dm1_id] = *(data + 3);
			dm1.data_len[dm1_id] = (*(data + 1) - 2) / 4 * 4;            //本控制单元故障总字节数，

			if(dm1.data_len[dm1_id] > DM1_1939_NUM * 4)
				dm1.data_len[dm1_id] = DM1_1939_NUM * 4;
			memset(data,0,8);
			return 1;
		}
		
		dm1.bam_flag[dm1_id] = 0;
		dm1.current_index[dm1_id] = 0;
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
	memset((uint8_t *)&ecu_hold.ecu_dm1_qb4,0,sizeof(struct gb1939_dm_str));
	ecu_hold.ecu_dm1_qb4.num  = dm1.data_len[dm1_id] / 4;
	
	if(ecu_hold.ecu_dm1_qb4.num > 0)
		ecu_data.engine_breakdown = 1;
	else
		ecu_data.engine_breakdown = 0;
	
	memset((uint8_t *)&ecu_hold.ecu_dm1_gb4,0,sizeof(struct gb1939_dm_str));
	
	for(i = 0; i < dm1.data_len[0] && dm1_id < DM1_1939_NUM;)
	{
		struct faultcode_str fc;
		uint32_t tmp = 0;

		m_tmp.byte[0] = dm1.data[0][i + 0];
		m_tmp.byte[1] = dm1.data[0][i + 1];
		m_tmp.byte[2] = (dm1.data[0][i + 2] >> 5) & 0x07;
		m_tmp.byte[3] = 0;

		ecu_hold.ecu_dm1_qb4.spn[dm1_id] = m_tmp.value;	
		ecu_hold.ecu_dm1_qb4.fmi[dm1_id] = (dm1.data[0][i + 2] & 0x1F);  
		
		tmp = *(uint32_t *)&dm1.data[0][i + 0];
		tmp &= 0x00FFFFFF;
		memcpy((uint8_t *)&ecu_hold.ecu_dm1_qb4.serial_code + i,(uint8_t *)&tmp,4);
		
		fc.spn = ecu_hold.ecu_dm1_qb4.spn[dm1_id];
		fc.fmi = ecu_hold.ecu_dm1_qb4.fmi[dm1_id];
		
		if(check_fault_code(fc) > 0)
		{
			ecu_hold.ecu_dm1_gb4.spn[ecu_hold.ecu_dm1_gb4.num] = fc.spn;
			ecu_hold.ecu_dm1_gb4.fmi[ecu_hold.ecu_dm1_gb4.num] = fc.fmi;
			tmp = *(uint32_t *)&dm1.data[0][i + 0];
			tmp &= 0x00FFFFFF;
			memcpy((uint8_t *)&ecu_hold.ecu_dm1_gb4.serial_code + ecu_hold.ecu_dm1_gb4.num * 4,(uint8_t *)&tmp,4);
			ecu_hold.ecu_dm1_gb4.num++;
		}
		
		i += 4;		
		dm1_id++;
	}
	return 0;
}






/********************************
**	解析1939故障码
*********************************/
static uint8_t analysis_tcu_dmc(uint32_t can_id,uint8_t *data)
{
	uint8_t   	    	dm1_msg_index;				//多包消息起始位置
	uint8_t   	    	dm1_id = 0;
	uint32_t  				dm1_id_cache = 0;
	uint32_to_byte 		m_tmp;
	uint16_t    			i = 0;
	uint16_t					j = 0;
	
	dm1_id = dm1_can_id(can_id & 0x000000FF);               					//关联相应的数组
	if(dm1_id != 2) 
		return 1;
	dm1_id = 1;
	
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
						
	//printf("-- the run is......%d\r\n",dm1.data_len[0]);
	memset((uint8_t *)&ecu_hold.tcu_dm1_gb4,0,sizeof(struct gb1939_dm_str));
	ecu_hold.tcu_dm1_gb4.num  = dm1.data_len[dm1_id] / 4;
	
	for(i = 0,j = 0; i < dm1.data_len[dm1_id] && j < DM1_1939_NUM;)
	{
		uint32_t tmp = 0;

		m_tmp.byte[0] = dm1.data[dm1_id][i + 0];
		m_tmp.byte[1] = dm1.data[dm1_id][i + 1];
		m_tmp.byte[2] = (dm1.data[dm1_id][i + 2] >> 5) & 0x07;
		m_tmp.byte[3] = 0;

		ecu_hold.tcu_dm1_gb4.spn[j] = m_tmp.value;	
		ecu_hold.tcu_dm1_gb4.fmi[j] = (dm1.data[dm1_id][i + 2] & 0x1F);  
		
		tmp = *(uint32_t *)&dm1.data[dm1_id][i + 0];
		tmp &= 0x00FFFFFF;
		memcpy((uint8_t *)&ecu_hold.tcu_dm1_gb4.serial_code + i,(uint8_t *)&tmp,4);
	
		i += 4;		
		j++;
	}

	return 0;
}




/***********************
**	潍柴发动机实际锁车状态
************************/

static void analysis_actual_lock_state(void)
{
	uint8_t i = 0;
	
	for(i = 0;i < 40;i++)
	{
		if(ecu_hold.ecu_dm1_qb4.spn[i] == 7728)   //国四锁车
		{
			ecu_hold.wc_lock_bank = ecu_hold.ecu_dm1_qb4.fmi[i] + 1;
		}
		
		if(ecu_hold.ecu_dm1_qb4.spn[i] == 522014) //国三锁车
		{
			ecu_hold.wc_lock_bank = ecu_hold.ecu_dm1_qb4.fmi[i];
		}
	}
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
					ecu_data.max_ref_torque *= 20;
				}
				step = 0;
			}
			
			break;
	}
}



/*****************************
**
******************************/

uint16_t  read_max_ref_torque(void)
{
	uint16_t rv = 0;
	
	rv = ecu_data.max_ref_torque;
	//rv = 1207;
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




/**********************
**	返回排放类型
**********************/

uint8_t read_emissions_type(void)
{
	uint8_t rv;
	
	rv = ecu_hold.emissions_type;
	
	return rv;
}



/**************************
**	返回摩擦扭矩
****************************/

uint8_t read_friction_torque(void)
{
	uint8_t rv = 0xFF;
	
	if(ecu_data.friction_torque >= 125)
		rv = ecu_data.friction_torque - 125;
	//rv = 9;
	return rv;
}




/******************************
**	发动机燃料流量
*******************************/

uint16_t read_engine_fuel_flow(void)
{
	uint16_t rv = 0;
	
	rv = ecu_data.engine_fuel_flow;
	
	return rv;
}




/******************************
**	发动机进气量
*******************************/

uint16_t read_enter_volume(void)
{
	uint16_t rv = 0;
	
	rv = ecu_data.enter_volume;
	
	//rv *= 0.05;
	
	return rv;
}



/****************************
**	SCR入口温度
******************************/

uint16_t read_scr_entrance_temp(void)
{	
	uint16_t rv;
	
	rv  = ecu_data.scr_entrance_temp;
	
	return rv;
}




/*******************************
**	
*********************************/

uint16_t read_scr_downstream_nox(void)
{
	uint16_t rv = 0xFFFF;
	
	if(ecu_data.scr_downstream_nox != 0xFFFF)
	{
		if((ecu_data.scr_downstream_nox * 0.05) >= 200)
			rv = ecu_data.scr_downstream_nox;
		else 
			rv = 0;
	}
	
	return rv;
}
/************************
** 
*************************/

uint8_t read_mil_light_state(void)
{
	uint8_t rv = 0;
	
	
 if(ecu_data.mil_light_state == 1)
		rv = 1;
	else if(ecu_data.mil_light_state == 4)
		rv = 2;
	
	
	return rv;
}



/*************************************
**
**************************************/


void read_ecu_qb4_dm_data(struct gb1939_dm_str *data)
{
	if(data == NULL)
		return;
	
	memcpy((uint8_t *)data,(uint8_t *)&ecu_hold.ecu_dm1_qb4,sizeof(struct gb1939_dm_str));
}




/*************************************
**
**************************************/


void read_ecu_gb4_dm_data(struct gb1939_dm_str *data)
{
	if(data == NULL)
		return;
	
	memcpy((uint8_t *)data,(uint8_t *)&ecu_hold.ecu_dm1_gb4,sizeof(struct gb1939_dm_str));
}



/*************************************
**
**************************************/


void read_tcu_gb4_dm_data(struct gb1939_dm_str *data)
{
	if(data == NULL)
		return;
	
	memcpy((uint8_t *)data,(uint8_t *)&ecu_hold.tcu_dm1_gb4,sizeof(struct gb1939_dm_str));
}




/********************************
**	返回CAN1连接状态
**	ECU类型
*********************************/

uint8_t read_ecu_type(void)
{
	uint8_t rv;
	
	rv = ecu_hold.ecu_type;

	return rv;
}



/********************************
**	返回CAN1连接状态
*********************************/

uint8_t read_can_connect_state(uint8_t ch)
{
	uint8_t rv;
	
	switch(ch)
	{
		case 1:
			rv = can_info.can1_state;
			break;
		case 2:
			rv = can_info.can2_state;
			break;
	}
	
	return rv;
}


/********************************
**	返回CAN2连接状态
*********************************/

uint8_t read_can2_connect_state(void)
{
	rt_int16_t rv;
	
	rv = 0;
	
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



/*******************
**	返回发动机工时
********************/

uint32_t read_engine_wt_counter(void)
{
	uint32_t rv;
	
	rv = ecu_data.engine_wt_counter;
	
	return rv;
}



/*******************
**	返回发动机工时
********************/

uint32_t read_fuel_wt_counter(void)
{
	uint32_t rv;
	
	rv = ecu_data.fuel_wt_counter;
	
	return rv;
}



uint32_t read_engine_work_time(void)
{
	uint32_t rv;
	
	if(ecu_data.engine_rotate <= 5600 || ecu_data.engine_rotate == 0xFFFF || read_in_acc_state() == 0 || can_info.can1_state > 0)
		rv = 0xFFFFFFFF; //can_data_back.total_fuel;
	else
		rv = ecu_data.engine_work_time;
	
	return rv;
}



/****************************
**	当前ECU预锁车状态
*****************************/

uint8_t read_wc_pre_lock_state(void)
{
	uint8_t rv;
	
	rv = ecu_hold.wc_pre_lock_state;          //反馈的锁车状态
	
	return rv;
}


/****************************
**	当前ECU实际锁车状态
**  返回值 0:未锁车 1:一级锁车 2:二级锁车 9:待锁车
*****************************/

uint8_t read_ecu_lock_state(void)
{
	uint8_t rv;
	
	if(read_ecu_type() == 0x02)
	{
		if(ecu_hold.wc_lock_state > 0)   //实际锁车状态
		{
			if(read_lock_expect_state()  != ecu_hold.wc_lock_bank)     //实际锁车状态
			{
				rv = read_lock_expect_state();
			}
			else
			{
				rv = ecu_hold.wc_lock_bank;
			}
		}
		else if(ecu_hold.wc_pre_lock_state == 1)
		{
			rv = 9;
		}
		else 
		{
			rv = 0;
		}
	}
	else if(read_ecu_type() == 0x03)
	{
		/* 锁车需要在下一个工作循环执行，判断是否为待锁车 */
		if(ecu_hold.sc_mon_state == 1 && read_lock_expect_bank() > 0)
		{
			if(ecu_hold.sc_lock_state == 0)
				rv = 9;
			else
				rv = 1;  //
		}
		else
		{
			rv = 0;
		}
	}
	else if(read_ecu_type() == 0x04)
	{
		rv = get_yuc_lock_state();
	}
	
	//rt_kprintf("-- Actual lock state:%d\r\n",rv);
	return rv;
}



/******************************
**	
********************************/

uint8_t read_wc_mon_state(void)
{
	uint8_t rv;
	
	rv = ecu_hold.wc_mon_state;
	
	return rv;
}



/******************************
**	返回上柴发动机监控状态
********************************/

uint8_t read_ecu_sc_mon_state(void)
{
	uint8_t rv;
	
	rv = ecu_data.sc_mon_state;
	
	return rv;
}




/****************************
**	这个ECU锁车功能状态，是来自ECU，如果当前没有接通ECU，
**	这个状态来自上次保存的。
**	
*****************************/
uint8_t read_ecu_mon_status(void)
{
	uint8_t rv = 0;
	
	if(read_ecu_type() == 2)
	{
		rv = ecu_hold.wc_mon_state;
	}
	else if(read_ecu_type() == 3)
	{
		rv = ecu_hold.sc_mon_state;
	}
	else if(read_ecu_type() == 4)
	{
		rv = get_yuc_mon_state();
	}
	
	return rv;
}



/****************************
**	
**	
*****************************/
uint8_t read_ecu_key_status(void)
{
	uint8_t rv;
	
	if(read_ecu_type() == 0x02)
	{
		rv = ecu_hold.wc_key_state;
	}
	else if(read_ecu_type() == 0x03)
	{
		rv = ecu_hold.sc_key_state;
	}
	else
	{
		rv = get_yuc_key_state();   //玉柴校验状态
	}
	
	return rv;
}


/****************************
**	
**	
*****************************/
uint8_t read_ecu_id_status(void)
{
	uint8_t rv;
	
	if(read_ecu_type() == 0x02)
	{
		rv = ecu_hold.wc_id_state;
	}
	else if(read_ecu_type() == 0x03)
	{
		rv = ecu_hold.sc_id_state;
	}
	else
	{
		rv = get_yuc_gps_state();
	}
	return rv;
}



/***************************
**	读取CAN数据
**	 返回整体CAN数据
****************************/

uint16_t read_ecu_data(struct ecu_data_str *buf,uint16_t buf_size)
{
	uint16_t rv;
	
	rv = sizeof(struct ecu_data_str);
	if(buf_size < rv)
		return 0;
	
	memcpy((uint8_t *)buf,(uint8_t *)&ecu_data,rv);
	
	return rv;
}



/***********************
**
************************/

uint32_t read_total_fuel(void)
{
	uint32_t rv;
	
	if(ecu_data.engine_rotate <= 5600 || ecu_data.engine_rotate == 0xFFFF || read_in_acc_state() == 0 || can_info.can1_state > 0)
		rv = 0xFFFFFFFF; //can_data_back.total_fuel;
	else
		rv = ecu_data.total_fuel;
	
	return rv;
}



/***********************
**	关闭CAN1
************************/

void can_port_close(void)
{
	if(can1_dev != NULL && ((can1_dev->flag & RT_DEVICE_FLAG_ACTIVATED) == RT_DEVICE_FLAG_ACTIVATED))
	{
		rt_device_close(can1_dev);
		can1_dev = NULL;
	}
	
	if(can2_dev != NULL && ((can2_dev->flag & RT_DEVICE_FLAG_ACTIVATED) == RT_DEVICE_FLAG_ACTIVATED))
	{
		rt_device_close(can2_dev);
		can2_dev = NULL;
	}
	
	rt_can_power_off();
}




/********************************
**	通过CAN接口发送数据
**	
********************************/

void can1_send_data(enum can_id_type type, uint32_t id, uint8_t *data,uint8_t num)
{
	struct rt_can_msg 				can_msg = {0};
	uint32_t 									msg_num = 1;
	
	if(read_in_acc_state() == 0)    //如果电锁没有打开，不进行发送
		return;
	
	if(num > 8)
		num = 8;
	
	can_msg.id = id;
  can_msg.ide = type;
  can_msg.rtr = 0;
  can_msg.rsv = 0;
  can_msg.len = num;
  can_msg.priv = 0;
  can_msg.hdr = 0;
  can_msg.reserved = 0;

  rt_memcpy(&can_msg.data[0], &data[0], 8);
	rt_sem_take(&can_info.can1_send_sema, RT_WAITING_FOREVER);
	
	if(can1_dev != NULL && ((can1_dev->flag & RT_DEVICE_FLAG_ACTIVATED) == RT_DEVICE_FLAG_ACTIVATED))
	{
		msg_num = rt_device_write(can1_dev, 0, &can_msg,sizeof(struct rt_can_msg));
		rt_thread_delay(0);
		if(msg_num == 0)
		{
			//rt_kprintf("-- can1 send error:%d\r\n",msg_num);
		}
	}
	
	rt_sem_release(&can_info.can1_send_sema);
}







/********************************
**	通过CAN接口发送数据
**	
********************************/

void can2_send_data(enum can_id_type type, uint32_t id, uint8_t *data)
{
	struct rt_can_msg 				can_msg = {0};
	uint32_t 									msg_num = 1;
	
	if(read_in_acc_state() == 0)    //如果电锁没有打开，不进行发送
		return;
	
	can_msg.id = id;
  can_msg.ide = type;
  can_msg.rtr = 0;
  can_msg.rsv = 0;
  can_msg.len = 8;
  can_msg.priv = 0;
  can_msg.hdr = 0;
  can_msg.reserved = 0;

  rt_memcpy(&can_msg.data[0], &data[0], 8);
	rt_sem_take(&can_info.can2_send_sema, RT_WAITING_FOREVER);
	
	if(can2_dev != NULL && ((can2_dev->flag & RT_DEVICE_FLAG_ACTIVATED) == RT_DEVICE_FLAG_ACTIVATED))
	{
		msg_num = rt_device_write(can2_dev, 0, &can_msg,sizeof(struct rt_can_msg));
		rt_thread_delay(0);
		if(msg_num == 0)
		{
			//rt_kprintf("-- can2 send error:%d\r\n",msg_num);
		}
	}
	
	rt_sem_release(&can_info.can2_send_sema);
}






/******************************
**	测试使用
**	解析装载机信息
*******************************/

void analysis_can1_recv_data(struct rt_can_msg *msg,rt_uint16_t msg_num)
{
	uint16_t 		i = 0;
	uint32_t 		can_id = 0;
	uint8_t 		*data = NULL;
	
	if(msg == RT_NULL || msg_num == 0)
		return;
	
	for(i = 0;i < msg_num;i++)
	{
		can_id = msg->id;
		data = msg->data;
		//rt_kprintf("-- id:0x%X\r\n",can_id);
		switch(can_id)
		{
			/********** 法规CAN报文开始 ****************/
			case 0x0CF00400:              /** EEC1发动机控制器 **/
				ecu_data.engine_rotate = *(uint16_t *)(data + 3);		 				//发动机转速
				ecu_data.engine_torque = *(data + 2);						//发动机实际扭矩
				break;
			case 0x0CF00A00:
				ecu_data.enter_volume = *(uint16_t *)(data + 2);//进气量
				break;
			case 0x18F00E51:
				ecu_data.scr_upstream_nox = *(uint16_t *)(data + 0);//SCR上游NOx传感器输出值
				break;
			case 0x18F00F52:
				ecu_data.scr_downstream_nox = *(uint16_t *)(data + 0);			//SCR下游NOx传感器输出值
				break;
		case 0x0CF0D200:  //EEC18
			  ecu_data.egr_opening = (*(uint16_t *)(data + 0) == 0xFEFF) ? 0xFFFF : (*(uint16_t *)(data + 0));   //EGR实际开度
				break;
			case 0x18FD0700:
				
				break;
			case 0x14FD3E00:    //SCRT1
				ecu_data.scr_entrance_temp = (*(uint16_t *)(data + 0) == 0xFEFF) ? 0xFFFF : (*(uint16_t *)(data + 0)); 			//后处理SCR入口温度			
				ecu_data.scr_exit_temp = *(uint16_t *)(data + 3);  					//后处理SCR出口温度
			break;
			case 0x18FD9400:   //EEC7
				ecu_hold.emissions_type = 1;    //EGR，默认 0， SCR
				ecu_data.egr_opening = (*(uint16_t *)(data + 0) == 0xFEFF) ? 0xFFFF : (*(uint16_t *)(data + 0));							//EGR阀实际开度
				break;
			case 0x18EF0046:
				ecu_data.egr_opening = (*(uint16_t *)(data + 0) == 0xFEFF) ? 0xFFFF : (*(uint16_t *)(data + 0));							//EGR阀实际开度
				break;
			case 0x18FDD500:
				ecu_data.egr_setting = (*(uint16_t *)(data + 4) == 0xFEFF) ? 0xFFFF : (*(uint16_t *)(data + 4));							//EGR阀设定开度
				
				break;
			case 0x18FDB200:  //AT1IMG		
				ecu_data.dpf_diffPressure = (*(uint16_t *)(data + 4) == 0xFEFF) ? 0xFFFF : (*(uint16_t *)(data + 4));				//后处理DPF压差
				break;
			case 0x18FE5600:   //ATT1TI1 
				ecu_data.reactant_allowance = (*(data + 0) == 0xFE) ? 0xFF : (*(data + 0));   // 后处理反应剂罐液位（尿素液位） //反应剂余量
				break;
			case 0x18FE563D:
				ecu_data.reactant_allowance = (*(data + 0) == 0xFE) ? 0xFF : (*(data + 0));   // 后处理反应剂罐液位（尿素液位） //反应剂余量
				break;
			case 0x18FEDF00:
				ecu_data.friction_torque = (*(data + 0) == 0xFE) ? 0xFF : (*(data + 0));    //摩擦扭矩（发动机最大基准扭矩百分比）
				break;
			case 0x18FEEE00:  //ET1
				ecu_data.fuel_temp = (*(data + 1) == 0xFF) ? 40 : (*(data + 1));								//燃油温度
				ecu_data.oil_temp = *(uint16_t *)(data + 2) * 0.3125;         				//机油温度	
				ecu_data.coolant_temp = (*(data + 0) == 0xFE) ? 0xFF : (*(data + 0));							//冷却液温度
				break;
			case 0x18FEF200:
				ecu_data.engine_fuel_flow = *(uint16_t *)(data + 0);//发动机燃料流量 （国四数据流）
				break;
			case 0x18FEF500:
				ecu_data.air_pressure = *(data + 0) * 0.5;         		//大气压力
				//ecu_data.temp= 0;			//发动机舱内温度
				ecu_data.air_temp = (*(uint16_t *)(data + 3)) * 0.3125;//大气温度
				ecu_data.entered_air_temp = *(data + 5);					//发动机进气温度
			
				ecu_data.air_pressure = *(data + 0);							//大气压力
				break;
			case 0x1CECFF00:    //TPCM
				break; 
			case 0x1CEBFF00:    //TPDT
				break;
			
			/********** ***********/
			case 0x10F510AA:
				//can_data_back.fuel_percent = *(data + 0) / 0.4;		//燃油量
				ecu_data.fuel_percent = *(data + 0) / 0.4;		//燃油量
				ecu_data.gearbox_oil_temp = *(data + 1);//变速箱油温
				ecu_data.braking_air_pressure = (*(uint16_t *)(data + 2));//制动气压
				ecu_data.gearbox_pressure_switch = (*(data + 7)) & 0x01;  //变速箱压力开关
				break;
			case 0x10FAF1AA:      //仪表解锁报文
				{
					struct rt_can_event event = {0};
					rt_mq_t  t_mq = NULL;
					
					if(*(uint16_t *)(data + 0) != 0x7700)
						break;
					event.cmd = 0;  										//解锁
					event.arg1 = 0;
					event.arg2 = 0;
					event.arg3 = 0;
					event.arg4 = 0; 
					
					t_mq = get_can_lock_mq();
					if(t_mq != NULL)
						rt_mq_send(t_mq,&event,sizeof(event));		
				}				
				break;
			
			case 0x18FD0100:							//潍柴ecu握手信号
				{
					struct rt_can_event 			event = {0};
					rt_mq_t 									t_mq = NULL;
					event.cmd = 3;  										//激活锁车功能
					event.arg1 = *(rt_uint32_t *)data;
					event.arg2 = *(rt_uint32_t *)(data + 4);
					event.arg3 = 0;
					event.arg4 = 0; 
					
					memcpy(event.data,data,8);
					t_mq = get_can_lock_mq();
					if(t_mq != NULL)
						rt_mq_send(t_mq,&event,sizeof(event));	
					//rt_kprintf("-- recv ecu hank ...\r\n");
				}				
			case 0x18fef700:
				ecu_data.accumulator_vol = *(uint16_t *)(data + 4) * 0.5;   //电平电压
				break;
			case 0x18FEE500:						           /** 发动机运行时间 **/
				ecu_data.engine_wt_counter++;
				ecu_data.engine_work_time = *(uint32_t *)(data + 0) * 0.5;					//发动机工作时间
				//can_data_back.engine_work_duration = ecu_data.engine_work_time;
				break;
			case 0x0CF00300:
				ecu_data.accelerator = *(data + 1) * 0.4;							//加速踏板行程值
				ecu_data.engine_load = *(data + 2);  						//发动机负荷
				break;
			case 0x18FEF100:  //CCVS (巡航控制)
				ecu_data.travel_speed = *(uint16_t *)(data + 1) * 10 / 256;							//行驶速度
				ecu_data.clutch_status = (*(data + 0) >> 6) & 0x03;						//离合开关状态
				ecu_data.cruise_speed = *(data + 6);							//巡航控制巡航速度
				ecu_data.car_speed = *(uint16_t *)(data + 1);								//车速
			
				break;
			case 0x18FEE000:
				ecu_data.once_travel = *(uint32_t *)(data + 0);							//单次行驶距离
				ecu_data.total_travel = *(uint32_t *)(data + 4);  					//总里程
				break;
			case 0x18fef600:
				ecu_data.relative_add_pressure = *(data + 1) * 2;								//相对增压压力
				ecu_data.absolute_add_pressure = *(data + 1) * 2;								//绝对增压压力
				ecu_data.entered_air_temp      = *(data + 2);               //发动机支气管温度
				break;
			
			case 0x18ff0800:     //潍柴ECU锁车
				ecu_hold.ecu_type = 0x02;
				ecu_hold.wc_lock_state = (*(data + 2) & 0x02) ? 1: 0;
				ecu_hold.wc_mon_state = (*(data + 2) & 0x01);   						//锁车功能状态
				ecu_hold.wc_key_state =  (*(data + 2) & 0x04) ? 1: 0;
				ecu_hold.wc_id_state = (*(data + 2) & 0x08) ? 1: 0;								//TBOX ID状态
				ecu_hold.wc_pre_lock_state =  (*(data + 1) & 0x01); // ECU欲锁车状态
				break;
			
		case 0x18FEF300:          //上柴锁车 (绑定或者解绑 应答信号)
				{
					struct rt_can_event 			event = {0};
					rt_mq_t 									t_mq = NULL;
					event.cmd = 8;  										//激活锁车功能
					memcpy(event.data,data,8);
					t_mq = get_can_lock_mq();
					if(t_mq != NULL)
						rt_mq_send(t_mq,&event,sizeof(event));	
					//rt_kprintf("-- recv ecu hank ...\r\n");
				}
				break;
			case 0x18FF2100:	/* 上柴ECU锁车 */
				ecu_hold.ecu_type = 0x03;
				switch (*(data + 0) & 0x0F)	
				{
					case 0x09:	/* 绑定未锁车 */
						ecu_hold.sc_pre_lock_state = 1;
						ecu_hold.sc_id_state = 1;
						
						ecu_hold.sc_key_state = 1;
						ecu_data.sc_mon_state = 1;
						ecu_hold.sc_mon_state = 1;
						ecu_hold.sc_lock_state = 0;
						break;
					case 0x0D:	/* 绑定且锁车 */
						ecu_hold.sc_pre_lock_state = 2;
						ecu_hold.sc_id_state = 1;
						ecu_hold.sc_key_state = 1;
						ecu_data.sc_mon_state = 1;
						ecu_hold.sc_mon_state = 1;
						ecu_data.sc_lock_state = 1;
						ecu_hold.sc_lock_state = 1;
						break;
					case 0x0A:	/* 非绑定  没有绑定*/
						ecu_hold.sc_pre_lock_state = 1;
						ecu_hold.sc_id_state = 0;
						ecu_hold.sc_key_state = 0;
						ecu_data.sc_mon_state = 0;
						ecu_hold.sc_mon_state = 0;
						ecu_hold.sc_lock_state = 0;
						break;
					default:
						ecu_hold.sc_pre_lock_state = 1;
						ecu_hold.sc_id_state = 0;
						ecu_hold.sc_key_state = 0;
						ecu_data.sc_mon_state = 0;
						ecu_hold.sc_mon_state = 0;
						ecu_hold.sc_lock_state = 0;
						break;
				}
				break;
			case 0x18FEEF00:
				ecu_data.relative_oil_pressure = ((*(data + 3) == 0xFF) ? 0 : *(data + 3)) * 4;		//相对机油压力
				ecu_data.absolute_oil_pressure = ((*(data + 3) == 0xFF) ? 0 : *(data + 3)) * 4 + 100;		//绝对机油压力
				ecu_data.oil_position = ((*(data + 2) == 0xFF) ? 0 : *(data + 2)) * 0.4;							//机油液位
				ecu_data.cool_pressure = 0;  					//冷却液压力(预留)
				ecu_data.cool_position = ((*(data + 7) == 0xFF) ? 0 : *(data + 2)) * 0.4;						//冷却液位置
				break;

			case 0x18FEE900:
				ecu_data.once_fuel = (*(uint32_t *)(data + 0)) * 5;      					//单次油耗
				//can_data_back.once_fuel = (*(uint32_t *)(data + 0)) * 5;
				ecu_data.total_fuel = (*(uint32_t *)(data + 4)) * 5;  						//累计油耗
				ecu_data.fuel_wt_counter++;
				//can_data_back.total_fuel = (*(uint32_t *)(data + 4)) * 5;
				break;	
			case 0x18FEF803:
				//ecu_data.gearbox_oil_temp = (*(uint16_t *)(data + 4) == 0xFFFF) ? 0 : *(uint16_t *)(data + 4);	//统一使用仪表发出来的
				break;
			case 0x18FD9503:
				//ecu_data.gearbox_oil_level_switch = (*(data + 0) >> 2) & 0x03;     																					//变速箱齿轮油液位开关状态 																							//变速箱油过滤器限位开关
				ecu_data.gearbox_oil_out_temp = (*(uint16_t *)(data + 1) == 0xFFFF) ? 0 : *(uint16_t *)(data + 1);         //变速器变矩器油出口温度
				break;
			case 0x18FEFF00: 
				ecu_data.oil_water_pilot = *(data + 0) & 0x03;   //油含水指示灯
				ecu_data.mil_light_state = (*(data + 1)) & 0x07;   //排放控制系统指示灯
				if(ecu_hold.ecu_type == 2)   //潍柴发动机
				{
					uint8_t m,n;
					
					ecu_data.drain_off_fault_class = 0xFF;
					m = (*(data + 1) >> 3) & 0x07;//驾驶员报警系统指示灯
					n = (*(data + 1)) & 0x07;
					
					if(m == 0x03 || m == 0x04 || m == 0x05 || n == 1 || n == 4)
					{
						ecu_data.drain_off_fault_class = 1;
					}
					else if(m == 2 || n == 0)
					{
						ecu_data.drain_off_fault_class = 0;
					}
				}
				
				if(ecu_hold.ecu_type == 3)
				{
					uint8_t m,n;
					
					ecu_data.drain_off_fault_class = 0xFF;
					m = (*(data + 1) >> 3) & 0x07;//驾驶员报警系统指示灯
					n = (*(data + 1)) & 0x07;
				
					if((m > 0 && m <= 5) || n == 1 || n == 2) 
					{
						ecu_data.drain_off_fault_class = 1;
					}
					else if(m == 0 && n == 0)
					{
						ecu_data.drain_off_fault_class = 0;
					}
				}
				
				break;
			case 0x18FEE400:            /** SHUTDN **/
				ecu_data.cold_boot_status = *(data + 3) & 0x01;//等待起动指示灯
				break;
			case 0x18FD7C00:
				{
					uint8_t tmp;
					
				 tmp = *(data + 0) & 0x07;				//再生指示灯
				 if(tmp == 0x00 || tmp == 0x01 ||tmp == 0x04 || tmp == 0x02)
					 ecu_data.dpf_build_light = tmp;
				 else
					 ecu_data.dpf_build_light = 0xFF;
				 
				 if(ecu_hold.ecu_type == 4)
					tmp = (*(data + 2) >> 2) & 0x03;
				 else
					tmp = *(data + 2) & 0x03;			//再生禁止灯
				
				 if(tmp == 0x00 || tmp == 0x01)
					 ecu_data.dpf_forbid_light = tmp;
				 else 
					 ecu_data.dpf_forbid_light = 0xFF;
				
			  } 
				break;
			case 0x18feca00:	 //DM1故障（以潍柴的为例）
				if(ecu_hold.ecu_type == 4)
				{
					uint8_t tmp;
					
					tmp = (*(data + 0) >> 2) & 0x03;//驾驶员报警系统指示灯
					if(tmp == 0 || tmp == 1)
						ecu_data.drain_off_fault_class = tmp;
					else
						ecu_data.drain_off_fault_class = 0xFF;
				}
			case 0x18FED400:   //潍柴DM12
			case 0x18ecff00:   //
			case 0x18ebff00:   //
				analysis_gb1939_dmc(can_id,data);   //解析故障码（）
				analysis_actual_lock_state();   //判断潍柴锁车状态
				analysis_reference_torque(can_id,data);		//解析发动机最大进准扭矩
				break;
			case 0x18FECA03:
			case 0x18ECFF03:
			case 0x18EBFF03:
				analysis_tcu_dmc(can_id,data);
				break;
			case 0x18FE4A03: //变速箱 1
				ecu_data.auto_model_light =  ((*(data + 2) >> 2) & 0x01) > 0 ? 0 : 1;//自动模式指示灯
				ecu_data.fnr_active_light = (*(data + 1) >> 2) & 0x01;//FNR激活指示灯
				break;
			case 0x18FF3203:    //变速箱 2
				ecu_data.fnr_active_light = (*(data + 4) >> 7) & 0x01;//FNR激活指示灯
			  ecu_data.auto_model_light = (*(data + 4) >> 1) & 0x01;//自动模式指示灯
				//变速箱油滤堵塞指示灯
				ecu_data.gearbox_filter_light = (*(data + 4) >> 5) & 0x01;
				ecu_data.power_dis_lilght = (*(data + 4) >> 2) & 0x01;  //动力断开指示灯
				break;
			case 0x0CF00203:
				ecu_data.gearbox_out_rotate = *(uint16_t *)(data + 1) * 0.125;		//变速箱输出轴转速
				ecu_data.power_dis_lilght = (*(data + 3) == 250) ? 1 : 0;  //动力断开指示灯
				break;
			case 0x18F00503:
				ecu_data.gearbox_switch = *(data + 3);
				break;
			case 0x18FF1E03:   //变速箱档位
				if((((*(data + 6)) >> 4) & 0x03) == 0)
				{
					ecu_data.gearbox_switch = 0x7D;   //空挡
					break;
				}
				else if((((*(data + 6)) >> 4) & 0x03) == 1)
				{
					switch((*(data + 6)) & 0x0F)
					{
							case 1:  //0x1
								ecu_data.gearbox_switch = 0x7E;
								break;
							case 2:  //0x10  
								ecu_data.gearbox_switch = 0x7F;
								break;
							case 3:  //
								ecu_data.gearbox_switch = 0x80;
								break;
							case 4:
								ecu_data.gearbox_switch = 0x81;
								break;
							default:
								ecu_data.gearbox_switch = 0x7D;
								break;
					}
				}
				else
				{
					switch((*(data + 6)) & 0x0F)
					{
							case 1:  //0x1
								ecu_data.gearbox_switch = 0x7C;
								break;
							case 2:  //0x10  
								ecu_data.gearbox_switch = 0x7B;
								break;
							case 3:  //
								ecu_data.gearbox_switch = 0x7A;
								break;
							case 4:
								ecu_data.gearbox_switch = 0x90;
								break;
							default:
								ecu_data.gearbox_switch = 0x7C;
								break;
					}
				}
										
				break;
			case 0x18FF3103:  //区分两款变速箱
				ecu_data.gearbox_type_e = 1;
				break;
			case 0x18FF1F03:
				ecu_data.gearbox_oil_out_temp = *(data + 0);
				break;
			case 0x18f3c100:
			case 0x18F2E300:
			case 0x18FF8149:
				ecu_hold.ecu_type = 0x04;
				break;
			case 0x1FFFFFAA:										//测试
				{
					struct products_data_t 		pdata = {0};
					rt_mq_t 									t_mq = NULL;
					
					pdata.cmd = 5;  //激活锁车功能
					memcpy(pdata.data,(data + 0),8);
					t_mq = get_products_mq();
					if(t_mq != NULL)
						rt_mq_send(t_mq,&pdata,sizeof(pdata));
				}
				break;
			default:
				break;
			
		}
		
		msg++;
	}
}



/*************************
**	
**************************/

void analysis_can2_recv_data(struct rt_can_msg *msg,rt_uint16_t msg_num)
{
	uint8_t 			i = 0;
	uint32_t 			can_id = 0;
	uint8_t 			*data = NULL;
	
	if(msg == RT_NULL || msg_num == 0)
		return;
	
	for(i = 0;i < msg_num;i++)
	{
		can_id = msg->id;
		data = msg->data;
		//rt_kprintf("-- id:0x%X\r\n",can_id);
		switch(can_id)
		{
			/********** 法规CAN报文开始 ****************/
			case 0x0CF00400:              /** EEC1发动机控制器 **/
			default:
				break;
			
		}
		
		msg++;
	}
}



/******************************
**	保存ECU运行数据
*******************************/

static void init_ecu_hold_data(void)
{
	rt_device_t 				bsram_dev = RT_NULL;       //
	uint8_t 						tmp;
	
	bsram_dev = rt_device_find("back_sram");
	rt_device_open(bsram_dev, RT_DEVICE_OFLAG_RDWR);
	rt_device_read(bsram_dev,1024, (uint8_t *)&ecu_hold,sizeof(ecu_hold));
	rt_device_close(bsram_dev);
	
	tmp = CalcCrc8((uint8_t *)&ecu_hold,sizeof(ecu_hold) - 1);
	if(ecu_hold.crc_value != tmp)
	{
		memset((uint8_t *)&ecu_hold,0xFF,sizeof(struct ecu_hold_t));
	}
	
	rt_kprintf("-- the ecu hold data:%d\r\n",ecu_hold.ecu_type);
}



/********************************************
**
**	写ECU备份信息
********************************************/

void write_ecu_hold_data(void)
{
	rt_device_t 								bsram_dev = RT_NULL;       //
	uint8_t tmp;
	
	bsram_dev = rt_device_find("back_sram");
	rt_device_open(bsram_dev, RT_DEVICE_OFLAG_RDWR);
	
	tmp = CalcCrc8((uint8_t *)&ecu_hold,sizeof(ecu_hold) - 1);
	
	ecu_hold.crc_value = tmp;
	
	rt_device_write(bsram_dev,1024,(uint8_t *)&ecu_hold,sizeof(ecu_hold));
	rt_device_close(bsram_dev);
}



/***************************
**	读取CAN数据
**	 返回整体CAN数据
****************************/

uint16_t read_ecu_can_data(uint8_t *buf,uint16_t buf_size)
{
	uint16_t rv;
	
	rv = sizeof(struct ecu_data_str);
	if(buf_size < rv)
		return 0;
	
	memcpy(buf,(uint8_t *)&ecu_data,rv);
	
	return rv;
}



/***************************
**	通过CAN发送 序列数据
****************************/

uint16_t 	write_buf_to_can1(uint8_t *buf,uint16_t size)
{
	uint8_t 	i = 0;
	uint16_t 	part_len = 0;
	uint8_t 	check = 0;
	uint8_t 	array[8] = {0};

	while(part_len < size)
	{
		for(i = 0;i < 8;i++)
		{
			if(part_len < size)
			{
				array[i] = *(buf + part_len);
				part_len++;
			}
			else 
			{
				array[i] = 0xFF;
			}
			
			check ^= array[i];
		}
		can1_send_data(SEND_EXTID, 0x18BCA058,array,8);
		rt_thread_delay(1);
	}
			
	array[0] = check;
	*(uint16_t *)&array[1] = part_len;
	array[3] = 0x55;
	array[4] = 0xAA;
	array[5] = 0x55;
	array[6] = 0xAA;
	array[7] = 0x55;
	can1_send_data(SEND_EXTID,  0x18BCB058,array,8);

	return 0;
}




/********************
**	国标数据默认
***********************/

void init_tbox_data(void)
{
	memset((uint8_t *)&ecu_data,0xFF,sizeof(ecu_data));
	can_info.can1_state = 0xFF;
	can_info.can2_state = 0xFF;
}

/********************************************
**	解析CAN数据
**	CAN接收数据
**	解析数据
**	执行周期50ms
**	
********************************************/

void thread_entry_can(void *parameter)
{
  uint32_t 					res = 0;																//
	uint8_t 					acc_status = 0;
	uint32_t 					thread_cnt = 0;
	
	parameter = parameter;
  
	rt_sem_init(&can_info.can1_send_sema, "can1_send", 1, 0);  //
	rt_sem_init(&can_info.can2_send_sema, "can2_send", 1, 0);
	
	#ifdef USING_BXCAN1
	//初始化CAN1
	can1_dev = rt_device_find("can1");
	rt_device_init(can1_dev);
	rt_device_control(can1_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_NORMAL);
	rt_device_control(can1_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_NORMAL);		//
	rt_device_open(can1_dev, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_RADIO_TX)); 
	#endif
	
	#ifdef USING_BXCAN2
	//初始化CAN2
	can2_dev = rt_device_find("can2");
	rt_device_init(can2_dev);
	rt_device_control(can2_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_NORMAL);
	rt_device_control(can2_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_NORMAL);		//
	rt_device_open(can2_dev, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_RADIO_TX)); 
	#endif
	
	init_ecu_hold_data();       //

	init_tbox_data();
	rt_can1_stb_off();				//
	rt_can2_stb_off();
	rt_can_power_on();    		//CAN电源

	ecu_hold.emissions_type = 0;     //默认排放类型为SCR
	
	for(;;)
	{
		//这里需要改进 (???????)
		if(thread_cnt++ % 500 == 0)
		{
			ecu_data.auto_model_light = 1;           //自动模式指示灯
			ecu_data.fnr_active_light = 0;           //
			ecu_data.power_dis_lilght = 0;
		}
		acc_status = read_in_acc_state();
		
		#ifdef USING_BXCAN1
		//处理CAN1的数据
    memset((uint8_t *)can_rx_msg_buff, 0, sizeof(can_rx_msg_buff));
		if(can1_dev != NULL && ((can1_dev->flag & RT_DEVICE_FLAG_ACTIVATED) == RT_DEVICE_FLAG_ACTIVATED))
		{
			rt_device_control(can1_dev, RT_CAN_CMD_GET_MSG_NUM, &res);
			if(res > 30)   
			{
				res = 30;
			}
			
			if(res > 0)
			{
				rt_device_read(can1_dev, 0, can_rx_msg_buff, res * sizeof(struct rt_can_msg));							//
				analysis_can1_recv_data(can_rx_msg_buff,res);  
				can_info.can1_recv_cnt = 0;																			//						
				can_info.can1_state = 0;																				//
				if(ecu_data.engine_rotate > 700 * 8)   //发动机转速才进行数据保存
					write_ecu_hold_data();    
			}
		}
		#endif
		
		#ifdef USING_BXCAN2
		//处理CAN2的数据
		memset((uint8_t *)can_rx_msg_buff, 0, sizeof(can_rx_msg_buff));
		if(can2_dev != NULL && ((can2_dev->flag & RT_DEVICE_FLAG_ACTIVATED) == RT_DEVICE_FLAG_ACTIVATED))
		{
			rt_device_control(can2_dev, RT_CAN_CMD_GET_MSG_NUM, &res);
			if(res > 30)   
			{
				res = 30;
			}
			
			if(res > 0)
			{
				rt_device_read(can2_dev, 0, can_rx_msg_buff, res * sizeof(struct rt_can_msg));							//
				analysis_can2_recv_data(can_rx_msg_buff,res);               		    //
				can_info.can2_recv_cnt = 0;																			//						
				can_info.can2_state = 0;																				//   
			}
		}
		
		#endif
  
		/*************************************
		**	在ACC接通的情况下，
		*************************************/
	
		if(++can_info.can1_recv_cnt % 3000 == 0)          //3秒收不到数据 认为物理总线异常
		{
			init_tbox_data();
			if(acc_status > 0)
				can_info.can1_state = 1;
		}
			
		if(++can_info.can2_recv_cnt % 3000 == 0)          //3秒收不到数据 认为物理总线异常
		{
			if(acc_status > 0)
				can_info.can2_state = 1;
		}
    
		rt_thread_delay(1); 
	}
}




