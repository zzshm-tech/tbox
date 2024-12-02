

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "common.h"

#include "drv_rtc.h"



uint8_t cfg_lvl = LOG_ERROR ;





/*****************************************
 * @desc  : 内存打印
 * @param : lvl:打印等级,mode: 打印模式,*data:数据指针,len:数据长度
 * @return: None
 * @Date  : 2018-6-5 09:50:16
 ****************************************/
 
 
void mem_printf(uint8_t lvl, print_mode mode,const uint8_t *data, uint16_t len)
{
    uint16_t i = 0;

    if (lvl & cfg_lvl)
    {
        for (i = 0; i < len; i++)
        {
            switch (mode)
            {
            case PRINT_HEX:
                printf("%02X", data[i]);
                break;
            case PRINT_DEC:
                printf("%d ", data[i]);
                break;
            case PRINT_CHAR:
                printf("%c", data[i]);
                break;
            default:
                break;
            }
        }
        printf("\r\n");
    }
}



/*********************************
**  打印RTC信息
*********************************/

void print_rtc_info(void)
{
    struct rt_tm            tm = {0};//     tt[6];

    get_rtc_time(&tm);

    printf("-- 20%d,%d,%d %d-%d-%d\r\n",tm.year,tm.mon,tm.day,tm.hour,tm.min,tm.sec);
}






