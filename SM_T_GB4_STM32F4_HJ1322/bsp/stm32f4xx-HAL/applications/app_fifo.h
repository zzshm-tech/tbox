
/********************************************
**	
**	
*********************************************/

#ifndef _APP_FIFO_H
#define _APP_FIFO_H

#include <rtthread.h>
#include <rtdevice.h>



#define BLIND_NUM  5        	/****/                                 
#define BLIND_BUF  512	      /****/




struct send_fifo
{
    const uint16_t          item_block;  
    const uint32_t          item_size; 
  
    uint32_t                w_offset_add; 
    uint32_t                r_offset_add; 
    uint32_t                item_cnt;     
    uint32_t                w_item_index; 
    uint32_t                r_item_index; 
    
    uint8_t                 *buff;   
    
};



struct blind_s
{
    uint32_t msg_cnt;
    uint32_t write_index;
    uint32_t read_index;
};



uint8_t send_fifo_write(struct send_fifo *wbuff, uint8_t *data, uint16_t len);
uint16_t send_fifo_read(struct send_fifo *rbuff, uint8_t *data,uint16_t len);


#endif



/***********************File End*********************/


