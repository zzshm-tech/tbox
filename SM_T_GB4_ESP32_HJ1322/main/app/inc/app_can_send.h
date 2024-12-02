


#ifndef _APP_CAN_SEND_H
#define _APP_CAN_SEND_H



struct can_send_mq_t
{
	uint32_t		 cmd;           //参数类型
	uint16_t 	 	 len;					//不同的参数类型，代表不同的解析方法
	uint8_t		 	 data[32];
};






struct vehicle_args_t
{
	uint8_t 				flag;
	uint32_t                parameter1;                  		//钵施然参数1								43			
	uint32_t				parameter2;							//钵施然参数2								44
	uint32_t 				parameter3;							//钵施然参数3								45

	uint8_t					lock_mon;                          //锁车功能，0:关闭功能，1：开启锁车功能
	uint8_t 				lock_state;                        //0：不锁车，1：一级锁车；2：二级锁车
	uint8_t 				lock_res_state;					   //锁车执行结果
	uint8_t 				mon_res_state;
	uint8_t 				gps_id[3];						   //
	uint8_t 				key[3];                   		   //
	uint8_t 				ecu_type;
};



struct lock_data_t
{
	uint8_t									mon_cmd;                //激活ECU锁车功能命令
	uint8_t									lock_cmd;               //激活仪表锁车功能命令（）
	uint8_t									cmd_res;                //命令是否应答平台状态
	uint8_t									step;
	uint8_t									index;
	uint8_t 								initiative_lock;               //主动锁车状态    00 解除限制  01 停机 10 限制转速或扭矩 11 未使用
	uint8_t 								passive_lock;                 //被动锁车状态     00 未锁车  01 停机 10 限转速或扭矩 11 未使用
	uint8_t 								active_status;								//激活状态         00 未激活   01 激活
	//uint8_t									check_status;									//校验状态         00 校验未通过 01 校验通过 10 未使用 11 未校验
	uint8_t 								emergency_unlock;             //紧急解锁状态     00 未使用 01 解锁 10 超时 11 未使用
	uint8_t 								emergency_start;              //紧急启动         00 未起动 01 起动 10 超时 11 未使用
  	//uint8_t 								alarm_status;                 //ECU故障状态      00 等待请求超时 01 等待key超时 11 正常，无超时 10 未使用
	//uint8_t 								com_status;                   //请求信号状态     00 未收到请求信号 01 收到请求信号
	
	uint8_t 								ecu_lock_res;             //锁车报文应答

	uint8_t 								gps_id[4];
	uint8_t 								seed[4];
	uint8_t 								mask[4];
	uint8_t 								check_code[8];             //key
	uint8_t 								bind_code[2];              //解绑密码
	uint8_t 								lock_code[4];              //主动锁车密码
	uint8_t 								fixed_key[2];			   //固定密钥  默认0xffff
	//uint8_t 								check_num;
	uint32_t 								aes128;
};




struct lock_back_t
{
	uint8_t									bind_seed[4];   				 //绑定seed
	uint8_t 								key_state;   						 //KEY状态
	uint8_t									check_status;						 //校验状态
	uint8_t 								initiative_lock;         //主动锁车状态    00 解除限制  01 停机 10 限制转速或扭矩 11 未使用
	uint8_t 								passive_lock;            //被动锁车状态
	uint8_t 								active_status;					 //激活状态 
};


uint8_t write_data_to_can(uint8_t *data,uint16_t size);
QueueHandle_t get_can_send_queue(void);
void thread_entry_can_send(void *parameter);

uint8_t read_vehicle_args_lock_mon(void);
uint8_t read_vehicle_args_lock_state(void);



uint8_t get_yuc_mon_state(void);
uint8_t get_yuc_lock_state(void);
uint8_t get_yuc_key_state(void);
uint8_t get_yuc_gps_state(void);

uint8_t get_qc_mon_state(void);     //
uint8_t get_qc_lock_state(void);	//
uint8_t get_qc_key_state(void);		//
uint8_t get_qc_gps_state(void);		//全柴GPS状态  握手
uint8_t read_yuc_ecu_type(void);
uint8_t erase_vehicle_args_info(void);

#endif



