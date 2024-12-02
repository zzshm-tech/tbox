



#ifndef _APP_LTE_H
#define _APP_LTE_H



#include "drv_rtc.h"

struct lte_info
{
    uint8_t                                 lte_state;              //LTE模块开机状态0：开机成功，1：开机失败
    uint8_t                                 sim_state;              //SIM卡状态
    uint8_t 								iccid[25];              //ICCID
	uint8_t									imei[20];               //IMEI号
    uint8_t                                 csq_value;              //网络信号值
    uint8_t 								net_reg_state;          //网络注册状态 1 本地网络 5 漫游网络
    uint8_t 								net_attach_state;       //网络附着状态  1 已附着网络 0未附着网络*/
    uint8_t                                 net_init_state;
    struct rt_tm						    net_time;               //LTE网络时间
	uint8_t								    local_ip[4];   //本地ID
    //uint8_t                                 socket_state[4];
};




struct lte_mq_t
{
	uint8_t cmd;           //参数类型
	uint32_t arg1;					//锁车级别
	uint32_t arg2;					//CMD	
	uint32_t arg3;					//SR
	uint32_t arg4;					//0
};





uint8_t read_lte_icc_id(uint8_t *buf,uint8_t buf_size);
uint8_t read_lte_csq(void);
uint8_t read_lte_imei_id(uint8_t *buf,uint8_t size);
uint8_t read_lte_net_reg_state(void);
uint8_t read_lte_attached_state(void);
uint8_t read_lte_sim_state(void);
uint8_t read_lte_init_state(void);
//uint8_t read_lte_sokcet_state(uint8_t index);
uint8_t read_lte_net_init_state(void);
uint8_t at_config_hex_recv(void);
void thread_entry_lte(void *parameter);
void view_lte_info(void);
QueueHandle_t  get_lte_qu(void);
#endif


