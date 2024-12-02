
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_sleep.h"

#include "board.h"
#include "drv_can.h"
#include "drv_acl16.h"
#include "drv_power.h"

#include "version.h"
#include "common.h"

#include "app_gnss.h"
#include "app_led.h"
#include "app_products.h"
#include "app_shell.h"
#include "app_can_recv.h"
#include "app_can_send.h"
#include "app_gb4.h"
#include "app_lte.h"
#include "app_at.h"
#include "app_in.h"
#include "app_packet.h"
#include "app_files.h"
#include "app_sms.h"
#include "app_main.h"
#include "app_iap.h"


#define TWDT_TIMEOUT_S  3


static TaskHandle_t             task_products_handle = NULL;        //创建生产处理任务
static TaskHandle_t             task_sms_handle = NULL;             //创建接收短信任务
static TaskHandle_t             task_shell_handle = NULL;  //创建SHELL任务
static TaskHandle_t             task_gnss_handle = NULL;  //创建GNSS处理任务
static TaskHandle_t             task_led_handle = NULL; //创建LED处理任务
static TaskHandle_t             task_gb4_handle = NULL;  //创建处理国四任务
static TaskHandle_t             task_lte_handle = NULL;  //创建LTE模块健康任务
static TaskHandle_t             task_at_parse_handle = NULL;  //创建LTE命令解析AT命令
static TaskHandle_t             task_data_packet_handle = NULL;  //创建组包数据任务
static TaskHandle_t             task_can_recv_handle = NULL;  //创建CAN接收任务
static TaskHandle_t             task_can_send_handle = NULL;  //创建GNSS处理任务


//static RTC_DATA_ATTR uint32_t  main_cnt = 0;

/************** 本地全局变量 ******************/

static uint8_t                  sys_run_status = 0;             //系统运行状态

static QueueHandle_t  			app_main_queue = NULL;          //生产队列


/********************************
**
**********************************/

QueueHandle_t get_app_main_queue(void)
{
	return app_main_queue;
}



/********************************
**  
**********************************/

uint8_t read_sys_run_state(void)
{
    uint8_t rv;

    rv = sys_run_status;

    return rv;
}




/***************************
**	外电断开
**  
*****************************/

uint8_t check_active_state(void)
{
	uint8_t rv = 0;
	
	if(read_in_acc_state() > 0 || read_iap_state() > 0)
	{
		rv = 1;
	}
	
	return rv;
}




/****************************
**  主功能函数
*****************************/

void app_main(void)
{
    uint32_t                    cnt = 0;
    struct app_main_mq_str      amq = {0};


    show_board_info();
    rt_hw_init_board();
    load_products_cfg_info();    //
    show_sys_version();
    rt_sleep_wakeup_cause();

    if(mount_sys_files() == 1)
    {
        printf("-- Mount sys files ok......\r\n");
    }

  
    CHECK_ERROR_CODE(esp_task_wdt_init(TWDT_TIMEOUT_S, true),ESP_OK);
    CHECK_ERROR_CODE(esp_task_wdt_add(NULL),ESP_OK);
    app_main_queue = xQueueCreate(3,sizeof(struct app_main_mq_str));

    xTaskCreate(thread_entry_products,          "thread_entry_products",        4096,       NULL,  4,  &task_products_handle);  //创建生产处理任务
    xTaskCreate(thread_entry_sms,               "thread_entry_sms",             3072,       NULL,  5,  &task_sms_handle);  //创建接收短信任务
    xTaskCreate(thread_entry_shell,             "thread_entry_shell",           3072,       NULL,  6,  &task_shell_handle);  //创建SHELL任务
    xTaskCreate(thread_entry_gnss,              "thread_entry_gnss",            3072,       NULL,  7,  &task_gnss_handle);  //创建GNSS处理任务
    xTaskCreate(thread_entry_led,               "thread_entry_led",             2048,       NULL,  8,  &task_led_handle);  //创建LED处理任务
    xTaskCreate(thread_entry_gb4,               "thread_entry_gb4",             4096,       NULL,  9,  &task_gb4_handle);  //创建处理国四任务
    xTaskCreate(thread_entry_lte,               "thread_entry_lte",             3072,       NULL,  10,  &task_lte_handle);  //创建LTE模块健康任务
    xTaskCreate(thread_entry_at_parse,          "thread_entry_at_parse",        3072,       NULL,  11,  &task_at_parse_handle);  //创建LTE命令解析AT命令
    xTaskCreate(thread_entry_data_packet,       "thread_entry_data_packet",     3072,       NULL,  12,  &task_data_packet_handle);  //创建组包数据任务
    xTaskCreate(thread_entry_can_recv,          "thread_entry_can_recv",        2048,       NULL,  13,  &task_can_recv_handle);  //创建CAN接收任务
    xTaskCreate(thread_entry_can_send,          "thread_entry_can_send",        3072,       NULL,  14,  &task_can_send_handle);  //创建GNSS处理任务

    vTaskDelay(100);
    
    print_rtc_info();   //打印时间

    for(;;)
    {
        esp_task_wdt_reset();
        //printf("-- The main reset....\r\n");
        process_in();
        if(xQueueReceive(app_main_queue,&amq,100) == pdTRUE)
	   	{    
            if(amq.state == 1)
            {
                sys_run_status = 3;
                cnt = 0;
            }
            if(cnt++ > read_config_delay_shutdown_time() - 15)
            {
                sys_run_status = 1;
                cnt = 0;
            }
            memset((uint8_t *)&amq,0,sizeof(struct app_main_mq_str));
        }

        switch(sys_run_status)
        {
            case 0:                 //正常运行状态
                if(check_active_state() > 0)
                {
                    cnt = 0;
                    break;
                }
                //printf("-- the main run...%d\r\n",cnt);
                if(cnt++ > read_config_delay_shutdown_time() - 15)
                {
                    sys_run_status = 1;
                    cnt = 0;
                }
                 
                if(read_in_batter_vol() <= 36 &&  read_in_power_vol() < 80)
                {
                    cnt = 10;
                    sys_run_status = 2;
                }

                break;
            case 1:                      //进入休眠
                if(cnt++ >= 10)
                {
                    sys_run_status = 2;
                    cnt = 0;
                    rt_rs232_485_power_off();
                    rt_rx_rs232_switch_off();
                    rt_rx_rs485_switch_off();
                    rt_led_off(LED_ALL);
                    unmount_sys_file();       //卸载文件系统
                    //删除所有的任务
                    if(task_products_handle != NULL)        //创建生产处理任务
                    {
                        vTaskDelete(task_products_handle);
                        task_products_handle = NULL;
                    }    
                    
                    if(task_sms_handle != NULL)             //创建接收短信任务
                    {
                        vTaskDelete(task_sms_handle);
                        task_sms_handle = NULL;
                    }    
                    
                    if(task_shell_handle != NULL)  //创建SHELL任务
                    {
                        vTaskDelete(task_shell_handle);
                        task_shell_handle = NULL;
                    }    
                    
                    if(task_gnss_handle != NULL)  //创建GNSS处理任务
                    {
                        vTaskDelete(task_gnss_handle);
                        task_gnss_handle = NULL;
                    }    
                    
                    if(task_led_handle != NULL) //创建LED处理任务
                    {
                        vTaskDelete(task_led_handle);
                        task_led_handle = NULL;
                    }   
                    
                    if(task_gb4_handle != NULL)  //创建处理国四任务
                    {
                        vTaskDelete(task_gb4_handle);
                        task_gb4_handle = NULL;
                    }    
                    
                    if(task_lte_handle != NULL)  //创建LTE模块健康任务
                    {
                        vTaskDelete(task_lte_handle);
                        task_lte_handle = NULL;
                    }    
                    
                    if(task_at_parse_handle != NULL)  //创建LTE命令解析AT命令
                    {
                        vTaskDelete(task_at_parse_handle);
                        task_at_parse_handle = NULL;
                    }   
                    
                    if(task_data_packet_handle != NULL)  //创建组包数据任务
                    {
                        vTaskDelete(task_data_packet_handle);
                        task_data_packet_handle = NULL;
                    }   
                    
                    if(task_can_recv_handle != NULL)  //创建CAN接收任务
                    {
                        vTaskDelete(task_can_recv_handle);
                        task_can_recv_handle = NULL;
                    }    
                    
                    if(task_can_send_handle != NULL)  //创建GNSS处理任务
                    {
                        vTaskDelete(task_can_send_handle);
                        task_can_send_handle = NULL;
                    }   
                }
                
                break;
            case 2:
                if(cnt++ > 5)
                {
                   if(read_in_power_vol() > 80)
                    {
                        rt_lte_power_off();
                        rt_gnss_power_off();
                        rt_can_standby();
                        //vTaskDelay(10);
                        get_aw9523b_gpio_status();
                        enter_deep_sleep_mode(read_config_sleep_cycle());
                    }
                    
                    sys_run_status = 4;
                }
                
                break; 
            case 3:
                printf("-- reboot .... %d\r\n",cnt);
               if(cnt++ >= 10)
               {
                    rt_lte_power_off();
                    rt_gnss_power_off();
                    rt_can_standby();
                    vTaskDelay(100);
                    //get_aw9523b_gpio_status();
                    unmount_sys_file();       //卸载文件系统
                    vTaskDelay(100);             //
                    rt_reboot_sys();     //发出重启信号
               }
               break;
            case 4:
                if(read_in_acc_state() > 0 || read_in_power_vol() > 80)
                {
                    rt_reboot_sys();
                }    
                break;
            default:
                sys_run_status = 0;
                break;  
        }
    }
}
