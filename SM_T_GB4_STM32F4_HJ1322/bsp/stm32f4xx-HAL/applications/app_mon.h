


#ifndef _APP_MON_H_
#define _APP_MON_H_



#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>



/*************************

**************************/
union di_state
{
	struct
	{
		uint8_t acc 	:1;
		uint8_t moto	:1;
		uint8_t q2 		:1;
		uint8_t q3 		:1;
		uint8_t q4 		:1;
		uint8_t q5 		:1;
		uint8_t q6 		:1;
		uint8_t q7 		:1;
	} bit;		
	uint8_t vaule;
};

/** 设备内采集到的DI，AI **/

struct sys_input_str
{
	union di_state    	io_state;
	uint8_t 						acc;								//ACC状态
	uint8_t							moto;								//MOTO信号（暂时理解为发动机启动信号）
	uint8_t							di_1;								//数字量输入信号
	uint8_t							di_2;               //数字量输入信号
	uint8_t							shell_state;        //外壳状态
	uint8_t							ant_state;          //定位天线状态，0：正常；1：断开；2：短路
	
	uint8_t							chrg_state;         //充电
	uint8_t 						stdby_state;        //充电状态
	uint8_t							batter_state;				//电池状态
	uint16_t 						batter_vol;					//电池电压
	uint16_t						power_vol;					//外部供电电压
	uint16_t						acc_vol;						//ACC(电锁信号)电压
	uint16_t						board_vol;          //主工作电源
	uint16_t						muc_temp;           //单片机温度，设备温度
	uint8_t							alarm;							//报警
};


//设备报警值
struct sys_alarm_str
{
    struct rt_semaphore alarm_sign;
    uint32_t value;  //报警值
		uint8_t state;   //备份值
};



/****************************
**
*****************************/

struct sys_input_back_t
{
	uint16_t 						verfy;
	uint16_t						power_vol;					//外部供电电压
	
};


uint16_t read_in_power_vol(void);
uint16_t read_in_batter_vol(void);
uint8_t read_in_acc_state(void);

uint16_t read_ai_board_vol(void);
uint8_t read_di_stdby_state(void);
uint8_t read_di_charg_state(void);
uint16_t read_in_acc_vol(void);
uint8_t read_di_batter_state(void);
uint32_t read_in_alarm(void);
uint8_t read_input_shell_state(void);
uint8_t read_input_ant_state(void);
uint8_t read_in_moto_state(void);
uint8_t read_in_io_state(void);
uint16_t read_in_power_back(void);
uint8_t load_sys_input_back(void);

void process_input(void);
void close_input(void);
void open_input(void);

#endif



