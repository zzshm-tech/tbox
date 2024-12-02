

#ifndef _APP_PACKET_H
#define _APP_PACKET_H






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



uint8_t read_exhaust_data(struct exhaust_data_t *source);

void thread_entry_data_packet(void *parameter);


#endif
