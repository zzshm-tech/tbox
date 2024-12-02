


#ifndef _APP_FILES_
#define _APP_FILES_

#include "board.h"


#define BLIND_DATA_MAX_NUM 	6000
#define BLIND_DATA_MAX_CNT
	




struct sys_run_log_str
{
	struct rt_semaphore								fifo_sem;
	uint16_t                          len;
	uint8_t 													log_buf[4096];
};



struct sd_info_str
{
	uint8_t sd_mout_state;    //文件系统挂在状态
	uint32_t sd_total_mb;     //文件系统总空间大小 MB
	uint32_t sd_free_mb;      //文件系统剩余空间大小 MB
};





struct files_sys_t
{
    uint8_t     state;
    uint32_t    total_capacity;    //总容量  （MB）
    uint32_t    used_capacity;     //已经使用的用量（MB）
    uint32_t    surplus_capacity;  //可用容量
    uint8_t     pre;
};



uint8_t read_files_sys_state(void);
uint8_t mount_files_sys(void);
uint8_t unmount_files_sys(void);
uint8_t mkfs_files_sys(void);
void show_files_sys_info(void);
uint16_t read_sd_total_mb(void);
uint16_t read_sd_free_mb(void);

#endif

