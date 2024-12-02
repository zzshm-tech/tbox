

#include <stdio.h>
#include <string.h>

#include <rtthread.h>
#include <rtdevice.h>

#include "dfs_posix.h"
#include "dfs_fs.h"

#include "pro_data.h"
#include "common.h"

#include "app_packet.h"
#include "app_gnss.h"
#include "app_lte.h"
#include "app_fifo.h"
#include "app_mon.h"
#include "app_can_recv.h"    
#include "app_shell.h"
#include "app_gb4.h"
#include "app_files.h"
#include "app_iap.h"
#include "app_products.h"



static rt_event_t 									rtc_event = RT_NULL;    //RTC接收事件

static rt_device_t 									rtc_dev = RT_NULL;       //RTC设备

static uint8_t 											data_buf[512] = {0};    //

struct sys_alarm_str								gb4_alarm_value;        //



/***************** 本地全局变量 *******************/

static struct send_fifo 					gb4_class = {GB4_DATA_LEN,GB4_FIFO_NUM ,0,0,0,0,0,NULL};   //

static uint8_t 										gb4_fifo_buff[GB4_DATA_LEN * GB4_FIFO_NUM] = {0};          //

struct rt_semaphore								gb4_fifo_sem;

static uint8_t 										gb4_blind_save_log_buff[15] = {0};    //

static struct blind_s 						*const gb4_blind_log = (struct blind_s *)gb4_blind_save_log_buff;    //


/************************ QB4 发送缓冲区  *********************/

static struct send_fifo 					qb4_class = {QB4_DATA_LEN,QB4_FIFO_NUM ,0,0,0,0,0,NULL};     //

static uint8_t 										qb4_fifo_buff[QB4_DATA_LEN * QB4_FIFO_NUM] = {0};            //

struct rt_semaphore								qb4_fifo_sem;

static uint8_t 										qb4_blind_save_log_buff[15] = {0};    //

static struct blind_s 						*const qb4_blind_log = (struct blind_s *)qb4_blind_save_log_buff;    //



/************************ QB4 发送缓冲区  *********************/

static struct send_fifo 					local_class = {QB4_DATA_LEN,QB4_FIFO_NUM ,0,0,0,0,0,NULL};     //

static uint8_t 										local_fifo_buff[QB4_DATA_LEN * QB4_FIFO_NUM] = {0};            //

struct rt_semaphore								local_fifo_sem;

static uint8_t 										local_save_log_buff[15] = {0};    //

static struct blind_s 						*const local_log = (struct blind_s *)qb4_blind_save_log_buff;    //

/***************************** 排放处理 ******************************/

static struct exhaust_data_t						exhaust_data = {0};

static struct exhaust_buf_t							exhaust_buff = {0};






/*********************************
 **	写DT平台
 *********************************/

static void write_local_log_file(void)
{
	FILE *fd = NULL;
	int res = -1;

	if(read_files_sys_state() == 0)
		return;
	
	fd = fopen("/local_log.txt","w");
	if(fd != NULL)
		fwrite((uint8_t *)local_save_log_buff,sizeof(local_save_log_buff),1,fd);
  
	res = fclose(fd);
    
	if(res != 0)
	{
		rt_kprintf("-- Local area log files close failed %d.\r\n",res);
	}   
}



/***************************
**	写采集数据
***************************/

uint8_t write_local_data_file(uint8_t *src)
{
  int fd = -1;

	if(read_files_sys_state() == 0)
		return 0;

  if(local_log->write_index >= GB4_STREAM_DATA_MAX_INDEX)
	{
		local_log->write_index = 0;
	}
     	
	if(local_log->write_index == local_log->read_index && local_log->msg_cnt > 0)  
	{
		local_log->read_index += GB4_DATA_LEN;
		if(local_log->read_index >= GB4_BLIND_DATA_MAX_INDEX)
			local_log->read_index = 0;
	}

    fd = open("/gb4_flash.dat", O_RDWR + O_CREAT,0777);
    if(fd != -1)
    {
//			struct stat buf = {0};
//	
//			stat("/gb4_flash.dat", &buf);
//			rt_kprintf("-- write_local_data_files size %lu,%lu\r\n",buf.st_size,buf.st_mtime);
      if(lseek(fd, local_log->write_index, SEEK_SET) != -1)
			{
				local_log->write_index += GB4_STREAM_LEN; /* 更新下次的写指针*/
			}
            
      write(fd,(uint8_t *)src,GB4_STREAM_LEN);
			//printf("-- write GB4 blind data %d,%d\r\n",gb4_blind_log->write_index,gb4_blind_log->msg_cnt);
      close(fd);
    	
     if(++local_log->msg_cnt > GB4_STREAM_DATA_MAX_CNT)
         local_log->msg_cnt = GB4_STREAM_DATA_MAX_CNT;
        
            
		// printf( "-- Write Lcoal data file. write index add[% 8d], read index add[% 8d],current msg number[% 8d].\r\n",local_log->write_index,
		// 			local_log->read_index,local_log->msg_cnt);
    }
	else
	{
		rt_kprintf("-- open local data file fail.....\r\n");
	}
    
	write_local_log_file();

	return 0;
}




/*****************************
**	检测数据有有效性
******************************/

uint8_t check_exhaust_data(uint16_t m)
{
	if(m == 0xFF || m == 0xFFFF)
		return 1;
	
	return 0;
}

/*****************************
** 	dpf_scr_real.scr_out_temp = 0xFFFF;							//SCR出口温度
		dpf_scr_real.scr_up_nox = 0xFFFF;				//SCR上游NOx传感器输出值
******************************/

static uint8_t exhaust_handle(void)
{
	uint16_t   i;
	
	exhaust_buff.cycle++;
	//rt_kprintf("-- 统计数据周期  %d\r\n",exhaust_buff.cycle);
	if(exhaust_buff.index > 599)
		exhaust_buff.index = 599;
	
	/**
		read_friction_torque()  摩擦扭矩
		read_max_ref_torque()  最大参考扭矩
		read_engine_torque()  发动机实际扭矩
		
	**/
	if(read_friction_torque() != 0xFF && read_max_ref_torque() != 0xFFFF && read_engine_torque() != 0xFF && (read_engine_torque() >= read_friction_torque()))
	{
		exhaust_buff.array[0][exhaust_buff.index] = 3.14 * (read_engine_torque() - read_friction_torque()) / 10.0 * read_max_ref_torque() * 0.05 *  read_engine_rotate() * 0.125 / 30000.0;				// 发动机平均功率
	}
	else
	{
		exhaust_buff.array[0][exhaust_buff.index] = 0xFFFF;  //瞬时功率
	}
	
	exhaust_buff.array[1][exhaust_buff.index] = 5;				// SCR上游NOx平均浓度   //无效值
	
	if(read_emissions_type() == 0)   //排放类型  SCR
		exhaust_buff.array[2][exhaust_buff.index] = read_scr_downstream_nox(); 				// SCR下游NOx平均浓度
	else
		exhaust_buff.array[2][exhaust_buff.index] = 5; 
	
	exhaust_buff.array[3][exhaust_buff.index] = 5;				// SCR上游NOx平均质量流量   //无效值
	
	if(read_emissions_type() == 0)
	{
		if(read_enter_volume() != 0xFFFF && read_scr_downstream_nox() != 0xFFFF)  //SCR下游NOx平均质量流量 
		{
			exhaust_buff.array[4][exhaust_buff.index] = 0.001587 * (read_enter_volume() / 3600.0 + 0.84 * read_engine_fuel_flow() / 3600.0) * read_scr_downstream_nox();	// SCR下游NOx平均质量流量
		}
		else
		{
			exhaust_buff.array[4][exhaust_buff.index] = 0xFFFF;
		}
	}
	else
	{
		exhaust_buff.array[4][exhaust_buff.index] = 5;
	}
	
	if(read_emissions_type() == 0)
		exhaust_buff.array[5][exhaust_buff.index] = read_scr_entrance_temp();				// SCR入口平均温度
	else
		exhaust_buff.array[5][exhaust_buff.index] = 5;
	
	exhaust_buff.array[6][exhaust_buff.index] = 5;		  	// SCR出口平均温度	  //无效值
	
	exhaust_buff.array[7][exhaust_buff.index] = read_engine_fuel_flow();		  	// 发动机燃料流量平均值	
	
	for(i = 0;i < 8;i++)
	{
		if(exhaust_buff.array[i][exhaust_buff.index] == 0xFFFF)
				return 0;
	}
	
	exhaust_buff.index++;   //有效数据周期
	//rt_kprintf("-- 有效数据周期  %d\r\n",exhaust_buff.index);
	
	return 0;
}



/*****************************
**
******************************/

uint8_t calculate_exhaust_data(void)
{
	uint16_t i;
	uint32_t 							m_tmp[8];
	
	
	memset((uint8_t *)m_tmp,0,sizeof(m_tmp));
	
	if(exhaust_buff.index == 0)
	{
		memset((uint8_t *)&exhaust_data,0xFF,sizeof(exhaust_data));
		return 0;
	}
	
	for(i = 0;i < exhaust_buff.index;i++)
	{
		m_tmp[0]  += exhaust_buff.array[0][i];   		  // 发动机平均功率			
		m_tmp[1]  += exhaust_buff.array[1][i];					// SCR上游NOx平均浓度   //无效值
		m_tmp[2]  += exhaust_buff.array[2][i];				// SCR下游NOx平均浓度
		m_tmp[3]  += exhaust_buff.array[3][i];					// SCR上游NOx平均质量流量
		m_tmp[4]  += exhaust_buff.array[4][i];     	// SCR下游NOx平均质量流量
		m_tmp[5]  += exhaust_buff.array[5][i];					// SCR入口平均温度
		m_tmp[6]  += exhaust_buff.array[6][i];  			// SCR出口平均温度
		m_tmp[7]  += exhaust_buff.array[7][i];						// 发动机燃料流量平均值
	}
	
	exhaust_data.engine_power = m_tmp[0] / (exhaust_buff.index);   		  // 发动机平均功率			
	
	exhaust_data.scr_up_nox = 0xFFFF;					// SCR上游NOx平均浓度   //无效值
	exhaust_data.scr_down_nox = m_tmp[2] / (exhaust_buff.index);				// SCR下游NOx平均浓度
	exhaust_data.scr_up_flow = 0xFFFF;					// SCR上游NOx平均质量流量
	exhaust_data.scr_down_flow = m_tmp[4] / (exhaust_buff.index) / 0.04;     	// SCR下游NOx平均质量流量
	exhaust_data.src_in_temp = m_tmp[5] / (exhaust_buff.index);					// SCR入口平均温度
	exhaust_data.src_out_temp = 0xFFFF;  			// SCR出口平均温度
	exhaust_data.fuel_flow = m_tmp[7] / (exhaust_buff.index);						// 发动机燃料流量平均值
	
	//rt_kprintf("-- 统计周期 （2）  %d,  %d\r\n",exhaust_data.index,exhaust_buff.cycle);
	exhaust_data.cycle_pwm = exhaust_buff.index * 1.0 / exhaust_buff.cycle * 100 + 125;
	exhaust_data.index = exhaust_buff.index * 100;	

	//rt_kprintf("-- 统计周期 （3）  %d\r\n",exhaust_data.cycle_pwm);
	//可以从这里打印出来
	
	memset((uint8_t *)&exhaust_buff,0,sizeof(exhaust_buff));
	
	return 0;
}



/*****************************
**
******************************/

uint8_t read_exhaust_data(struct exhaust_data_t *source)
{
	
	if(source == NULL)
		return 1;
	
	memcpy((uint8_t *)source,(uint8_t *)&exhaust_data,sizeof(exhaust_data));  //更新平均数据缓冲区

	return 0;
}



/*********************************
**	初始化国标数据缓冲区
**********************************/

uint8_t init_gb4_fifo_buff(void)
{
	gb4_class.buff = gb4_fifo_buff;

	rt_sem_init(&gb4_fifo_sem,"gb4_fifo_sem", 1, 0);
   
	local_class.buff = local_fifo_buff;
	rt_sem_init(&local_fifo_sem,"local_log_sem", 1, 0);
	
	return 0;
}


/*********************************
**	初始化企标数据缓冲区
**********************************/

uint8_t init_qb4_fifo_buff(void)
{
	qb4_class.buff = qb4_fifo_buff;

	rt_sem_init(&qb4_fifo_sem,"qb4_fifo_sem", 1, 0);
   
	return 0;
}


/*******************************
**	
*******************************/

void send_rtc_event(void)
{
	if(rtc_event != RT_NULL)
		rt_event_send(rtc_event,1);
}




/****************************
**
*****************************/

void init_gb4_blind_log_file(void)
{
  FILE *fd = NULL;
	int res = -1;

  gb4_blind_log->msg_cnt = 0;
  gb4_blind_log->read_index = 0;
  gb4_blind_log->write_index = 0;

  if(read_files_sys_state() == 0)
		return;
		
  fd = fopen("/gb4_log.txt","r");
  if(fd != NULL)
	{
		fread((uint8_t *)gb4_blind_save_log_buff, sizeof(gb4_blind_save_log_buff),1,fd);
		rt_kprintf("-- gb4 blind area log files open ok...\r\n");
		rt_kprintf( "-- Read GB4 blind file. write index add[%02d],read index add[%02d] current msg number[%02d].\r\n",gb4_blind_log->write_index,
               gb4_blind_log->read_index,gb4_blind_log->msg_cnt);
		//fwrite((uint8_t *)gb4_blind_save_log_buff,sizeof(gb4_blind_save_log_buff),1,fd);
	}
	else
	{
		rt_kprintf("-- gb4 blind area log files open failed.\r\n");
	}

    res = fclose(fd);
    if (res != 0)
        rt_kprintf("-- gb4 blind area log files close failed.\r\n");
}


/*************************
**	返回报警状态
***************************/

uint8_t read_gb4_alarm_state(void)
{
	uint8_t rv = 0;
	
	rv = gb4_alarm_value.state;
	
	gb4_alarm_value.state = 0;
	
	return rv;
}


/**********************
**	判断拆除报警状态
***********************/
uint8_t read_dismantle_state(void)
{
	if(read_in_power_vol() <= 90)
		bitset(gb4_alarm_value.value,0);
	else
		bitclr(gb4_alarm_value.value,0);
	
	
	if(read_in_acc_state() > 0)
	{
		if(read_can_connect_state(1) > 0)
			bitset(gb4_alarm_value.value,1);
		else
			bitclr(gb4_alarm_value.value,1);
	}
	
	
	return 0;
}




/*********************************
 **	写DT平台
 *********************************/

static void write_gb4_blind_log_file(void)
{
	FILE *fd = NULL;
	int res = -1;

	if(read_files_sys_state() == 0)
		return;
	
	fd = fopen("/gb4_log.txt","w");
	if(fd != NULL)
		fwrite((uint8_t *)gb4_blind_save_log_buff,sizeof(gb4_blind_save_log_buff),1,fd);
  
	res = fclose(fd);
    
	if(res != 0)
	{
		rt_kprintf("-- GB4 Blind area log files close failed %d.\r\n",res);
	}   
}







/***************************
**	保存国标数据盲区数据
***************************/

uint8_t write_gb4_blind_data_file(uint8_t *src)
{
  int fd = -1;
	
	if(read_files_sys_state() == 0)
		return 0;

  if(gb4_blind_log->write_index >= GB4_BLIND_DATA_MAX_INDEX)
	{
		gb4_blind_log->write_index = 0;
	}
     	
	if(gb4_blind_log->write_index == gb4_blind_log->read_index && gb4_blind_log->msg_cnt > 0)  
	{
		gb4_blind_log->read_index += GB4_DATA_LEN;
		if(gb4_blind_log->read_index >= GB4_BLIND_DATA_MAX_INDEX)
			gb4_blind_log->read_index = 0;
	}

  fd = open("/gb4_blind.dat",O_RDWR + O_CREAT,0777);
  if(fd != -1)
  {
//		struct stat buf = {0};
//		
//		stat("/gb4_blind.dat", &buf);
//		rt_kprintf("-- the gb4_data.bin size %u\r\n",buf.st_size);
//		
    if(lseek(fd, gb4_blind_log->write_index, SEEK_SET) != -1)
		{
			gb4_blind_log->write_index += GB4_DATA_LEN; /* 更新下次的写指针*/
		}
            
    write(fd,(uint8_t *)src,GB4_DATA_LEN);
		close(fd);
    if(++gb4_blind_log->msg_cnt > GB4_BLIND_DATA_MAX_CNT)
            gb4_blind_log->msg_cnt = GB4_BLIND_DATA_MAX_CNT;
        
            
		rt_kprintf( "-- Write GB4 blind file. write index add[%02d], read index add[%02d],current msg number[%02d].\r\n",gb4_blind_log->write_index,
					gb4_blind_log->read_index,gb4_blind_log->msg_cnt);
    }
	else
	{
		rt_kprintf("-- open GB4 blind data file fail.....\r\n");
	}
  
	write_gb4_blind_log_file();

	return 0;
}




/***************************************
**	写国标数据到缓冲区
****************************************/

uint16_t write_gb4_fifo_buff(uint8_t *data,uint16_t len)
{
	uint8_t res = 0;
	uint8_t 											array[512] = {0};				//
	uint8_t i;
	
	if(len > GB4_DATA_LEN)
		return 0;

	rt_sem_take(&gb4_fifo_sem, RT_WAITING_FOREVER);
	res = send_fifo_write(&gb4_class,data,len);
	rt_sem_release(&gb4_fifo_sem);

	//rt_kprintf("-- GB4 class runing is ....%d,%d,%d,%d\r\n",gb4_class.item_cnt,gb4_class.w_item_index,gb4_class.w_offset_add,res);
	if(res == 1)
	{
		rt_sem_take(&gb4_fifo_sem, RT_WAITING_FOREVER);
		for(i = 0;i < GB4_FIFO_NUM;i++)
		{
			struct start_str *p = NULL;

	 		res = send_fifo_read(&gb4_class,array,GB4_DATA_LEN);
	 		
			p = (struct start_str *)array;
			res = swap_uint16_t(p->len) + 25;

			if(p->cmd == 0x02)
			{
				p->cmd = 0x03;
				array[res - 1] = calc_xor_verify(array + 2,res - 3);
				//printf("-- the gb data len:%d,%d,%d\r\n",p->cmd,swap_uint16_t(p->len),res);
				//mem_printf(LOG_ERROR, PRINT_HEX, array, res + 5);
	 			write_gb4_blind_data_file(array);
			}
		}
		
		rt_sem_release(&gb4_fifo_sem);
	}
	write_local_data_file(data);
	if(res == 0)
		return len;
	//
	return 0;
}





/****************************
**	初始化企标数据Log 文件
*****************************/

void init_qb4_blind_log_file(void)
{
    FILE *fd = NULL;
		int res = -1;

    qb4_blind_log->msg_cnt = 0;
    qb4_blind_log->read_index = 0;
    qb4_blind_log->write_index = 0;

    if(read_files_sys_state() == 0)
			return;
		
    fd = fopen("/qb4_log.txt","r");
    if(fd != NULL)
		{
			fread((uint8_t *)qb4_blind_save_log_buff, sizeof(qb4_blind_save_log_buff),1,fd);
//			rt_kprintf("-- qb4 blind area log files open ok...\r\n");
//			rt_kprintf( "-- Read QB4 blind file. write index add[%02d],read index add[%02d],current msg number[%02d].\r\n",qb4_blind_log->write_index,
//											qb4_blind_log->write_index,qb4_blind_log->msg_cnt);
			//fwrite((uint8_t *)gb4_blind_save_log_buff,sizeof(gb4_blind_save_log_buff),1,fd);
	}
	else
	{
		rt_kprintf("-- qb4 blind area log files open failed.\r\n");
	}

    res = fclose(fd);
    if (res != 0)
        rt_kprintf("-- qb4 blind area log files close failed.\r\n");
}





/***************************
**	读取国四平台盲区数据
***************************/

uint16_t read_gb4_blind_data_file(uint8_t *buf,uint16_t size)
{
	int fd = -1;

	if(read_files_sys_state() == 0)
		return 0;
	if(gb4_blind_log->msg_cnt == 0)
		return 0;

	if(gb4_blind_log->read_index >= GB4_BLIND_DATA_MAX_INDEX)
	{
		gb4_blind_log->read_index = 0;
	}

	fd = open("/gb4_blind.dat", O_RDONLY);
  if(fd != -1)
  {
    if(lseek(fd, gb4_blind_log->read_index, SEEK_SET) != -1)
		{
			gb4_blind_log->read_index += GB4_DATA_LEN; /* 更新下次的写指针*/
		}
            
    read(fd,(uint8_t *)buf,GB4_DATA_LEN);
		
    close(fd);
    
		gb4_blind_log->msg_cnt--;
            
		rt_kprintf( "-- Read GB4 blind file. write index add[%02d], read index add[%02d],current msg number[%02d].\r\n",gb4_blind_log->write_index,
					gb4_blind_log->read_index,gb4_blind_log->msg_cnt);
  }
	else
	{
		rt_kprintf("-- Open read GB4 blind data file fail.....\r\n");
		return 0;
	}
    
	write_gb4_blind_log_file();

	return size;
}



/**********************************
**	从国标数据缓冲区内部读取数据
***********************************/

uint16_t read_gb4_fifo_buff(uint8_t *data,uint16_t len)
{
	uint16_t res = 0;
	struct start_str  *p_start = NULL;

	if(len > GB4_DATA_LEN)
		return 0;

	//读取盲区数据 (国四数据)  国检时候 
	rt_sem_take(&gb4_fifo_sem, RT_WAITING_FOREVER);
	res = read_gb4_blind_data_file(data,len);
	rt_sem_release(&gb4_fifo_sem);
	//rt_kprintf("-- the blind........%d,%d\r\n",res,len);
	
	if(res == len)
	{
		p_start = (struct start_str *)data;
		if(p_start->head[0] != 0x23 || p_start->head[1] != 0x23)
			return 0;

		//rt_kprintf("-- GB4 ... read gb4 fifo :0x%x,%d,%d\r\n",p_start->cmd,res,swap_uint16_t(p_start->len) + 25);
		return swap_uint16_t(p_start->len) + 25;
	}
	
	
	rt_sem_take(&gb4_fifo_sem, RT_WAITING_FOREVER);
	res = send_fifo_read(&gb4_class,data,len);
	rt_sem_release(&gb4_fifo_sem);

	//rt_kprintf("-- Runing . :%d\r\n",res);
	if(res == len)
	{
		p_start = (struct start_str *)data;
		if(p_start->head[0] != 0x23 || p_start->head[1] != 0x23)
			return 0;

		//rt_kprintf("-- Runing ........  read gb4 fifo :%d,%d\r\n",res,swap_uint16_t(p_start->len));
		return swap_uint16_t(p_start->len) + 25;
	}
	return 0;	
}




/*********************************
 **	写DT平台
 *********************************/

static void write_qb4_blind_log_file(void)
{
	FILE *fd = NULL;
	int res = -1;

	if(read_files_sys_state() == 0)
		return;
	
	fd = fopen("/qb4_log.txt","w");
	if(fd != NULL)
		fwrite((uint8_t *)qb4_blind_save_log_buff,sizeof(qb4_blind_save_log_buff),1,fd);
  
	res = fclose(fd);
    
	if(res != 0)
	{
		rt_kprintf("-- QB4 Blind area log files close failed %d.\r\n",res);
	}   
}



/***************************
**	保存国标数据盲区数据
***************************/

uint8_t write_qb4_blind_data_file(uint8_t *src)
{
  int fd = 0;

	if(read_files_sys_state() == 0)
		return 0;

  if(qb4_blind_log->write_index >= QB4_BLIND_DATA_MAX_INDEX)
	{
		qb4_blind_log->write_index = 0;
	}
     	
	if(qb4_blind_log->write_index == qb4_blind_log->read_index && qb4_blind_log->msg_cnt > 0)  
	{
		qb4_blind_log->read_index += QB4_DATA_LEN;
		if(qb4_blind_log->read_index >= QB4_BLIND_DATA_MAX_INDEX)
			qb4_blind_log->read_index = 0;
	}
														
	fd = open("/qb4_blind.dat", O_RDWR + O_CREAT,0777);
  if(fd != -1)
  {
//		struct stat buf = {0};
//		
//		stat("/qb4_data.bin", &buf);
//		rt_kprintf("-- the qb4_data.bin size %u\r\n",buf.st_size);
		if(lseek(fd, qb4_blind_log->write_index, SEEK_SET) != -1)
		{
			//rt_kprintf("-- the qb4_data index 2 %u\r\n",fd);
			qb4_blind_log->write_index += QB4_DATA_LEN; /* 更新下次的写指针*/
		}
  
    write(fd,(uint8_t *)src,QB4_DATA_LEN);
		//rt_kprintf("-- Write QB4 blind data 20%d-%d-%d:%d,%d,%d\r\n",*(src + 24),*(src + 25),*(src + 26),*(src + 27),*(src + 28),*(src + 29));
    close(fd);
		
    if(++qb4_blind_log->msg_cnt > QB4_BLIND_DATA_MAX_CNT)
			qb4_blind_log->msg_cnt = QB4_BLIND_DATA_MAX_CNT;
    
//		rt_kprintf( "-- Write QB4 blind file. write index add[%02d], read index add[%02d],current msg number[%02d].\r\n",qb4_blind_log->write_index,
//					qb4_blind_log->read_index,qb4_blind_log->msg_cnt);
  }
	else
	{
		rt_kprintf("-- open QB4 blind data file fail.....\r\n");
	}
    
	write_qb4_blind_log_file();

	return 0;
}



/***************************************
**	写企标数据到企标数据缓冲区
****************************************/

uint16_t write_qb4_fifo_buff(uint8_t *data,uint16_t len)
{
	uint16_t 											res = 0;
	uint8_t 											array[512] = {0};				//
	uint8_t 											i = 0;
	
	if(len > QB4_DATA_LEN)
		return 0;

	rt_sem_take(&qb4_fifo_sem,RT_WAITING_FOREVER);
	res = send_fifo_write(&qb4_class,data,len);
	rt_sem_release(&qb4_fifo_sem);
	
	if(res == 1)
	{
		rt_sem_take(&qb4_fifo_sem,RT_WAITING_FOREVER);
		//rt_kprintf("-- gb4 fifo buf is over ... \r\n");   //
	 	for(i = 0;i < QB4_FIFO_NUM;i++)
	 	{
			struct start_str *p = NULL;
			
	 		res = send_fifo_read(&qb4_class,array,QB4_DATA_LEN);
	 	
			p = (struct start_str *)array;
			res = swap_uint16_t(p->len) + 25;

			if(p->cmd == 0x70)
			{
				p->cmd = 0x71;
				array[res - 1] = calc_xor_verify(array + 2,res - 3);
	 			write_qb4_blind_data_file(array);
			}
	 	}
		rt_sem_release(&qb4_fifo_sem);
	}
	
	if(res == 0)
		return len;
	//
	return 0;
}







/***************************
**	读取国四平台盲区数据
***************************/

uint16_t read_qb4_blind_data_file(uint8_t *buf,uint16_t size)
{
	int fd = -1;

	if(read_files_sys_state() == 0)
		return 0;
	if(qb4_blind_log->msg_cnt == 0)
		return 0;

	if(qb4_blind_log->read_index >= QB4_BLIND_DATA_MAX_INDEX)
	{
		qb4_blind_log->read_index = 0;
	}

	fd = open("/qb4_data.bin",O_RDONLY);
  if(fd != NULL)
  {
    if(lseek(fd, qb4_blind_log->read_index, SEEK_SET) != -1)
		{
			qb4_blind_log->read_index += QB4_DATA_LEN; /* 更新下次的写指针*/
		}
            
    read(fd,(uint8_t *)buf,QB4_DATA_LEN);
    close(fd);

		qb4_blind_log->msg_cnt--;
            
  }
	else
	{
		rt_kprintf("-- Open read QB4 blind data file fail.....\r\n");
		return 0;
	}
	
  write_qb4_blind_log_file();

	return size;
}






/**********************************
**	从企标数据缓冲区读取数据
***********************************/

uint16_t read_qb4_fifo_buff(uint8_t *data,uint16_t len)
{
	uint16_t res = 0;
	struct start_str  *p_start = NULL;

	if(len > QB4_DATA_LEN)
		return 0;

	rt_sem_take(&qb4_fifo_sem,RT_WAITING_FOREVER);
	res = send_fifo_read(&qb4_class,data,len);
	rt_sem_release(&qb4_fifo_sem);
	
	if(res == len)
	{
		p_start = (struct start_str *)data;
		if(p_start->head[0] != 0x23 || p_start->head[1] != 0x23)
			return 0;
		return swap_uint16_t(p_start->len) + 25;
	}
	
	//读取盲区数据 (企标数据) 
	rt_sem_take(&qb4_fifo_sem,RT_WAITING_FOREVER); 
	res = read_qb4_blind_data_file(data,len);
	rt_sem_release(&qb4_fifo_sem);
	
	if(res == len)
	{
		p_start = (struct start_str *)data;
		if(p_start->head[0] != 0x23 || p_start->head[1] != 0x23)
			return 0;
		
		return swap_uint16_t(p_start->len) + 25;
	}

	
	return 0;
}

/**********************************
**	
***********************************/

void thread_entry_packet(void *parameter)
{
	uint32_t 				recved = 0;
	uint8_t 				acc_state = 0;
	uint8_t 				c_cnt = 0;
	uint32_t 				gb4_cnt = 0;
	uint32_t        thread_cnt = 0;
	uint32_t        alarm_back = 0;
	uint32_to_byte 	back = {0};
	uint32_to_byte  data = {0};
	uint8_t					step = 0;
	struct rt_tm 		tm;
				
	parameter = parameter;
	
	rt_thread_delay(500); 

	//rt_kprintf("-- thread_entry_rtc run.....\r\n");
	if(mount_files_sys() == 0)                //挂载文件系统
	{
		rt_kprintf("-- mount files sys...ok\r\n");
	}	
	
	rt_thread_delay(500);
	
	rtc_dev = rt_device_find("rtc");

	recved = 0;
	rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_WKUP, &recved);        //
	rtc_event = rt_event_create("rtc", 1);
	rt_irq_wkup_sethook(send_rtc_event);
	
	init_gb4_fifo_buff();		 //
	init_qb4_fifo_buff();
	init_qb4_blind_log_file();
	init_gb4_blind_log_file(); 
  
	memset((uint8_t *)&exhaust_data,0xFF,sizeof(exhaust_data));
	
	for(;;)
	{
    if(rt_event_recv(rtc_event, 1, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &recved) != RT_EOK)
		{
			rt_kprintf("-- rt_event_recv failed\r\n");   //打印错误信息
			continue;  //uint16_t  read_max_ref_torque(void)
		}
		
		if(read_iap_state() > 0)  //正在升级停止打包  （）
			continue;
		
		
		reset_serial_num();
		
		rt_get_rtc(&tm);
		acc_state = read_in_acc_state();    //ACC状态
		
		read_dismantle_state();    //刷新报警状态
		if(gb4_alarm_value.value != alarm_back)
		{
			alarm_back = gb4_alarm_value.value;
			if(gb4_alarm_value.value > 0)	
			{
				gb4_alarm_value.state = 1;
				recved = vehicle_dismantle_alarm(data_buf,sizeof(data_buf));
				if(recved > 0)
				{
					write_gb4_fifo_buff(data_buf,recved);
					rt_kprintf("-- Demolition of alarm:20%d,%d,%d %d-%d-%d  %d\r\n",tm.year,tm.mon,tm.day,tm.hour,tm.min,tm.sec,recved);
				}
			}
			
			memset(data_buf,0,sizeof(data_buf));
			recved = build_qb4_platform_data(data_buf,sizeof(data_buf));
				
			if(recved > 0)
			{
				rt_kprintf("-- Build QB4 Data ALARM :20%d,%d,%d %d-%d-%d   %d\r\n",tm.year,tm.mon,tm.day,tm.hour,tm.min,tm.sec,recved,acc_state);
				write_qb4_fifo_buff(data_buf,recved);          //
			}		
		}				
		
		
		
		switch(step)
		{
			case 0:                    //
				if(acc_state == 0 || read_config_vin_state() != 0x5A || read_config_emission() != 4)
					break;
				step++;
				memset((uint8_t *)&exhaust_buff,0,sizeof(exhaust_buff));
				break;
			case 1:
				exhaust_handle();  //1秒钟执行一次
				if((gb4_cnt++ % read_config_gb_four_upload_cycle() == 0))
				{
					calculate_exhaust_data();
					memset(data_buf,0,sizeof(data_buf));
					recved = build_gb4_platform_data(data_buf,sizeof(data_buf));   //实时数据
					if(recved > 0)
					{
						rt_kprintf("-- Build GB4 Data ACC ON:20%d,%d,%d %d-%d-%d   %d\r\n",tm.year,tm.mon,tm.day,tm.hour,tm.min,tm.sec,recved);
						write_gb4_fifo_buff(data_buf,recved);
						//mem_printf(LOG_ERROR, PRINT_HEX, data_buf,recved);
					}
				}
				if(acc_state == 0)
				{
					gb4_cnt = 0;
					step = 0;
				}
				break;
			default:
				step = 0;
				break;
		}
		
		/*****************************
		**	1. 组包周期
		******************************/
		//rt_kprintf("-- rtc Data:%d,%d,%d\r\n",data.byte[0],data.byte[1],data.byte[2]);
		if((thread_cnt++ % read_config_travel_upload_cycle() == 0))	
		{  
			memset(data_buf,0,sizeof(data_buf));
			recved = build_qb4_platform_data(data_buf ,sizeof(data_buf));
				
			if(recved > 0)
			{
				rt_kprintf("-- Build QB4 Data Cycle:20%d,%d,%d %d-%d-%d   %d    %d\r\n",tm.year,tm.mon,tm.day,tm.hour,tm.min,tm.sec,recved,acc_state);
				write_qb4_fifo_buff(data_buf,recved);          //
			}
			continue;
		}
		
		
		/*****************************
		**	1. 组包周期
		**	2. 电锁变化
		**	3. 锁车状态及锁车功能状态
		******************************/
		data.byte[0] = acc_state;     					//ACC状态
		data.byte[1] = read_ecu_lock_state();   //锁车状态
		data.byte[2] = read_ecu_mon_status();		//锁车监控状态
		
		//rt_kprintf("-- rtc Data:%d,%d,%d\r\n",data.byte[0],data.byte[1],data.byte[2]);
		if(data.value != back.value)	
		{  
			if(c_cnt++ > 5)
			{
				memset(data_buf,0,sizeof(data_buf));
				recved = build_qb4_platform_data(data_buf ,sizeof(data_buf));
				
				if(recved > 0)
				{
					rt_kprintf("-- Build QB4 data condition change:20%d,%d,%d %d-%d-%d   %d    %d\r\n",tm.year,tm.mon,tm.day,tm.hour,tm.min,tm.sec,recved,acc_state);
					write_qb4_fifo_buff(data_buf,recved);          //
				}	
				back.value = data.value;
				c_cnt = 0;
			}
		}
	}
}


















