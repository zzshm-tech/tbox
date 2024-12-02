


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>


#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_system.h"

#include "app_fifo.h"



/************************************************
**  
***********************************************/
uint8_t send_fifo_write(struct send_fifo *wbuff, uint8_t *data, uint16_t len)
{
    struct send_fifo *pbuff = NULL;
	
    pbuff = wbuff;  

    memcpy(pbuff->buff + pbuff->w_offset_add, data, len);

    if (++pbuff->w_item_index >= pbuff->item_size)
        pbuff->w_item_index = 0;

    pbuff->w_offset_add = wbuff->item_block * pbuff->w_item_index;

    if(++pbuff->item_cnt >= pbuff->item_size)
    {
        pbuff->item_cnt = pbuff->item_size;
        pbuff->r_item_index = pbuff->w_item_index;                
        pbuff->r_offset_add = pbuff->item_block * pbuff->w_item_index; 
        
        return 1;    //满了
    }
    //printf("-- fifo:%d,\r\n",pbuff->w_item_index);
    return 0;
}




/************************************************
 * @desc  : 数据从fifo读出 
 * @param : 
 * @return: 
 * @Date  : 2021-3-16
 ************************************************/
uint16_t send_fifo_read(struct send_fifo *rbuff, uint8_t *data,uint16_t len)
{
    struct send_fifo *pbuff = NULL;
    
    pbuff = rbuff;

    //printf("-- Read fifo :%d,%d\r\n",pbuff->item_cnt,pbuff->r_item_index);
    if (pbuff->item_cnt == 0)
    {
        return 0;
	}
    memcpy(data, pbuff->buff + pbuff->r_offset_add, len);
    pbuff->item_cnt--;
    if ((++pbuff->r_item_index) >= pbuff->item_size)
        pbuff->r_item_index = 0;
    pbuff->r_offset_add = pbuff->item_block * pbuff->r_item_index;

    return len;
}

