

#ifndef _APP_LTE_H_
#define _APP_LTE_H_


#define AT_CMD_WAIT_TIME_MS 10 //发送at指令后延时的时间 单位 ms
#define RING_BUFF_SIZE (UART1_BUFF_SIZE)

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_usart.h"
#include "drv_rtc.h"

#define SOCKET_TCP 0
#define SOCKET_UDP 1

#define RETRY 1
#define NOT_RETRY 0





struct at_info
{
	struct rt_semaphore 				at_cmd_sem; 					/* AT 命令信号量*/
  struct rt_semaphore 				send_sem;   					/* GPRS 任务信号量*/
	struct rt_semaphore					cmd_mutex;					  /* */
	
	uint32_t 										uart_tick;
	uint32_t										data_len;
};






/***********************/

struct lte_mq_t
{
	uint8_t cmd;           //参数类型
	uint32_t arg1;					//锁车级别
	uint32_t arg2;					//CMD	
	uint32_t arg3;					//SR
	uint32_t arg4;					//0
};




struct socket_down_str
{
	uint16_t len;
	uint8_t data[512];
};



struct lte_info
{
	uint8_t                 lte_state;              //LTE模块开机状态0：开机成功，1：开机失败
  uint8_t                 sim_state;              //SIM卡状态
  uint8_t 								iccid[25];              //ICCID
	uint8_t									imei[20];               //IMEI号
  uint8_t                 csq_value;              //网络信号值
  uint8_t 								net_reg_state;          //网络注册状态 1 本地网络 5 漫游网络
  uint8_t 								net_attach_state;       //网络附着状态  1 已附着网络 0未附着网络*/
  uint8_t                 net_init_state;
  struct rt_tm						net_time;               //LTE网络时间
	uint8_t								  local_ip[4];   					//本地ID
	uint8_t 								lte_module_state;				//
	uint8_t 								csq_counter;            //
	uint8_t 								ntp_state;							//
};




uint8_t read_lte_icc_id(uint8_t *source,uint8_t buf_size);
rt_mq_t	get_lte_link_mq(void);


uint8_t read_lte_csq(void);
uint8_t read_lte_sim_state(void);
uint8_t read_lte_imei_id(uint8_t *buf,uint8_t buf_size);
uint8_t read_lte_module_state(void);
uint8_t read_lte_attached_state(void);
uint8_t read_lte_net_init_state(void);
void close_lte_module(void);

void thread_entry_lte(void *parameter);

#endif   /*******/






