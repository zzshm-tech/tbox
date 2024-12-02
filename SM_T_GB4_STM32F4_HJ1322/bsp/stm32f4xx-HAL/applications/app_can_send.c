

#include <stdio.h>
#include <string.h>

#include <rtthread.h>

#include "pro_data.h"
#include "md5_code.h"
#include "md5.h"

#include "drv_timer.h"

#include "app_packet.h"
#include "app_fifo.h"
#include "app_shell.h"
#include "app_gnss.h"
#include "app_ver.h"
#include "app_gnss.h"
#include "app_gb4.h"
#include "app_products.h"
#include "app_can_send.h"
#include "app_can_recv.h"
#include "app_mon.h"
#include "app_lte.h"





#define WEIC_LOCK_TYPE  1   /** 1:英轩重工 0：青岛雷沃 **/




/****************全局变量使用****************/

static rt_mq_t 									can_lock_mq	= RT_NULL;      //CAN锁车队列

static struct lock_expect_str		lock_expect_info = {0};     //期望锁车状态（需要保存到外部EEPROM）

static rt_event_t 							can_lock_event 	= RT_NULL;

static struct lock_data_str			lock_data = {0};

static struct lock_back_t				lock_back = {0};    //针对玉柴锁车数据备份

static uint8_t 									cnt_con = 0;




/******************************
**	保存ECU运行数据
*******************************/

void init_ecu_lock_back(void)
{
	rt_device_t 				bsram_dev = RT_NULL;       //
	uint8_t 						tmp;
	
	bsram_dev = rt_device_find("back_sram");
	rt_device_open(bsram_dev, RT_DEVICE_OFLAG_RDWR);
	rt_device_read(bsram_dev,768,(uint8_t *)&lock_back,sizeof(lock_back));
	rt_device_close(bsram_dev);
	
	tmp = CalcCrc8((uint8_t *)&lock_back,sizeof(lock_back) - 1);
	if(lock_back.flag != tmp)
	{
		memset((uint8_t *)&lock_back,0,sizeof(lock_back));
		//rt_kprintf("--the data fail....\r\n");
	}
}



/*************************
**	把ECU DM1保存到 backsram
**************************/

void write_ecu_lock_back(void)
{
	rt_device_t 								bsram_dev = RT_NULL;       //
	
	bsram_dev = rt_device_find("back_sram");
	rt_device_open(bsram_dev, RT_DEVICE_OFLAG_RDWR);
	
	lock_back.flag = CalcCrc8((uint8_t *)&lock_back,sizeof(lock_back) - 1);
	
	rt_device_write(bsram_dev,768,(uint8_t *)&lock_back,sizeof(lock_back));
	rt_device_close(bsram_dev);
}




/********************************
**	返回CAN连接状态
*********************************/

void can_lock_thread_event(void)
{
	cnt_con++;
	if(cnt_con >= 10)
	{
		cnt_con = 0;
		if(can_lock_event != RT_NULL)
			rt_event_send(can_lock_event,1);
	}
}




/*****************************
**	返回期望锁车状态，设备内部的
**	1:一级锁车
**	2:二级锁车
**	3：解锁
******************************/

uint8_t read_lock_expect_state(void)
{
	if(lock_expect_info.lock_expect_state > 2)
		return 2;
	
	return lock_expect_info.lock_expect_state;
}



/*****************************
**	返回期望锁车状态，设备内部的
**	1:一级锁车
**	2:二级锁车
**	3：解锁
******************************/

uint8_t read_lock_expect_bank(void)
{
	uint8_t rv;
	
	rv = lock_expect_info.lock_expect_state;
	
	if(rv > 2)
		return 2;
	
	return rv;
}




/*****************************
**	返回期望监控状态，设备内部的，
******************************/

uint8_t read_expect_mon_state(void)
{
	uint8_t rv; 
	
	if(lock_expect_info.mon_expect_state == 1)
		rv = 1;
	else
		rv = 0;

	return rv;
}



/*****************************
**	
******************************/

rt_mq_t	get_can_lock_mq(void)      //CAN锁车队列
{
	return can_lock_mq;
}



/*****************************
**	
******************************/

void read_lock_expect_info(void)
{
	rt_device_t eeprom_dev = RT_NULL;
	rt_err_t result = RT_EOK;
	
  eeprom_dev = rt_device_find("at24cxx");
	
	rt_device_open(eeprom_dev,RT_DEVICE_OFLAG_RDWR);
	
	result = rt_device_read(eeprom_dev,1280,(uint8_t *)&lock_expect_info,sizeof(lock_expect_info));
	//rt_kprintf("-- Read Lock expect info:%d,%u,%u\r\n",result,lock_expect_info.lock_expect_state,lock_expect_info.mon_expect_state);
	if(result != RT_EOK || lock_expect_info.flag != 0x55)
	{
		//lock_expect_info.mon_expect_state = 2;          //
		//lock_expect_info.lock_expect_state = 0;					//
		memset((uint8_t *)&lock_expect_info,0,sizeof(lock_expect_info));
	}
}






/***************************************
**	配置信息保存在外部EEPROM内部
****************************************/

static uint8_t save_lock_expect_info(void)
{
	rt_device_t eeprom_dev = RT_NULL;
	rt_uint16_t result = 0;
	
  eeprom_dev = rt_device_find("at24cxx");
	rt_device_open(eeprom_dev,RT_DEVICE_OFLAG_RDWR);
	lock_expect_info.flag = 0x55;
	result = rt_device_write(eeprom_dev,1280,(uint8_t *)&lock_expect_info,sizeof(lock_expect_info));
	//rt_kprintf("-- save lock expect info:%d\r\n",result,lock_expect_info.mon_expect_state);
	if(result == sizeof(lock_expect_info))
	{
		rt_device_close(eeprom_dev);           //注意这个地方的代码
		return 1;
	}
	rt_device_close(eeprom_dev);
	return 1;
}



/************************
**	返回玉柴激活状态
*************************/

uint8_t get_yuc_mon_state(void)
{
	uint8_t rv;

	rv = lock_back.active_status;
	
	if(rv > 1)
		rv = 0;
	
	return rv;
}




/*******************
 **	返回玉柴锁车状态
 ****/

uint8_t get_yuc_lock_state(void)
{
	uint8_t rv = 0;

	if(lock_back.passive_lock > 0)      //被动锁车
	{
		rv = lock_back.passive_lock;
		
		return rv; 
	}
	
	if(lock_back.initiative_lock == 0 && lock_expect_info.lock_expect_state > 0)
	{
		return 9;
	}
	else
	{
		rv = lock_back.initiative_lock;
	}
	
	return rv;
}




/*******************
 **	返回玉柴锁车状态
 ****/

uint8_t get_yuc_key_state(void)
{
	uint8_t rv = 0;

	if(lock_back.key_state == 3)
		rv = 1;
	else 
		rv = 0;
		
	return rv;
}




/*************************************
 **	返回玉柴锁车状态 （握手状态）
************************************/

uint8_t get_yuc_gps_state(void)
{
	uint8_t rv = 0;

	if(lock_back.check_status == 1)
		rv = 1;
	else 
		rv = 0;
		
	return rv;
}





/****************************
**	上柴发动机锁车锁车
*****************************/

static void ecu_lock_sc_handle(void)
{
	static uint32_t 					cnt = 0;
	static uint32_t						cnt_heat = 0;

	uint8_t 									array[100] = {0};    //
	
	
	cnt++;
	if((cnt % 10) > 0)
		return;
	
	switch(lock_data.step)		   //状态机1 激活ECU锁车功能
	{
		case 0:
			if(lock_data.mon_cmd == 1)          //激活命令,关闭激活命令
			{
				array[0] = 0x53;
				array[1] = 0x44;
				array[2] = 0x45;
				array[3] = 0x43;
				
				array[4] = 0x11;	/** seed **/
				array[5] = 0x22;
				array[6] = 0x33;
				array[7] = 0x44;
				
				if(read_ecu_sc_mon_state() != lock_expect_info.mon_expect_state)
				{
					can1_send_data(SEND_EXTID, 0x18FEF328, (uint8_t *)&array,8);
					lock_data.step++;
					return;
				}
				lock_data.mon_cmd = 0;
				return ;
			}
			
			if(lock_data.lock_cmd == 1)
			{
				lock_data.lock_cmd = 0;
				return ;
			}
			break;
		case 1: 
			if(lock_data.index++ >= 5)   //3秒之后去判断 SC_ECU监控状态
			{
				lock_data.mon_cmd  = 0;   //结束命令
				lock_data.step = 0;
				
				if(read_ecu_sc_mon_state() == lock_expect_info.mon_expect_state && read_can_connect_state(1) == 0)
				{
					lock_expect_info.mon_cmd_res_state = 0;
					save_lock_expect_info();
				}
			}
			return;
		}
	
		/**********************
		**	
		************************/
		cnt_heat++;
		if((cnt_heat % 3) > 1)
			return;
		
		if(lock_expect_info.lock_expect_state == 0 && read_ecu_sc_mon_state() == 1)   //监控打开
		{
			*((uint32_t *)array) = 0xFFFFFFFF;
			array[4] = 0x11;	
			array[5] = 0x22;
			array[6] = 0x33;
			array[7] = 0x44;
			can1_send_data(SEND_EXTID, 0x18FEF328,array,8);	
		}
}

/*************************************
** 潍柴ECU锁车命令
*************************************/

static void ecu_lock_wc_handle(void)
{
	static uint32_t 					cnt;
	uint8_t										buf[100];			
	cnt++;
	
	if(cnt % 10 == 0)        //1秒钟执行一次
	{
		switch(lock_data.step)         //状态机1 激活ECU锁车功能
		{
			case 0:
					if(lock_data.mon_cmd == 1)          //激活命令
					{
						lock_data.index = 1;
						lock_data.step = 1;
						if(lock_expect_info.mon_expect_state == 1)									//激活命令
						{
							#if WEIC_LOCK_TYPE ==  0     // 
							*(uint16_t *)buf = 0xCA0E;    //青岛雷沃  （测试使用）
							#else
							*(uint16_t *)buf = 0x6715;      //英轩重工
							#endif
						}
						else           //关闭激活
						{
							#if WEIC_LOCK_TYPE ==  0     // 0:工程机械
							*(uint16_t *)buf = 0x8747;   //青岛雷沃（测试使用）
							#else
							*(uint16_t *)buf = 0x5176;   //英轩重工
							#endif
						}
	
						read_config_dev_id(buf + 2,3);    //设备编号
						buf[5] = 0xA7;
						buf[6] = 0x6F;
						buf[7] = 0x3B;
							
						rt_kprintf("-- Send Mon Cmd (1) %d\r\n",lock_data.index);
						#if WEIC_LOCK_TYPE ==  0     //
						can1_send_data(SEND_EXTID, 0x180000FB, (uint8_t *)&buf[0],8);//青岛雷沃
						#else
						can1_send_data(SEND_EXTID, 0x18FE0BEE, (uint8_t *)&buf[0],8);   //英轩重工
						#endif
						break;
					}
					if(lock_data.lock_cmd == 1)            //执行锁车命令
					{
						if(lock_expect_info.lock_expect_state == 0)          //解锁
						{
							*(uint16_t *)buf = 3500 * 8;
						}
						else
						{
							if(lock_expect_info.lock_expect_state == 1)     //一级锁车
								*(uint16_t *)buf = 950 * 8;
							else if(lock_expect_info.lock_expect_state == 0)     //二级锁车
								*(uint16_t *)buf = 0;
						}
							
						buf[2] = 0xFF;
						buf[3] = 0xFF;
						buf[4] = 0xFF;
						read_config_dev_id(&buf[5],3);           //设备ID
						
						#if WEIC_LOCK_TYPE ==  0     // 
						can1_send_data(SEND_EXTID, 0x180002FB, (uint8_t *)&buf,8);   //青岛雷沃
						#else
						can1_send_data(SEND_EXTID, 0x18FE0DEE, (uint8_t *)&buf,8);  //英轩重工
						#endif
						
						lock_data.index = 1;
						lock_data.step = 2;        //锁车
						rt_kprintf("-- Send Lock Cmd %d\r\n",lock_data.index);
					}
					break;				
				case 1:                                      //状态1  激活命令
					if(read_wc_mon_state() == lock_expect_info.mon_expect_state && read_can_connect_state(1) == 0)
					{
						lock_data.mon_cmd  = 0;  //停止发送命令，
						lock_data.step = 0;
						lock_expect_info.mon_cmd_res_state = 0;  
						save_lock_expect_info();
						break;
					}
						
					if(lock_data.index >= 5)
					{
						lock_data.mon_cmd  = 0;
						lock_data.step = 0;
						break;
					}
					if(lock_expect_info.mon_expect_state == 1)
					{
						#if WEIC_LOCK_TYPE ==  0     //
						*(uint16_t *)buf = 0xCA0E;    //青岛雷沃  （测试使用）
						#else
						*(uint16_t *)buf = 0x6715;   //英轩重工
						#endif
					}
					else
					{
						#if WEIC_LOCK_TYPE ==  0     //
						*(uint16_t *)buf = 0x8747;   //青岛雷沃（测试使用）
						#else
						*(uint16_t *)buf = 0x5176;
						#endif
					}

					read_config_dev_id(buf + 2,3);
					buf[5] = 0xA7;
					buf[6] = 0x6F;
					buf[7] = 0x3B;
					
					#if WEIC_LOCK_TYPE ==  0     //
					can1_send_data(SEND_EXTID, 0x180000FB, (uint8_t *)&buf[0],8);  //青岛雷沃
					#else
					can1_send_data(SEND_EXTID, 0x18FE0BEE, (uint8_t *)&buf[0],8);   //英轩重工
					#endif
					lock_data.index++;
					rt_kprintf("-- Send Mon Cmd (2) %d\r\n",lock_data.index);
					break;
				case 2:
					if(read_wc_pre_lock_state() == ((lock_expect_info.lock_expect_state > 0) ? 1:0) && read_can_connect_state(1) == 0)
					{
						lock_data.lock_cmd = 0;
						lock_data.step = 0;
						lock_expect_info.lock_cmd_res_state = 0;
						save_lock_expect_info();
						break;
					}
					if(lock_data.index >= 5)
					{
						lock_data.lock_cmd = 0;
						lock_data.step = 0;
						break;
					}
					if(lock_expect_info.lock_expect_state == 0)          //解锁
					{
						*(uint16_t *)buf = 3500 * 8;
					}
					else if(lock_expect_info.lock_expect_state == 1)     //一级锁车
					{
							*(uint16_t *)buf = 950 * 8;
					}
					else if(lock_expect_info.lock_expect_state == 2)     //二级锁车
					{
							*(uint16_t *)buf = 0;
					}
					buf[2] = 0xFF;
					buf[3] = 0xFF;
					buf[4] = 0xFF;
					read_config_dev_id(&buf[5],3);           //设备ID
					#if WEIC_LOCK_TYPE ==  0     // 
					can1_send_data(SEND_EXTID, 0x180002FB, (uint8_t *)&buf,8);  //青岛雷沃
					#else
					can1_send_data(SEND_EXTID, 0x18FE0DEE, (uint8_t *)&buf,8);//英轩重工 
					#endif
					lock_data.index++;
					rt_kprintf("-- Send Lock Cmd %d\r\n",lock_data.index);
					break;
			}
	}
}






/*************************************
** 处理玉柴锁车
*************************************/
///*
static void ecu_lock_ychai_handle(void)
{
	static uint32_t 		cnt = 0;
	static uint8_t			step = 0;
	static uint8_t 			ls = 0;
	static uint8_t 			meter_cnt = 0;
	static uint8_t 			flag = 0;
	
	uint16_t		 		speed;
  uint8_t					buf[8];
	
	cnt++;

	if(cnt % 10 == 0)     //仪表握手 1S一次
	{
		rt_memset(buf,0,8);
		buf[0] = 0x5F;					 //
		buf[1] = 0xAA;					 //
		buf[2] = meter_cnt++;    //
		if(lock_expect_info.lock_expect_state == 0) 
			buf[3] = 'N';
		else
			buf[3] = 'L';
		can1_send_data(SEND_EXTID, 0x10FAF155, buf,8);
	}
	
	if(cnt % 100 == 0)     //10S
	{
		struct rt_tm tm;
		
		rt_get_rtc(&tm);
		rt_memset(buf,0,8);	
		buf[0] = tm.year;      //LTE模块状态
		buf[1] = tm.mon;      //SIM卡状态
		buf[2] = tm.day;
		buf[3] = tm.hour;
		buf[4] = tm.min;
		buf[5] = tm.sec;
		
		buf[6] = read_lte_net_init_state();
		
		can1_send_data(SEND_EXTID, 0x18FAF158, buf,8);
	}
	
	if(cnt % 12 == 0)  //8S周期
	{
		if(flag == 0)
		{
			flag = 1;
			memset(buf,0,8);
			switch(step)
			{
				case 0:
					buf[0] = 0xE5;
					buf[1] = 0xFE;
					can1_send_data(SEND_EXTID,0x18EA0021,buf,8);
					step++;
					break;
				
				case 1:
					buf[0] = 0xE9;
					buf[1] = 0xFE;
					can1_send_data(SEND_EXTID,0x18EA0021,buf,8);
					step++;
					break;
				case 2:
					buf[0] = 0xD5;            //请求 EGR信息
					buf[1] = 0xFD;
					can1_send_data(SEND_EXTID,0x18EA0021,buf,8);
					step = 0;
					break;
			}
		}
		else
		{
			switch(ls)
			{
				case 0:      //玉柴  锁车
					memset(buf,0,8);
					buf[0] = 0x01;            //请求 锁车信息
					buf[1] = 0xFD;
					can1_send_data(SEND_EXTID,0x18EA0021,buf,8);
					ls++;
					break;
				case 1:
					flag = 0;
					//rt_kprintf("-- the lock cmd: %d %x,%x,%x,%x\r\n",lock_data.mon_cmd,lock_back.bind_seed[0],lock_back.bind_seed[1],lock_data.seed[0],lock_data.seed[1]);
					//if(lock_data.mon_cmd == 1 && (str_compare(lock_back.bind_seed,lock_data.seed,4) == 0 || *(uint32_t *)lock_data.seed == 0))          //激活命令  （玉柴）
					if(lock_data.mon_cmd == 1 && (*(uint32_t *)lock_data.seed == 0))          //激活命令  （玉柴）
					{
						//printf("-- this is :%d,%d\r\n",lock_data.active_status,vehicle_args.lock_mon);
						if(lock_back.active_status == lock_expect_info.mon_expect_state && read_can_connect_state(1) == 0)
						{
							lock_data.mon_cmd  = 0;
							lock_expect_info.mon_cmd_res_state = 0;  
							lock_data.index = 0;
							save_lock_expect_info();
							ls = 0;
							break ;
						}
				
						lock_data.mon_cmd  = 0;
						memset(buf,0,sizeof(buf));
						buf[0] = lock_data.bind_code[0];
						buf[1] = lock_data.bind_code[1];
						if(lock_expect_info.mon_expect_state == 1)									//激活命令
						{
							read_config_gps_id(&buf[2],4);
							buf[6] = 0xFF;
							buf[7] = 0xFF;
						}
				
						rt_kprintf("-- Send YUCHAI Mon Cmd %d\r\n",lock_data.index);
						can1_send_data(SEND_EXTID,0x18FE01FB, (uint8_t *)&buf[0],8);   //
						memcpy(lock_back.bind_seed,lock_data.seed,4);
						write_ecu_lock_back();
						ls = 0;
						break;		
					}
					else
					{
						lock_data.mon_cmd = 0;
					}
					
					if(lock_data.lock_cmd == 1)            //执行锁车命令 (玉柴)
					{
						//rt_kprintf("-- run this is.....%d,%d\r\n",lock_data.ecu_lock_res,lock_expect_info.lock_expect_state);
				
						if(lock_expect_info.lock_expect_state == 0)          //解锁
						{
							if(lock_back.initiative_lock == lock_expect_info.lock_expect_state && read_can_connect_state(1) == 0)
							{
								lock_data.index = 0;
								lock_data.lock_cmd = 0;
								lock_expect_info.lock_cmd_res_state = 0;
								save_lock_expect_info();
								ls = 0;
								break;
							}
						}
						else  //锁车
						{
							if(lock_data.ecu_lock_res == lock_expect_info.lock_expect_state && read_can_connect_state(1) == 0)
							{
								lock_data.index = 0;
								lock_data.lock_cmd = 0;
								lock_expect_info.lock_cmd_res_state = 0;
								save_lock_expect_info();
								ls = 0;
								break;
							}
						}
					
						memset(buf,0,sizeof(buf));

						buf[4] = lock_data.lock_code[0];
						buf[5] = lock_data.lock_code[1];
						buf[6] = lock_data.lock_code[2];
						buf[7] = lock_data.lock_code[3];
						if(lock_expect_info.lock_expect_state == 0)          //解锁
						{
							speed = 0xFFFF;
							buf[0] = 0;        //限制转速  //解锁
							buf[1] = speed & 0xFF;
							buf[2] = (speed >> 8) & 0xFF;
							buf[3] = 0xFF;
							
						}
						else if(lock_expect_info.lock_expect_state == 1)
						{
							speed = 1000 * 8;
							buf[0] = 2;        //限制转速
							buf[1] = speed & 0xFF;
							buf[2] = (speed >> 8) & 0xFF;
							buf[3] = 0xFF;   //扭矩
								
						}
						else
						{
							speed = 0;
							buf[0] = 1;        //停机  
							buf[1] = speed & 0xFF;
							buf[2] = (speed >> 8) & 0xFF;
							buf[3] = 0xFF;
						}

						lock_data.lock_cmd = 0;					
						can1_send_data(SEND_EXTID,0x18FE03FB, (uint8_t *)&buf,8);  //英轩重工
						rt_kprintf("-- Send YUCHAI Lock Cmd %d  0x02%x,0x02%x,0x02%x,0x02%x\r\n",lock_data.index,buf[4],buf[5],buf[6],buf[7]);
					}	
					ls = 0;
					break;
			}	
		}	
	}
}

//*/


/****************************
**	循环发送
**	CAN-ID:
**	周期发送 50ms
****************************/

 void can_cycle_send_handler(void)
{
	static uint32_t 				cnt = 0;
	static uint8_t 					meter_cnt = 0;
	static uint8_t 					step = 0;
	
	uint8_t							buf[8];

	if(cnt++ < 50)
		return;
	
	
	if(cnt % 10 == 0)     //
	{
		rt_memset(buf,0,8);
		buf[0] = 0x5F;					 //
		buf[1] = 0xAA;					 //
		buf[2] = meter_cnt++;    //
		
		#if 1
		if(lock_expect_info.lock_expect_state == 0) 
			buf[3] = 'N';
		else
			buf[3] = 'L';
		#else
		buf[3] = lock_expect_info.lock_expect_state;
		#endif
		
		can1_send_data(SEND_EXTID, 0x10FAF155, buf,8);
	}
	
	if(cnt % 100 == 0)     //10S
	{
		struct rt_tm tm;
		
		rt_get_rtc(&tm);
		rt_memset(buf,0,8);	
		buf[0] = tm.year;      //LTE模块状态
		buf[1] = tm.mon;      //SIM卡状态
		buf[2] = tm.day;
		buf[3] = tm.hour;
		buf[4] = tm.min;
		buf[5] = tm.sec;
		
		buf[6] = read_lte_net_init_state();
		
		can1_send_data(SEND_EXTID, 0x18FAF158, buf,8);
	}

	
	if(cnt % 20 == 0)  //10S周期
	{
		memset(buf,0,8);
		switch(step)
		{
			case 0:
				buf[0] = 0xE5;
				buf[1] = 0xFE;
				can1_send_data(SEND_EXTID, 0x18EA0021, buf,8);
				step++;
				break;
			case 1:
				buf[0] = 0xE5;
				buf[1] = 0xFE;
				can1_send_data(SEND_EXTID, 0x18EAFF21, buf,3);
				step++;
				break;
			case 2:
				buf[0] = 0xE9;
				buf[1] = 0xFE;
				can1_send_data(SEND_EXTID, 0x18EA0021, buf,8);
				step++;
				break;
			case 3:
				buf[0] = 0xE9;
				buf[1] = 0xFE;
				can1_send_data(SEND_EXTID, 0x18EAFF21, buf,3);
				step++;
				break;
			case 4:          //请求EGR
				buf[0] = 0xD5;
				buf[1] = 0xFD;
				can1_send_data(SEND_EXTID, 0x18EA0021, buf,8);
				step++;
				break;
			case 5:
				buf[0] = 0xD4;
				buf[1] = 0xFE;
				can1_send_data(SEND_EXTID, 0x18EAFF21, buf,3);
				step = 0;
				break;
		}	
	}
}





/*****************************
**	潍柴MD5锁车
**	挖机仪表锁车
******************************/

void thread_entry_can_send(void *parameter)
{
	uint32_t 										res 			= 0;
  uint8_t  										acc_back 	= 0;
	struct rt_can_event 				event = {0};
	uint8_t 										buf[50];
	
	struct down_cmd_res_str 		cmd_res;   //应答
						
	parameter = parameter;
	
	can_lock_mq = rt_mq_create("can_lock_mq",sizeof(struct rt_can_event),3,RT_IPC_FLAG_FIFO);    //

	read_lock_expect_info();							//读锁车状态
	rt_irq_timer3_sethook(can_lock_thread_event);    // 
	can_lock_event = rt_event_create("can1_lock", 1);
	
	init_ecu_lock_back();
	
	for(;;)
	{
		res = rt_event_recv(can_lock_event, 1, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &res);
    if (res != RT_EOK)
		{
			rt_kprintf("can_lock_event failed\r\n");   //打印错误信息
			continue;  //
		}
		
		/********************************
		**	根据电锁变化，重新发送命令
		*********************************/
		////*  
		if(acc_back != read_in_acc_state())    //ACC变化-ACC打开
		{
			if(acc_back == 0)       //ACC有变化，且是打开ACC
			{
				if(lock_expect_info.mon_cmd_res_state == 1)
				{
					lock_data.cmd_res = 0;   //不需要应答  ()
					rt_kprintf("-- Continue Send mon cmd ....\r\n");
					lock_data.mon_cmd = 1;   
				}
				
				if(lock_expect_info.lock_cmd_res_state == 1)
				{
					lock_data.cmd_res = 0;
					rt_kprintf("-- Continue Send Lock cmd....\r\n");
					lock_data.lock_cmd = 1;	   //
				}
			}
			acc_back = read_in_acc_state(); 
		}
		//*/
		switch(read_config_car_type())
		{						
			case 0x02:						 //国三,国四设备
				if(read_ecu_type() == 2)
				{
					can_cycle_send_handler();
					ecu_lock_wc_handle();     //2：潍柴燃油装载机锁车 
				}
				else if(read_ecu_type() == 3)
				{
					can_cycle_send_handler();
					ecu_lock_sc_handle();     //3：上柴燃油装载机锁车
				}
				else if(read_ecu_type() == 4)
				{
					ecu_lock_ychai_handle();
				}
				break;
		}
		
		//rt_kprintf("-- Read Lock expect info:%u,%u\r\n",lock_expect_info.lock_expect_state,lock_expect_info.mon_expect_state);

		if(rt_mq_recv(can_lock_mq,&event,sizeof(event),0) == RT_EOK)         //
		{
			switch(event.cmd)
			{
				case 0:                         //解锁车   来自平台锁车指令
					if(event.arg1 > 6 || read_ecu_mon_status() == 0)
					{
						cmd_res.cmd_id = event.arg4;
						cmd_res.msg_type = event.arg5;
						cmd_res.ser_num[0] = event.arg2;
						cmd_res.ser_num[1] = event.arg3;
						cmd_res.res = 1;
						memset((uint8_t *)&lock_data,0,sizeof(lock_data));
						
						if(cmd_res.cmd_id > 0)    //
						{
							res =  build_gb4_response_packets(buf,sizeof(buf),&cmd_res);	  //应答平台执行成功
							if(res > 0)
								write_qb4_fifo_buff(buf,res);          //
							rt_kprintf("-- recv unlock cmd fail.%d,%d\r\n",event.arg1,lock_expect_info.mon_expect_state);
						}
						break;
					}
					
					lock_expect_info.lock_expect_state = (event.arg1 > 2) ? 2 : event.arg1;           //锁车级别
				
					lock_expect_info.lock_cmd_res_state = 1;   //
						
					save_lock_expect_info();                      //保存锁车状态
					if(read_in_acc_state() > 0)
					{
						lock_data.lock_cmd = 1;										//
						lock_data.cmd_res = 1;												//
					}			
					cmd_res.cmd_id = event.arg4;
					cmd_res.msg_type = event.arg5;
					cmd_res.ser_num[0] = event.arg2;
					cmd_res.ser_num[1] = event.arg3;
					cmd_res.res = 0;
				
					if(cmd_res.cmd_id > 0)    //
					{
						res =  build_gb4_response_packets(buf,sizeof(buf),&cmd_res);	  //应答平台执行成功
						if(res > 0)
							write_qb4_fifo_buff(buf,res);          //
						rt_kprintf("-- recv unlock cmd Ok.%d,%d,%d\r\n",res,event.arg1,lock_expect_info.lock_expect_state);
					}
					break;
				case 1:               //激活锁车功能
					if(event.arg1 > 2)  //如果命令有错误  回复失败
					{
						cmd_res.cmd_id = event.arg4;
						cmd_res.msg_type = event.arg5;
						cmd_res.ser_num[0] = event.arg2;
						cmd_res.ser_num[1] = event.arg3;
						cmd_res.res = 1;  //结果失败
						rt_kprintf("-- recv Mon cmd Fail:%d\r\n",lock_expect_info.lock_expect_state);
					
						memset((uint8_t *)&lock_data,0,sizeof(lock_data));
						res =  build_gb4_response_packets(buf,sizeof(buf),&cmd_res);
						if(res > 0)
							write_qb4_fifo_buff(buf,res);          //
						break;
					}
					
					
					if(event.arg1 == 0 || event.arg1 == 2)  //0 or 2 代表打开锁车功能
					{
						lock_expect_info.mon_expect_state = 1;
						//rt_kprintf("-- Open lock fun....\r\n");						
					}
					else
					{
						memset((uint8_t *)&lock_expect_info,0,sizeof(lock_expect_info));
						//rt_kprintf("-- Close lock fun....\r\n");
					}
					
					lock_expect_info.mon_cmd_res_state = 1;    //标注命令接受状态
					save_lock_expect_info();               //保存锁车状态
					
					if(read_in_acc_state() > 0)
					{
						lock_data.mon_cmd = 1;               //标注发送ECU激活报文
						lock_data.cmd_res = 0;                   //标注ECU激活报文是否回复
					}
			
					
					cmd_res.cmd_id = event.arg4;
					cmd_res.msg_type = event.arg5;
					cmd_res.ser_num[0] = event.arg2;
					cmd_res.ser_num[1] = event.arg3;
					cmd_res.res = 0;  //结果失败
					
					
					if(cmd_res.cmd_id > 0)    //
					{
						res =  build_gb4_response_packets(buf,sizeof(buf),&cmd_res);
						if(res > 0)
							write_qb4_fifo_buff(buf,res);          //
						//rt_kprintf("-- recv mon cmd OK:%d,%d,%d\r\n",res,lock_expect_info.lock_expect_state,lock_expect_info.mon_expect_state);
					}
					break;
				case 3:              //握手信号（潍柴握手信号）
					{
						uint8_t tmp_buf[16];
						uint16_t i;
						uint8_t seed_n[8];
						MD5_CTX mdContext; 
						
						if(read_ecu_type() == 2)
						{
							tmp_buf[0] = 0xA7;
							tmp_buf[1] = 0x6F;
							tmp_buf[2] = 0x3B;
						
							*(uint32_t *)&tmp_buf[3] = event.arg1;
							*(uint32_t *)&tmp_buf[7] = event.arg2;
		
							md5(tmp_buf,(uint32_t *)tmp_buf,8);
							#if WEIC_LOCK_TYPE ==  0     //
							can1_send_data(SEND_EXTID, 0x180001FB, tmp_buf,8);  //青岛雷沃(测试使用)
							#else
							can1_send_data(SEND_EXTID, 0x18FE0CEE, tmp_buf,8);   //英轩重工
							#endif
							rt_kprintf("-- ecu wc handl\r\n");
							break;
						}
						
						if(read_ecu_type() == 4)   //玉柴发动机
						{
							memcpy(lock_data.seed,event.data,4);
							if(((event.data[4] >> 6) & 0x03) == 0)	//主动锁车状态
							{
								lock_back.initiative_lock = 0;
							}
							else if(((event.data[4] >> 6) & 0x03) == 1)
							{
								lock_back.initiative_lock = 2;
							}
							else if(((event.data[4] >> 6) & 0x03) == 2)
							{
								lock_back.initiative_lock = 1;
							}
							
							
							if(((event.data[4] >> 2) & 0x03) == 0)
							{
								lock_back.passive_lock =  0;		//被动锁车状态
							}
							else if(((event.data[4] >> 2) & 0x03) == 1)
							{
								lock_back.passive_lock =  2;
							}
							else if(((event.data[4] >> 2) & 0x03) == 2)
							{
								lock_back.passive_lock = 1;
							}
							
							lock_back.active_status = (event.data[5] >> 6) & 0x03;		 //激活状态
							lock_back.check_status = (event.data[5] >> 4) & 0x03;		   //校验状态
							lock_data.emergency_unlock = (event.data[5] >> 2) & 0x03;	 //紧急解锁状态
							lock_data.emergency_start = (event.data[5]) & 0x03;			   //紧急启动状态
							lock_back.key_state = (event.data[7] >> 2) & 0x03;
							lock_data.res_status = (event.data[7]) & 0x03;
							
							if(((event.data[7] >> 4) & 0x03) == 1)
							{
								lock_data.ecu_lock_res = 2;
							}
							else if(((event.data[7] >> 4) & 0x03) == 2)
							{
								lock_data.ecu_lock_res = 1;
							}
							else
							{
								lock_data.ecu_lock_res = 0;
							}
							
							write_ecu_lock_back();
							
							//rt_kprintf("-- YuChai ECU State:%d,%d,%d,%d,%d,%d\r\n",lock_back.initiative_lock,lock_back.passive_lock,lock_back.active_status,lock_back.check_status,lock_data.emergency_unlock,lock_data.emergency_start);
							
							if(*(uint32_t *)event.data != 0)
							{				
								memcpy(&tmp_buf[0],event.data,4);                     //
								read_config_mask(&tmp_buf[4],4);
								get_rand_str(&tmp_buf[0],&seed_n[0],8);          			   //进行初步转换
 		 						MD5Init(&mdContext);                                   //初始化加密结构
  							MD5Update(&mdContext, seed_n, 8);                      //对数据进行加密
  							MD5Final(tmp_buf,&mdContext);

								lock_data.bind_code[0] = tmp_buf[10];              //解绑密码
								lock_data.bind_code[1] = tmp_buf[11];
								//rt_kprintf("-- the bind code:0x%x,0x%x\r\n",lock_data.bind_code[0],lock_data.bind_code[1]);
								
								//rt_kprintf("-- the lock code:");
								for(i = 0; i < 4; i++)                                //主动锁车密码
								{
									lock_data.lock_code[i] = tmp_buf[i + 4];  
									//rt_kprintf("0x%02x ",lock_data.lock_code[i]);
								}
								
								read_config_gps_id(seed_n,sizeof(seed_n));
								
								//rt_kprintf("\r\n");
								//rt_kprintf("-- the hand code:");
								for(i = 0; i < 4; i++)                               
								{
									lock_data.check_code[i * 2 + 1] = seed_n[i];  //握手校验码
									lock_data.check_code[i * 2] = tmp_buf[i + 6];
									
									//rt_kprintf("0x%02x 0x%02x ",lock_data.check_code[i * 2],lock_data.check_code[i * 2 + 1]);
								}
								
								rt_kprintf("\r\n");
								
								lock_data.fixed_key[0] = 0xFF;
								lock_data.fixed_key[1] = 0xFF;  
								
								if(lock_back.check_status == 3 && lock_back.active_status > 0 && lock_data.res_status > 0)    //握手状态
								{
									can1_send_data(SEND_EXTID,0x18FE02FB, lock_data.check_code,8);   //英轩重工
									rt_kprintf("-- yuchai ecu handl\r\n");
								}	
								else
								{
									if(lock_back.check_status == 1 && lock_back.active_status > 0)
									{
										memset(tmp_buf,0,sizeof(tmp_buf));
										can1_send_data(SEND_EXTID,0x18FE02FB,tmp_buf,8);   //英轩重工
									}
								}
							}
					}
					break;
				case 6:                //生产配置的时候，清除激活锁车功能
					rt_memset((uint8_t *)&lock_expect_info,0,sizeof(lock_expect_info));
					save_lock_expect_info();
					break;
				}
			}
		}
	}
}


