


#ifndef _APP_CAN_SEND_H
#define _APP_CAN_SEND_H


#include <rtthread.h>


#pragma pack(1)
/*********************************
**	设备内部记录锁车状态结构体
**********************************/
struct lock_expect_str
{
	uint8_t 							flag;
	uint8_t 							lock_expect_state;   					//0:解锁；1：锁车;
	uint8_t			 					mon_expect_state;    					//期望监控状态  1：锁车功能打开;2:锁车功能关闭
	uint8_t								lock_cmd_res_state;						//锁车命令回复状态
	uint8_t								mon_cmd_res_state;           	//激活命令回复状态
};



/***********************************
** 锁车队列
************************************/

struct rt_can_event
{
	uint8_t 	cmd;           //参数类型
	uint32_t 	arg1;						//锁车级别
	uint32_t 	arg2;						//CMD	
	uint32_t 	arg3;						//SR
	uint32_t 	arg4;						//0
	uint32_t 	arg5;
	
	uint8_t   len;
	uint8_t		data[32];
};



/***********************************
** 
************************************/

struct lock_data_str
{
	uint8_t									step;                    //ECU
	uint8_t									mon_cmd;                 //激活ECU锁车功能命令
	uint8_t									index;                   //ECU命令下发次数
	uint8_t									cmd_res;                 //命令状态
	uint8_t									lock_cmd;                //
	uint8_t									res_status;							 //
	
	uint8_t 								emergency_unlock;        //紧急解锁状态     00 未使用 01 解锁 10 超时 11 未使用
	uint8_t 								emergency_start;         //紧急启动         00 未起动 01 起动 10 超时 11 未使用
  //uint8_t 							  alarm_status;          //ECU故障状态      00 等待请求超时 01 等待key超时 11 正常，无超时 10 未使用
	//uint8_t 								com_status;            //请求信号状态     00 未收到请求信号 01 收到请求信号
	uint8_t 								ecu_lock_res;            //锁车报文应答
	uint8_t 								gps_id[4];							 //
	uint8_t 								seed[4];        				 //当前seed
	uint8_t 								check_code[8];           //key
	uint8_t 								bind_code[2];            //解绑密码
	uint8_t 								lock_code[4];            //主动锁车密码
	uint8_t 								fixed_key[2];			   		 //固定密钥  默认0xffff
	
};



struct lock_back_t
{
	uint8_t									bind_seed[4];   				 //绑定seed
	uint8_t 								key_state;   						 //KEY状态
	uint8_t									check_status;						 //校验状态
	uint8_t 								initiative_lock;         //主动锁车状态    00 解除限制  01 停机 10 限制转速或扭矩 11 未使用
	uint8_t 								passive_lock;            //被动锁车状态
	uint8_t 								active_status;					 //激活状态 
	
	uint8_t 								flag;
};


#pragma pack()


rt_mq_t	get_can_lock_mq(void);

void thread_entry_can_send(void *parameter);
uint8_t read_lock_expect_state(void);
uint8_t read_expect_mon_state(void);
uint8_t read_lock_expect_bank(void);
uint8_t read_vcu_pre_lock_status(void);



uint8_t get_yuc_mon_state(void);
uint8_t get_yuc_lock_state(void);
uint8_t get_yuc_key_state(void);
uint8_t get_yuc_gps_state(void);


#endif






