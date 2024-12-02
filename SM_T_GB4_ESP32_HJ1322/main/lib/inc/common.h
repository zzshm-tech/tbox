


#ifndef _COMMON_H
#define _COMMON_H

#define bitset(var, bitno) ((var) |= 1UL << (bitno))

#define bitclr(var, bitno) ((var) &= ~(1UL << (bitno)))


#include <stdint.h>



#define LOG_UART_IAP 					0x00
#define LOG_ERROR 						0x01
#define LOG_WARNING 					0x02
#define LOG_DEBUG 						0x04
#define LOG_AT_CMD 						0x08
#define LOG_GB_DATA 					0x10
#define LOG_SERVER_DOWN_DATA 	        0x20
#define LOG_GPRS_SEND_DATA 		        0x40
#define LOG_GPS_DATA 					0x80	




typedef enum
{
	RES_TIMEOUT = -4,   /* 超时 */
	RES_OPEN  = -3,     /* 开路 */
	RES_SHORT = -2,     /* 短路 */
	RES_ERROR = -1,     /* 错误 */
	RES_OK = 0,         /* 正常 */
	RES_NORMAL,
	SIM_OK,
	SIM_ERROR,
	NET_REG,
	NET_UNREG,
	CAN_NORMAL,
	CAN_FALUT,
	TIME_SYN,
}ERROR_CODE;


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



typedef enum
{
    PRINT_HEX = 0,
    PRINT_DEC = 1,
    PRINT_CHAR = 2,
}print_mode;



extern uint8_t cfg_lvl;             //


#define log_printf(lvl, fmt, ...)       \
    do                                  \
    {                                   \
        if (lvl & cfg_lvl)              \
            printf(fmt, ##__VA_ARGS__); \
    } while (0)
		



#define CHECK_ERROR_CODE(returned, expected) ({                        \
            if(returned != expected){                                  \
                printf("TWDT ERROR\n");                                \
                abort();                                               \
				fflush(stdout);											\
        		esp_restart();											\
            }                                                          \
})



void mem_printf(uint8_t lvl, print_mode mode,const uint8_t *data, uint16_t len);
void print_rtc_info(void);


#endif
