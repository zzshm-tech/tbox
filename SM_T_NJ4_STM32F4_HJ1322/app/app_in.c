

#include "board.h"



#include "drv_gpio.h"


#include "app_in.h"





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



