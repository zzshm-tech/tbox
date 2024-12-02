


#ifndef _COMMON_H
#define _COMMON_H


#include <stdint.h>

typedef union 
{
    uint8_t byte[2];
    uint16_t value;
}uint16_to_byte;




typedef union 
{
    uint8_t byte[4];
    uint32_t value;
}uint32_to_byte;



#endif




