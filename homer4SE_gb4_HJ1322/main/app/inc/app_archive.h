


#ifndef _APP_ARCHIVE_H
#define _APP_ARCHIVE_H


struct archive_plat_str
{
	uint8_t         state;
	uint8_t         count; 
	uint8_t 		res;
	uint8_t 		step;
	uint8_t 		ententry_state;
};



/*************************************************************
**					安全信息备案
************************************************************/
struct archival_info_str
{
	uint8_t     	time[6];
	uint8_t 		chip_id[16];   		    //加密芯片ID 
    uint8_t 	    public_key[64]; 	    //公钥 
	uint8_t		    vehicle_vin[17];  	    //VIN	
};



void thread_entry_archivel(void *parameter);
uint8_t read_archive_en_state(void);

#endif

