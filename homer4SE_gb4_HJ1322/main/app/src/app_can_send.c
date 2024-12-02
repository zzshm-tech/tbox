



#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_task_wdt.h"
#include "esp_netif.h"
#include "esp_partition.h"

#include <esp_event.h>


#include "version.h"

#include "drv_can.h"

#include "common.h"
#include "pro_data.h"

#include "app_lte.h"
#include "app_in.h"
#include "app_can_send.h"
#include "app_products.h"
#include "app_gnss.h"
#include "app_can_recv.h"
#include "md5_code.h"
#include "md5.h"
#include "mbedtls/aes.h"


#define WEIC_LOCK_TYPE  1  /** 1:山东肯石 0：青岛雷沃 **/




static struct vehicle_args_t 					vehicle_args = {0};

static struct lock_data_t						lock_data = {0};

static RTC_DATA_ATTR struct lock_back_t			lock_back = {0};    //针对玉柴锁车数据备份

/***************************************************************************/

static QueueHandle_t  							can_send_queue = NULL;   //生产队列




/******************
 * 返回玉柴ECU类型的版本
*/
uint8_t read_yuc_ecu_type(void)
{
	uint8_t rv = 0;

	rv =  vehicle_args.ecu_type;

	return rv;
}


/**********************************
 **	返回玉柴锁车状态
 **********************************/

uint8_t get_yuc_key_state(void)
{
	uint8_t rv = 0;

	if(lock_back.key_state == 3)
		rv = 1;
	else 
		rv = 0;
		
	return rv;
}


uint8_t get_yuc_mon_state(void)
{
	uint8_t rv;

	rv = lock_back.active_status;
	
	if(rv > 1)
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




/*********************************
 **	返回玉柴锁车状态
 *********************************/

uint8_t get_yuc_lock_state(void)
{
	uint8_t rv;

	if(lock_back.passive_lock > 0)      //被动锁车
	{
		rv = lock_back.passive_lock;
		
		return rv; 
	}
	
	if(lock_back.initiative_lock == 0 && vehicle_args.lock_state > 0)
	{
		return 9;
	}
	else
	{
		rv = lock_back.initiative_lock;
	}
	return rv;
}



/********************************************
**
*********************************************/

QueueHandle_t get_can_send_queue(void)
{
	return can_send_queue;
}




/********************************************
**
*********************************************/

uint8_t read_vehicle_args_lock_mon(void)
{
	uint8_t rv = 0;

	rv = vehicle_args.lock_mon;

	return rv;
}





/********************************************
**
*********************************************/
uint8_t read_vehicle_args_lock_state(void)
{
	uint8_t rv = 0;

	rv = vehicle_args.lock_state;

	return rv;
}





/**********************************
**	保存配置信息
***********************************/

uint8_t erase_vehicle_args_info(void)
{
	const esp_partition_t *partition = NULL;

    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY,"nvs");
   
    if(partition == NULL)
    {
        printf("-- vehicle args esp partition find first fail...\r\n");
        return 1;
    }
	printf("-- The vehicle args Area:%d\r\n",partition->size);
                                          //计算CRC32值
	if (ESP_OK != esp_partition_erase_range(partition, 0, partition->size))
    {
        printf("-- erase struct vehicle_argst faile \r\n");
        return 1;
    }
	
	printf("-- erase vehicle args info ... \r\n");
	return 0;
}



/**********************************
**	保存配置信息
***********************************/

uint8_t save_vehicle_args_info(void)
{
	const esp_partition_t *partition = NULL;
    uint8_t read_data[1024] = {0};

    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY,"nvs");
   
    if(partition == NULL)
    {
        printf("-- vehicle args esp partition find first fail...\r\n");
        return 1;
    }
	//printf("-- The vehicle args Area:%d\r\n",partition->size);
	esp_partition_read(partition, 0, read_data,sizeof(struct vehicle_args_t));
	
	vehicle_args.flag = 0x55;
                                          //计算CRC32值
	if (ESP_OK != esp_partition_erase_range(partition, 0, partition->size))
    {
        printf("-- erase struct vehicle_argst faile \r\n");
        return 1;
    }

    if (ESP_OK != esp_partition_write(partition, 0, (uint8_t *)&vehicle_args,sizeof(struct vehicle_args_t)))
    {
        printf("-- write vehicle args faile \r\n");
        if(ESP_OK != esp_partition_write(partition, 0, read_data, sizeof(struct vehicle_args_t)))
        {
            return 1;
        }
    }

	printf("-- Save lock info ................. OK\r\n");
	
	return 0;
}






/****************************************
**  加载整车参数
*****************************************/

uint8_t load_vehicle_args_info(void)
{
    const esp_partition_t *partition = NULL;
    
    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY,"nvs");
   
    if(partition == NULL)
    {
        printf("-- vehicle args esp partition find first fail...\r\n");
        return 1;
    }
    // Read back the data, checking that read data and written data match
    if(ESP_OK == esp_partition_read(partition, 0,(uint8_t *)&vehicle_args,sizeof(struct vehicle_args_t)))
	{
		if(vehicle_args.flag != 0x55)
		{
			memset((uint8_t *)&vehicle_args,0,sizeof(struct vehicle_args_t));
			
		}
	}
	else
	{
		printf("-- load vehicle args info error....\r\n");
	}
       
	printf("-- load vehicle :%d,%d,%d\r\n",vehicle_args.flag,vehicle_args.mon_res_state,vehicle_args.lock_mon);
    
	return 0;
}




/********************************************
**
**********************************************/

static void anti_dismantle_handle(void)
{
	static uint32_t 		cnt = 1;
	static uint8_t 			flag1 = 0;
	static uint8_t 			heart_cnt = 0;
	static uint8_t 			step = 0;

    uint8_t					buf[8] = {0};


	if(read_in_acc_state() == 0)
		return ;

	cnt++;

	if(cnt % 20 == 0)     //
	{
		memset(buf,0,8);
		if(flag1 == 0)
		{
			buf[0] = 0x55; 
			flag1 = 1;
		}			
		else
		{
			buf[0] = 0xAA;
			flag1 = 0;
		}

		buf[1] = read_in_batter_vol();    //TBOX内部电池电压值
		*(uint16_t *)&buf[2] = read_in_power_vol();    //TBOX设备外部输入电源电压值
		
		rt_can_send(EXTID, 0x18BC1058,buf,8);
	}

	if(cnt % 150 == 0)
	{
	
		memset(buf,0,8);
		buf[0] = heart_cnt++;
			
		buf[1] = 0x01;
		if(read_lte_net_init_state() == 0)
			buf[2] = 0x02;
		else
			buf[2] = 0x01;
			
		if(read_gnss_positing_state() == 'V')
			buf[3] = 0x02;
		else
			buf[3] = 0x01;
		rt_can_send(EXTID,0x18FF36F9,buf,8);   //东风井关（也是用）	
	}

	if(cnt % 150 == 0)
	{
		switch(step)
		{
			case 0:
				memset(buf,0xFF,8);
				*(uint16_t *)buf= 0xA001;
				buf[6] = 0x00;
				if(read_lte_net_init_state() == 0)
					buf[6] |= 0x01;
			
				if(read_gnss_positing_state() == 'V')
					buf[6] |= 0x02;
				rt_can_send(EXTID,0x18FEA04B,buf,8);   //
				step++;
				break;
			case 1:
				if(read_hmi_mon_state() == 1)
				{
					memset(buf,0xFF,8);
					*(uint16_t *)buf= 0xB001;
					buf[6] = 0x00;
					if(read_lte_net_init_state() == 0)
						buf[6] |= 0x01;
			
					if(read_gnss_positing_state() == 'V')
						buf[6] |= 0x02;
					rt_can_send(EXTID,0x18FEA04B,buf,8);   //
				}
				else
				{
					step = 0;
				}
				break;
			default:
				step = 0;
				break;
		}
	}

}



/****************************
**	循环发送
**	CAN-ID:
**	周期发送 50ms
****************************/

 void can_gb27145_send_handler(uint8_t state)
{
	static uint32_t cnt = 0;
    uint8_t					buf[8];
	
	cnt++;

	if(state == 1)
	{	
		cnt = 1;
		memset(buf,0xAA,8);
		buf[0] = 0x05;
		buf[1] = 0x19;
		buf[2] = 0x42;
		buf[3] = 0x33;
		buf[4] = 0x0C;
		buf[5] = 0x1E;
		rt_can_send(EXTID, 0x18DA00F1,buf,8);
	}

	if(state == 2)
	{
		cnt = 1;
		memset(buf,0xAA,8);
		buf[0] = 0x30;
		buf[1] = 0x00;
		buf[2] = 0x00;
		rt_can_send(EXTID, 0x18DA00F1,buf,8);
	}

	if(cnt % 400 == 0)     //
	{
		memset(buf,0xAA,8);
		buf[0] = 0x03;
		buf[1] = 0x22;
		buf[2] = 0xF4;
		buf[3] = 0x01;
		rt_can_send(EXTID, 0x18DA00F1,buf,8);	
	}
}


/***************************************
**
****************************************/

uint8_t write_data_to_can(uint8_t *data,uint16_t size)
{
	uint8_t 	i = 0;
	uint16_t 	part_len = 0;
	uint8_t 	check = 0;
	uint8_t 	array[8] = {0};

	while(part_len < size)
	{
		for(i = 0;i < 8;i++)
		{
			if(part_len < size)
			{
				array[i] = *(data + part_len);
				part_len++;
			}
			else 
			{
				array[i] = 0xFF;
			}
			
			check ^= array[i];
		}
		rt_can_send(EXTID, 0x18BCA058,array,8);
		vTaskDelay(1);
	}
			
	array[0] = check;
	*(uint16_t *)&array[1] = part_len;
	array[3] = 0x55;
	array[4] = 0xAA;
	array[5] = 0x55;
	array[6] = 0xAA;
	array[7] = 0x55;
	rt_can_send(EXTID, 0x18BCB058,array,8);


	return 0;
}









/*************************************
** 潍柴ECU锁车命令
*************************************/

static void ecu_lock_ychai_handle(void)
{
	static uint32_t 		cnt = 0;
	static uint8_t			step = 0;
	static uint8_t 			flag = 0;
	static uint8_t 			ls = 0;
	static uint8_t 			acc_back = 0;
	uint16_t		 		speed;
    uint8_t					buf[8];
	


	cnt++;
	if(acc_back != read_in_acc_state())
	{
		if(acc_back == 0)
		{
			cnt = 0;
			flag = 1;
			ls = 0;
		}
		acc_back = read_in_acc_state();
	}


	if(cnt % 20 == 0)  //15S周期
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
					rt_can_send(EXTID,0x18EA0021,buf,8);
					step++;
					break;
				case 1:
					buf[0] = 0xE9;
					buf[1] = 0xFE;
					rt_can_send(EXTID,0x18EA0021,buf,8);
					step++;
					break;
				case 2:
					buf[0] = 0xD5;            //请求 EGR信息
					buf[1] = 0xFD;
					rt_can_send(EXTID,0x18EA0021,buf,8);
					step = 0;
					break;
			
			}	
		}
		else
		{
			switch(ls)
			{
				case 0:
					memset(buf,0,sizeof(buf));
					buf[0] = 0x01;            //请求 锁车信息
					buf[1] = 0xFD;
					rt_can_send(EXTID,0x18EA0021,buf,8);
					ls++;
					break;
				case 1:
					flag = 0;
					if(lock_data.mon_cmd == 1 && (str_compare(lock_back.bind_seed,lock_data.seed,4) == 0 || *(uint32_t *)lock_data.seed == 0))          //激活命令  （玉柴）
					{
						if(lock_back.active_status == vehicle_args.lock_mon)
						{
							lock_data.mon_cmd  = 0;
							vehicle_args.mon_res_state = 0;  
							lock_data.index = 0;
							save_vehicle_args_info();
							return ;
						}
						lock_data.mon_cmd = 0;
						memset(buf,0,sizeof(buf));
						buf[0] = lock_data.bind_code[0];
						buf[1] = lock_data.bind_code[1];
						if(vehicle_args.lock_mon == 1)									//激活命令
						{
							read_config_gps_id(&buf[2],4);
							buf[6] = 0xFF;
							buf[7] = 0xFF;
						}
						printf("-- Send YUCHAI Mon Cmd %d\r\n",lock_data.index);
						rt_can_send(EXTID, 0x18FE01FB, (uint8_t *)&buf[0],8);   
						memcpy(lock_back.bind_seed,lock_data.seed,4);
						ls = 0;
						break;;		
					}
					else
					{
						lock_data.mon_cmd = 0;
					}

					if(lock_data.lock_cmd == 1)            //执行锁车命令 (玉柴)
					{
						if(vehicle_args.lock_state == 0)          //解锁
						{
							if(lock_back.initiative_lock == vehicle_args.lock_state)
							{
								lock_data.index = 0;
								lock_data.lock_cmd = 0;
								vehicle_args.lock_res_state = 0;
								save_vehicle_args_info();
								ls = 0;
								break;
							}
						}
						else  //锁车
						{
							if(lock_data.ecu_lock_res == vehicle_args.lock_state)
							{
								lock_data.index = 0;
								lock_data.lock_cmd = 0;
								vehicle_args.lock_res_state = 0;
								save_vehicle_args_info();
								ls = 0;
								break;
							}
						}

						memset(buf,0,sizeof(buf));

						buf[4] = lock_data.lock_code[0];
						buf[5] = lock_data.lock_code[1];
						buf[6] = lock_data.lock_code[2];
						buf[7] = lock_data.lock_code[3];
						if(vehicle_args.lock_state == 0)          //解锁
						{
							speed = 0xFFFF;
							buf[0] = 0;        //限制转速  //解锁
							buf[1] = speed & 0xFF;
							buf[2] = (speed >> 8) & 0xFF;
							buf[3] = 0xFF;
							
						}
						else if(vehicle_args.lock_state == 1)
						{
							speed = 1000 * 8;
							buf[0] = 2;        //限制转速
							buf[1] = speed & 0xFF;
							buf[2] = (speed >> 8) & 0xFF;
							buf[3] = 0xFF;
								
						}
						else
						{
							speed = 0;
							buf[0] = 1;        //限制转速  //解锁
							buf[1] = speed & 0xFF;
							buf[2] = (speed >> 8) & 0xFF;
							buf[3] = 0xFF;
						}	
						lock_data.lock_cmd = 0;	
						rt_can_send(EXTID,0x18FE03FB, (uint8_t *)&buf,8);  //英轩重工
						printf("-- Send YUCHAI Lock Cmd %d  0x02%x,0x02%x,0x02%x,0x02%x\r\n",lock_data.index,buf[4],buf[5],buf[6],buf[7]);
					}	

					ls = 0;
					break;
			}
		}
	}
}




/*************************************
** 潍柴ECU锁车命令
*************************************/

static void ecu_lock_wc_handle(void)
{
	static uint32_t 		cnt;
	static uint8_t 			step = 0;

	uint8_t					buf[100];		

	cnt++;

	if(cnt % 20 == 0)  //15S周期
	{
		memset(buf,0,8);
		switch(step)
		{
			case 0:
				buf[0] = 0xE5;
				buf[1] = 0xFE;
				rt_can_send(EXTID,0x18EA0021,buf,8);
				step++;
				break;
			case 1:
				buf[0] = 0xE9;
				buf[1] = 0xFE;
				rt_can_send(EXTID,0x18EA0021,buf,8);
				step++;
				break;
			case 2:
				buf[0] = 0xD5;            //请求 EGR信息
				buf[1] = 0xFD;
				rt_can_send(EXTID,0x18EA0021,buf,8);
				step = 0;
				break;
			default:
				step = 0;
				break;
		}	
	}
	
	if(cnt % 10 == 0)        //1秒钟执行一次
	{
		switch(lock_data.step)         //状态机1 激活ECU锁车功能
		{
			case 0:
				if(lock_data.mon_cmd == 1)          //激活命令
				{
					lock_data.index = 1;
					lock_data.step = 1;
					if(vehicle_args.lock_mon == 1)									//激活命令
					{
						#if WEIC_LOCK_TYPE ==  0     // 
						*(uint16_t *)buf = 0x18AB;    //肯石
						#elif WEIC_LOCK_TYPE == 1
						*(uint16_t *)buf = 0xCA0E;    //青岛雷沃  （测试使用）
						#else
						*(uint16_t *)buf = 0x6715;      //英轩重工
						#endif
					}
					else           //关闭激活
					{
						#if WEIC_LOCK_TYPE ==  0     // 0:
						*(uint16_t *)buf = 0x36FE;   // 肯石工程
						#elif WEIC_LOCK_TYPE == 1
						*(uint16_t *)buf = 0x8747;   //青岛雷沃（测试使用）
						#else
						*(uint16_t *)buf = 0x5176;   //英轩重工
						#endif
					}
	
					read_config_dev_id(buf + 2,3);    //设备编号
					buf[5] = 0xA7;
					buf[6] = 0x6F;
					buf[7] = 0x3B;
							
					printf("-- Send Mon Cmd %d\r\n",lock_data.index);
					#if WEIC_LOCK_TYPE ==  0     //
					rt_can_send(EXTID, 0x18FFD6F1, (uint8_t *)&buf[0],8);//青岛雷沃
					#elif WEIC_LOCK_TYPE == 1
					rt_can_send(EXTID, 0x180000FB, (uint8_t *)&buf[0],8);//青岛雷沃
					#else
					rt_can_send(EXTID, 0x18FE0BEE, (uint8_t *)&buf[0],8);   //英轩重工
					#endif
					break;
				}
					
				if(lock_data.lock_cmd == 1)            //执行锁车命令
				{
					if(vehicle_args.lock_state == 0)          //解锁
					{
						*(uint16_t *)buf = 3500 * 8;
					}
					else
					{
						if(vehicle_args.lock_state == 1)     //一级锁车
							*(uint16_t *)buf = 950 * 8;
						else if(vehicle_args.lock_state == 0)     //二级锁车
							*(uint16_t *)buf = 0;
					}
							
					buf[2] = 0xFF;
					buf[3] = 0xFF;
					buf[4] = 0xFF;
					read_config_dev_id(&buf[5],3);           //设备ID
						
					#if WEIC_LOCK_TYPE ==  0     // 
					rt_can_send(EXTID, 0x18FFD8F1, (uint8_t *)&buf,8);   //青岛雷沃
					#elif WEIC_LOCK_TYPE ==  1     //
					rt_can_send(EXTID, 0x180002FB, (uint8_t *)&buf,8);   //青岛雷沃
					#else
					rt_can_send(EXTID, 0x18FE0DEE, (uint8_t *)&buf,8);  //英轩重工
					#endif
						
					lock_data.index = 1;
					lock_data.step = 2;        //锁车
					printf("-- Send Lock Cmd %d\r\n",lock_data.index);
				}
				break;				
			case 1:                                      //状态1  激活命令
				if(read_wc_mon_state() == vehicle_args.lock_mon)
				{
					lock_data.mon_cmd  = 0;  //停止发送命令，
					lock_data.step = 0;
					vehicle_args.mon_res_state = 0;
					save_vehicle_args_info();
					break;
				}
						
				if(lock_data.index >= 5)
				{
					lock_data.mon_cmd  = 0;
					lock_data.step = 0;
					break;
				}
				if(vehicle_args.lock_mon == 1)
				{
					#if WEIC_LOCK_TYPE ==  0     //
					*(uint16_t *)buf = 0x18AB;    //青岛雷沃  （测试使用）
					#else
					*(uint16_t *)buf = 0x36DE;   //英轩重工
					#endif
				}
				else
				{
					#if WEIC_LOCK_TYPE ==  0     //
					*(uint16_t *)buf = 0x36FE;   //青岛雷沃（测试使用）
					#else
					*(uint16_t *)buf = 0x5176;
					#endif
				}

				read_config_dev_id(buf + 2,3);
				buf[5] = 0xA7;
				buf[6] = 0x6F;
				buf[7] = 0x3B;
					
				#if WEIC_LOCK_TYPE ==  0     //
				rt_can_send(EXTID, 0x18FFD6F1, (uint8_t *)&buf[0],8);  //青岛雷沃
				#elif WEIC_LOCK_TYPE ==  1
				rt_can_send(EXTID, 0x180000FB, (uint8_t *)&buf[0],8);  //青岛雷沃
				#else
				rt_can_send(EXTID, 0x18FE0BEE, (uint8_t *)&buf[0],8);   //英轩重工
				#endif
				lock_data.index++;
				printf("-- Send Mon Cmd %d,%d,%d\r\n",lock_data.index,read_wc_mon_state(),vehicle_args.lock_mon);
				break;
			case 2:
				if(read_wc_pre_lock_state() == ((vehicle_args.lock_state > 0) ? 1:0))
				{
					lock_data.lock_cmd = 0;
					lock_data.step = 0;
					vehicle_args.lock_res_state = 0;
					save_vehicle_args_info();
					break;
				}
				if(lock_data.index >= 5)
				{
					lock_data.lock_cmd = 0;
					lock_data.step = 0;
					break;
				}
				if(vehicle_args.lock_state == 0)          //解锁
				{
					*(uint16_t *)buf = 3500 * 8;
				}
				else if(vehicle_args.lock_state == 1)     //一级锁车
				{
					*(uint16_t *)buf = 950 * 8;
				}
				else if(vehicle_args.lock_state == 2)     //二级锁车
				{
					*(uint16_t *)buf = 0;
				}
				buf[2] = 0xFF;
				buf[3] = 0xFF;
				buf[4] = 0xFF;
				read_config_dev_id(&buf[5],3);           //设备ID
				#if WEIC_LOCK_TYPE ==  0     // 
				rt_can_send(EXTID, 0x18FFD8F1, (uint8_t *)&buf,8);  //青岛雷沃
				#elif WEIC_LOCK_TYPE ==  1 
				rt_can_send(EXTID, 0x180002FB, (uint8_t *)&buf,8);  //青岛雷沃
				#else
				rt_can_send(EXTID, 0x18FE0DEE, (uint8_t *)&buf,8);//英轩重工 
				#endif
				lock_data.index++;
				printf("-- Send Lock Cmd %d\r\n",lock_data.index);
				break;
			}
	}
}






/**********************
**  CAN 发送
************************/

void thread_entry_can_send(void *parameter)
{
	// uint8_t 				acc_back = 0;
	struct can_send_mq_t 	cs_mq = {0};

    parameter = parameter;

	can_send_queue = xQueueCreate(1,sizeof(struct can_send_mq_t));

	load_vehicle_args_info();    //放到主函数读取

	//电锁有变化  重新发送锁车

    for (;;)
    {
		// if(acc_back != read_in_acc_state())    //ACC变化-ACC打开
		// {
		// 	printf("-- the acc state open ......  \r\n");
		// 	if(acc_back == 0)       //ACC有变化，且是打开ACC
		// 	{
		// 		if(vehicle_args.mon_res_state == 1)
		// 		{
		// 			lock_data.cmd_res = 0;   //不需要应答  ()
		// 			printf("-- Continue Send mon cmd ....\r\n");
		// 			lock_data.mon_cmd = 1;   
		// 		}
				
		// 		if(vehicle_args.lock_res_state == 1)
		// 		{
		// 			lock_data.cmd_res = 0;
		// 			printf("-- Continue Send Lock cmd....\r\n");
		// 			lock_data.lock_cmd = 1;	   //
		// 		}
		// 	}
		// 	acc_back = read_in_acc_state(); 
		// }
		
		anti_dismantle_handle();

		if(read_ecu_manu_type() == 4)
		{
			ecu_lock_ychai_handle();
		}
		else
		{
			ecu_lock_wc_handle();  			//潍柴锁车
		}

		memset((uint8_t *)&cs_mq,0,sizeof(struct can_send_mq_t));
		if(xQueueReceive(can_send_queue,&cs_mq,10) == pdTRUE)
		{
			switch(cs_mq.cmd)
			{
				case 0:       //保存玉柴Mask码的类型  登录时仪表自动下发。
					{
						uint8_t m = *(uint16_t *)&cs_mq.data[0];
				
						if(m != vehicle_args.ecu_type)
						{
							vehicle_args.ecu_type = m;
							save_vehicle_args_info();
							printf("-- Save Mask Code type.... %d\r\n",vehicle_args.ecu_type);
						}
					}
					break;
				case 4:          //激活锁车功能，解锁车命令
					{
						if(cs_mq.data[0] == 0)
						{
							vehicle_args.lock_mon = cs_mq.data[1];
							printf("-- Mon CMD..... %d\r\n",vehicle_args.lock_mon);
							lock_data.mon_cmd = 1;   //发送激活命令
							vehicle_args.mon_res_state = 1;
						}
						else   //解锁车
						{
							vehicle_args.lock_state = cs_mq.data[1];
							printf("-- Lock CMD..... %d\r\n",vehicle_args.lock_state);
							lock_data.lock_cmd = 1;
							vehicle_args.lock_res_state = 1;
						}
						
						
						save_vehicle_args_info();
					}
					break;
				case 7:            //应答潍柴、玉柴握手信号
					{
						uint8_t tmp_buf[16];
						uint8_t seed_n[8];
						uint32_t  i;

						MD5_CTX mdContext; 
						
						memset(tmp_buf,0,sizeof(tmp_buf));
						memset(seed_n,0,sizeof(seed_n));

						if(read_ecu_manu_type() == 2)
						{
							tmp_buf[0] = 0xA7;
							tmp_buf[1] = 0x6F;
							tmp_buf[2] = 0x3B;
						
							*(uint32_t *)&tmp_buf[3] = *(uint32_t *)&cs_mq.data[0];
							*(uint32_t *)&tmp_buf[7] = *(uint32_t *)&cs_mq.data[4];
		
							md5(tmp_buf,(uint32_t *)tmp_buf,8);
							#if WEIC_LOCK_TYPE ==  0     //
							rt_can_send(EXTID,0x18FFD7F1,tmp_buf,8);  //
							#elif WEIC_LOCK_TYPE ==  1     //
							rt_can_send(EXTID, 0x180001FB, tmp_buf,8);  //青岛雷沃(测试使用)
							#else
							rt_can_send(EXTID,0x18FE0CEE,tmp_buf,8);   //
							#endif
							printf("-- ecu wc handl\r\n");
							break;
						}

						if(read_ecu_manu_type() == 4)   //玉柴发动机
						{
							memcpy(lock_data.seed,cs_mq.data,4);    //当前的SEED

							if(((cs_mq.data[4] >> 6) & 0x03) == 0)	//主动锁车状态
							{
								lock_back.initiative_lock = 0;
							}
							else if(((cs_mq.data[4] >> 6) & 0x03) == 1)
							{
								lock_back.initiative_lock = 2;
							}
							else if(((cs_mq.data[4] >> 6) & 0x03) == 2)
							{
								lock_back.initiative_lock = 1;
							}
							
							
							if(((cs_mq.data[4] >> 2) & 0x03) == 0)
							{
								lock_back.passive_lock =  0;		//被动锁车状态
							}
							else if(((cs_mq.data[4] >> 2) & 0x03) == 1)
							{
								lock_back.passive_lock =  2;
							}
							else if(((cs_mq.data[4] >> 2) & 0x03) == 2)
							{
								lock_back.passive_lock = 1;
							}
							
							

							lock_back.active_status = (cs_mq.data[5] >> 6) & 0x03;		//激活状态
							lock_back.check_status = (cs_mq.data[5] >> 4) & 0x03;		//校验状态
							lock_data.emergency_unlock = (cs_mq.data[5] >> 2) & 0x03;	//紧急解锁状态
							lock_data.emergency_start = (cs_mq.data[5]) & 0x03;			//紧急启动状态
							lock_back.key_state = (cs_mq.data[7] >> 2) & 0x03;          //

							if(((cs_mq.data[7] >> 4) & 0x03) == 1)
							{
								lock_data.ecu_lock_res = 2;
							}
							else if(((cs_mq.data[7] >> 4) & 0x03) == 2)
							{
								lock_data.ecu_lock_res = 1;
							}
							else
							{
								lock_data.ecu_lock_res = 0;
							}
							
							//printf("-- YuChai ECU State:%d,%d,%d,%d,%d,%d\r\n",lock_back.initiative_lock,lock_back.passive_lock,lock_back.active_status,lock_back.check_status,lock_data.emergency_unlock,lock_data.emergency_start);
							
							if(*(uint32_t *)cs_mq.data != 0)
							{
								memcpy(&tmp_buf[0],cs_mq.data,4);                     //  p_ecu->SEED
								read_config_mask(&tmp_buf[4],4,vehicle_args.ecu_type);
								get_rand_str(&tmp_buf[0],&seed_n[0],8);          			   //进行初步转换
 		 						MD5Init(&mdContext);                                   //初始化加密结构
  								MD5Update(&mdContext, seed_n, 8);                      //对数据进行加密
  								MD5Final(tmp_buf,&mdContext);

								lock_data.bind_code[0] = tmp_buf[10];              //解绑密码
								lock_data.bind_code[1] = tmp_buf[11];
								//printf("-- the bind code:0x%x,0x%x\r\n",lock_data.bind_code[0],lock_data.bind_code[1]);
								for(i = 0; i < 4; i++)                                //主动锁车密码
								{
									lock_data.lock_code[i] = tmp_buf[i + 4];  
									//printf("0x%02x ",lock_data.lock_code[i]);
								}
								//printf("\r\n");

								read_config_gps_id(seed_n,sizeof(seed_n));
								for(i=0; i < 4; i++)                               
								{
									lock_data.check_code[i * 2 + 1] = seed_n[i];  //握手校验码
									lock_data.check_code[i * 2] = tmp_buf[i + 6];
								}
								lock_data.fixed_key[0] = 0xFF;
								lock_data.fixed_key[1] = 0xFF;  

								

								if(lock_back.check_status != 1)    //握手状态
								{
									rt_can_send(EXTID, 0x18FE02FB, lock_data.check_code,8);   //
									printf("-- yuchai ecu handl%d\r\n",vehicle_args.ecu_type);
								}	
								else
								{
									memset(tmp_buf,0,sizeof(tmp_buf));
									rt_can_send(EXTID, 0x18FE02FB,tmp_buf,8);   //
								}
							}
							
						}
					}
					break;
				default:
					break;
			}
		}
    } 
}



