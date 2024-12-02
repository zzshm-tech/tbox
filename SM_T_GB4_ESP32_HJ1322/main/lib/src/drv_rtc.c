

#include <stdio.h>
#include <time.h>


#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"




#include "drv_rtc.h"




/******************* 本地全局变量 **********************/

static RTC_DATA_ATTR  struct rt_tm tm;

static RTC_DATA_ATTR struct rtc_t rtc;         


// static  struct rt_tm tm;

// static struct rtc_t rtc;   




/*****************************
 ** 检测RTC时间
 *****************************/

static uint8_t check_rtc_time(struct rt_tm *tm)
{
    uint16_t tmp;

    if(tm == NULL)
        return 1;
    if(tm->year > 99)
        return 1;

    
    if(tm->mon > 12 || tm->mon == 0)
        return 1;

    switch(tm->mon)
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            if(tm->day > 31 || tm->day == 0)
                return 1;
            
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            if(tm->day > 30 || tm->day == 0)
                return 1;

            break;
        case 2:
            tmp = tm->year + 2000;
            if(tmp % 400==0 || (tmp % 4==0 && tmp % 100 != 0))
            {
                if(tm->day > 29 || tm->day == 0)
                   return 1;
            }
            else
            {
                if(tm->day > 28 || tm->day == 0)
                   return 1;
            }
            break;
    }
    
    if(tm->hour > 23)
        return 1;
    if(tm->min > 59)
        return 1;
    if(tm->sec > 59)
        return 1;

    return 0;
}

/**************************
 ** 读取RTC
 *****************************/

uint8_t get_rtc_time(struct rt_tm *tm)
{
    struct tm *p_t;
    struct timeval now = {0};

    if(tm == NULL)
        return 1;

    gettimeofday(&now,NULL);

    p_t = localtime(&now.tv_sec);

    if(p_t->tm_year < 100)
        return 1;

    tm->year = p_t->tm_year - 100;
    tm->mon = p_t->tm_mon + 1;
    tm->day = p_t->tm_mday;
    tm->hour = p_t->tm_hour;
    tm->min = p_t->tm_min;
    tm->sec = p_t->tm_sec;

    return 0;
}



/**************************
 ** 设置RTC
 *****************************/

uint8_t set_rtc_time(struct rt_tm *tm)
{
    struct tm  tmp = {0} ;
    struct timeval now = {0};
    struct timezone zone = {0};

  

    if(check_rtc_time(tm) > 0)
        return 1;
    
    tmp.tm_year = tm->year + 100;
    tmp.tm_mon = tm->mon - 1;
    tmp.tm_mday = tm->day;
    tmp.tm_hour = tm->hour;
    tmp.tm_min = tm->min;
    tmp.tm_sec = tm->sec;

    now.tv_sec = mktime(&tmp);
    settimeofday(&now,&zone);

    rtc.flag = 0x5A5A;

    return 0;
}




/**********************
**  
***********************/

uint8_t read_rtc_state(void)
{
    uint8_t rv = 0;

    if(rtc.flag == 0x5A5A)
        rv = 1;

    return rv;
}


/*******************************
*** 返回系统时间戳
*******************************/

uint32_t get_rtc_timestamp(void)
{
    struct timeval now = {0};

    gettimeofday(&now,NULL);

    return now.tv_sec;
}




/***********************************
**	函数名称:
**	功能描述:时间戳转日历
**	返回一个指向
***********************************/

struct rt_tm *rt_localtime(const uint32_t *time)
{
    struct tm *p_t = NULL;

    p_t = localtime((const time_t *)time);
    
    if(p_t->tm_year < 100)
        return NULL;

    tm.year = p_t->tm_year - 100;
    tm.mon = p_t->tm_mon + 1;
    tm.day = p_t->tm_mday;
    tm.hour = p_t->tm_hour;
    tm.min = p_t->tm_min;
    tm.sec = p_t->tm_sec;

    return &tm;
}



/**********************************************************************
**	函数名称:
**	功能描述:时间转时间戳
**	输入参数:
**	输出参数:
**********************************************************************/


time_t rt_mktime(struct rt_tm *t)
{
    time_t rv;
    struct tm tmp;

    tmp.tm_year = t->year + 100;
    tmp.tm_mon = t->mon - 1;
    tmp.tm_mday = t->day;
    tmp.tm_hour = t->hour;
    tmp.tm_min = t->min;
    tmp.tm_sec = t->sec;

    rv = mktime(&tmp);

	return rv;               //
}



/***************************
**  
***************************/

uint8_t rt_hw_init_rtc(void)
{
    struct tm  tmp = {0} ;
    struct timeval now = {0};
    struct timezone zone = {0};

    if(rtc.flag != 0x5A5A)
    {
        tmp.tm_year = 122;
        tmp.tm_mon = 0;
        tmp.tm_mday = 1;
        tmp.tm_hour = 0;
        tmp.tm_min = 0;
        tmp.tm_sec = 0;

        now.tv_sec = mktime(&tmp);
        settimeofday(&now,&zone);
    }
   
    return 0;
}



