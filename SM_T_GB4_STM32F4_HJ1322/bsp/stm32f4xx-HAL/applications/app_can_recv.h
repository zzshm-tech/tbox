


#ifndef _APP_CAN_RECV_H
#define _APP_CAN_RECV_H



#define DM1_1939_NUM  20



#include <rtthread.h>
#include "app_gb4.h"



enum can_id_type
{
    SEND_EXTID = 1,
    SEND_STDID
};

#define _SET_EXTID(id) ((uint16_t)((uint32_t)id >> 13)), \
                       ((uint16_t)((uint32_t)id << 3) | CAN_ID_EXT)

#define _SET_STDID(id) (uint16_t)((uint32_t)id << 5), \
                       0

#define __CAN_Filter(index, mode, scale, list_id, mask_id) \
    {                                                      \
        index, mode, scale, list_id, mask_id               \
    }



/*****************************************
**
**	
*******************************************/
#pragma pack(1)

struct tcu_data_str
{
	uint32_t							nc;
};
		
//仪表数据
struct hmi_data_str
{
	uint32_t							time;       //仪表计整车工作时间
	uint8_t								vin_value[20];    //仪表广播VIN
	uint8_t 							vin_cnt;
	uint8_t 							vin_state;    //
};
	


//ECU数据
struct ecu_data_str
{
	 uint16_t 							accumulator_vol;          //电源电压
	 uint16_t 							engine_rotate;		 				//发动机转速
	 uint8_t 								engine_torque;						//发动机实际扭矩
	 uint8_t 								fuel_temp;								//燃油温度
	 uint16_t 							oil_temp;         				//机油温度
	 uint8_t 								air_pressure;         		//大气压力
	 uint16_t 							air_temp;									//大气温度
	 uint8_t 								entered_air_temp;					//发动机进气温度
	 uint32_t								engine_work_time;					//发动机工作时间
	 uint8_t 								accelerator;							//加速踏板行程值
	 uint8_t 								fuel_wear;                //油耗
	 uint32_t 	   					total_fuel;  							//累计油耗
	 uint8_t 								oil_position;							//机油液位
	 uint8_t 								cool_position;						//冷却液位置
	 uint16_t								relative_add_pressure;		//相对增压压力
	 uint16_t 							absolute_add_pressure;		//绝对增压压力
	 uint16_t 							entered_air_intake_temp;	//发动机进气歧管温度
	 uint8_t 							  rub_torque_percent;					//摩擦扭矩百分比
	 uint8_t   						  engine_set_rotate_offset;		//发动机目标转速不对称调整
	 uint8_t								venting_flow_lsb;					//排气质量流量(LSB)
   uint8_t								venting_flow_msb;					//排气质量流量(MSB)
	 uint8_t								low_oli_way_pre;					//低压油路压力
	 uint8_t								svs_status_lamp;									//SVS（故障灯）灯状态
	 uint8_t								environment_status_lamp;					//环境警告灯状态
	 uint8_t								stop_status_lamp_red;				//红色停止灯状态
	 uint8_t								mil_status_lamp;						//MIL（OBD）灯状态
	 uint8_t								svs_flicker_status_lamp;		//SVS（故障灯）灯闪烁状态
	 uint8_t								oil_water_pilot;    //油中有水指示
	 uint8_t								wait_start_lamp;//等待起动指示灯 
	 uint8_t								engine_load;  						//发动机负荷
	 uint16_t 							travel_speed;							//行驶速度
	 uint32_t 							once_travel;							//单次行驶距离
	 uint32_t 							total_travel;  						//总里程
	 uint32_t 							once_fuel;      					//单次油耗
	 uint16_t								relative_oil_pressure;		//相对机油压力
	 uint16_t 							absolute_oil_pressure;		//绝对机油压力
	 uint16_t 							gearbox_oil_temp;					//变速箱油温
	 uint16_t 							braking_air_pressure;			//制动气压
	 uint8_t 								gearbox_pressure_switch;  //变速箱压力开关
	 uint8_t 								clutch_status ;						//离合开关状态
	 uint8_t 								cruise_speed;							//巡航控制巡航速度
	 uint16_t 							gearbox_oil_out_temp;         //变速器变矩器油出口温度
	 uint8_t								lock_actual_status;			//ECU锁车状态
	 uint8_t						    cold_boot_status;
	 uint8_t 								cool_pressure;  					//冷却液压力(预留)
	 uint16_t	   						ic_travel_speed;    					//仪表计行驶速度  预留
	 uint32_t 							ic_travel_mileage;    				//仪表计算行驶速度  预留
	 uint32_t  							ic_travel_duration;    				//仪表计运行时间  预留
	 uint16_t	 							car_speed;								//车速
	 uint8_t								friction_torque;					//摩擦扭矩
	 uint16_t								engine_fuel_flow;					//发动机燃料流量
	 uint16_t								scr_upstream_nox;					//SCR上游NOx传感器输出值
	 uint16_t								scr_downstream_nox;				//SCR下游NOx传感器输出值
	uint8_t 								reactant_allowance; 			//反应剂余量
	uint16_t 								enter_volume;							//进气量
	uint16_t 								scr_entrance_temp; 				//SCR入口温度
	uint16_t 								scr_exit_temp;						//SCR出口温度
	uint16_t								dpf_diffPressure;					//DPF压差
	uint8_t									coolant_temp;							//冷却液温度
	uint16_t 								egr_opening ;							//EGR阀开度
	uint16_t 								egr_setting;							//EGR设定值
	
	uint8_t 								mil_light_state;        //排放报警灯状态;
	uint8_t 								sc_mon_state;   				//上柴实际监控状态
	uint8_t 								sc_lock_state;   				//上柴实际锁车状态
	uint32_t 								engine_wt_counter;     	//发动机工时报文计数器
	uint16_t								engine_breakdown;   		//发动机故障灯
	uint16_t								gearbox_out_rotate;  		//变速箱输出轴转速
	uint8_t 								drain_off_fault_class;  //排放系统驾驶员严重程度指示灯
	uint8_t									dpf_build_light;     		//
	uint8_t 								dpf_forbid_light;				//
	uint8_t 								fnr_active_light;				//
	uint8_t 								auto_model_light;				//
	uint8_t 								power_dis_lilght;				//
	uint8_t 								gearbox_filter_light;  	//
	uint8_t 								gearbox_switch;    			//档位
	uint16_t 								gearbox_oil_temp_e;  		//电控变速箱油温
	uint8_t									engine_control_model;		// 发动机控制模式
	uint8_t 							  gearbox_type_e;         //	电控变速箱类型
	uint8_t 								fuel_percent;       				//燃油位
	uint32_t								fuel_wt_counter;				//
	uint16_t								max_ref_torque;					//最大发动机参考扭矩
};

/*******************************
**	冷却风扇
*******************************/

struct cool_fan_data
{
	uint16_t    fan_one_speed;     //水冷风扇1
	uint16_t 		fan_two_speed;     //水冷风扇2
	uint16_t 		fan_three_speed;   //油冷风扇1
	uint16_t 		fan_four_speed;    //油冷风扇2
	uint8_t     fan_one_status;     //水冷风扇1
	uint8_t 		fan_two_status;    //水冷风扇2
	uint8_t 		fan_three_status;  //油冷风扇1
	uint8_t 		fan_four_status;   //油冷风扇2
};







/*********************
**
***********************/

struct can_info_str
{
  struct rt_semaphore 				can1_send_sema; 			// can 报文发送信号量
	struct rt_semaphore 				can2_send_sema;				// can数据信号量
	
	uint32_t										can1_recv_cnt;       	// CAN1 接收计数器
	uint32_t										can2_recv_cnt;        // CAN1 接收计数器
	
	uint32_t										can1_send_cnt;        		  // CAN1 接收计数器
	uint32_t										can2_send_cnt;        		  // CAN1 接收计数器
	
	uint8_t 										can1_state;						// CAN2链接状态,
	uint8_t 										can2_state;      	// can 连接状态 连接正常:
};




/**************DM1 故障码*****************/

struct gb1939_dm_str
{
	uint16_t num;
	uint32_t spn[DM1_1939_NUM];
	uint32_t fmi[DM1_1939_NUM];
	uint8_t serial_code[DM1_1939_NUM * 4 + 10];
};


/****************************
**	备份保留数据
*****************************/

struct ecu_hold_t
{
	uint8_t  										ecu_type;        						//ECU类型
	
	uint8_t 										emissions_type;							//排放类型
	
	uint8_t											wc_lock_state;   						//实际车状态 
	uint8_t 										wc_lock_bank;         			//实际锁车级别	
	
	uint8_t 										wc_mon_state;   						//锁车功能状态 
	uint8_t 										wc_key_state;     					//KEY码状态
	uint8_t 										wc_id_state;								//TBOX ID状态
	uint8_t											wc_pre_lock_state;					//ECU预锁车状态
	
	uint8_t 										sc_mon_state;   						//锁车功能状态 
	uint8_t 										sc_key_state;     					//KEY码状态
	uint8_t 										sc_id_state;								//TBOX ID状态
	uint8_t											sc_pre_lock_state;					//ECU预锁车状态
	uint8_t 										sc_lock_state;	
	
	struct gb1939_dm_str				ecu_dm1_qb4;	 			//DM1 接收到的故障码 把总线上的 所有DM1 全部解析出来，
	struct gb1939_dm_str				ecu_dm1_gb4;	 			//DM1 过滤之后 环保的数据，
	struct gb1939_dm_str				tcu_dm1_gb4;	 			// TCU变速箱故障
	
	uint8_t 										crc_value;                	//校验值
};





struct faultcode_str
{
    uint32_t fmi;
    uint32_t spn;
};
		



//
typedef union  
{
	uint16_t 							value;
	uint8_t 							byte[2];
}int16_to_char;


//
typedef union  
{
	uint32_t    					value;
	uint8_t 							byte[4];
}int32_to_char;



/************** DM1信息 **************/

struct md1                     
{
	uint8_t             sys_num; 			    //当前存在DM1的电控单元数量，有发动机，整车系统，变速箱，仪表四种
	uint8_t             current_index[2];       //当前已经收到的包数
	uint8_t	            bam_flag[2];	        //已收到该数据多包声明数据包
	uint8_t             msg_num[2];             //对应的电控单元ID，消息包总数，[0]--发动机，[1]--整车系统，[2]--变速箱，[3]--仪表  ,消息包数
	uint8_t             data_len[2];            //每个故障单元对应的数据长度
	uint8_t             data[2][166];            //对应电控单元的总字节长度，直接单包CAN数据，为 报数N*8  ,最多5包数据，8个故障码
	uint8_t             led_state;              //
} ;






#pragma pack()


uint8_t read_engine_torque(void);
uint8_t read_friction_torque(void);
uint16_t read_engine_fuel_flow(void);
uint16_t read_scr_entrance_temp(void);
uint16_t read_scr_downstream_nox(void);
uint16_t read_enter_volume(void);


uint16_t read_ecu_data(struct ecu_data_str *buf,uint16_t buf_size);
uint8_t read_current_lock_state(void);
uint8_t read_wc_mon_state(void);
void can1_send_data(enum can_id_type type, uint32_t id, uint8_t *data,uint8_t num);
void can2_send_data(enum can_id_type type, uint32_t id, uint8_t *data);
uint16_t read_engine_rotate(void);
uint8_t read_current_meter_mon_state(void);
uint8_t read_wc_pre_lock_state(void);
uint8_t read_can_connect_state(uint8_t ch);
uint8_t read_ecu_type(void);
uint8_t read_ecu_lock_state(void);
void can_port_close(void);
uint8_t read_ecu_mon_status(void);
uint8_t read_ecu_key_status(void);
uint8_t read_ecu_id_status(void);
uint32_t read_can1_total_fuel(void);
uint16_t read_ecu_can_data(uint8_t *buf,uint16_t buf_size);
uint8_t read_mil_light_state(void);
uint16_t  read_gb4_dm_serial(uint8_t *buf,uint16_t size);
uint16_t  read_qb4_dm_serial(uint8_t *buf,uint16_t size);
uint16_t  read_tcu_dm_serial(uint8_t *buf,uint16_t size);
uint32_t read_engine_wt_counter(void);
uint32_t read_engine_work_time(void);

uint8_t read_ecu_sc_mon_state(void);
void read_ecu_qb4_dm_data(struct gb1939_dm_str *data);
void read_ecu_gb4_dm_data(struct gb1939_dm_str *data);
void read_tcu_gb4_dm_data(struct gb1939_dm_str *data);
uint32_t read_total_fuel(void);
uint32_t read_fuel_wt_counter(void);
uint16_t 	write_buf_to_can1(uint8_t *buf,uint16_t size);
void thread_entry_can(void *parameter);						//
uint16_t  read_max_ref_torque(void);
uint8_t read_emissions_type(void);


#endif






