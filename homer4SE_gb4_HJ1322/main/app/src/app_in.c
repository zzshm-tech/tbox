

#include "board.h"
#include "drv_in.h"
#include "drv_gpio.h"


#include "app_in.h"
#include "app_gnss.h"
#include "app_can_recv.h"
#include "app_gb4.h"
#include "app_files.h"




static struct input_str     input_data;




/******************************
**  外部输入电压
*******************************/

uint16_t read_in_power_vol(void)
{
    uint16_t rv;

    rv = input_data.power_vol;

    return rv;
}



/******************************
**  内部电池电压
*******************************/

uint16_t read_in_batter_vol(void)
{
    uint16_t rv;

    rv = input_data.batter_vol;
    
    return rv;
}   



/******************************
**  内部电池电压
*******************************/

uint16_t read_in_board_vol(void)
{
    uint16_t rv;

    rv = input_data.batter_vol;
    
    return rv;
} 


/******************************
**  ACC输入状态
*******************************/

uint8_t  read_in_acc_state(void)
{
    uint8_t rv;

    rv = input_data.acc_state;
    //rv = 1;
    return rv;

}   										// ACC状态
	

/******************************
**  Moto输入状态
*******************************/

uint8_t  read_in_moto_state(void)
{
    uint8_t rv;

    rv = input_data.moto_state;

    return rv;

} 


/******************************
**  返回外壳状态
*******************************/

uint8_t read_in_shell_state(void)
{
    uint8_t rv;

    rv = input_data.shell_state;

    return rv;
}


/****************************
**  充电状态
*****************************/

uint8_t read_in_charg_state(void)		  //充电状态			    31 			
{
    uint8_t rv;

    rv = input_data.charge_state;

    return rv;
}
	

/****************************
**  充电电量状态
*****************************/

uint8_t read_in_stdby_state(void)      //电池充满状态     32 
{
    uint8_t rv;

    rv = input_data.stdby_state;
    
    return rv;
}


/****************************
**  返回TBOX内部电池状态
*****************************/

uint8_t  read_in_batter_state(void) 
{
    uint8_t rv;

    rv = input_data.batter_state;

    return rv;
}



/*********************************
**  返回ACC电压
*********************************/

uint16_t read_in_acc_vol(void)
{
    uint16_t rv;

    rv = input_data.acc_vol;

    return rv;
    
}



/***************************
**  
****************************/

uint8_t read_in_io_state(void)							//IO状态
{
    uint8_t rv;

    rv = input_data.io_state.value;

    return rv;
}




/***************************
**  TBOX报警状态
****************************/

uint8_t read_in_alarm(void)
{
    uint8_t rv;

    rv = input_data.alarm;

    return rv;
}




/*****************************
**  
******************************/

void process_in(void)
{
    struct adc_value_t tmp;

    get_adc_value(&tmp);

    input_data.power_vol = tmp.vcc / 100;
    input_data.acc_vol = tmp.acc / 100;

    
    if(input_data.acc_vol > 80 && input_data.power_vol > 80)
    {
        input_data.acc_state = 1;
        input_data.io_state.bit.acc = 1;
    } 
    else 
    {
        input_data.acc_state = 0;
        input_data.io_state.bit.acc = 0;
    }

   
    input_data.moto_state = 0;
    input_data.io_state.bit.moto = 0;

    input_data.batter_vol = tmp.bat / 100;      //锂电池电压

    input_data.charge_state = rt_get_chrg_done_status();    //
    input_data.stdby_state  = rt_get_chrg_standby_status();

    if(input_data.charge_state == 1 && input_data.stdby_state == 1)
        input_data.batter_state = 1;
    else
        input_data.batter_state = 0;

    // #define bitset(var, bitno) ((var) |= 1UL << (bitno))
    // #define bitclr(var, bitno) ((var) &= ~(1UL << (bitno)))

    if(input_data.power_vol < 50)      //拆除报警（没有外电，作为拆除报警的条件）
        bitset(input_data.alarm,0);
    else
        bitclr(input_data.alarm,0);

    if(tmp.shell > 100)     //
    {
        input_data.shell_state = 1;
        input_data.io_state.bit.shell = 1;
        bitset(input_data.alarm,1);
    }
    else
    {
        input_data.shell_state = 0;
        input_data.io_state.bit.shell = 0;
        bitclr(input_data.alarm,1);
    }
       
    //天线状态
    if(read_gnss_ant_state() > 0)
    {
        bitset(input_data.alarm,2);
    }
    else
    {
        bitclr(input_data.alarm,2);
    }
    
    //
    if(read_in_acc_state() > 0)        //增加ACC条件，在有ACC条件下判定CAN链接状态
    {
        if(read_can_connect_state() > 0)
        {
            bitset(input_data.alarm,3);
        }
        else
        {
            bitclr(input_data.alarm,3);
        }
    }

    if(input_data.batter_state == 1)
    {
        bitset(input_data.alarm,4);
    }
    else
    {
        bitclr(input_data.alarm,4);
    }
    
    //加密芯片工作状态
    if(read_acl16_work_state() == 0)
    {
        bitset(input_data.alarm,5);
    }
    else
    {
        bitclr(input_data.alarm,5);
    }
    
    //文件系统挂载状态
	if(read_files_sys_state() > 0)       //文件系统挂载状态
	{
		bitset(input_data.alarm,8);
	}
	else
	{
		bitclr(input_data.alarm,8);
	}

    //printf("-- the batter:%d,%d,%d\r\n",input_data.charge_state,input_data.stdby_state,input_data.batter_state);
    //printf("-- the vol:%d,%d,%d,%d\r\n",input_data.shell_state,input_data.alarm,input_data.shell_state,read_can_connect_state());
    //printf("-- the vol:%d,%d,%d\r\n",input_data.power_vol,input_data.batter_vol,input_data.acc_vol);
}


