

#include <rtthread.h>
#include <rtdevice.h>


#include "drv_iwg.h"
#include "drv_timer.h"
#include "board.h"
#include "drv_adc.h"
#include "drv_gpio.h"
#include "app_mon.h"
#include "app_gnss.h"
#include "app_can_recv.h"
#include "app_lte.h"
#include "app_files.h"


/************* 本地全局变量 **************/

static struct sys_input_back_t			sys_input_back;

static struct sys_alarm_str					sys_alrm_value;             //

static struct sys_input_str 				sys_input_data;							//

static rt_device_t  								adc_dev = RT_NULL;



/********************
**	返回外部供电电源电压
**	保留一位有效小数
************************/

uint16_t read_in_power_vol(void)
{
	uint16_t rv;
	
	rv = sys_input_data.power_vol;
	
	return rv;
}



/***************************
**	
****************************/

uint16_t read_in_power_back(void)
{
	uint16_t rv;
	
	rv = sys_input_back.power_vol;
	
	return rv;
}


/********************
**	返回电池电压，
**	保留一位有效小数，
************************/

uint16_t read_in_batter_vol(void)
{
	uint16_t rv;
	
	rv = 40;//sys_input_data.batter_vol;            //设备内部电池电压
	
	return rv;
}


/*******************************
**	反馈ACC电源电压
*********************************/

uint16_t read_in_acc_vol(void)
{
	uint16_t rv;
	
	rv = sys_input_data.acc_vol;
	
	return rv;
}



/*********************************
**	返回主电源电压
**	外部供电DC-DC之后
**********************************/

uint16_t read_ai_board_vol(void)
{
	uint16_t rv;
	
	rv = 45;// sys_input_data.board_vol;    //主工作电压
	
	return rv;
}


/********************
**	ACC状态
**	1:打开
**	0：关闭
**	判断条件：
**	24V系统  大于20V
**	
************************/
uint8_t read_in_acc_state(void)
{
	uint8_t rv;
	
	if(sys_input_data.acc == 0 || sys_input_data.power_vol <= 90)
	//if(sys_input_data.acc == 0 || sys_input_data.power_vol <= 90 || read_can1_connect_state() > 0)     //英轩使用
	{
		rv = 0;
	}
	else
	{
		rv = 1;
	}
	
	return rv;
}


/*******************************
**	moto信号
*******************************/

uint8_t read_in_moto_state(void)
{
	uint8_t rv;
	
	if(sys_input_data.moto == 0 || sys_input_data.power_vol <= 90)
	{
		rv = 0;
	}
	else
	{
		rv = 1;
	}
	
	return rv;
}



/*******************************
**	moto信号
*******************************/

uint8_t read_in_io_state(void)
{
	uint8_t rv;
	
	rv = sys_input_data.io_state.vaule;
	
	return rv;
}




/********************
**	外壳状态
********************/

uint8_t read_input_shell_state(void)
{
	uint8_t rv;
	
	rv = sys_input_data.shell_state;
	
	return rv;
}



/**************************
**	返回定位天线状态
***************************/

uint8_t read_input_ant_state(void)
{
	uint8_t rv;
	
	rv = sys_input_data.ant_state;
	
	return rv;
}



/*****************************
**	返回充电是否已经完成
*******************************/

uint8_t read_di_stdby_state(void)
{
	uint8_t rv;
	
	if(sys_input_data.stdby_state == 0)
		rv = 1;
	else
		rv = 0;
	
	return rv;
}




/*****************************
**	充电状态指示
*******************************/

uint8_t read_di_charg_state(void)
{
	uint8_t rv;
	
	if(sys_input_data.chrg_state == 0)
		rv = 1;
	else
		rv = 0;
	
	return rv;
}



/******************************
**	返回电池状态
*******************************/

uint8_t read_di_batter_state(void)
{
	uint8_t rv;
	
	rv = sys_input_data.batter_state;
	
	return rv;
}


/****************************
**	设备终端报警
*****************************/

uint32_t read_in_alarm(void)
{
	uint32_t rv;

	rv = sys_alrm_value.value;
	
	return rv;
}





/****************************************************************
**	写报警值
**	
****************************************************************/

static void write_alarm_value(char bit,unsigned char n)
{
	if(n > 0)
		bitset(sys_alrm_value.value,bit);
	else
		bitclr(sys_alrm_value.value,bit);
}






/***********************************
**	采集外部输入
************************************/

void process_input(void)
{
	struct adc_result adc_value = {0};
	signed int tmp;
	
	
	adc_dev= rt_device_find("adc");
	
	sys_input_data.alarm = 0;
	rt_device_control(adc_dev,ADC_GET_RESULT,&adc_value);
		
	sys_input_data.acc_vol = adc_value.acc_vol;          //acc输入电压
	sys_input_data.board_vol = adc_value.board_vol;       //板卡电压
	sys_input_data.batter_vol = adc_value.battery;					//外部模拟量输入，未使用
	sys_input_data.power_vol = adc_value.power;           //外部供电电压
		
	sys_input_data.muc_temp = adc_value.temperature;     //单片机温度 （可以整个设备的工作温度）
		
	
	//拆除报警
	if(sys_input_data.power_vol <= 90 || (sys_input_data.acc > 0 && read_can_connect_state(1) > 0))        //报警电源 (拆除)
	{
		write_alarm_value(0,1);
	}
	else
	{
		write_alarm_value(0,0);
	}
	
	if(adc_value.shell_vol > 2000)               //外壳状态
	{
		sys_input_data.shell_state = 1;
		write_alarm_value(1,1);
	}
	else
	{
		sys_input_data.shell_state = 0;
		write_alarm_value(1,0);
	}
	
	//定位天线故障
	tmp = adc_value.ant_h_vol - adc_value.ant_l_vol;
	if(tmp < 50 || tmp > 1000)    
	{
		write_alarm_value(2,1);
		sys_input_data.ant_state = 1;
	}
	else
	{
		write_alarm_value(2,0);
		sys_input_data.ant_state = 0;
	}
	
	//接收不到CAN数据报警
	if(sys_input_data.acc > 0 && read_can_connect_state(1) > 0)
	{
		write_alarm_value(3,1);
	}
	else
	{
		write_alarm_value(3,0);
	}
	
	//锂电池报警
	if(sys_input_data.batter_vol < 25)    //判断电池  OK 
	{
		sys_input_data.batter_state = 1; 
		write_alarm_value(4,1);    
	}
	else
	{
		write_alarm_value(4,0);
		sys_input_data.batter_state = 0;
	}
		
	//加密芯片工作状态
	if(read_acl16_work_state() == 0)
	{
		write_alarm_value(5,0);
	}
	else
	{
		write_alarm_value(5,1);
	}
		
	//文件系统挂载状态
	if(read_files_sys_state() == 0)       //文件系统挂载状态
	{
		write_alarm_value(8,1);
	}
	else
	{
		write_alarm_value(8,0);
	}
		     						
	sys_input_data.io_state.bit.acc = rt_read_acc_state();
	sys_input_data.io_state.bit.moto = rt_read_moto_state();
	sys_input_data.moto = rt_read_moto_state();  			//MOTO 状态
	sys_input_data.acc = rt_read_acc_state();					//ACC状态（电锁状态）
	sys_input_data.di_1 =  rt_read_di_1_state();			//DI输入1
	sys_input_data.di_2 = rt_read_di_2_state();				//DI输入2

	if(sys_input_data.power_vol > 80)
	{
		sys_input_back.power_vol = sys_input_data.power_vol;
	}
}



/*************************
**
**************************/

void open_input(void)
{
	adc_dev= rt_device_find("adc");
	if(adc_dev != NULL)
	{
		rt_device_control(adc_dev,ADC_SWITCH_ON,NULL);
	}
}



/*************************
**
**************************/

void close_input(void)
{
	adc_dev= rt_device_find("adc");
	if(adc_dev != NULL)
	{
		rt_device_control(adc_dev,ADC_SWITCH_OFF,NULL);
	}
}



