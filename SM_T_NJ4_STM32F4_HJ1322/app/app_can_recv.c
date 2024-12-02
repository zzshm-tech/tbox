


#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "pro_data.h"
#include "common.h"


#include "drv_can.h"
#include "app_can_recv.h"
#include "app_can_send.h"


static struct can_rx_msg 					can1_rx_buf[20] = {0};


/********************** 本地全局变量 *********************/

static struct gb1939_dm_str				ecu_dm1_qb4 = {0};	 	//DM1 接收到的故障码 把总线上的 所有DM1 全部解析出来，

static struct md1                 dm1 = {0};              //DM故障数据(包括发动机，整车系统，变速箱，仪表)

static struct tcu_data_str 				tcu_data = {0};			//变速箱数据

static struct hmi_data_str				hmi_data = {0};   		//仪表数据
	
static struct ecu_data_str				ecu_data = {0};         //发动机ECU数据

static struct gb27145_dm_str    	gb27145_dm = {0};		//用作国标故障码(27145传送）






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
		memset(data,0,8);
	}
	else if((dm1_id_cache == 0x18EBFF) && (dm1.bam_flag[dm1_id] == 1))          //已经收到多包声明数据
	{
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
	for(i = 0; i < dm1.data_len[0] && dm1_id < DM1_MAX_NUM;)
	{
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
		i += 4;
		dm1_id++;
	}

	return 0;
}



/******************************
**	测试使用
**	英轩叉车
********************************/

static void analysis_can_data(struct can_rx_msg *msg,uint16_t num)
{
	int16_t             i;
	uint32_t            can_id;
	uint8_t             *data;
	
	if(msg == NULL || num > 100)
		return;
	
	for(i = 0;i < num;i++)
	{
		can_id = (msg + i)->id;
		data = (msg + i)->data;
		switch(can_id)
		{
			case 0x18FEEE00:
				ecu_data.water_temp = (*(data + 0) == 0xFF) ? 40 : (*(data + 0));           //冷却液温度  (叉车水温)
				ecu_data.fuel_temp = (*(data + 1) == 0xFF) ? 40 : (*(data + 1));			//燃油温度    （叉车燃油温度）
				ecu_data.oil_temp = *(uint16_t *)(data + 2) * 0.03125;         				//机油温度
				ecu_data.coolant_temp = (*(data + 0) == 0xFF) ? 40 : (*(data + 0));							//冷却液温度
				break;
			case 0x18FEEF00:
				ecu_data.relative_oil_pressure = ((*(data + 3) == 0xFF) ? 0 : *(data + 3)) * 4;		//相对机油压力
				ecu_data.absolute_oil_pressure = ((*(data + 3) == 0xFF) ? 0 : *(data + 3)) * 4;		//绝对机油压力
				ecu_data.oil_position = ((*(data + 2) == 0xFF) ? 0 : *(data + 2)) * 0.4;							//机油液位
				ecu_data.crank_pressure = 0;   				//曲轴箱压力(预留)
				ecu_data.cool_pressure = 0;  					//冷却液压力(预留)
				ecu_data.cool_position = ((*(data + 7) == 0xFF) ? 0 : *(data + 2)) * 0.4;						//冷却液位置
				ecu_data.fuel_pressure = (*(data + 0)) * 4;      //燃油压力
				break;
			case 0x0CF00400:
																	//发动机扭矩模式
				ecu_data.engine_rotate = *(uint16_t *)(data + 3);		 	//发动机转速
				ecu_data.engine_torque = *(data + 2);	                    //发动机实际扭矩	
													//发动机需求扭矩百分比
				break;
			case 0x0CF00300:
				ecu_data.accelerator = *(data + 1) * 0.4;							//加速踏板行程值
																//远程油门踏板位置
				ecu_data.engine_load = *(data + 2);  						//发动机负荷
				ecu_data.speed_status = (*(data + 0) >> 4) & 0x03;							//速度限制状态
				ecu_data.acc_switch_status = (*(data + 0) >> 2) & 0x03;     	  //踏板开关
				ecu_data.acc_idling_status = (*(data + 0)) & 0x03;   		  //加速踏板怠速开关状态
				break;
			case 0x18FEE400:
			     //进气余热指示灯

				break;
			case 0x18FD7C00:
					//DPF再生提醒灯
					//DPF主动再生状态指示灯
					//DPF的再生禁止指示灯
					//系统故障抑制DPF的活性再生判断灯
					//排气系统高温灯

				break;
			case 0x18FEF200:
				ecu_data.engine_fuel_flow = *(uint16_t *)(data + 0);//发动机燃料流量 （国四数据流）
				//瞬时油耗-燃油经济性	
				//平均油耗
				break;
			case 0x18FEFF00:
				ecu_data.mil_light_state = (*(data + 1)) & 0x07;
				//油中有水
				break;
			case 0x18FEF700:
				ecu_data.accumulator_vol = *(uint32_t *)(data + 4) * 0.5;   //电平电压
				break;
			case 0x18FFF400:
					//PTO开关状态指示
					//变速箱空挡开关状态

				break;
			case 0x18FEDF00:
				ecu_data.engine_set_rotate = *(uint16_t *)(data + 1) * 0.125;     		//发动机设定转速
				ecu_data.friction_torque = (*(data + 0) == 0xFF) ? 0 : (*(data + 0));    //摩擦扭矩（发动机最大基准扭矩百分比）
					//发动机工作速度异步调节
					//预计发动机寄生损耗-扭矩百分比
					//废气质量流量 (LSB)
					//废气质量流量 (MSB)
				break;
			case 0x18FF3500:
				//写 K?Epsilon常数
				//读取传感器序列号
				//读取硬件和软件版本
				//读取K?Epsilon常数
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
				ecu_data.entered_air_temp      = *(data + 2);               //发动机支气管温度

										// 		微粒捕集器入口压力
										// 相对增压压力
										// 进气歧管空气温度
										// 进气压力
										// 排气温度

				break;
			case 0x18FEF500:
				ecu_data.air_pressure = *(data + 0);         		//大气压力
				//ecu_data.temp= 0;			//发动机舱内温度
				ecu_data.air_temp = *(uint16_t *)(data + 3) * 0.03125;//大气温度
				ecu_data.entered_air_temp = *(data + 5);					//发动机进气温度
				ecu_data.way_temp = 0;									//路面温度
				break;
			case 0x18FE6900:
				//	发动机增压空气冷却器出口温度
				break;
			case 0x18FFAA00:
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
			case 0x18FDB300:
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
			case 0x18FD0700:
				//排放控制报警灯状态
				break;
			case 0x14FD3E00:
				ecu_data.scr_entrance_temp = *(uint16_t *)(data + 0); 				//后处理SCR入口温度
				//ecu_data.
				//diesel_flow_info.scr_exit_temp = *(uint16_t *)(data + 3);						//后处理SCR出口温度
				break;
			case 0x0CFD9400:
			case 0x18FD9400:
				ecu_data.egr_opening = *(uint16_t *)(data + 0);//EGR阀实际开度
				break;
			case 0x18FDD500:
				ecu_data.egr_setting = *(uint16_t *)(data + 4);//EGR阀设定开度
				break;
			case 0x18FDB200:
				ecu_data.dpf_diffPressure = *(uint16_t *)(data + 4);			//后处理DPF压差
				break;
			case 0x18FE5600:  
				ecu_data.reactant_allowance = *(data + 0);   // 后处理反应剂罐液位（尿素液位） //反应剂余量
				break;
			case 0x1CECFF00:    //TPCM
				break; 
			case 0x1CEBFF00:    //TPDT
				break;
			case 0x10FAF1AA:      //仪表解锁报文
				{
				}				
				break;
			case 0x18FD0100:							//潍柴ecu握手信号
				{
				}	
                break;			
				
			case 0x18FEE500:						           /** 发动机运行时间 **/
				ecu_data.engine_work_time = *(uint32_t *)(data + 0) * 0.5;					//发动机工作时间
				//can_data_back.engine_work_duration = ecu_data.engine_work_time;
				break;
			case 0x18FEF100:  //CCVS (巡航控制)
				ecu_data.travel_speed = *(uint16_t *)(data + 1) * 10 / 256;							//行驶速度
				ecu_data.clutch_status = (*(data + 0) >> 6) & 0x03;						//离合开关状态
				ecu_data.brake_status = (*(data + 0) >> 2) & 0x03;							//刹车开关状态
				ecu_data.cruise_speed = *(data + 6);							//巡航控制巡航速度
				ecu_data.cruise_control_status = (*(data + 3) >> 5) & 0x07;;    //巡航控制状态
				ecu_data.cruise_enable_status = (*(data + 3) >> 2) & 0x03;;			//巡航使能
				ecu_data.cruise_acitve_status = (*(data + 3)) & 0x03;;			//巡航激活状态
				
				ecu_data.car_speed = *(uint16_t *)(data + 1);								//车速
			
				break;
			case 0x18FEE000:
				ecu_data.once_travel = *(uint32_t *)(data + 0) * 0.125 * 10;							//单次行驶距离
				ecu_data.total_travel = *(uint32_t *)(data + 4) * 0.125 * 10;  					//总里程
				break;
			
			case 0x18ff0800:     //潍柴ECU锁车
				
				break;
			case 0x18FEE900:
				ecu_data.once_fuel = (*(uint32_t *)(data + 0)) * 5;      					//单次油耗
				ecu_data.total_fuel = (*(uint32_t *)(data + 4)) * 5;  						//累计油耗
				break;	
			case 0x18FEF803:
				ecu_data.cluth_pressure = (*(data + 0) == 0xFF) ? 0 : *(data + 0);							  							//离合器压力
				ecu_data.gearbox_oil_level = (*(data + 1) == 0xFF) ? 0 : *(data + 1);					  								//变速箱齿轮油液位
				ecu_data.gearbox_oil_diff_pressure = (*(data + 2) == 0xFF) ? 0 : *(data + 2);	  								//变速箱齿轮油滤压差
				ecu_data.gearbox_oil_pressure = (*(data + 3) == 0xFF) ? 0 : *(data + 3);				  							//变速箱齿轮油压力
				ecu_data.gearbox_oil_temp = (*(uint16_t *)(data + 4) == 0xFFFF) ? 0 : *(uint16_t *)(data + 4);	//传动系机油温度
				ecu_data.gearbox_oil_level_status = ((*(data + 7) == 0xFF) ? 0 : *(data + 7) >> 4) & 0x0F;		  //变数变速箱齿轮油液位测量状态
				break;
			case 0x18FD9503:
				ecu_data.gearbox_oil_level_switch = (*(data + 0) >> 2) & 0x03;     																					//变速箱齿轮油液位开关状态
				ecu_data.gearbox_oil_filter_switch = (*(data + 0)) & 0x03;    																							//变速箱油过滤器限位开关
				ecu_data.gearbox_oil_out_temp = (*(uint16_t *)(data + 1) == 0xFFFF) ? 0 : *(uint16_t *)(data + 1);         //变速器变矩器油出口温度
				break;
			case 0x18DAF100:          //ISO27145  (国标环保故障)
				analysis_gb27145_dm(data);
				break;
			case 0x18feca00:	 	//DM1故障（以潍柴的为例）
			case 0x18ecff00:   		//
			case 0x18ebff00:   		//
				analysis_gb1939_dmc(can_id,data);
				break;
			default:
				break;
			
		}
		
		msg++;
	}
}



/****************************
**
*****************************/

void thread_entry_can_recv(void *parameter)
{
	uint16_t num = 0;
	
	for(;;)
	{
		vTaskDelay(1);
		num = rt_read_can_rx_buf(1,can1_rx_buf,sizeof(can1_rx_buf));
		if(num > 0 && num < 20)
		{
			analysis_can_data(can1_rx_buf,num);
		}
	}
}



