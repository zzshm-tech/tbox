







#ifndef _APP_IN_H
#define _APP_IN_H

#include <stdint.h>


/*************************

**************************/
union di_state
{
	struct
	{
		uint8_t acc 	:1;    //ACC状态
		uint8_t moto	:1;     //MOTO信号
		uint8_t q2 		:1;     //BIt2:0低电平，1高电平（预留)
		uint8_t q3 		:1;		//Bit3:0低电平，1高电平（预留）
		uint8_t shell 	:1;		//Bit4:0外壳关闭，1：外壳打开
		uint8_t q5 		:1;
		uint8_t q6 		:1;
		uint8_t q7 		:1;
	} bit;		
	uint8_t value;
};

/** 设备内采集到的DI，AI **/

struct input_str
{
	union di_state    	io_state;
	uint8_t 			acc_state;				//ACC状态
	uint8_t				moto_state;				//MOTO信号（暂时理解为发动机启动信号）
	uint8_t				di_1;				//数字量输入信号
	uint8_t				di_2;               //数字量输入信号
	uint8_t				ant_state;          //定位天线状态，0：正常；1：断开；2：短路
	uint8_t 			shell_state;
	uint8_t				charge_state;         //充电
	uint8_t 			stdby_state;        //充电状态
	uint8_t				batter_state;		//电池状态
	uint16_t 			batter_vol;			//电池电压
	uint16_t			power_vol;		    //外部供电电压
	uint16_t			acc_vol;			//ACC(电锁信号)电压
	uint16_t			board_vol;          //主工作电源
	uint16_t			muc_temp;           //单片机温度，设备温度
	uint8_t				alarm;							//报警
};





uint16_t read_in_power_vol(void);
uint16_t read_in_board_vol(void);
uint8_t  read_in_acc_state(void);
uint16_t read_in_batter_vol(void);
uint8_t read_in_shell_state(void);
uint8_t  read_in_moto_state(void);
uint8_t read_in_charg_state(void);
uint8_t read_in_stdby_state(void);
uint8_t  read_in_batter_state(void);
uint16_t read_in_acc_vol(void);
uint8_t read_in_io_state(void);						//IO状态
uint8_t read_in_alarm(void);


void process_in(void);

#endif




