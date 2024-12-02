

#ifndef __MD5_CODE_H__
#define __MD5_CODE_H__

//#include "includes.h"

#define FIX_KEY   0xFF

typedef unsigned int           uint32;
typedef unsigned int           uint32_t;
typedef unsigned char          uint8_t;
typedef unsigned short int     uint16_t;

#include <stdio.h>
#include <string.h>




/* POINTER defines a generic pointer type */ typedef unsigned char * POINTER; 


/**************************************
**
**  锁车状态存储数据
********************************/
typedef struct
{
		uint32_t	device_id;
		uint8_t		terminal_id[4];							//终端id
		uint8_t		bind_status;                //设备绑定状态， 0：未定义  1：激活绑定， 2：解绑  只有激活绑定状态下发送锁车指令
		uint8_t		lock_state;                 //锁车状态  0:未锁车 1:远程一级预锁车  2:远程一级锁车  3:远程二级预锁车 4:远程二级锁车 5:本地预锁车 6:本地锁车 7:故障主动预锁车 8:故障主动锁车 9:故障被动预锁车 10:故障被动锁车 15:未激活
		uint32_t  lock_delay;									//锁车延时计数  起始时间
		uint8_t		lock_cmd;										//锁车命令       0:未使用 1：远程停机 2：限速  3：解锁   4：本地停机
		uint16_t	lock_speed;               	//锁车转速
		uint8_t		lock_torque;                //锁车扭矩
		uint16_t	device_type;								//设备类型
		uint8_t		cfg_flag;	 									//数据存储标志 ，
}SYSTEM_CONFIG;


 
 
/* MD5 context. */ 
typedef struct 
{  
	uint32 state[4];        /* state (ABCD) */  
	uint32 count[2];        /* number of bits, modulo 2^64 (lsb first) */  
	unsigned char buffer[64];    /* input buffer */ 
} MD5_CTX; 






void MD5Init (MD5_CTX *context); 
void MD5Update (MD5_CTX *context, unsigned char *input, unsigned int inputLen); 
void MD5Final (unsigned char digest[16], MD5_CTX *context); 

void get_rand_str(unsigned char rand[],unsigned char new[],int number) ;




#endif

