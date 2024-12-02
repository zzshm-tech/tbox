/*****************************
**	FileName:
**	Time:
******************************/



#ifndef _APP_VER_H__
#define _APP_VER_H__


#include <stdint.h>



#define MAJOR_NUMBER        		1                           /** 主版本  **/
#define MINOR_NUMBER        		9                	      		/** 次版本  **/
#define REVI_NUMBER                                         /** 修正版本 **/
#define BUILD_NUMBER																			  /** 编译版本 **/


#define USER_NUMBER_YEAR        24                          /******/
#define USER_NUMBER_MON					10												  	/******/
#define USER_NUMBER_DAY					11											  /******/
	
#define USER_MAJOR_NUMBER       2														/******/	
#define USER_MINOR_NUMBER       0    												/******/





uint32_t read_app_version(void);      //
uint32_t read_rtos_ver(void);		  //
uint32_t read_user_ver(void);     //



#endif


/*******************File End******************/


