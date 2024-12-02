





#ifndef _APP_PACKET_H_
#define _APP_PACKET_H_

#include <rtthread.h>
#include "drv_rtc.h"




#define GB4_SOCKET_ID   					1



#define GB4_DATA_LEN   						256

#define GB4_FIFO_NUM   						5


#define GB4_BLIND_DATA_MAX_INDEX			0xEC4000   /** 国标数据10秒钟一条，最少存储7天  0xEC4000 **/

#define GB4_BLIND_DATA_MAX_CNT  			(GB4_BLIND_DATA_MAX_INDEX / GB4_DATA_LEN)




#define QB4_DATA_LEN   						512

#define QB4_FIFO_NUM   						3


#define QB4_BLIND_DATA_MAX_INDEX			0x1D88000    /** 10S一条，存储7天  0x1D88000 **/ 

#define QB4_BLIND_DATA_MAX_CNT  			(QB4_BLIND_DATA_MAX_INDEX / QB4_DATA_LEN)



/**********************************  保存数据 ***************************************** */

#define GB4_STREAM_LEN   					256   /** 数据长度 **/

#define GB4_STREAM_FIFO_NUM   				1    /**  **/


#define GB4_STREAM_DATA_MAX_INDEX			0x1D88000    /** 10S一条，存储7天  0x1D88000 **/ 

#define GB4_STREAM_DATA_MAX_CNT  			(GB4_STREAM_DATA_MAX_INDEX / GB4_STREAM_LEN)    /****  ****/




#pragma pack(1)



struct exhaust_data_t
{
	uint32_t			engine_power;				// 发动机平均功率
	uint32_t			scr_up_nox;					// SCR上游NOx平均浓度   //无效值
	uint32_t			scr_down_nox;				// SCR下游NOx平均浓度
	uint32_t 			scr_up_flow;					// SCR上游NOx平均质量流量
	uint32_t			scr_down_flow;     	// SCR下游NOx平均质量流量
	uint32_t			src_in_temp;					// SCR入口平均温度
	uint32_t			src_out_temp;  			// SCR出口平均温度
	uint32_t			fuel_flow;						// 发动机燃料流量平均值
	
	uint16_t			index;    								// 统计周期时长
	uint8_t				cycle_pwm; 		  						// 统计周期内有效时间占比
};


struct exhaust_buf_t
{
	uint16_t		array[8][600];	
	uint16_t		index;    								// 统计周期时长  有效数据
	uint16_t    cycle;      //统计周期
};


#pragma pack()

uint8_t read_gb4_alarm_state(void);

uint16_t write_qb4_fifo_buff(uint8_t *data,uint16_t len);
uint16_t write_gb4_fifo_buff(uint8_t *data,uint16_t len);
uint16_t read_qb4_fifo_buff(uint8_t *data,uint16_t len);
uint16_t read_gb4_fifo_buff(uint8_t *data,uint16_t len);
uint8_t read_exhaust_data(struct exhaust_data_t *source);

void thread_entry_packet(void *parameter);

#endif


