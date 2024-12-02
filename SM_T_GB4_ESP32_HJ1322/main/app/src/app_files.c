


#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "common.h"

#include "app_files.h"



#define MOUNT_POINT             "/nandflash"



/***************  ******************/

static struct files_sys_t files_sys = {0};


/**********************************
**
***********************************/

uint8_t  read_files_sys_state(void)
{
    uint8_t rv;

    rv = files_sys.state;

    return rv;
}




/******************************
**
*******************************/

void show_files_sys_info(void)
{
    if(files_sys.state == 0)
    {
        printf("-- total_capacity ... %d\r\n",files_sys.total_capacity);     //总容量
        printf("-- used_capacity ... %d\r\n",files_sys.total_capacity);     //总容量
        printf("-- surplus_capacity ... %d\r\n",files_sys.total_capacity);     //总容量
    }
    else if(files_sys.state == 1)
    {
        printf("-- mount_sys_files ... fail\r\n");   //挂载失败
    }
    else if(files_sys.state == 2)
    {
        printf("-- Init file stro ... fail\r\n");
    }
}



/*****************************************
**  挂在文件系统
*****************************************/

uint8_t mount_sys_files(void)
{
    uint32_t    tmp;
    esp_err_t   ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config =
    {
        .format_if_mount_failed = true,
        .max_files = 8,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE //16 * 1024
    };

    sdmmc_card_t    *card;
    const char mount_point[] = MOUNT_POINT;

    printf("-- Initializing SD card");

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;

    gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
          
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) 
    {
        if (ret == ESP_FAIL)
        {
            printf("-- Failed to mount filesystem. ""If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
            files_sys.state = 1;
            return 1;
        } 
        else 
        {
            printf("-- Failed to initialize the card (%s). ""Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
            files_sys.state = 2;
            return 1;
        }
    }

    sdmmc_card_print_info(stdout, card);

    printf("-- CSD Sector size:%u\r\n",card->csd.sector_size);
    printf("-- CSD Capacity size:%u\r\n",card->csd.capacity);

    tmp = card->csd.capacity >> 10;
    tmp *= card->csd.sector_size;

    tmp >>= 10;

    printf("-- thie nan capacity :%uMB,\r\n",tmp);

    if(tmp < 120 ) //大于120M认为nand没问题
    {
        printf("-- read nand capacity error\r\n");
       files_sys.state = 3;
       return 1;
    } 
    
    return 0;
}



/******************************
**  卸载文件系统
*******************************/

uint8_t unmount_sys_file(void)
{
    uint8_t rv = 0;
    esp_err_t   ret;

    ret = esp_vfs_fat_sdmmc_unmount();
    if(ret == ESP_OK)
    {
        printf("\r\n-- unmount files sys OK...\r\n");
        files_sys.state = 0;
    }

    return rv;
}




/******************************
**  格式化文件系统
*******************************/

uint8_t format_files_sys(void)
{

    return 0;
}



/********************************
**  测试文件系统
*********************************/

uint8_t test_files(void)
{
    static uint8_t      step = 0;
    static uint8_t      cnt = 0;
	FILE                *fd;
	int                 res = -1;
    
    uint8_t array[50];
    
    switch(step)
    {
        case 0:

            fd = fopen("/nandflash/Wang.txt", "w");
            if(fd != NULL)
            {
                memset(array,'\0',50);
                sprintf((char *)array, "Homer4SE Files Sys Test : %d\r\n",cnt);
                printf("-- %s\r\n",array);
                fseek(fd,50 * cnt,SEEK_SET);
                fwrite((uint8_t *)array,50,1,fd);
                res = fclose(fd);
                
                if(res != 0)
	            {
		            printf("-- Blind area dt log files close failed\r\n");
                }
            }
            else
            {
                printf("-- Open Teest Files Fail\r\n");
            }
            
            cnt++;
            if(cnt > 50)
            {
                cnt = 0;
                step++;
            }
	           
            break;;
        case 1:
            fd = fopen("/nandflash/Wang.txt", "r");
	        if (fd != NULL)
	        {
                memset(array,'\0',50);
                fseek(fd,50 * cnt,SEEK_SET);
		        res = fread((uint8_t *)array,50,1,fd);
                printf("-- Read Files %d... %s\r\n",res,array);
		        res = fclose(fd);
            }
            else
            {
                printf("-- Open Read File....Fail....\r\n");
            }

            cnt++;
            if(cnt > 50)
            {
                cnt = 0;
                step++;
            }
                
            break;
        default:
            break;
    }	

    return 0;
}

