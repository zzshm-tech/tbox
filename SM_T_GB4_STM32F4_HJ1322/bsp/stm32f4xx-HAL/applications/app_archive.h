


#ifndef _APP_ARCHIVE_H
#define _APP_ARCHIVE_H


#include <stdint.h>


struct archive_plat_str
{
	uint8_t         state;
	uint8_t         count; 
	uint8_t 		res;
	uint8_t 		step;
};




/*************************************************************
**					安全信息备案
************************************************************/
struct emissions_activate_t
{
	uint8_t     time[6];							//RTC时间
	uint8_t 		chip_id[16];   		    //加密芯片ID 
	uint8_t 	  public_key[64]; 	    //公钥 
	uint8_t		  vehicle_vin[17];  	  //VIN	
};




/*************************************************************
**					定位终端激活信息
************************************************************/
struct position_activate_t
{
	uint8_t     time[6];							//RTC时间
	uint8_t		  vehicle_vin[17];  	  //VIN	
};


void thread_entry_archivel(void *parameter);

#endif

