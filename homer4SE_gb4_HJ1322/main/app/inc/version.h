
/******************************************

*******************************************/




#ifndef _VERSION_H
#define _VERSION_H




#include <stdint.h>


#define MAJOR_NUMBER                2                           /** 主版本 **/
#define MINOR_NUMBER                7 	                        /** 次版本 **/
#define REVI_NUMBER                                             /** 修正版本 **/ 
#define BUILD_NUMBER											/** 编译版本 **/


#define USER_NUMBER_YEAR             24                         /** 编译日期 - 年 **/
#define USER_NUMBER_MON				 6							/** 编译日期 - 月  **/
#define USER_NUMBER_DAY				 17					        /** 编译日期 - 日  **/


#define USER_MAJOR_NUMBER            2                          /**   **/                  
#define USER_MINOR_NUMBER            0                          /**   **/






uint32_t read_app_version(void);                                //
uint32_t read_user_version(void);                               //
uint32_t read_rtos_version(void);                               //
void show_sys_version(void);



#endif


