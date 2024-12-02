/***************************************
**	FileName:Version.c
**	Time:
**	程序版本控制
**	输出版本信息
**	版本控制信息
****************************************/

#include "app_ver.h"


/***********************
** 	返回版本号
**	应用程序版本号
**	
************************/

uint32_t read_app_version(void)
{
	unsigned int rv;
	
	rv = MAJOR_NUMBER * 10 + MINOR_NUMBER;
	
	return rv;
}


/***********************
** 	返回嵌入式系统软件版本
************************/

uint32_t read_rtos_ver(void)
{
	unsigned int rv;
	
	rv = 40;
	
	return rv;
}



/***********************
** 	客户定义版本号
************************/

uint32_t read_user_ver(void)
{
	uint32_t rv;
	
	rv = USER_MAJOR_NUMBER * 10 + USER_MINOR_NUMBER;
	
	return rv;
}
