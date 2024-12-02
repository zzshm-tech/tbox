


#ifndef _APP_EDGE_H
#define _APP_EDGE_H


#include <stdint.h>

#include <rtthread.h>
#include <rtdevice.h>



#pragma pack(1)

struct edge_data_t
{
	uint32_t			 	day_total_time;     						//ACC开启时间     1
	uint32_t			 	engine_total_time;    					//GPS统计的发动机运行时间   2
	uint32_t 				ant_fault_time; 								//定位天线故障    3
	uint32_t				vehicle_work_time;							//整车工作时间    4
	int32_t					vehicle_offset_time;            //整车工作时间偏移量   5
	uint8_t					day;														// 7
	uint32_t 				unix;														// 8
	uint32_t 				total_fuel;											// 累计油耗 9
	int32_t 				fuel_offset;										// 累计油耗偏移量 10
	uint8_t 				crc_value;											// 校验值
};



struct edge_mq_t
{
	uint32_t		 cmd;           //参数类型
	uint16_t 	 	 len;					//不同的参数类型，代表不同的解析方法
	uint8_t		 	 data[50];
};
#pragma pack()

rt_mq_t get_edge_cmd_mq(void);
uint8_t reset_edge_data(void);
uint32_t read_vehicle_work_time(void);
uint32_t read_day_total_time(void);
uint32_t read_engine_total_time(void);
int32_t read_vehicle_offset_time(void);
int32_t read_vehicle_fuel_offset(void);
uint32_t read_vehicle_total_fule(void);
void thread_entry_edge(void *parameter);

#endif



