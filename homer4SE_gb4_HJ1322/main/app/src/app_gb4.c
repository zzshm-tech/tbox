
/************************************
 * FileName:app_gb4.c
 * Time:2022.7.7
 ************************************/



#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"


#include "app_main.h"

#include "pro_data.h"
#include "common.h"
#include "version.h"

#include "drv_rtc.h"
#include "drv_acl16.h"
#include "drv_spi.h"

#include "app_products.h"
#include "app_gb4.h"
#include "app_lte.h"
#include "app_at.h"
#include "app_gnss.h"
#include "app_can_recv.h"
#include "app_in.h"
#include "app_acl16.h"
#include "app_fifo.h"
#include "app_iap.h"
#include "app_archive.h"
#include "app_files.h"
#include "app_archive.h"
#include "app_can_send.h"
#include "app_packet.h"





#define GB4_SOCKET_ID   					1



#define GB4_DATA_LEN   						256

#define GB4_FIFO_NUM   						3


#define GB4_BLIND_DATA_MAX_INDEX			0xEC4000  /** 国标数据10秒钟一条，最少存储7天  0xEC4000 **/

#define GB4_BLIND_DATA_MAX_CNT  			(GB4_BLIND_DATA_MAX_INDEX / GB4_DATA_LEN)



#define QB4_DATA_LEN   						512

#define QB4_FIFO_NUM   						3


#define QB4_BLIND_DATA_MAX_INDEX			0x1D88000    /** 10S一条，存储7天  0x1D88000 **/ 

#define QB4_BLIND_DATA_MAX_CNT  			(QB4_BLIND_DATA_MAX_INDEX / QB4_DATA_LEN)



/************************** 本地全局变量 ******************************/

static struct encryption_chip_str              				encrypt_chip = {0};    		//

static struct gb4_str                          				gb4 = {0};					//

static struct serial_num_str                  				gb4_serial_num = {0};	    //

static QueueHandle_t                           				gb4_down_qh = NULL; 	    // 

static struct socket_down_str                  				gb_down_data = {0};			//

static uint8_t 												local_buff[1460] = {0};		//发送缓冲区

static struct gb4_alarm_str									gb4_alarm;					//







/************************ GB4 发送缓冲区  *********************/

static struct send_fifo 									gb4_class = {GB4_DATA_LEN,GB4_FIFO_NUM ,0,0,0,0,0,NULL};   //

static uint8_t 												gb4_fifo_buff[GB4_DATA_LEN * GB4_FIFO_NUM] = {0};          //

static  SemaphoreHandle_t  									gb4_fifo_sem = NULL;                                       //

static uint8_t 												gb4_blind_save_log_buff[15] = {0};    //

static struct blind_s 										*const gb4_blind_log = (struct blind_s *)gb4_blind_save_log_buff;    //



/************************ QB4 发送缓冲区  *********************/

static struct send_fifo 									qb4_class = {QB4_DATA_LEN,QB4_FIFO_NUM ,0,0,0,0,0,NULL};     //

static uint8_t 												qb4_fifo_buff[QB4_DATA_LEN * QB4_FIFO_NUM] = {0};            //

static  SemaphoreHandle_t  									qb4_fifo_sem = NULL;                                         //


static uint8_t 												qb4_blind_save_log_buff[15] = {0};    //

static struct blind_s 										*const qb4_blind_log = (struct blind_s *)qb4_blind_save_log_buff;    //




/*********************************
**	初始化国标数据缓冲区
**********************************/

uint8_t init_gb4_fifo_buff(void)
{
	gb4_class.buff = gb4_fifo_buff;

	gb4_fifo_sem = xSemaphoreCreateMutex();
    xSemaphoreGive(gb4_fifo_sem);

	qb4_class.buff = qb4_fifo_buff;

	qb4_fifo_sem = xSemaphoreCreateMutex();
	xSemaphoreGive(qb4_fifo_sem);

	

	return 0;
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

    if(read_files_sys_state() > 0)
		return;
		
    fd = fopen("/nandflash/gb4_log.txt","r");
    if(fd != NULL)
	{
		fread((uint8_t *)gb4_blind_save_log_buff, sizeof(gb4_blind_save_log_buff),1,fd);
		//printf("-- gb4 blind area log files open ok...\r\n");
		printf( "-- Read gb4_log.txt file info: write index add[% 8d], read index add[% 8d],current msg number[% 8d].\r\n",gb4_blind_log->write_index,
            gb4_blind_log->read_index,gb4_blind_log->msg_cnt);
		//fwrite((uint8_t *)gb4_blind_save_log_buff,sizeof(gb4_blind_save_log_buff),1,fd);
	}
	else
	{
		printf("-- gb4 blind area log files open failed.\r\n");
	}

    res = fclose(fd);
    if (res != 0)
        printf("-- gb4 blind area log files close failed.\r\n");
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

    if(read_files_sys_state() > 0)
		return;
		
    fd = fopen("/nandflash/qb4_log.txt","r");
    if(fd != NULL)
	{
		fread((uint8_t *)qb4_blind_save_log_buff, sizeof(qb4_blind_save_log_buff),1,fd);
		//printf("-- qb4 blind area log files open ok...\r\n");
		printf( "-- Read qb4_log.txt file info: write index add[% 8d],read index add[% 8d],current msg number[% 8d].\r\n",qb4_blind_log->write_index,
                    qb4_blind_log->read_index,qb4_blind_log->msg_cnt);
		//fwrite((uint8_t *)gb4_blind_save_log_buff,sizeof(gb4_blind_save_log_buff),1,fd);
	}
	else
	{
		printf("-- qb4 blind area log files open failed.\r\n");
	}

    res = fclose(fd);
    if (res != 0)
        printf("-- qb4 blind area log files close failed.\r\n");
}






/*********************************
 **	写DT平台
 *********************************/

static void write_qb4_blind_log_file(void)
{
	FILE *fd = NULL;
	int res = -1;

	if(read_files_sys_state() > 0)
		return;
	
	fd = fopen("/nandflash/qb4_log.txt","w");
	if(fd != NULL)
		fwrite((uint8_t *)qb4_blind_save_log_buff,sizeof(qb4_blind_save_log_buff),1,fd);
  
	res = fclose(fd);
    
	if(res != 0)
	{
		printf("-- QB4 Blind area log files close failed %d.\r\n",res);
	}   
}






/***************************
**	保存国标数据盲区数据
***************************/

uint8_t write_qb4_blind_data_file(uint8_t *src)
{
    int fd = -1;

	if(read_files_sys_state() > 0)
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

    fd = open("/nandflash/qb4_data.bin", O_RDWR + O_CREAT,0777);
    if(fd != -1)
    {
        if(lseek(fd, qb4_blind_log->write_index, SEEK_SET) != -1)
		{
			qb4_blind_log->write_index += QB4_DATA_LEN; /* 更新下次的写指针*/
		}
            
        write(fd,(uint8_t *)src,QB4_DATA_LEN);
		//printf("-- write GB4 blind data %d,%d\r\n",gb4_blind_log->write_index,gb4_blind_log->msg_cnt);
        close(fd);
    	
        if(++qb4_blind_log->msg_cnt > QB4_BLIND_DATA_MAX_CNT)
            qb4_blind_log->msg_cnt = QB4_BLIND_DATA_MAX_CNT;
        
            
		printf( "-- Write QB4 blind file. write index add[% 8d], read index add[% 8d],current msg number[% 8d].\r\n",qb4_blind_log->write_index,
					qb4_blind_log->read_index,qb4_blind_log->msg_cnt);
    }
	else
	{
		printf("-- open QB4 blind data file fail.....\r\n");
		fd = creat("/nandflash/qb4_data.bin",0777);
		if(fd != -1)
			printf("-- creat dt bind data file ok... %d\r\n",fd);
	}
    write_qb4_blind_log_file();

	return 0;
}



/***************************
**	读取国四平台盲区数据
***************************/

uint16_t read_qb4_blind_data_file(uint8_t *buf,uint16_t size)
{
	int fd = -1;
	
	if(read_files_sys_state() > 0)
		return 0;
	if(qb4_blind_log->msg_cnt == 0)
		return 0;

	if(qb4_blind_log->read_index >= QB4_BLIND_DATA_MAX_INDEX)
	{
		qb4_blind_log->read_index = 0;
	}

	fd = open("/nandflash/qb4_data.bin", O_RDONLY);
    if(fd != -1)
    {
        if(lseek(fd, qb4_blind_log->read_index, SEEK_SET) != -1)
		{
			qb4_blind_log->read_index += QB4_DATA_LEN; /* 更新下次的写指针*/
		}
            
        read(fd,(uint8_t *)buf,QB4_DATA_LEN);
		//printf("-- write GB4 blind data %d,%d\r\n",gb4_blind_log->write_index,gb4_blind_log->msg_cnt);
        close(fd);
    
		qb4_blind_log->msg_cnt--;
            
		printf( "-- Read QB4 blind file. write index add[%02d], read index add[%02d],current msg number[%02d].\r\n",qb4_blind_log->write_index,
					qb4_blind_log->read_index,qb4_blind_log->msg_cnt);
    }
	else
	{
		printf("-- Open read QB4 blind data file fail.....\r\n");
		return 0;
	}
    write_qb4_blind_log_file();

	return size;
}





/*********************************
 **	写DT平台
 *********************************/

static void write_gb4_blind_log_file(void)
{
	FILE *fd = NULL;
	int res = -1;

	if(read_files_sys_state() > 0)
		return;
	
	fd = fopen("/nandflash/gb4_log.txt","w");
	if(fd != NULL)
		fwrite((uint8_t *)gb4_blind_save_log_buff,sizeof(gb4_blind_save_log_buff),1,fd);
  
	res = fclose(fd);
    
	if(res != 0)
	{
		printf("-- GB4 Blind area log files close failed %d.\r\n",res);
	}   
}



/***************************
**	读取国四平台盲区数据
***************************/

uint16_t read_gb4_blind_data_file(uint8_t *buf,uint16_t size)
{
	int fd = -1;

	if(read_files_sys_state() > 0)
		return 0;
	if(gb4_blind_log->msg_cnt == 0)
		return 0;

	if(gb4_blind_log->read_index >= GB4_BLIND_DATA_MAX_INDEX)
	{
		gb4_blind_log->read_index = 0;
	}

	fd = open("/nandflash/gb4_data.bin", O_RDONLY);
    if(fd != -1)
    {
        if(lseek(fd, gb4_blind_log->read_index, SEEK_SET) != -1)
		{
			gb4_blind_log->read_index += GB4_DATA_LEN; /* 更新下次的写指针*/
		}
            
        read(fd,(uint8_t *)buf,GB4_DATA_LEN);
		//printf("-- write GB4 blind data %d,%d\r\n",gb4_blind_log->write_index,gb4_blind_log->msg_cnt);
        close(fd);
    	
		gb4_blind_log->msg_cnt--;
            
		printf( "-- Read GB4 blind file. write index add[%02d], read index add[%02d],current msg number[%02d].\r\n",gb4_blind_log->write_index,
					gb4_blind_log->read_index,gb4_blind_log->msg_cnt);
    }
	else
	{
		printf("-- Open read GB4 blind data file fail.....\r\n");
		return 0;
	}
    write_gb4_blind_log_file();

	return size;
}


/***************************
**	保存国标数据盲区数据
***************************/

uint8_t write_gb4_blind_data_file(uint8_t *src)
{
    int fd = -1;

	if(read_files_sys_state() > 0)
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

    fd = open("/nandflash/gb4_data.bin", O_RDWR + O_CREAT,0777);
    if(fd != -1)
    {
        if(lseek(fd, gb4_blind_log->write_index, SEEK_SET) != -1)
		{
			gb4_blind_log->write_index += GB4_DATA_LEN; /* 更新下次的写指针*/
		}
            
        write(fd,(uint8_t *)src,GB4_DATA_LEN);
		//printf("-- write GB4 blind data %d,%d\r\n",gb4_blind_log->write_index,gb4_blind_log->msg_cnt);
        close(fd);
    	
        if(++gb4_blind_log->msg_cnt > GB4_BLIND_DATA_MAX_CNT)
            gb4_blind_log->msg_cnt = GB4_BLIND_DATA_MAX_CNT;
        
            
		printf( "-- Write GB4 blind file. write index add[% 8d], read index add[% 8d],current msg number[% 8d].\r\n",gb4_blind_log->write_index,
					gb4_blind_log->read_index,gb4_blind_log->msg_cnt);
    }
	else
	{
		printf("-- open GB4 blind data file fail.....\r\n");
		fd = creat("/nandflash/gb4_data.bin",0777);
		if(fd != -1)
			printf("-- creat gb4_data.bin bind data file ok... %d\r\n",fd);
	}
    write_gb4_blind_log_file();

	return 0;
}






/***************************************
**	写国标数据到缓冲区
****************************************/

uint16_t write_gb4_fifo_buff(uint8_t *data,uint16_t len)
{
	uint16_t res = 0;
	uint8_t array[300] = {0};
	uint8_t i;

	if(len > GB4_DATA_LEN)
		return 0;

	xSemaphoreTake(gb4_fifo_sem,portMAX_DELAY);
	res = send_fifo_write(&gb4_class,data,len);
	xSemaphoreGive(gb4_fifo_sem);

	//printf("-- GB4 class runing is ....%d,%d,%d,%d\r\n",gb4_class.item_cnt,gb4_class.w_item_index,gb4_class.w_offset_add,res);

	if(res == 1)
	{
		//printf("-- gb4 fifo buf is over ... \r\n");   //
		xSemaphoreTake(gb4_fifo_sem,portMAX_DELAY);
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
				//printf("-- the len:%d,%d,%d\r\n",p->cmd,swap_uint16_t(p->len),res);
				//mem_printf(LOG_ERROR, PRINT_HEX, array, res + 5);
	 			write_gb4_blind_data_file(array);
			}
	 	}
		xSemaphoreGive(gb4_fifo_sem);
	}

	if(res == 0)
		return len;
	//
	return 0;
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

	xSemaphoreTake(gb4_fifo_sem,portMAX_DELAY);
	res = send_fifo_read(&gb4_class,data,len);
	xSemaphoreGive(gb4_fifo_sem);

	if(res == len)
	{
		p_start = (struct start_str *)data;
		if(p_start->head[0] != 0x23 || p_start->head[1] != 0x23)
			return 0;

		return swap_uint16_t(p_start->len) + 25;
	}

	//读取盲区数据 (国四数据) 
	xSemaphoreTake(gb4_fifo_sem,portMAX_DELAY); 
	res = read_gb4_blind_data_file(data,len);
	xSemaphoreGive(gb4_fifo_sem);
	//printf("-- the blind........%d,%d\r\n",res,len);
	
	if(res == len)
	{
		p_start = (struct start_str *)data;
		if(p_start->head[0] != 0x23 || p_start->head[1] != 0x23)
			return 0;

		return swap_uint16_t(p_start->len) + 25;
	}

	return 0;	
}




/***************************************
**	写企标数据到企标数据缓冲区
****************************************/

uint16_t write_qb4_fifo_buff(uint8_t *data,uint16_t len)
{
	uint16_t res = 0;
	uint8_t array[512] = {0};
	uint32_t i = 0;

	if(len > QB4_DATA_LEN)
		return 0;

	xSemaphoreTake(qb4_fifo_sem,portMAX_DELAY);
	res = send_fifo_write(&qb4_class,data,len);
	xSemaphoreGive(qb4_fifo_sem);
	
	//printf("-- QB4 class runing is ....%d,%d,%d,%d\r\n",qb4_class.item_cnt,qb4_class.w_item_index,qb4_class.w_offset_add,res);

	if(res == 1)
	{
		xSemaphoreTake(qb4_fifo_sem,portMAX_DELAY);
		//printf("-- gb4 fifo buf is over ... \r\n");   //
	 	for(i = 0;i < QB4_FIFO_NUM;i++)
	 	{
			struct start_str *p = NULL;
			
	 		res = send_fifo_read(&qb4_class,array,QB4_DATA_LEN);
			p = (struct start_str *)array;
			res = swap_uint16_t(p->len) + 25;
			if(res > 25 && p->cmd == 0x70)
			{
				p->cmd = 0x71;
				array[res - 1] = calc_xor_verify(array + 2,res - 3);
				//mem_printf(LOG_ERROR, PRINT_HEX, array, res + 5);
	 			write_qb4_blind_data_file(array);
			}
	 	}

		xSemaphoreGive(qb4_fifo_sem);
	}
	if(res == 0)
		return len;
	//
	return 0;
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

	xSemaphoreTake(qb4_fifo_sem,portMAX_DELAY);
	res = send_fifo_read(&qb4_class,data,len);
	xSemaphoreGive(qb4_fifo_sem);
	if(res == len)
	{
		p_start = (struct start_str *)data;
		if(p_start->head[0] != 0x23 || p_start->head[1] != 0x23)
			return 0;
		return swap_uint16_t(p_start->len) + 25;
	}
	
	//读取盲区数据 (企标数据) 
	xSemaphoreTake(qb4_fifo_sem,portMAX_DELAY); 
	res = read_qb4_blind_data_file(data,len);
	xSemaphoreGive(qb4_fifo_sem);
	//printf("-- the blind........%d,%d\r\n",res,len);
	
	if(res == len)
	{
		p_start = (struct start_str *)data;
		if(p_start->head[0] != 0x23 || p_start->head[1] != 0x23)
			return 0;

		return swap_uint16_t(p_start->len) + 25;
	}


	return 0;
}


/***********************
**	系统时间(调试使用)
************************/

void build_rtc_time(uint8_t *buf,uint16_t size)
{
	struct rt_tm tmp;

	if(size < 6 || buf == NULL)
		return;

	get_rtc_time(&tmp);          //获取系统时间

	*(buf + 0) = (uint8_t)tmp.year;
	*(buf + 1) = (uint8_t)tmp.mon;
	*(buf + 2) = (uint8_t)tmp.day;
	*(buf + 3) = (uint8_t)tmp.hour;
	*(buf + 4) = (uint8_t)tmp.min;
	*(buf + 5) = (uint8_t)tmp.sec;
}



/********************************
**	返回加密芯片状态
*********************************/

uint8_t read_acl16_work_state(void)
{
	uint8_t rv;
	
	rv = encrypt_chip.state;
	
	return rv;
}




/************************
**	返回加密芯片应用程序版本
*************************/

uint8_t read_acl16_app_version(void)
{
	uint8_t rv;
	
	rv = encrypt_chip.version;
	
	return rv;
}	


/***********************************************
**	返回国企平台（链接的企业平台）socket状态
***********************************************/

uint8_t read_gb4_socket_state(void)
{
	uint8_t rv;

	rv = gb4.socket_state;

	return rv;
}



/***********************
**	复位
************************/

uint8_t reset_serial_num(void)
{
	struct rt_tm         tm;
	
    time_t          t_t;

    get_rtc_time(&tm);
	
    t_t = rt_mktime(&tm);

	if(tm.day != gb4_serial_num.day)
	{
		if(t_t >= gb4_serial_num.unix_time)
		{
			gb4_serial_num.login_num = 1;
			gb4_serial_num.serial_num = 1;
			gb4_serial_num.serial_gb = 1;
			gb4_serial_num.alarm_num = 1;
			gb4_serial_num.unix_time = t_t;
			gb4_serial_num.day = tm.day;
			printf("-- 改变流水号\r\n");
		}
	}

	return 0;
}



/******************************
**	保存ECU运行数据
*******************************/

void write_login_out_num(void)
{
	
	FILE *fd = NULL;
	int res = 0;
	uint8_t array[128];

	if(read_files_sys_state() > 0)
		return ;

	fd = fopen("/nandflash//serial.txt", "w");
    if(fd != NULL)
    {
		gb4_serial_num.verfy = 0x5A5A;

		memset(array,'\0',50);
    	sprintf((char *)array,"%d,%d,%d,%d,%d,%d,%d,%d\r\n",	gb4_serial_num.verfy,				 //
																gb4_serial_num.login_num,    		 //登入登出流水号
																gb4_serial_num.serial_num,   		 //企标流水号
																gb4_serial_num.serial_gb,    		 //国标流水号
																gb4_serial_num.unix_time, 	 		 //
																gb4_serial_num.day,			 	 //更改
																gb4_serial_num.alarm_num,    		 //报警流水号
																gb4_serial_num.qb_login_num); 		 //登录流水号
		//printf("%s\r\n",array);
        fseek(fd,0,SEEK_SET);       
        res = fwrite((uint8_t *)array,128,1,fd);
		//printf("-- run hsi d df asd (1):%d\r\n",res);
		if(res > 0)
		{
			//printf("-- Save serial num OK...\r\n");
		}
		else
		{
			printf("-- Save serial num Fail...\r\n");
		}
        res = fclose(fd);
        if(res != 0)
	    {
		    printf("-- Serial num log files close failed\r\n");
        }
    }
	else
	{
		printf("-- gb4 serial num log files Open failed\r\n");
	}

}


/******************************
**	复位
*******************************/

uint8_t load_login_out_num(void)
{
	FILE 		*fd;
	int 		res;
	uint8_t 	array[128];

	if(read_files_sys_state() > 0)
		return 1;

	fd = fopen("/nandflash/serial.txt", "r");
    if(fd != NULL)
    {
		memset(array,'\0',50);
        fseek(fd,0,SEEK_SET);
		res = fread((uint8_t *)array,128,1,fd);
			
        sscanf((const char *)array,"%d,%d,%d,%d,%d,%d,%d,%d\r\n",	&gb4_serial_num.verfy,				 //
																 	&gb4_serial_num.login_num,    		 //登入登出流水号
																	&gb4_serial_num.serial_num,   		 //企标流水号
																	&gb4_serial_num.serial_gb,    		 //国标流水号
																	&gb4_serial_num.unix_time, 	 		 //
																	&gb4_serial_num.day,			 	 //更改
																	&gb4_serial_num.alarm_num,    		 //报警流水号
																	&gb4_serial_num.qb_login_num 		 //登录流水号
		);
		printf("-- init gb4 serial num ok... 0x%x\r\n",gb4_serial_num.verfy);     
        res = fclose(fd);
        if(res != 0)
	    {
		    printf("-- Init gb4 serial num log files close failed\r\n");
        }

		if(gb4_serial_num.verfy != 0x5A5A)
		{
			gb4_serial_num.serial_num = 1;   		 //企标流水号
			gb4_serial_num.serial_gb = 1;    		 //国标流水号
			gb4_serial_num.unix_time = 1; 	 		 //
			gb4_serial_num.day = 1;			 	 //更改
			gb4_serial_num.alarm_num = 1;    		 //报警流水号
			gb4_serial_num.qb_login_num = 1; 		 //登录流水号
		}
    }
	else
	{
		gb4_serial_num.login_num = 1;    		 //登入登出流水号
		gb4_serial_num.serial_num = 1;   		 //企标流水号
		gb4_serial_num.serial_gb = 1;    		 //国标流水号
		gb4_serial_num.unix_time = 1; 	 		 //
		gb4_serial_num.day = 1;			 	 //更改
		gb4_serial_num.alarm_num = 1;    		 //报警流水号
		gb4_serial_num.qb_login_num = 1; 		 //登录流水号
		printf("-- Init gb4 serial num log files Open failed\r\n");
	}

	return 1;
}




/******************************
**	删除上行数据流水号
*******************************/

void delete_login_out_num(void)
{
	if(remove("/nandflash/serial.txt") == 0)
		printf("-- delete delete_login_out_num serial file ok...\r\n");
	else
		printf("-- delete delete_login_out_num serial file fial...\r\n");

	reset_serial_num();
}






/*****************************
**	返回芯片ID
*******************************/
	
uint8_t read_encryption_chip_id(uint8_t *source,uint8_t size)
{
	if(source == NULL || size < 16)
		return 0;

	memcpy(source,encrypt_chip.id_lot,16);

	return 16;
}









/************************************************
**	国标数据登录
**	cmd:登录或者登出
 ************************************************/
uint16_t build_gb_vehicle_login_out(uint16_t cmd,uint8_t *source,uint16_t size)
{
	struct start_str    		            *p_start = NULL;
	struct gb_login_out_str                 *p_login = NULL;
	uint16_t 								len = 0;
	uint8_t 								buf[100];
	
	memset(buf,0,sizeof(buf));
	
	p_start = (struct start_str *)buf;
	p_start->head[0] = 0x23;
	p_start->head[1] = 0x23;
	p_start->cmd = cmd;
	
	memset(p_start->vin,0,17);
	if(read_config_vin_state() == 0x5A)
	{
		read_config_vin_info(p_start->vin,17);
	}
	else
	{
		read_config_terminal_id(p_start->vin,16);   //如果没有
	}

	p_start->soft_ver = SOFEWARE_VER;
	p_start->encrypt = SM2;
	
	if(cmd == LOGIN)
		p_start->len = swap_uint16_t(sizeof(struct gb_login_out_str));
	else
		p_start->len = swap_uint16_t(sizeof(struct gb_login_out_str) - 20);
	
	memcpy(source, buf, sizeof(struct start_str));
	len += sizeof(struct start_str);
	
	
	p_login = (struct gb_login_out_str *)buf;
	build_rtc_time((uint8_t *)p_login->time,6);    //数据采集时间
	
	if(cmd == LOGIN)//登入
	{
		p_login->serial_num = swap_uint16_t(gb4_serial_num.login_num);
		printf("-- GB4 Login serial num:%d\r\n",gb4_serial_num.login_num);
		gb4_serial_num.login_num++;
		write_login_out_num();
		read_lte_icc_id(p_login->iccid,sizeof(p_login->iccid));
		memcpy(source + len,buf,sizeof(struct gb_login_out_str));	
		len +=	sizeof(struct gb_login_out_str);
	}
	else//登出
	{
		p_login->serial_num = swap_uint16_t((gb4_serial_num.login_num - 1));//登出流水号必须和登入流水号一致(登入时流水号+1 存储)
		printf("-- GB4 LogOut serial num:%d\r\n",gb4_serial_num.login_num - 1);
		memcpy(source + len,buf,sizeof(struct gb_login_out_str) - sizeof(p_login->iccid));	
		len +=	(sizeof(struct gb_login_out_str) - sizeof(p_login->iccid));
	}
	*(source + len) = calc_xor_verify(source + 2, len - 2);
	len++;
	
	return len;
}






/*****************************
**	企标数据登录 登出
**	
*****************************/
uint16_t build_qb_vehicle_login_out(uint16_t cmd,uint8_t *source,uint16_t size)
{
	struct start_str    	*p_start;
	struct login_qb_str   	*p_log;
	struct serial_qb_str	*p_serial;
	
	uint16_t							len = 0;
	uint8_t 							buf[100];
	
	p_start = (struct start_str *)buf;
	
	p_start->head[0] = 0x23;
	p_start->head[1] = 0x23;
	
	p_start->cmd = cmd;// 车辆登入/登出
	
	memset(p_start->vin,0,17);
	read_config_terminal_id(p_start->vin,16);   //设备号
	
	p_start->soft_ver = SOFEWARE_VER;
	p_start->encrypt = SM2;
	
	
	p_start->len = swap_uint16_t(sizeof(struct login_qb_str) + sizeof(struct serial_qb_str));
	
	memcpy(source + len, buf,sizeof(struct start_str));
	len = sizeof(struct start_str);

	p_serial = (struct serial_qb_str *)buf;
	build_rtc_time(p_serial->time,6);
	gb4_serial_num.qb_login_num++;
	write_login_out_num();
	p_serial->num[0] = 0;
	p_serial->num[1] = swap_uint32_t(gb4_serial_num.qb_login_num);
	
	memcpy(source + len,buf,sizeof(struct serial_qb_str));
	len += sizeof(struct serial_qb_str);
	
	p_log = (struct login_qb_str *)buf;
	memset(buf,0,sizeof(buf));
	if(read_config_vin_state() == 0x5A)   //机械环保代码
	{
		read_config_vin_info(p_log->vin,17);
	}
	else
	{
		memset(p_start->vin,0,17);
	}
	memset(p_log->dev_id,0,17);
	read_config_terminal_id(p_log->dev_id,16);   //设备号
	p_log->type = 0;    										//设备类型
	
	read_lte_icc_id(p_log->icc_id,20);  		//TBOX设备ICCID
	read_lte_imei_id(p_log->imei,17);   		//TBOX设备LTE模块IMEI
	
	memcpy(source + len,buf,sizeof(struct login_qb_str));
	len += sizeof(struct login_qb_str);
	
	*(source + len) = calc_xor_verify(source + 2, len - 2);  //校验
	len++;
	
	return len;
}




/*********************************
**	OBD故障码
**********************************/

uint16_t make_frame_gb4_fc(uint8_t *buf,uint16_t size)
{
	uint16_t 						len = 0;
	uint8_t 						array[256];
	uint16_t 						i = 0;
	struct obd_fc_t					*p_obd = NULL;

	if(buf == NULL || size < 256)
		return 0;
	
	len += sizeof(struct obd_fc_t);
	
	i =	read_gb4_dm_serial(array,sizeof(array));
	//printf("-- gb1939 :%d\r\n",i);
	memcpy(buf + len,array,i);
	len += i;

	p_obd = (struct obd_fc_t *)array;

	p_obd->type = 0x01;    //
	build_rtc_time(p_obd->rtc,6);
	p_obd->protocol = 2;		//OBD协议

	p_obd->alarm_state = (i > 0) ? 1 : 0;// read_mil_light_state();	//报警状态 ()

	p_obd->num = i / 4;

	memcpy(buf,array,sizeof(struct obd_fc_t));
	//printf("-- gb1939 fc code :%d\r\n",len);

	return len;
}






/*********************************
**	OBD故障码
**********************************/

uint16_t make_frame_qb4_fc(uint8_t *buf,uint16_t size)
{
	uint16_t 						len = 0;
	uint8_t 						array[256];
	uint16_t 						i = 0;
	struct qb4_fc_t			*p = NULL;

	if(buf == NULL || size < 256)
		return 0;
	
	len += sizeof(struct qb4_fc_t);
	
	i =	read_qb4_dm_serial(array,sizeof(array));
	//printf("-- gb1939 :%d\r\n",i);
	memcpy(buf + len,array,i);
	len += i;

	p = (struct qb4_fc_t *)array;

	p->type = 0xA2;    	//
	p->len = swap_uint16_t(i + 1);			//长度
	p->num = i / 4;			//

	memcpy(buf,array,sizeof(struct qb4_fc_t));
	//printf("-- QB FC Dode :%d\r\n",len);

	return len;
}





/*********************************
**	OBD故障码
**********************************/

uint16_t make_frame_gb27145_dm(uint8_t *buf,uint16_t size)
{
	uint16_t 						len = 0;
	uint8_t 						array[256];
	struct gb27145_dm_msg_t 		*p_dm_msg;
	uint16_t 						i;

	if(buf == NULL || size < 256)
		return 0;


	len += sizeof(struct gb27145_dm_msg_t);

	
	i = read_gb27145_dm_serial(array,sizeof(array));
	memcpy((buf + len),array,i);
	len += i;

	//printf("-- run is 1 .........................%d\r\n",i);
	
	p_dm_msg = (struct gb27145_dm_msg_t *)array;

	p_dm_msg->type = 0x01;
	p_dm_msg->protocol = 1;
	p_dm_msg->alarm_state =  read_gb27145_light_state();
	p_dm_msg->num = i / 4;

	memcpy(buf,array,sizeof(struct gb27145_dm_msg_t));

	return len;
}



/***************************************
**	国四排放数据流
***************************************/

uint16_t make_frame_data_stream_dpf_scr_real(uint8_t *buf,uint16_t size)
{
	uint16_t 						len = 0;
	uint8_t 						array[256];
	struct dpf_scr_real_t			dpf_scr_real;   //02
	struct ecu_data_str				*p_ecu;
	uint32_t 						tmp = 0;
	
	if(buf == NULL || size < 120)
		return 0;

	p_ecu = (struct ecu_data_str *)array;
	read_ecu_data(p_ecu);	
	
	dpf_scr_real.type = 0x02; 
	build_rtc_time(dpf_scr_real.rtc,6);
	tmp = (read_gnss_speed() * 256.0 / 100.0);   //
	
	dpf_scr_real.speed =  swap_uint16_t(tmp);							//车速  使用GPS速度
	dpf_scr_real.air_pressure = p_ecu->air_pressure;													//大气压力
	dpf_scr_real.engine_torque = p_ecu->engine_torque;															//发动机实际扭矩
	//printf("-- 发动机实际扭矩:%d\r\n",p_ecu->engine_torque);
	dpf_scr_real.friction_torque = p_ecu->friction_torque;										//摩擦扭矩
	//printf("-- 摩擦扭矩 : %d\r\n",p_ecu->friction_torque);
	dpf_scr_real.engine_rotate = swap_uint16_t(p_ecu->engine_rotate); 					//发动机转速
	dpf_scr_real.fuel_flow = swap_uint16_t(p_ecu->engine_fuel_flow);					//发动机燃料流量
	//printf("-- 发动机燃料流量：%d\r\n",p_ecu->engine_fuel_flow);
	dpf_scr_real.scr_up_nox = 0xFFFF;	 //swap_uint16_t(p_ecu->scr_upstream_nox);				//SCR上游NOx传感器输出值
	dpf_scr_real.scr_down_nox = 0xFFFF;  //swap_uint16_t(p_ecu->scr_downstream_nox);			//SCR下游NOx传感器输出值
	//printf("-- SCR 上下游值：%d,%d\r\n",p_diesel->scr_upstream_nox,p_diesel->scr_downstream_nox);
	dpf_scr_real.reactant_allowance = 0xFF;  // p_ecu->reactant_allowance; 										//反应剂余量
	dpf_scr_real.enter_volume = swap_uint16_t(p_ecu->enter_volume);								//进气量

	dpf_scr_real.scr_in_temp = 0xFFFF; //swap_uint16_t(p_ecu->scr_entrance_temp); 			//SCR入口温度
	dpf_scr_real.scr_out_temp = 0xFFFF;  //swap_uint16_t(p_ecu->scr_exit_temp);							//SCR出口温度
	dpf_scr_real.dpf_diffPressure = swap_uint16_t(p_ecu->dpf_diffPressure);			//DPF压差
	//printf("-- dpf diffprressur value:%d\r\n",p_ecu->dpf_diffPressure);
	dpf_scr_real.cooling_temp = p_ecu->coolant_temp;														//冷却液温度
	dpf_scr_real.fuel_position = p_ecu->fuel_percent;          //燃油位
	dpf_scr_real.egr_opening = swap_uint16_t(p_ecu->egr_opening);				//EGR阀开度
	dpf_scr_real.egr_setting = swap_uint16_t(p_ecu->egr_setting);				//EGR设定值
	
	memcpy(buf,(uint8_t *)&dpf_scr_real,sizeof(struct dpf_scr_real_t));	
	len  =	sizeof(struct dpf_scr_real_t);

	return len;
}







/***************************************
**	国四排放数据流
***************************************/

uint16_t make_frame_data_stream_dpf_scr_average(uint8_t *buf,uint16_t size)
{
	uint16_t 										len = 0;
	struct exhaust_data_t 			tmp;
	struct dpf_scr_average_t 		dpf_scr_average;    //
	
	if(buf == NULL || size < 120)
		return 0;

	dpf_scr_average.type = 0x04;  	    		//信息标识
	build_rtc_time(dpf_scr_average.rtc,6);
	dpf_scr_average.ref_torque = swap_uint16_t(read_max_ref_torque());			// 参考扭矩  固定值
	
	read_exhaust_data(&tmp);
	dpf_scr_average.engine_power = swap_uint16_t(tmp.engine_power);		// 发动机平均功率
	//dpf_scr_average.engine_power = swap_uint16_t(50);		// 发动机平均功率
	
	dpf_scr_average.scr_up_nox = swap_uint16_t(tmp.scr_up_nox);			// SCR上游NOx平均浓度
	dpf_scr_average.scr_down_nox = swap_uint16_t(tmp.scr_down_nox);		// SCR下游NOx平均浓度
	dpf_scr_average.scr_up_flow = (uint8_t)tmp.scr_up_flow;		// SCR上游NOx平均质量流量
	dpf_scr_average.scr_down_flow = (uint8_t)tmp.scr_down_flow;     	// SCR下游NOx平均质量流量
	dpf_scr_average.src_in_temp = swap_uint16_t(tmp.src_in_temp);		// SCR入口平均温度
	dpf_scr_average.src_out_temp = swap_uint16_t(tmp.src_out_temp);  		// SCR出口平均温度
	dpf_scr_average.fuel_flow = swap_uint16_t(tmp.fuel_flow);			// 发动机燃料流量平均值
	dpf_scr_average.cycle_calculate = swap_uint16_t(tmp.index);    // 统计周期时长
	dpf_scr_average.cycle_pwm = tmp.cycle_pwm; 		  	// 统计周期内有效时间占比
	//printf("-- the dpf scr average engine packet ... %d,%d\r\n",tmp.index,tmp.cycle_pwm);
	memcpy(buf + len,(uint8_t *)&dpf_scr_average,sizeof(struct dpf_scr_average_t));	
	len  =	sizeof(struct dpf_scr_average_t);

	return len;
}




/**************************************
** 0x80 设备状态信息 
**************************************/

uint16_t make_frame_device_info(uint8_t *buf,uint16_t size)
{
	struct terminal_msg_str			tmp_msg;
	uint16_t	    				len = 0;
	uint8_t 						array[32];
	
	tmp_msg.msg_type = 0x80;
	tmp_msg.msg_len = swap_uint16_t(sizeof(struct terminal_msg_str) - 3);

	memset(tmp_msg.vin,0,17);
	if(read_config_vin_state() == 0x5A)
	{
		read_config_vin_info(array,sizeof(array));
		memcpy(tmp_msg.vin,array,17);           		//VIN号
	}
	
	memset(tmp_msg.id,0,17);   //设备编号   
	read_config_terminal_id(array,sizeof(array));
	//rt_kprintf("-- the terminal is:%s\r\n",buf);
	memcpy(tmp_msg.id,array,16);   //设备编号     			
	tmp_msg.manu_num = 0x01;          			//厂家编码  0x01标识博创
	tmp_msg.terminal_type = 0x51;     			//终端型号
	tmp_msg.user_num = read_config_user_code();          			//使用方编号
	tmp_msg.car_type = read_config_car_type();     		 			//安装车型 
	
	//read_boot_loader_version(buf,4);              //BOOTLoader版本号
	tmp_msg.app_ver1 = read_app_version();      //终端上传协议版本(修改为)
	
	tmp_msg.app_ver2 = read_acl16_app_version();          			//应用程序版本号    
	tmp_msg.hd_ver = 22;        		 			//硬件版本号
	tmp_msg.io_status = read_in_io_state();							//IO状态
	tmp_msg.acc_status = read_in_acc_state();     				//ACC状态
	//printf("-- acc state:%d\r\n",tmp_msg.acc_status);
	tmp_msg.moto_status = read_in_moto_state();						//MOTO状态
	tmp_msg.input_frq1 = swap_uint16_t(0);							//外部输入频率
	tmp_msg.input_frq2 = swap_uint16_t(read_hmi_mon_state());							//外输输入频率
	tmp_msg.output_frq3 = swap_uint16_t(get_yuc_key_state());						//PWM1输出频率
	tmp_msg.output_frq4 = swap_uint16_t(get_yuc_mon_state());						//PWM2 输出频率
	tmp_msg.input_vol1 = swap_uint16_t(read_yuc_ecu_type());							//外部输入电压1
	tmp_msg.input_vol2 = swap_uint16_t(read_ecu_manu_type());							//外部输入电压2
	tmp_msg.power_vol = swap_uint16_t(read_in_power_vol() * 100);							//外部输入电压
	tmp_msg.batter_vol = swap_uint16_t(read_in_batter_vol() * 100);							//内部电池输入电压
	//printf("-- the vechel data:%d,%d\r\n",tmp_msg.power_vol,tmp_msg.batter_vol);
	tmp_msg.warn_value = swap_uint32_t(read_in_alarm());							//设备报警值
	//printf("-- the alarm .... %d\r\n",read_in_alarm());
	tmp_msg.gnss_speed = swap_uint16_t(read_gnss_speed() / 10);   					//GNSS 速度
	tmp_msg.gnss_heading = swap_uint16_t(read_gnss_heading());   				//GNSS 方向
	tmp_msg.gnss_altitude = swap_uint16_t(read_gnss_altitude());          //GNSS 海拔高度
	//rt_kprintf("-- the Gnss info:%d,%d,%d\r\n",tmp_msg.gnss_speed,tmp_msg.gnss_heading,tmp_msg.gnss_altitude);
	tmp_msg.gnss_used_satellite = read_gnss_satellite_num();    //GNSS 使用卫星数
	tmp_msg.gnss_view_satellite = read_gnss_bd_sate_num() + read_gnss_gps_sate_num();    //GNSS 可视卫星数
	tmp_msg.gnss_hdop = swap_uint16_t(read_gnss_hdop());             	//GNSS 水平经度因子
	//rt_kprintf("-- 水平精度因子:%d\r\n",tmp_msg.gnss_hdop);
	
	tmp_msg.gnss_mondel_status = 0;
	if('A' == read_gnss_positing_state())
	{
		tmp_msg.gnss_mondel_status |= 0x0A; //  定位状态
	}
	
	tmp_msg.nj_mon_state = read_config_nj_state();    //农机三合一功能状态；0：功能关闭；1：功能开启
	tmp_msg.nj_socket_state = 0;   //农机三合一链接状态
	tmp_msg.data_model = 0;        //数据模式
	tmp_msg.csq = read_lte_csq();    // LTE信号值
	tmp_msg.arch_state = read_config_archival_state();            //备案状态 （最近一次 备案状态）
	
	tmp_msg.ver_security = read_acl16_app_version();
	read_encryption_chip_id(tmp_msg.id_security,sizeof(tmp_msg.id_security));
	//read_lte_imei_id(tmp_msg.id_security,sizeof(tmp_msg.id_security));
	tmp_msg.nj_send_num = swap_uint32_t(0);
	//printf("-- the nj send num: %d\r\n",tmp_msg.nj_send_num);
	
	memcpy(buf,&tmp_msg,sizeof(struct terminal_msg_str));	
	len  +=	sizeof(struct terminal_msg_str);
	
	//printf("-- the device_info_make_frame %d\r\n",len);

	return len;
}


/***********************************
**	组包车身扩展消息
**	
************************************/

uint16_t make_fram_vehicle_info(uint8_t *buf,uint16_t size)
{
	uint16_t 						len = 0;
	struct vehicle_msg_0x11_t		tmp_msg;
	struct ecu_data_str				*p_ecu;
	uint8_t 						array[256];

	if(buf == NULL || size < 120)
		return 0;

	//printf("-- The struct vehicle_msg_0x11_t len:%d\r\n",sizeof(struct vehicle_msg_0x11_t));			
	
	tmp_msg.msg_type = 0x11;
	tmp_msg.msg_len = swap_uint16_t(sizeof(struct vehicle_msg_0x11_t) - 3);
	
	p_ecu = (struct ecu_data_str *)array;
	read_ecu_data(p_ecu);

	tmp_msg.accumulator_vol = swap_uint16_t(read_in_power_vol());          			//电源电压
	tmp_msg.reference_torque = swap_uint16_t(p_ecu->reference_torque);		  			//发动机参考扭矩
	tmp_msg.cold_boot_status = p_ecu->cold_boot_status;     	  			//冷启动加热速状态
	tmp_msg.lock_status = read_ecu_lock_state();			  			//ECU锁车状态
	tmp_msg.mon_status = read_ecu_mon_state();   			  			//锁车功能状态
	//printf("-- the mon state:%d,%d\r\n",tmp_msg.mon_status,tmp_msg.lock_status);
	tmp_msg.key_status = read_ecu_key_state();     		  			//KEY码状态
	tmp_msg.id_status = read_ecu_id_state();				  			//TBOX ID状态
	tmp_msg.fuel_temp = p_ecu->oil_temp;				  			//燃油温度
	tmp_msg.oil_temp = swap_uint16_t(p_ecu->oil_temp);         		  			//机油温度
	tmp_msg.fuel_pressure = swap_uint16_t(p_ecu->fuel_pressure);            			//燃油压力
	tmp_msg.oil_position = p_ecu->oil_position;			  			//机油液位
	tmp_msg.cool_position = p_ecu->cool_position;			  			//冷却液位置
	tmp_msg.engine_torque_model = p_ecu->engine_torque_model;	  			//发动机扭矩模式   （没有）
	tmp_msg.driver_cmd_torque_percent = p_ecu->driver_cmd_torque_percent;			//驾驶员指令扭矩百分比                 
	tmp_msg.engine_need_torque_percent = p_ecu->engine_need_torque_percent;			//发动机需求扭矩百分比 
	tmp_msg.engine_affiliated_torque_percent = p_ecu->engine_affiliated_torque_percent;	//发动机附件扭矩百分比
	tmp_msg.ave_fuel_consume_rate = swap_uint16_t(p_ecu->ave_fuel_consume_rate);				//平均燃油消耗率
	tmp_msg.instant_fuel_consume_rate = swap_uint16_t(p_ecu->instant_fuel_consume_rate);			//瞬时燃油消耗率
	tmp_msg.once_fuel_consume_rate = swap_uint32_t(p_ecu->once_fuel);      		//单次油耗
	tmp_msg.total_fuel_consume_rate = swap_uint32_t(read_total_fuel());  			//累计油耗
	tmp_msg.dfp_inlet_pressure = p_ecu->dfp_inlet_pressure;					//颗粒捕捉器入口压力
	//printf("-- the dfp inlet :%d\r\n",tmp_msg.dfp_inlet_pressure);
	tmp_msg.boost_pressure = p_ecu->absolute_add_pressure;						//增压压力
	tmp_msg.entered_air_intake_temp = p_ecu->entered_air_intake_temp;			//发动机进气歧管温度（）
	tmp_msg.enter_pressure = p_ecu->enter_pressure;           			//进气压力
	tmp_msg.air_temp = swap_uint16_t(p_ecu->air_temp);							//大气温度
	tmp_msg.entered_air_temp = p_ecu->entered_air_temp;					//发动机进气温度
	tmp_msg.carbamide_temp = p_ecu->carbamide_temp;						//尿素温度
	tmp_msg.carbamide_position = p_ecu->carbamide_position;					//尿素液位
	tmp_msg.carbamide_concentration = p_ecu->carbamide_concentration;			//尿素浓度
	tmp_msg.dpf_dirt_retention = p_ecu->dpf_dirt_retention;					//DPF积尘量	实际值=传送值	4453	
	tmp_msg.dpf_reproduce_indicate_light = p_ecu->dpf_reproduce_indicate_light;		//DPF再生提醒灯	实际值=传送值 4418	
	tmp_msg.dpf_reproduce_active_light = p_ecu->dpf_reproduce_active_light;			//DPF主动再生状态指示灯	实际值=传送值4419	
	tmp_msg.dpf_reproduce_forbid_light = p_ecu->dpf_reproduce_forbid_light;			//DPF的再生禁止指示灯	实际值=传送值4420	
	tmp_msg.dpf_reproduce_fault_light = p_ecu->dpf_reproduce_fault_light;			//DPF再生故障指示灯	实际值=传送值
	tmp_msg.oil_water_pilot = p_ecu->oil_water_pilot;    				//油中有水指示
	tmp_msg.energy_saving_state = p_ecu->energy_saving_state;				//能量节约状态  
	tmp_msg.auxiliary_flameout_state = p_ecu->auxiliary_flameout_state;			//辅助熄火状态;
	tmp_msg.absolute_oil_pressure = swap_uint16_t(p_ecu->absolute_oil_pressure);				//绝对机油压力
	tmp_msg.relative_oil_pressure = swap_uint16_t(p_ecu->relative_oil_pressure);				//相对机油压力
	tmp_msg.accelerator_percent = p_ecu->accelerator_percent;				//加速踏板行程值
	tmp_msg.engine_load_percent = p_ecu->engine_load_percent;  				//发动机负荷
	tmp_msg.engine_work_time = swap_uint32_t(read_engine_work_time());					//发动机工作时间
	tmp_msg.engine_total_speed = swap_uint32_t(p_ecu->engine_total_speed);					//发动机总转速
	tmp_msg.buffer_torque = p_ecu->buffer_torque;                		//缓冲区扭矩
	tmp_msg.once_travel = swap_uint32_t(p_ecu->once_travel);						//单次行驶距离
	tmp_msg.total_travel = swap_uint32_t(p_ecu->total_travel);  						//总里程 
	
	 
	memcpy(buf,&tmp_msg,sizeof(struct vehicle_msg_0x11_t));	
	len  +=	sizeof(struct vehicle_msg_0x11_t);

	return len;
}



/************************************************
**  构建GB4数据包
**	（不包含整体架构的消息体，及校验）
**	返回状态  1：
**	
************************************************/
uint16_t build_gb4_platform_data(uint8_t *buf,uint16_t size)
{
	struct start_str			*p_start = NULL;
	struct gnss_msg_str			*p_gnss = NULL;      
	uint8_t 					array[300] = {0};
    uint16_t	                len = 0;
	uint16_to_byte              tmp16_t; 

    uint16_t            		index = 0;


    if(buf == NULL || size < 200)
	{
		return 0;
	}
        
	len = sizeof(struct start_str);
	/* 数据打包时间 */
	build_rtc_time(array,sizeof(array));
	memcpy(buf + len,array,6);
	len += 6;
	/* 信息流水号 */
	tmp16_t.value = swap_uint16_t(gb4_serial_num.serial_gb); 
	
	memcpy(buf + len,&tmp16_t.byte[0],2);
	len += 2;
	gb4_serial_num.serial_gb++;
	//printf("-- GB4 Data Serial Num...%d\r\n",login_out_num.serial_gb);
	write_login_out_num();
	
    /*GNSS定位信息*/
    p_gnss = (struct gnss_msg_str *)array;
	
    p_gnss->status = 0;
	if('A' != read_gnss_positing_state())
		p_gnss->status |= 0x01; //  定位状态
	else
		p_gnss->status &= 0xFE;
	
	if('S' == read_gnss_latitude_sn()) 	//南纬
		p_gnss->status |= 0x02;
	
	if('W' == read_gnss_longitude_ew())	//西经
		p_gnss->status |= 0x04;
	
	p_gnss->latitude = swap_uint32_t(read_gnss_latitude(0));	 //纬度
	p_gnss->longitude =swap_uint32_t(read_gnss_longitude(0));	 //经度
	
	memcpy(buf + len,array,sizeof(struct gnss_msg_str));	
	len += sizeof(struct gnss_msg_str);
	
	index = index;
	
	/** OBD诊断信息 0x01 **/
	//英轩客户  (燃油叉车)
	//山东肯石客户 （挖掘机，玉柴发动机）
	//山东卡特客户（履带式挖掘机）
	index = make_frame_gb4_fc(array,sizeof(array));	  //J1939故障码
	memcpy(buf + len,array,index);
	len += index;

	//printf("-- gb4 the obd data len:%d\r\n",index);
	/** 数据流信息 0x02 **/
	index = make_frame_data_stream_dpf_scr_real(array,sizeof(array));
	memcpy(buf + len,array,index);
	len += index;
	//printf("-- gb4 make_frame_data_stream:%d\r\n",index);
	//memcpy(buf + len,encrypt_chip.id_lot,16);

	index = make_frame_data_stream_dpf_scr_average(array,sizeof(array));
	memcpy(buf + len,array,index);
	len += index;

	read_encryption_chip_id(array,16);
	memcpy(buf + len,array,16);
	memset(array,0,sizeof(array));
	if(64 != acl16_write_then_read(CRY_SIGN,buf + sizeof(struct start_str),array,len + 16 - sizeof(struct start_str)))
	{
		printf("-- CRY_SIGN is error...\r\n");
        //return 0;
	}


	buf[len++] = 0x20;
	memcpy(buf + len,&array[0],32);
	len += 32;
	buf[len++] = 0x20;
	memcpy(buf + len,&array[32],32);
	len += 32;
	
	p_start = (struct start_str *)array;
	p_start->head[0] = 0x23;
	p_start->head[1] = 0x23;

	p_start->cmd = 0x02;
	memset(p_start->vin,0,17);

	read_config_vin_info(p_start->vin,17);
	p_start->soft_ver = (uint8_t)read_user_version();
	p_start->encrypt = SM2;
	p_start->len = swap_uint16_t(len - sizeof(struct start_str));
	
	memcpy(buf, array, sizeof(struct start_str));
	buf[len] = calc_xor_verify(buf + 2,len - 2);
	len++;
	//printf("-- build gb4 packe:%d,%d\r\n",swap_uint16_t(p_start->len),len);
	return len;
}





/*********************************
**	通用故障上传
**********************************/

uint16_t make_frame_general_fc(uint8_t *buf,uint16_t size)
{
	uint16_t 								len = 0;
	uint8_t 								array[256];
	uint16_t 								i = 0;
	struct general_fc_t						*p = NULL;

	if(buf == NULL || size < 256)
		return 0;
	
	len += sizeof(struct general_fc_t);

	i =  read_qb4_dm_serial(array,sizeof(array));

	if(i > 0)
	{
		*(buf + len++) = 1;
		*(buf + len++) = i / 4;
		
		memcpy(buf + len,array,i);
		len += i;
		//printf("-- qb4 serial len:%d\r\n",i);
	}
	
	if(len == sizeof(struct general_fc_t))
		return 0;
	

	p = (struct general_fc_t *)array;

	p->type = 0x1F;    	//
	p->total_len = swap_uint16_t(len - sizeof(struct general_fc_t));			//长度

	memcpy(buf,array,sizeof(struct general_fc_t));
	//printf("-- General FC Dode :%d\r\n",len);

	return len;
}






/************************************************
**	企标数据
** ()
************************************************/

uint16_t build_qb4_platform_data(uint8_t *buf,uint16_t size)
{      
	uint8_t 				array[300] = {0};
	struct start_str		*p_start;
	struct gnss_msg_str		*p_gnss = NULL;
	uint16_t	            len = 0;
	uint16_t                index = 0;
	uint16_to_byte          tmp16_t;

    if(buf == NULL || size < 300)
		return 0;
    
	len += sizeof(struct start_str);
	
	/* 数据打包时间 */
	build_rtc_time(array,sizeof(array));
	memcpy(buf + len,array,6);
	len += 6;
	
	/* 信息流水号 */
	tmp16_t.value = swap_uint16_t(gb4_serial_num.serial_num); 
	memcpy(buf + len,&tmp16_t.byte[0],2);
	len += 2;
	gb4_serial_num.serial_num++;
	write_login_out_num();
	//printf("-- the qb serial.... %d\r\n",login_out_num.serial_num);
	/*GNSS定位信息*/
	p_gnss = (struct gnss_msg_str *)array;
	
    p_gnss->status = 0;
	if('A' != read_gnss_positing_state())
		p_gnss->status |= 0x01; //  定位状态
	else
		p_gnss->status &= 0xFE;
	
	if('S' == read_gnss_latitude_sn()) 	//南纬
		p_gnss->status |= 0x02;
	
	if('W' == read_gnss_longitude_ew())	//西经
		p_gnss->status |= 0x04;
	
	p_gnss->latitude = swap_uint32_t(read_gnss_latitude(0));	 //纬度
	p_gnss->longitude =swap_uint32_t(read_gnss_longitude(0));	 //经度

	memcpy(buf + len,array,sizeof(struct gnss_msg_str));	
	len += sizeof(struct gnss_msg_str );
	
	index = make_frame_gb4_fc(array,(uint16_t)sizeof(array));        //OBD故障码
	memcpy(buf + len,array,index);
	len += index;
	//printf("-- run is make_frame_gb4_fc len....%d\r\n",index);

	/** 数据流信息 0x02 **/
	index = make_frame_data_stream_dpf_scr_real(array,(uint16_t)sizeof(array));      
	memcpy(buf + len,array,index);
	len += index;
	//printf("-- run is  make_frame_data_stream....%d\r\n",index);

	index = make_frame_data_stream_dpf_scr_average(array,sizeof(array));
	memcpy(buf + len,array,index);
	len += index;

	/* 0x80 设备状态信息 */
	index = make_frame_device_info(array,(uint16_t)sizeof(array));  
	memcpy(buf + len,array,index);     
	len += index;
	//printf("-- the device info len:%d\r\n",index);
	/* 车身扩展消息 扩展信息 */
	index = make_fram_vehicle_info(array,(uint16_t)sizeof(array));  
	memcpy(buf + len,array,index);
	len += index;
	//printf("-- make_fram_vehicle_fc:%d\r\n",index);

	index = make_frame_general_fc(array,(uint16_t)sizeof(array));        //通用故障码传送
	memcpy(buf + len,array,index);
	len += index;

	p_start = (struct start_str *)array;
	p_start->head[0] = 0x23;
	p_start->head[1] = 0x23;

	p_start->cmd = 0x70;
	memset(p_start->vin,0,17);
	read_config_terminal_id(p_start->vin,17);  //企标数据报文，直接使用设备号

	p_start->soft_ver = (uint8_t)read_app_version();
	p_start->encrypt = SM2;
	p_start->len = swap_uint16_t(len - sizeof(struct start_str));
	
	memcpy(buf, array, sizeof(struct start_str));
	buf[len] = calc_xor_verify(buf + 2,len - 2);
	len++;

	return len;
}



/****************************
**	拆除报警
**	
****************************/

uint16_t vehicle_dismantle_alarm(uint8_t *buf,uint16_t size)
{
	uint8_t 				array[200] = {0};
	struct start_str		*p_start;
	struct gnss_msg_str		*p_gnss = NULL;
	uint16_t	            len = 0;
	uint16_to_byte			tmp16_t;

	 if(buf == NULL || size < 300)
		return 0;
    
	len += sizeof(struct start_str);
	
	/* 数据打包时间 */
	build_rtc_time(array,sizeof(array));
	memcpy(buf + len,array,6);
	len += 6;
	
	/* 信息流水号 */
	tmp16_t.value = swap_uint16_t(gb4_serial_num.alarm_num);
	memcpy(buf + len,&tmp16_t.byte[0],2);
	len += 2;
	gb4_serial_num.alarm_num++;
	write_login_out_num();

	p_gnss = (struct gnss_msg_str *)array;
	
    p_gnss->status = 0;
	if('A' != read_gnss_positing_state())
		p_gnss->status |= 0x01; //  定位状态
	else
		p_gnss->status &= 0xFE;
	
	if('S' == read_gnss_latitude_sn()) 	//南纬
		p_gnss->status |= 0x02;
	
	if('W' == read_gnss_longitude_ew())	//西经
		p_gnss->status |= 0x04;
	
	p_gnss->latitude = swap_uint32_t(read_gnss_latitude(0));	 //纬度
	p_gnss->longitude =swap_uint32_t(read_gnss_longitude(0));	 //经度

	memcpy(buf + len,array,sizeof(struct gnss_msg_str));	
	len += sizeof(struct gnss_msg_str );
	
	//拆除报警状态
	//*(Send_Buf + DataLen) = read_dismantle_state();
	//DataLen++;
	
	p_start = (struct start_str *)array;
	p_start->head[0] = 0x23;
	p_start->head[1] = 0x23;
	p_start->cmd = 0x05;
	
	memset(p_start->vin,0,17);
	read_config_vin_info(p_start->vin,17);

	p_start->soft_ver = SOFEWARE_VER;
	p_start->encrypt = SM2;
	p_start->len = swap_uint16_t(len - sizeof(struct start_str));
	
	memcpy(buf, array, sizeof(struct start_str));
	buf[len] = calc_xor_verify(buf + 2,len - 2);
	len++;
	
	return len;
}






/*******************************
**	国四企业平台通用应答
********************************/

uint16_t build_gb4_response_packets(uint8_t *buf,uint16_t size,struct down_cmd_res_str *res)
{
	struct start_str    		*p_start;
	uint16_t 					len;
	uint32_to_byte   			tmp_data; 
	uint8_t 					array[50];
	
	len = sizeof(struct start_str);
	
	/* 数据打包时间 */
	build_rtc_time(array,sizeof(array));
	memcpy(buf + len,array,6);
	len += 6;
	/* 信息流水号 */
	tmp_data.value = swap_uint32_t(res->ser_num[0]); 
	memcpy(buf + len,&tmp_data.byte[0],4);
	len += 4;
	
	tmp_data.value = swap_uint32_t(res->ser_num[1]); 
	memcpy(buf + len,&tmp_data.byte[0],4);
	len += 4;
	
	*(buf + len++) = res->msg_type;    //消息类型
	*(uint16_t *)(buf + len) = swap_uint16_t(1);
	len += 2;
	*(buf + len++) = res->res;

	p_start = (struct start_str *)array;
	p_start->head[0] = 0x23;
	p_start->head[1] = 0x23;
	p_start->cmd = res->cmd_id;
	
	memset(p_start->vin,0,17);
	read_config_terminal_id(p_start->vin,17);
	
	p_start->soft_ver = SOFEWARE_VER;
	p_start->encrypt = SM2;
	p_start->len = swap_uint16_t(len - sizeof(struct start_str));

	memcpy(buf, array, sizeof(struct start_str));
	*(buf + len) = calc_xor_verify(buf + 2,len - 2);
	len++;
	
	return len;
}




/**********************
**	判断拆除报警状态
***********************/
uint32_t read_dismantle_state(void)
{
	if(read_in_power_vol() <= 90)
		bitset(gb4_alarm.value,0);
	else
		bitclr(gb4_alarm.value,0);
	
	
	if(read_in_acc_state() > 0)
	{
		if(read_can_connect_state() > 0)
			bitset(gb4_alarm.value,1);
		else
			bitclr(gb4_alarm.value,1);
	}

	return gb4_alarm.value;
}




/************************************************
**	下行数据解析
************************************************/
uint16_t gb4_platform_down_parse(uint8_t *data, uint16_t len,uint8_t *buf,uint16_t size)
{
	struct start_str    		    *p_start;
	struct down_cmd_con_str         *p_down_cmd;
	uint16_t 	                    offset = 0; 
	uint8_t 				        *index_p = NULL;
	uint8_t     	                array[100];
	struct down_cmd_res_str 		*p_res;

	// printf("\r\n-- Platform  receive GB4 Platform:");
   	// mem_printf(LOG_ERROR, PRINT_HEX, data, len);

    if(data == NULL || len == 0)
	{
		return 0;
	}
        

	if(len < sizeof(struct start_str))
		return 0;	
   	
	p_start = (struct start_str *)data;
	
	if((p_start->head[0] != 0x23) && (p_start->head[1] != 0x23))
	{
		printf("-- Down Cmd Header error.......\r\n");
		return 0;
	}
	
	//设备编号编号判断
	memset(array,0,sizeof(array));
	read_config_terminal_id(array,sizeof(array));  //
	if(str_compare(p_start->vin,array,16) == 0)
	{
		read_config_vin_info(array,sizeof(array));
		if(str_compare(p_start->vin,array,17) == 0)
		{
			printf("-- Down Cmd Dev ID error ... \r\n");
			return 0;
		}
	}
	
	if((*(data + len - 1)) != (calc_xor_verify(data + 2,len - 3)))
	{
		printf("-- Down Cmd Check the error.......\r\n");
		return 0;
	}
	
	memset(array,0,sizeof(array));
	
	switch(p_start->cmd)
	{
		case 0x01: //国标登录  登入回应

			index_p = data + sizeof(struct start_str) + 8;	

			if(*index_p == 1)   //判断登入状态
			{
				index_p++;
				if(swap_uint16_t(*(uint16_t *)index_p) == 1)    //解析环保代码长度，不是17 退出
				{
					index_p += 2;
					if(*index_p == 0)
						gb4.login_out_state = 1;
					else
						gb4.login_out_state = 0;

					//printf("-- the gb4 login out state.....%d,%d\r\n",gb4.login_out_state,*index_p);
				}
			} 

			//一下解析CAN协议编号
			index_p++;
			if(*index_p == 2)
			{
				struct can_send_mq_t tmp_mq;
				QueueHandle_t t_q = NULL;

				index_p++;
				if(swap_uint16_t(*(uint16_t *)index_p) == 2)    //解析环保代码长度，不是17 退出
				{
					index_p += 2;
					
					tmp_mq.cmd = 0;
					tmp_mq.len = 2;
					*(uint16_t *)&tmp_mq.data[0] = swap_uint16_t(*(uint16_t *)index_p);
					
					t_q = get_can_send_queue();
					if(t_q != NULL)
						xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
				}
				
			}

			//
			index_p += 2;
			if(*index_p == 3)
			{
				struct products_mq_str mq;
				QueueHandle_t t_q = NULL;

				index_p++;
				if(swap_uint16_t(*(uint16_t *)index_p) != 17)    //解析环保代码长度，不是17 退出
					break;

				index_p += 2;   //跳过长度

				mq.cmd = 3;
				mq.len = 18;
				mq.data[0] = 29;
				memset(&mq.data[1],'\0',20);
				memcpy(&mq.data[1],index_p,17);   //拷贝机械环保代码
				printf("-- the vin:%s\r\n",&mq.data[1]);
				t_q = get_products_queue();
        		if(t_q != NULL)
            		xQueueSend(t_q,&mq,sizeof(struct products_mq_str));	

			
				if(read_config_archival_state() != 1 && read_config_archival_state() != 2 &&  get_products_cfg_model() == 0x55 && read_archive_en_state() == 0 && read_in_acc_state() > 0)
				{
					xTaskCreate(thread_entry_archivel,          "thread_entry_archivel",        4096,       NULL,  15,  NULL);  //创建生产处理任务
					printf("-- Creat thread entry archivel (1)\r\n");
				}
			}
			
			break;
		case 0x04:
			gb4.login_out_state = 0;
			printf("-- LogOut packet response... ok\r\n");
			break;
		case 0x81:    //设置  VIN 
			{
				p_down_cmd = (struct down_cmd_con_str *)(data + sizeof(struct start_str));
				//rt_kprintf("-- recv cmd.....%d,%d\r\n",Cmd_Type,p_down_cmd->cmd);
				switch(p_down_cmd->cmd)
				{
					case 0x80:               //设置VIN
						{
							struct products_mq_str 		mq;
							QueueHandle_t 				t_q = NULL;
							uint32_t   					tmp;
							uint8_t						m_array[50];

							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							mq.cmd = 3;

							mq.len = 50;

							memset(m_array,'\0',sizeof(m_array));
							tmp = swap_uint16_t(p_down_cmd->len);
							if(tmp > 50)
								tmp = 50;
							
							memset(mq.data,'\0',sizeof(m_array));
							mq.data[0] = 5;
							memcpy(m_array,index_p,tmp);
							sscanf((const char *)m_array,"%[^:]s",&mq.data[1]);
							mq.len = (uint8_t)strlen((const char *)&mq.data[1]);
							sscanf((const char *)m_array,"%*[^:]:%d",(uint32_t *)&mq.data[50]);

							printf("-- Set GatWay:%s:%d\r\n",&mq.data[1],*(uint32_t *)&mq.data[50]);
							
							t_q = get_products_queue();
        					if(t_q != NULL)
            					xQueueSend(t_q,&mq,sizeof(struct products_mq_str));
							
							vTaskDelay(300);
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x80;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
						}
						break;
					case 0x89:    //设置VIN
						{
							struct products_mq_str mq;
							QueueHandle_t t_q = NULL;

							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							mq.cmd = 3;

							mq.len = 18;
							mq.data[0] = 29;
							memset(&mq.data[1],'\0',20);
							memcpy(&mq.data[1],index_p,17);
							//printf("-- the vin:%s\r\n",mq.data);
							t_q = get_products_queue();
        					if(t_q != NULL)
            					xQueueSend(t_q,&mq,sizeof(struct products_mq_str));
							
							vTaskDelay(300);
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x89;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;
							
							if(read_archive_en_state() == 0 && read_in_acc_state() > 0)
							{
								xTaskCreate(thread_entry_archivel,          "thread_entry_archivel",        4096,       NULL,  15,  NULL);  //创建生产处理任务
								printf("-- Creat thread entry archivel (2)\r\n");
							}	

							return build_gb4_response_packets(buf,size,p_res);
						}
						break;
					case 0x92:           //设置机械环保代码 
						{
							struct products_mq_str mq;
							QueueHandle_t t_q = NULL;

							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							mq.cmd = 3;
							mq.len = 18;
							mq.data[0] = 29;
							memset(&mq.data[1],'\0',20);
							memcpy(&mq.data[1],index_p,17);
							//printf("-- the vin:%s\r\n",mq.data);
							t_q = get_products_queue();
        					if(t_q != NULL)
            					xQueueSend(t_q,&mq,sizeof(struct products_mq_str));
							
							vTaskDelay(300);
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x92;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;


							//创建一个备案任务
						if(read_archive_en_state() == 0 && read_in_acc_state() > 0)
						{
							xTaskCreate(thread_entry_archivel,          "thread_entry_archivel",        4096,       NULL,  15,  NULL);  //创建生产处理任务
							printf("-- Creat thread entry archivel (3)\r\n");
						}	


							return build_gb4_response_packets(buf,size,p_res);
						}
						break;
					case 0x94:          //设置三合一状态
						{
							struct products_mq_str mq;
							QueueHandle_t t_q = NULL;

							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							mq.cmd = 3;
							mq.len = 3;
							mq.data[0] = 36;     //

							mq.data[1] = *index_p;

							printf("-- set nj state:%d\r\n",mq.data[1]);

							t_q = get_products_queue();
        					if(t_q != NULL)
            					xQueueSend(t_q,&mq,sizeof(struct products_mq_str));
							
							vTaskDelay(300);
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x94;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
						}
						break;
					case 0x96:          // 设置国标数据上传时间间隔
						{
							struct products_mq_str mq;
							QueueHandle_t t_q = NULL;

							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							mq.cmd = 3;
							mq.len = 3;
							mq.data[0] = 34;

							*(uint16_t *)&mq.data[1] = swap_uint16_t(*(uint16_t *)index_p);

							//printf("-- set gb4 upload cycle:%d\r\n",*(uint16_t *)&mq.data[1]);

							t_q = get_products_queue();
        					if(t_q != NULL)
            					xQueueSend(t_q,&mq,sizeof(struct products_mq_str));
							
							vTaskDelay(300);
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x96;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
							
						}
						break;
					case 0x81:
						{
							struct products_mq_str mq;
							QueueHandle_t t_q = NULL;

							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							mq.cmd = 3;
							mq.len = 3;
							mq.data[0] = 9;

							*(uint16_t *)&mq.data[1] = swap_uint16_t(*(uint16_t *)index_p);

							//printf("-- set en upload cycle:%d\r\n",*(uint16_t *)&mq.data[1]);

							t_q = get_products_queue();
        					if(t_q != NULL)
            					xQueueSend(t_q,&mq,sizeof(struct products_mq_str));
							
							vTaskDelay(300);
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x81;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
							
						}
					default:
						
						break;
				}
			}
			break;
		case 0x82: //终端控制
			{
				p_down_cmd = (struct down_cmd_con_str *)(data + sizeof(struct start_str));
				//rt_kprintf("-- recv cmd.....%d,%d\r\n",Cmd_Type,p_down_cmd->cmd);
				switch(p_down_cmd->cmd)
				{
					case 0x88:
					{
						struct can_send_mq_t tmp_mq;
						QueueHandle_t t_q = NULL;

						tmp_mq.cmd = 4;
						tmp_mq.len = 2;
						tmp_mq.data[0] = 0;
						tmp_mq.data[1] = *((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));

						if(tmp_mq.data[1] == 0 || tmp_mq.data[1]  == 2)
							tmp_mq.data[1] = 1;
						else
							tmp_mq.data[1] = 0;

						t_q = get_can_send_queue();
						if(t_q != NULL)
							xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
				
				
						printf("-- recv mon cmd...Down..%d\r\n",tmp_mq.data[0]);

						p_res = (struct down_cmd_res_str *)array;
						p_res->cmd_id = 0x81;
						p_res->msg_type = 0x80;
						p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
						p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
						p_res->res = 0;

						return build_gb4_response_packets(buf,size,p_res);
					}
					break;
					case 0x86:    //锁车命令
						{
							struct can_send_mq_t tmp_mq;
							QueueHandle_t t_q = NULL;

							tmp_mq.cmd = 4;
							tmp_mq.len = 2;
							tmp_mq.data[0] = 1;
							tmp_mq.data[1] = *((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str)) + 1;
							t_q = get_can_send_queue();
							if(t_q != NULL)
								xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
								
							printf("-- recv lock cmd...Down..%d\r\n",tmp_mq.data[1]);
						
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x80;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
						}
						break;
					case 0x87:    //解锁命令
						{
							struct can_send_mq_t tmp_mq;
							QueueHandle_t t_q = NULL;

							tmp_mq.cmd = 4;
							tmp_mq.len = 2;
							tmp_mq.data[0] = 1;
							tmp_mq.data[1] = *((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							t_q = get_can_send_queue();
							if(t_q != NULL)
								xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
							 printf("-- recv unlock cmd...Down..%d\r\n",tmp_mq.data[1]);
							//rt_mq_send(get_can_lock_mq(),&event,sizeof(event));
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x80;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
						}
						break;
					case 0x01:     //远程升级命令
						{
							struct iap_ftp_str ftp_info;
							uint16_t 	tmp_len;
							
							tmp_len = swap_uint16_t(p_start->len);
							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							//printf("-- the ftp update..  %d ...%s\r\n",tmp_len,index_p);
							memset((uint8_t *)&ftp_info,'\0',sizeof(struct iap_ftp_str));
							if(get_buf_str(8,9,';',index_p,array,tmp_len) > 0)      
							{
								index_p = array + 6;
								offset += get_given_string((uint8_t *)index_p, ':',ftp_info.user);
								offset += get_given_string((uint8_t *)index_p + offset, '@', ftp_info.passwd);
								offset += get_given_string((uint8_t *)index_p + offset, ':', ftp_info.host);
								offset += get_given_number((uint8_t *)index_p + offset, '/', &ftp_info.port);
								offset += get_given_string((uint8_t *)index_p + offset, ':', ftp_info.files);
								
								printf("\r\n-- FTP FtpUserName:%s\r\n",ftp_info.user);
								printf("-- FTP FtpUserPassd:%s\r\n",ftp_info.passwd);
								printf("-- FTP Server Addr:%s\r\n",ftp_info.host);
								printf("-- FTP Server Port:%d\r\n",ftp_info.port);
								printf("-- FTP File Name:%s\r\n",ftp_info.files);	

								if(read_iap_state() == 0)
								{
									xTaskCreate(thread_entry_iap, "thread_entry_iap",        4096,&ftp_info,  14,  NULL);  //创建FTP升级任务
									vTaskDelay(100);
								}
								
								
								p_res = (struct down_cmd_res_str *)array;
								p_res->cmd_id = 0x82;
								p_res->msg_type = 0x01;
								p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
								p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
								p_res->res = 0;

								return build_gb4_response_packets(buf,size,p_res);
							}
							break;
						}
					}
					break;
				}
				default:
					break;
		}

		return 0;
}










/*****************************
**  处理国四的任务
*******************************/

void thread_entry_gb4(void *parameter)
{
	uint16_t 		send_len = 0;
	uint16_t 		qb4_len = 0;
	uint8_t 		i = 0;

    parameter = parameter;

    gb4_down_qh = xQueueCreate(2,sizeof(struct socket_down_str));
	
	init_gb4_fifo_buff();

	vTaskDelay(100);
	init_gb4_blind_log_file();
	init_qb4_blind_log_file();
	load_login_out_num();

	

	memset(encrypt_chip.id_lot,0,sizeof(encrypt_chip.id_lot));

	if(rt_hw_init_acl16() == 0)
	{
		printf("-- Init ACL16 ok.....\r\n");
	}

	if(acl16_write_then_read(CRY_GETCHIPID,local_buff,encrypt_chip.id_sn,16) == 16)
	{
		encrypt_chip.state = 1;

		local_buff[0] = encrypt_chip.id_sn[0];   		//LotID  第一个字节
		local_buff[1] = encrypt_chip.id_sn[1];    		//LotID  第二个字节
		local_buff[2] = encrypt_chip.id_sn[2];			//LotID  第三个字节
		local_buff[3] = encrypt_chip.id_sn[3];			//LotID  第四个字节
		
		local_buff[4] = encrypt_chip.id_sn[4];   			//Wafer ID  (晶片ID)
		local_buff[5] = encrypt_chip.id_sn[8];	  		//Loc   X
		local_buff[6] = encrypt_chip.id_sn[10];  			//Loc   Y
		local_buff[7] = encrypt_chip.id_sn[13];  			// Month 生产日期-月 
		
		hex_to_str(local_buff,8,encrypt_chip.id_lot,16);
		
		//read_config_eco_mark(&encrypt_chip.id_lot[0],4);
		encrypt_chip.id_lot[0] = 'A'; 
		encrypt_chip.id_lot[1] = 'I';
		encrypt_chip.id_lot[2] = 'H';
		encrypt_chip.id_lot[3] = 'Q';
		printf("-- the chip id lot:%s\r\n",encrypt_chip.id_lot);
	}
	else
	{
		encrypt_chip.state = 0;
		printf("-- Read ACL chip id Fail \r\n");
		memset((uint8_t *)&encrypt_chip,0,sizeof(struct encryption_chip_str));
	}
	
	vTaskDelay(10);

	if(acl16_write_then_read(CRY_GETAPPVER,local_buff,(uint8_t *)&encrypt_chip.version,4) == 4)
	{
		encrypt_chip.state = 1;
		printf("-- the encry chip app version :%d\r\n",encrypt_chip.version);
	}
	else
	{
		encrypt_chip.state = 0;
		printf("-- the encry chip app version fail\r\n");
	}

	vTaskDelay(50);

    for(;;)
    {
        if(read_lte_net_init_state() == 0)
        {
            vTaskDelay(100);
			memset((uint8_t *)&gb4,0,sizeof(gb4));
            continue;
        }

		reset_serial_num();
		//printf("-- thread_entry_gb4 ... \r\n");
        if(xQueueReceive(gb4_down_qh,&gb_down_data,10) == pdTRUE)
        {
			if(gb_down_data.len > 0)
			{
				if(strstr((char *)gb_down_data.data,"+QIURC:") != NULL)
				{
					//printf("-- len  data\r\n");
					if(at_close_socket_connect(GB4_SOCKET_ID) == 0)
					{
						printf("-- Close gb4 socket ok.......\r\n");
					}

					gb4.socket_state = 0;
					gb4.qb_step = 0;
					gb4.gb_step = 0;

					gb4.gb_cnt = 1;
					gb4.qb_cnt = 1;
					gb4.login_out_state = 0;

					continue;
				}
				//下行命令机械
				send_len = gb4_platform_down_parse(gb_down_data.data,gb_down_data.len,local_buff,sizeof(local_buff));
				//mem_printf(LOG_ERROR, PRINT_HEX,local_buff, send_len);
            	if(send_len > 0)
				{
					if(at_send_socket_data(GB4_SOCKET_ID,local_buff,send_len) == 0)
                	{
                   		printf("-- Send GB4 Down Cmd Res OK......%d\r\n",send_len);
                	}
				}
			}  
        }
	
		
		if(read_sys_run_state() > 1 && gb4.gb_step == 0)
		{
			if(at_query_socket_state(GB4_SOCKET_ID) > 0)
			{
				if(at_close_socket_connect(GB4_SOCKET_ID) == 0)
				{
					gb4.socket_state = 0;
					printf("-- close gb4 socket onnect ok...\r\n");
				}	
			}
			continue;
		}

		switch(gb4.qb_step)    // 企标数据
		{
			case 0:
				if(gb4.qb_cnt++ % 300 == 0)
				{
               		struct socket_addr_str tmp;
					if(at_query_socket_state(GB4_SOCKET_ID) > 0)    //查询Socket创建链接的状态
					{
						if(at_close_socket_connect(GB4_SOCKET_ID) == 0)
						{
							gb4.socket_state = 0;
							printf("-- close gb4 socket onnect ok...\r\n");
						}	
					}

					read_enterprise_gw_addr(tmp.addr,sizeof(tmp.addr));
					tmp.port = read_enterprise_gw_port();

					if(at_creat_socket_connect(GB4_SOCKET_ID,&tmp,gb4_down_qh) == 0)
					{
						gb4.socket_state = 1;
						gb4.qb_step++;
						printf("-- Creat gb4 socket onnect:%s:%d.....OK\r\n",tmp.addr,tmp.port);  
					}
					else
					{
						printf("-- Creat gb4 socket onnect:%s:%d.....Fail\r\n",tmp.addr,tmp.port); 
					}         
            	}
				break;
			case 1:
				qb4_len = 0;
				for(i = 0;i < 2;i++)
				{
					send_len = read_qb4_fifo_buff(local_buff + qb4_len,QB4_DATA_LEN);
					if(send_len > 0)
					{
						qb4_len += send_len;
					}
				}
				
				if(qb4_len > 0)
				{
					if(at_send_socket_data(GB4_SOCKET_ID,local_buff,qb4_len) == 0)
        		    {
                		printf("-- Send QB4 Data OK......%d\r\n",qb4_len);
        		   	}
					else
					{
						printf("-- Send QB4 Data Fail......%d\r\n",qb4_len);
						gb4.qb_step = 0;
						gb4.qb_cnt = 0;
					}
					qb4_len = 0;	
				}
				break;
			default:
				gb4.qb_step = 0;
				gb4.qb_cnt = 0;
				break;
		}
		
        switch(gb4.gb_step)   //处理相关国四
        {
            case 0:
               if(read_in_acc_state() == 0 || gb4.socket_state == 0 || read_iap_state() > 0 || get_products_cfg_model() != 0x55)             //如果电锁没开
					break;
				
				if(gb4.login_out_state == 0)
				{
					//printf("-- the run is... 1 ..%d\r\n",gb4.gb_cnt);
					if(gb4.gb_cnt++ % 300 > 0)             //电锁打开
						break;
					send_len = build_gb_vehicle_login_out(0x01,local_buff,sizeof(local_buff));
					//printf("-- the run is.. 2 ...%d\r\n",send_len);
					if(send_len > 0)
					{
						if(at_send_socket_data(GB4_SOCKET_ID,local_buff,send_len) == 0)
						{
							printf("-- Send QB4 Vehicle login IN Data OK 0x01......%d\r\n",send_len);
						}
						else
						{
							printf("-- Send QB4 Vehicle login IN Data Fail 0x01......%d\r\n",send_len);
						}
					}
				}
				else
				{
					gb4.gb_step++;	
				}
				
                break;
            case 1:                   //发送数据  国标数据
			 	qb4_len = 0;
				for(i = 0;i < 3;i++)
				{
					send_len = read_gb4_fifo_buff(local_buff + qb4_len,GB4_DATA_LEN);
					if(send_len > 0)
					{
						qb4_len += send_len;
					}
				}
				
				if(qb4_len > 0)
				{
					if(at_send_socket_data(GB4_SOCKET_ID,local_buff,qb4_len) == 0)
        		    {
                		printf("-- Send GB4 Data OK ACC ON ...... %d\r\n",qb4_len);
        		   	}
					else
					{
						printf("-- Send GB4 Data OK ACC Fail......%d\r\n",qb4_len);
						gb4.qb_step = 0;
						gb4.qb_cnt = 0;
					}
					qb4_len = 0;	
				}
			
				if(read_in_acc_state() == 0 || read_sys_run_state() > 0 || read_iap_state() > 0)
				{
					gb4.gb_step++;
					gb4.gb_cnt = 0;	
				}			
                break;
			case 2:
				send_len = read_gb4_fifo_buff(local_buff,GB4_DATA_LEN);
				if(send_len > 0)
				{
					if(at_send_socket_data(GB4_SOCKET_ID,local_buff,send_len) == 0)
					{
						printf("-- Send GB4 Data OK ACC OFF ...... %d\r\n",send_len);
					}
				}
			
				if(++gb4.gb_cnt % 30 == 0)
					gb4.gb_step++;
				break;
			case 3:           //登出
				if(gb4.login_out_state == 0 || gb4.gb_cnt > 100)
				{
					gb4.gb_step = 0;
					gb4.login_out_state = 0;
					gb4.gb_cnt = 0;
					break;
				}

				if(++gb4.gb_cnt % 50 > 0)
					break;
				
				send_len = build_gb_vehicle_login_out(0x04,local_buff,sizeof(local_buff));
				if(send_len > 0)
				{
					if(at_send_socket_data(GB4_SOCKET_ID,local_buff,send_len) == 0)
					{
						printf("-- Send QB4 Vehicle login Out Data OK 0x04......%d\r\n",send_len);
					}
				}
				break;
            default:
                gb4.gb_step = 0;
				gb4.login_out_state = 0;
                break;
        }
    }
}







