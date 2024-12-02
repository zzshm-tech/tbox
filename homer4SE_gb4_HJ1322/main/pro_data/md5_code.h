

#ifndef _MD5_CODE_H
#define _MD5_CODE_H


#define FIX_KEY   0xFF


#include <stdio.h>
#include <stdint.h>
#include <string.h>


typedef unsigned char * POINTER;    /* POINTER defines a generic pointer type */ 


/**************************************
**
**  
********************************/
typedef struct
{
		uint32_t	device_id;					//
		uint8_t		terminal_id[4];				//
		uint8_t		bind_status;                //
		uint8_t		lock_state;                 //
		uint32_t  	lock_delay;					//
		uint8_t		lock_cmd;					//
		uint16_t	lock_speed;               	//
		uint8_t		lock_torque;                //
		uint16_t	device_type;				//
		uint8_t		cfg_flag;	 				//
}SYSTEM_CONFIG;


 
 
/* MD5 context. */ 
typedef struct 
{  
	uint32_t state[4];        /* state (ABCD) */  
	uint32_t count[2];        /* number of bits, modulo 2^64 (lsb first) */  
	unsigned char buffer[64];    /* input buffer */ 
} MD5_CTX; 






void MD5Init (MD5_CTX *context); 
void MD5Update (MD5_CTX *context, unsigned char *input, unsigned int inputLen); 
void MD5Final (unsigned char digest[16], MD5_CTX *context); 

void get_rand_str(unsigned char rand[],unsigned char new[],int number) ;




#endif

