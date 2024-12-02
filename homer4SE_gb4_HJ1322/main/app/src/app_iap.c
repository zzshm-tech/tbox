

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_ota_ops.h"

#include "board.h"

#include "app_iap.h"
#include "app_at.h"
#include "app_lte.h"
#include "app_products.h"
#include "app_main.h"
#include "app_in.h"




#define BUFFSIZE            4128

//#define READ_FTP_SIZE       4096

static uint32_t          read_ftp_size = 2048;



/*********************  *******************************/

static uint8_t                  local_data[BUFFSIZE] = {0};

static uint32_t                 file_total_size = 0;

static uint32_t                 read_offset = 0;

static  uint32_t                file_size = 0;           //要升级的文件大小

static const esp_partition_t    *update_partition = NULL;   //

static  esp_ota_handle_t        update_handle = 0;

static uint8_t                  iap_state = 0;      //0:未升级；1：升级中



/**********************************
**  返回升级状态
***********************************/

uint8_t read_iap_state(void)
{
    uint8_t rv = 0;

    rv = iap_state;

    return rv;
}



/****************************************
**	擦除配置信息
*****************************************/

uint8_t erase_otadata_info(void)
{
	const esp_partition_t *partition = NULL;
    
    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY,"otadata");
   
    if(partition == NULL)
    {
        printf("-- esp otadata partition find first fail...\r\n");
        return 1;
    }

	if(ESP_OK != esp_partition_erase_range(partition, 0, partition->size))
    {
        printf("-- erase otadata info faile \r\n");
        return 1;
    }

    printf("-- erase otadata info OK... %d\r\n",partition->size);
	vTaskDelay(10);
	
	return 0;
}

/**********************************
**  开始升级
**  0:升级完成
**  1:升级失败
***********************************/

uint8_t start_ftp_iap(void)
{
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
   

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if(configured != running)
    {
        printf("-- Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        printf("-- (This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    printf("-- Running partition type %d subtype %d (offset 0x%08x)\r\n",
             running->type, running->subtype, running->address);

    update_partition = esp_ota_get_next_update_partition(NULL);
    printf("-- Writing to partition subtype %d at offset 0x%x\r\n",
             update_partition->subtype, update_partition->address);
    
    if(update_partition == NULL)
        return 1;


    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK)
    {
        printf("-- esp_ota_begin failed (%s)", esp_err_to_name(err));
        return 1;
    }

    printf("-- the start update_handle:%u\r\n",update_handle);

    return 0;
}



/******************************
 **
 *****************************/

uint8_t down_and_wirte_esp_ota(uint8_t *file_name)
{
    uint32_t read_size = 0;
    uint32_t data_read = 0;
    esp_err_t err;

    if(read_lte_type() == 0)
        read_ftp_size = 4096;
    else
        read_ftp_size = 2048;

    while (file_size > 0)
    {
        
        if (file_size > read_ftp_size)
        {
            read_size = read_ftp_size;
        }
        else
        {
            read_size = file_size;
        }

        if(read_in_power_vol() < 80)
        {
            printf("-- the tbox extern power dis...... iap over.....\r\n");
            return 3;
        }
        data_read = at_read_ftp_files_size(file_name,(uint8_t *)local_data,read_offset,read_size);
        
        if(data_read > 0)  //判断外部电源电压
        {
            //printf("-- recv ftp file data len ... %d,%u\r\n",data_read,update_handle);
            err = esp_ota_write(update_handle, (const void *)local_data, data_read);
            if (err != ESP_OK)
            {
                printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
                return 2;
            }
            file_size -= data_read;
            read_offset += data_read;

            printf("-- Download ota file from ftp  %.2f%% ... %u,%u,%u,%u\r\n",((read_offset * 1.0) / file_total_size) * 100,file_size,read_offset,file_total_size,data_read);
        }
        else
        {
            return 1;
        }
    }

    return 0;
}

    //

/***********************************
**
************************************/

uint8_t end_ftp_iap(void)
{
    esp_err_t err = ESP_OK;

    err = esp_ota_end(update_handle);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED)
        {
            //erase_otadata_info(); //测试使用
            printf("-- Image validation failed, image is corrupted \r\n");
        }
        
        printf("-- esp_ota_end failed (%s)! \r\n", esp_err_to_name(err));
        return 2;
   }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
        printf("-- esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        return 2;
    }

    return 0;
}



/*******************************
**
*********************************/

void thread_entry_iap(void *parameter)
{
    uint8_t                     step = 0;
    struct iap_ftp_str          iap_info = {0};
    struct ftp_info_str         sa = {0};
    uint8_t                     res = 0;
    QueueHandle_t  	            qu_t = NULL;          //生产队列
    struct app_main_mq_str      a_mq = {0};

    if(parameter != NULL)
    {
        iap_info = *(struct iap_ftp_str *)parameter;
        printf("\r\n-- IAP FtpUserName:%s\r\n",iap_info.user);
		printf("-- IAP FtpUserPassd:%s\r\n",iap_info.passwd);
		printf("-- IAP Server Addr:%s\r\n",iap_info.host);
		printf("-- IAP Server Port:%d\r\n",iap_info.port);
		printf("-- IAP File Name:%s\r\n",iap_info.files);	
    }

    memset((uint8_t *)&sa,'\0',sizeof(sa));

    file_total_size = 0;
    read_offset = 0;
    file_size = 0;
    update_partition = NULL;
    update_handle = 0;

    iap_state = 1;

    for(;;)
    {
        if(read_lte_net_init_state() == 0)
        {
            vTaskDelay(100);
			vTaskDelete(NULL);
        }

        //printf("-- run step:%d\r\n",step);
        switch(step)
        {
            case 0:                 //配置FTP相关
                memcpy(sa.host,iap_info.host,sizeof(iap_info.host));   //主机地址
                sa.port = iap_info.port;                                        //
                memcpy(sa.user,iap_info.user,sizeof(iap_info.user));            //
                memcpy(sa.passwd,iap_info.passwd,sizeof(iap_info.passwd));      //
                
                if(at_config_ftp_account(&sa) == 0)
                {
                    printf("-- config ftp account ok......\r\n");
                }
                else
                {
                    step = 5;
                }

                if(at_config_ftp_transmode(1) == 0)
                {
                    printf("-- config ftp transmode ok......\r\n");
                }
                else
                {
                    step = 5;
                }

                if(at_config_ftp_rsptimeout(90) == 0)
                {
                    printf("-- config ftp rsptimeout ok......\r\n");
                }
                else
                {
                    step = 5;
                }
                 
                if(at_config_ftp_file_type(0) == 0)
                {
                    printf("-- config ftp file_type ok......\r\n");
                    step++;
                }
                else
                {
                    step = 5;

                }
                break;
            case 1:                 //下载FTP文件
                res = start_ftp_iap();
                if(res == 0)
                {
                    printf("-- start ftp iap ok.....\r\n");
                    read_offset = 0;
                    step++;
                }
                else
                {
                    step = 5;
                }
                break;

            case 2:   //链接 FTP服务器
                if(at_get_ftp_link_state() == 1)
				{	
					if(at_close_ftp_connect() == 0)
						printf("-- close ftp server connect......ok\r\n");
				}

                if(at_creat_ftp_connect(&sa) == 0)
                {
                    printf("-- Creat ftp connect ok......\r\n");
                }
                else
                {
                    printf("-- Creat ftp connect Fail......\r\n");
                    step = 5;
                    break;
                }

                    /*获取文件大小*/
                if((file_total_size = at_get_ftp_files_size(iap_info.files)) == 0)
                {

                    printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
                    step = 5;
                    break;
                }
                

                printf("-- OTA file total size = %d\r\n", file_total_size);
                
                if (file_total_size <= 0)
                {
                    printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
                   step = 5;
                   break;
                }
                file_size = file_total_size - read_offset;
                if(at_download_ftp_files(iap_info.files,read_offset) == 0)
				{
					printf("-- DownLoad Ftp Files Ok ....  %d\r\n",read_offset);
				}
				else
				{
					step = 0;   //下载文件失败
					break;;
				}	
                step++;

                break;
            case 3:    //下载FTP文件并写入OTA
                res = down_and_wirte_esp_ota(iap_info.files);
                if(res == 0)
                {
                    printf("-- down load iap files ok.....\r\n");
                    step++;
                }
                else if(res == 1)   //下载过程中断开，可以重新链接
                {
                    printf("-- ftp down disconnect......\r\n");
                    if(at_close_ftp_connect() == 0)
                        printf("-- close ftp connect......ok\r\n");
                     
                    step = 2;   //重新链接
                    break; 
                }
                else
                {
                    step = 5;   //升级失败
                }
                break;
            case 4:    //结束FTP升级
                 if(end_ftp_iap() == 0)
                {
                    printf("-- IAP function .............. OK\r\n");
                }
                else
                {
                    printf("-- IAP function .............. Fail\r\n");
                }
                 if(at_close_ftp_connect() == 0)
                    printf("-- close ftp connect......ok\r\n");
                
                printf("-- Reset be from fpt iap ...\r\n");
                a_mq.state = 1;

                qu_t =  get_app_main_queue();
                if(qu_t != NULL)
                {
                    xQueueSend(qu_t,&a_mq,sizeof(struct app_main_mq_str));
                }
                vTaskDelay(1000);
                step++;
                break;
            case 5:
                step = 0;
                 if(at_close_ftp_connect() == 0)
                    printf("-- close ftp connect......ok\r\n");
                printf("-- Del thread entry iap\r\n");
                iap_state = 0;
			    vTaskDelete(NULL);

                break;
            default:
                step = 0;

                break;
        }
    }
}






