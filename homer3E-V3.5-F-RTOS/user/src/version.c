

/***************************************
**	FileName:Version.c
**	Time:
**	程序版本控制
**	输出版本信息
****************************************/

#include "version.h"


/***********************
** 返回版本号
************************/

unsigned int read_fir_ver(void)
{
	unsigned int rv;
	
	rv = Major_Number * 10 + Minor_Number;
	
	return rv;
}



/****************File End*******************/


