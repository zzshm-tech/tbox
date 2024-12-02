


#ifndef _APP_CAN_RECV_H
#define _APP_CAN_RECV_H


#include <stdint.h>




#define DM1_MAX_NUM  40

#define CAN_RX_MSG_NUM 3




#pragma pack(1)

//仪表数据
struct tcu_data_str
{
	uint32_t							nc;       //
};
		
//仪表数据
struct hmi_data_str
{
	uint32_t			time;       //仪表计整车工作时间
	uint8_t				vin_value[20];    //仪表广播VIN
	uint8_t 			vin_cnt;
	uint8_t 			vin_state;    //
  uint16_t	   	ic_travel_speed;    					//仪表计行驶速度  预留
	uint32_t 			ic_travel_mileage;    				//仪表计算行驶速度  预留
	uint32_t  		ic_travel_duration;    				//仪表计运行时间  预留
};
	

//ECU数据
struct ecu_data_str
{
	uint16_t 							accumulator_vol;          //电源电压
	uint8_t 							water_temp;           		//冷却水温度
	uint16_t 							engine_rotate;		 				//发动机转速
	uint16_t 							engine_set_rotate;     		//发动机设定转速
	uint8_t 							engine_torque;						//发动机实际扭矩
	uint8_t 							fuel_temp;								//燃油温度
	uint16_t 							oil_temp;         				//机油温度
	uint8_t 							air_pressure;         		//大气压力
	uint16_t 							air_temp;									//大气温度
	uint8_t 							entered_air_temp;					//发动机进气温度
	uint16_t							way_temp;									//路面温度
	uint32_t							engine_work_time;					//发动机工作时间
	uint8_t 							accelerator;							//加速踏板行程值
	uint8_t 							water_out_temp;
	uint32_t 	   					total_fuel;  							//累计油耗
	uint8_t 							oil_position;							//机油液位
	uint8_t 							cool_position;						//冷却液位置
	uint8_t								relative_add_pressure;		//相对增压压力
	uint8_t 							absolute_add_pressure;		//绝对增压压力
	
	uint16_t 							entered_air_intake_temp;	//发动机进气歧管温度
	uint8_t     					lock_preparative_status;	//ECU锁车状态
	uint8_t								mon_status;   						//锁车功能状态
	uint8_t 							key_status;     					//KEY码状态
	uint8_t 							id_status;								//TBOX ID状态
	uint8_t 							engine_torque_mode;       //发动扭矩模式
	uint8_t 							driver_cmd_torque_percent;//驾驶员指令扭矩百分比
	uint8_t								oil_pressure;//机油压力
	uint8_t 							water_position;//冷却液液位
	uint8_t  							engine_need_torque_percent;	//发动机需求扭矩百分比 
	uint8_t 							rub_torque_percent;					//摩擦扭矩百分比
	uint8_t   						engine_set_rotate_offset;		//发动机目标转速不对称调整
	uint8_t 							engine_affiliated_torque_percent;	//发动机附件扭矩百分比
	uint8_t								low_oli_way_pre;					//低压油路压力
	uint8_t								svs_status_lamp;									//SVS（故障灯）灯状态
	uint8_t								environment_status_lamp;					//环境警告灯状态
	uint8_t								oil_water_pilot;    //油中有水指示
	uint8_t								wait_start_lamp;//等待起动指示灯 
	 
	uint8_t								engine_load;  						//发动机负荷
	uint16_t 							travel_speed;							//行驶速度
	uint32_t 							once_travel;							//单次行驶距离
	uint32_t 							total_travel;  						//总里程
	uint32_t 							once_fuel;      					//单次油耗

	uint8_t								relative_oil_pressure;		//相对机油压力
	uint8_t 							absolute_oil_pressure;		//绝对机油压力
	 
	uint16_t 							gearbox_oil_temp;					//变速箱油温
	uint16_t 							braking_air_pressure;			//制动气压
	uint8_t 							gearbox_pressure_switch;  //变速箱压力开关
	 
	uint8_t 							speed_status;							//速度限制状态
	uint8_t 							acc_switch_status ;     	  //踏板开关
	uint8_t 							acc_idling_status ;   		  //加速踏板怠速开关状态
	 
	uint8_t 							clutch_status ;						//离合开关状态
	uint8_t 							brake_status;							//刹车开关状态
	uint8_t 							cruise_speed;							//巡航控制巡航速度
	uint8_t 							cruise_control_status;    //巡航控制状态
	uint8_t 							cruise_enable_status;			//巡航使能
	uint8_t 							cruise_acitve_status;			//巡航激活状态
				
	uint8_t 							cluth_pressure;						//离合器压力
	uint8_t 							gearbox_oil_level;				//变速箱齿轮油液位
	uint8_t 							gearbox_oil_diff_pressure;	//变速箱齿轮油滤压差
	uint8_t 							gearbox_oil_pressure;			//变速箱齿轮油压力
	uint8_t 							gearbox_oil_level_status;	//变数变速箱齿轮油液位测量状态
	 
	uint8_t 							gearbox_oil_level_switch;    	//变速箱齿轮油液位开关状态
	uint8_t 							gearbox_oil_filter_switch;    //变速箱油过滤器限位开关
	uint16_t 							gearbox_oil_out_temp;         //变速器变矩器油出口温度
	uint8_t								lock_actual_status;			//ECU锁车状态
	uint8_t						    	cold_boot_status;
	uint8_t 							crank_pressure;   				//曲轴箱压力(预留)
	uint8_t 							cool_pressure;  					//冷却液压力(预留)
	uint8_t 							engine_nacelle_temp;      //发动机舱内温度

	
	uint16_t	 						car_speed;								//车速
	uint8_t								friction_torque;					//摩擦扭矩
	uint16_t							engine_fuel_flow;					//发动机燃料流量
	uint16_t							scr_upstream_nox;					//SCR上游NOx传感器输出值
	uint16_t							scr_downstream_nox;				//SCR下游NOx传感器输出值
	uint8_t 							reactant_allowance; 			//反应剂余量
	uint16_t 							enter_volume;							//进气量
	uint16_t 							scr_entrance_temp; 				//SCR入口温度
	uint16_t 							scr_exit_temp;						//SCR出口温度
	uint16_t							dpf_diffPressure;					//DPF压差
	uint8_t								coolant_temp;							//冷却液温度
	uint8_t 							fuel_percent;						//燃油液位
	uint16_t 							egr_opening ;							//EGR阀开度
	uint16_t 							egr_setting;							//EGR设定值
	uint16_t 							fuel_pressure;						//燃油压力
	
	uint8_t 							mil_light_state;         //排放报警灯状态;
};


/************** DM1信息 **************/

struct md1                     
{
	uint8_t             sys_num; 			    //当前存在DM1的电控单元数量，有发动机，整车系统，变速箱，仪表四种
	uint8_t             current_index[1];       //当前已经收到的包数
	uint8_t	            bam_flag[1];	        //已收到该数据多包声明数据包
	uint8_t             msg_num[1];             //对应的电控单元ID，消息包总数，[0]--发动机，[1]--整车系统，[2]--变速箱，[3]--仪表  ,消息包数
	uint8_t             data_len[1];            //每个故障单元对应的数据长度
	uint8_t             data[1][160];            //对应电控单元的总字节长度，直接单包CAN数据，为 报数N*8  ,最多5包数据，8个故障码
	uint8_t             led_state;              //
} ;



/*******************************/

struct dm1_filter_str
{
    uint32_t fmi;
    uint32_t spn;
};
		


/**************DM1 故障码*****************/
struct gb1939_dm_str
{
	uint16_t num;
	uint32_t spn[DM1_MAX_NUM];
	uint32_t fmi[DM1_MAX_NUM];
	uint8_t serial_code[DM1_MAX_NUM * 4];
};




struct gb27145_dm_str
{
	uint8_t 	light_state;		//灯的状态
	uint16_t 	num;				//数量
	uint8_t  	cls[20];      	//故障等级
	uint16_t 	dtco[20];         	//暂时定义成 SPN 
	uint8_t 	ftp[20];         	//暂时定义成FMI
	uint8_t     status[20];
	uint8_t 	serial[100];        //
};



#pragma pack()


uint8_t read_ecu_data(struct ecu_data_str *data);



uint8_t read_can_data_current_actual_lock_state(void);
uint8_t read_can_ecu_mon_status(void);
uint8_t read_can_ecu_key_status(void);
uint8_t read_can_ecu_id_status(void);	
uint8_t read_can1_total_fuel(void);
uint32_t read_can1_engine_work_duration(void);
uint8_t read_can_connect_state(void);
uint8_t read_mil_light_state(void);
void read_ecu_dm1_data(struct gb1939_dm_str *data);
uint16_t  read_gb27145_dm_serial(uint8_t *buf,uint16_t size);
uint8_t read_gb27145_light_state(void);
uint16_t  read_gb1939_dm_serial(uint8_t *buf,uint16_t size);


void view_gb27145_dm(void);

void thread_entry_can_recv(void *parameter);




#endif



