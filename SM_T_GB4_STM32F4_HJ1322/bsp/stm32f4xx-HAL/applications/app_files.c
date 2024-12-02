

#include "board.h"
#include "app_files.h"
#include "dfs_posix.h"
#include "dfs_fs.h"


#include "drv_rtc.h"


struct sd_info_str									sd_info = {0};

static struct files_sys_t 					files_sys = {0};   //临时使用

/*********************************
**	返回文件系统挂载状态
*********************************/
	
uint8_t read_files_sys_state(void)
{
	uint8_t rv;
	
	rv = sd_info.sd_mout_state;
	//rv = 1;
	
	return rv;
}	

	


//文件系统总空间大小 MB
uint16_t read_sd_total_mb(void)   
{
	uint16_t rv;
	
	rv = (uint16_t)sd_info.sd_total_mb;
	
	return rv;
}  
	

//文件系统剩余空间大小 MB
uint16_t read_sd_free_mb(void)
{
	uint16_t rv;
	
	rv = (uint16_t)sd_info.sd_free_mb;
	
	return rv;
}	


/***************************
**	挂载EMMC文件系统
****************************/

uint8_t  mount_files_sys(void)
{
	int rv = 0;
	struct statfs buf = {0};
	rt_device_t dev_id;
	
	dfs_unmount("/");
	
	if((dev_id = rt_device_find("sd0")) == NULL)
	{
	    /* no this device */
	    rt_set_errno(-ENODEV);
	    return 1;
	}
  dev_id->flag &= ~RT_DEVICE_FLAG_ACTIVATED;
	
	rv = dfs_mount("sd0", "/", "elm", 0, 0);
	if(rv == 0)
	{
		sd_info.sd_mout_state = 1;
		dfs_statfs("/", &buf);		
		sd_info.sd_total_mb = ((long long)buf.f_blocks * (long long)buf.f_bsize) >> 20;
    sd_info.sd_free_mb = ((long long)buf.f_bfree * (long long)buf.f_bsize) >> 20;
		//
		rt_kprintf("-- File System total capacity:%dMB\r\n",sd_info.sd_total_mb); 
		rt_kprintf("-- File System free capacity :%dMB\r\n",sd_info.sd_free_mb);  
		
		
		files_sys.total_capacity = sd_info.sd_total_mb;
		
		return 0;
	}
	else
	{
		sd_info.sd_mout_state = 0;
		dfs_mkfs("elm", "sd0");
		rt_kprintf("-- EMMC Files Sys mount failed !\r\n");
	}
	
	return 1;
}



/************************
**	下载文件系统
**************************/

uint8_t  unmount_files_sys(void)
{
	if(dfs_unmount("/") == 0)
	{
		memset((uint8_t *)&sd_info,0,sizeof(sd_info));
		return 0;
	}
	
	return 1;
}



/************************
**	格式化文件系统
**************************/

uint8_t mkfs_files_sys(void)
{
	int value = 0;
    
	value = dfs_mkfs("elm", "sd0");
	
	if(RT_EOK == value)
  {
		rt_kprintf("-- File system format OK ... \r\n"); 
		
		rt_thread_delay(10);
		
		mount_files_sys();
  }
  else
  {
		rt_kprintf("-- File system format failed ... \r\n"); 
		return 1;
  }
	
	return 0;
}




/*********************************
 *  打印文件信息
 ********************************/

uint32_t print_files_info(uint8_t *name)
{
    struct stat ss = {0};
    uint8_t array[32] = {0};
    struct rt_tm *tm = NULL;
    uint32_t  tmp = 0,m = 0,n = 0;

    if(name == NULL)
        return 0;

    sprintf((char *)array,"/%s\r\n",(char *)name);
    //printf("-- %s\r\n",array);
		stat((const char *)array, &ss);
		//printf("-- gnss info files size %u,%u\r\n",ss.st_size,ss.st_mtime);
	
    ss.st_mtime += 28800;
    tm = rt_localtime((const uint32_t *)&ss.st_mtime);
    if(tm != NULL)
    {
        tmp = ss.st_size * 1.0 / 1024.0 * 1000;
        m = tmp % 1000;
        tmp /= 1000;
        n = strlen((const char *)name);
        if(n > 20)
            n = 20;
        memset(array,' ',sizeof(array));
        memcpy(array,name,n);
        array[21] = '\0';
        rt_kprintf("-- %s  % 6d.%03dKB       20%02d,%02d,%02d  %02d:%02d:%02d            ROOT                EMMC\r\n",array,tmp,m,tm->year,tm->mon,tm->day,tm->hour,tm->min,tm->sec);

        return ss.st_size;
    }

    return 0;  

}



/******************************
**
*******************************/

void show_files_sys_info(void)
{
    uint32_t m = 0;

    if(sd_info.sd_mout_state == 1)
    {
        struct rt_tm tm;

        rt_get_rtc(&tm);

        rt_kprintf("\r\n-----------------------------------------------------------------------------------------------------------\r\n");
        rt_kprintf("-- 当前系统时间:20%02d.%02d.%02d:%02d-%02d-%02d (RTC)\r\n",tm.year,tm.mon,tm.day,tm.hour,tm.min,tm.sec);
        rt_kprintf("-- 终端型号: Homer4C-FDL\r\n");
        rt_kprintf("-- 终端类型: 国四排放终端\r\n");
        rt_kprintf("-- 终端应用程序版本: V2.0\r\n");
        rt_kprintf("-----------------------------------------------------------------------------------------------------------\r\n");

        

        rt_kprintf("\r\n------------------------------------------ 本地文件信息 ---------------------------------------------------\r\n");
        rt_kprintf("-- FilesName                     Size        M_time                          Position             Mounted\r\n");
        files_sys.used_capacity += print_files_info((uint8_t *)"qb4_blind.dat");
				files_sys.used_capacity += print_files_info((uint8_t *)"qb4_log.txt");
        files_sys.used_capacity += print_files_info((uint8_t *)"gb4_blind.dat");
				files_sys.used_capacity += print_files_info((uint8_t *)"gb4_log.txt");
        files_sys.used_capacity += print_files_info((uint8_t *)"gb4_flash.dat");
				files_sys.used_capacity += print_files_info((uint8_t *)"local_log.txt");
			
        rt_kprintf("-----------------------------------------------------------------------------------------------------------\r\n");

        m = files_sys.total_capacity  * 1024.0;

        files_sys.surplus_capacity = m - (files_sys.used_capacity / 1024.0);
        files_sys.pre = (files_sys.used_capacity / 1024.0) / (files_sys.total_capacity * 1024.0) * 10000.0;



        rt_kprintf("\r\n------------------------------------------ 本地存储空间信息 -----------------------------------------------\r\n");
        rt_kprintf("-- Filesystem                 Size            Used               Available         Use(%%)      Mounted\r\n");
        
				if((files_sys.used_capacity / 1024) > 1024)
				{
					rt_kprintf("-- EMMC                    % 7dMB          %d.%03dMB           %d.%03dMB       %d.%d(%%)      EMMC\r\n",files_sys.total_capacity,\
					files_sys.used_capacity / (1024),files_sys.used_capacity % (1024),files_sys.surplus_capacity / (1024),\
					files_sys.surplus_capacity % (1024),files_sys.pre / 100,files_sys.pre % 100);
        }
				else
				{
					files_sys.used_capacity /= 1024;
					rt_kprintf("-- EMMC                    % 7dMB          %d.%03dKB           %d.%03dMB       %d.%d(%%)      EMMC\r\n",files_sys.total_capacity,\
					files_sys.used_capacity / (1024),files_sys.used_capacity % (1024),files_sys.surplus_capacity / (1024),\
					files_sys.surplus_capacity % (1024),files_sys.pre / 100,files_sys.pre % 100);
				}
				
				rt_kprintf("-----------------------------------------------------------------------------------------------------------\r\n\r\n\r\n");

       
    }
    else if(sd_info.sd_mout_state == 0)
    {
        rt_kprintf("-- mount_sys_files ... fail\r\n");   //挂载失败
    }
    else if(sd_info.sd_mout_state == 2)
    {
        rt_kprintf("-- Init file stro ... fail\r\n");
    }
}




