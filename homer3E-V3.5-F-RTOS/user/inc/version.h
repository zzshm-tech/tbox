/*****************************
**	FileName:
**	Time:
******************************/



#ifndef __VERSION_H__
#define __VERSION_H__

//  version number.
#define __FIRMWARE_INF   "homer3-S_20171105" 					// 固件信息
#define __VERSION_NUM    0 														//STM32回传协议版本号，用来检验是否升级成功




#define Major_Number        1                               //主版本
#define Minor_Number        0                             //次版本
#define Revi_Number                                         //修正版本 
#define Build_Number																			  //编译版本



unsigned int read_fir_ver(void);





#endif


/*******************File End******************/


