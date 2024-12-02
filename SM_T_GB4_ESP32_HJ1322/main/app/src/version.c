


#include <stdio.h>
#include <string.h>

#include "version.h"
#include "app_products.h"


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
	
	rv = USER_MAJOR_NUMBER * 10 + USER_MINOR_NUMBER;
	
	return rv;
}





/***************************************
**  显示版本号
***************************************/

void show_sys_version(void)
{
	uint8_t  tmp = 0;

	tmp = read_config_hard_ware();
	
	printf("|---------------------------------|\r\n");
	printf("|             Homer4SE-GC         |\r\n");
	printf("| T-BOX HardWare VER:%d.%d          |\r\n",(tmp / 10),(tmp % 10));
	printf("| T-BOX SofeWare VER:%d.%d          |\r\n", MAJOR_NUMBER,MINOR_NUMBER);
	printf("| Build Date:%s %s |\r\n", __DATE__,__TIME__);
	printf("|---------------------------------|\r\n");
}



