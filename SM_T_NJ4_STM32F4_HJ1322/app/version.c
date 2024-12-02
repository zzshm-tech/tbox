


#include <stdio.h>
#include <string.h>

#include "version.h"


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

uint32_t read_rtos_version(void)
{
	unsigned int rv;
	
	rv = 40;
	
	return rv;
}



/***********************
** 	客户定义版本号
************************/

uint32_t read_user_version(void)
{
	uint32_t rv;
	
	rv = MAJOR_NUMBER * 10 + MINOR_NUMBER;
	
	return rv;
}



