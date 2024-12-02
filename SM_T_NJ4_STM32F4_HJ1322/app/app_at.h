



#ifndef _APP_AT_H
#define _APP_AT_H


#include <stdint.h>


#include "semphr.h"


#include "drv_rtc.h"





struct at_mutex_str
{
	SemaphoreHandle_t    cmd_mutex;
	SemaphoreHandle_t    send_sem;

};

typedef enum
{
    NOT_RETRY = 0,
    RETRY
}retry_flag;




struct at_res_str
{
    int8_t      res;//结果
		char        data[50];//数据
    uint8_t     len;
};//AT指令返回结果 








struct socket_addr_str
{
	uint16_t port;
	uint8_t addr[50];
};



struct ftp_info_str
{
	uint8_t host[50];
	uint16_t port;
	uint8_t user[50];
	uint8_t passwd[50];
};




struct socket_down_str
{
	uint16_t len;
	uint8_t data[512];
};




struct socket_state_t
{
    uint32_t        connect_id;
    uint8_t         service_type[10];
    uint8_t         ip_addr[50];
    uint32_t        remote_port;
    uint32_t        local_prot;
    uint32_t        socket_state;
    uint32_t        context_id;
    uint32_t        server_id;
    uint32_t        acssess_mode;
    uint8_t         at_port[10];
};


struct at_info
{
  SemaphoreHandle_t 	semaphore; //信号量
  uint32_t 						ticks;         //接收数据ticks
	uint16_t 							len;
};


int8_t  at_wait_cmd_ok_syn(void);
uint8_t at_get_imei_syn(uint8_t *source,uint8_t len);
uint8_t at_ctrl_echo_syn(void);
uint8_t at_set_cgreg_syn(void);
uint8_t at_get_sim_syn(uint8_t *data);
uint8_t at_get_ccid_syn(uint8_t *source,uint8_t size);
uint8_t at_get_cgreg_syn(uint8_t *data);
uint8_t at_get_cgatt_syn(uint8_t *data);
uint8_t at_set_apn_syn(void);
uint8_t at_get_lte_cclk(struct rt_tm *tt);
uint8_t at_activate_pdp(void);
uint8_t at_get_local_ip_syn(uint8_t *ip);
uint8_t at_get_csq_syn(uint8_t *data);
uint8_t at_send_socket_data(uint8_t id,uint8_t *data,uint16_t len);
uint8_t at_creat_socket_connect(uint8_t id,struct socket_addr_str *sa,QueueHandle_t qh);
uint8_t at_close_socket_connect(uint8_t id);

uint8_t at_config_ftp_account(struct ftp_info_str *sa);
uint8_t at_creat_ftp_connect(struct ftp_info_str *sa);
uint8_t at_list_ftp_files(uint8_t *fp);
uint8_t at_close_ftp_connect(void);
uint32_t at_get_ftp_files_size(uint8_t *fp);
uint32_t at_download_ftp_files(uint8_t *fp,uint8_t *buf,uint32_t offset,uint16_t len);
uint8_t at_config_ftp_transmode(uint8_t n);
uint8_t at_config_ftp_rsptimeout(uint8_t n);
uint8_t at_config_ftp_file_type(uint8_t n);
uint8_t at_query_socket_state(uint8_t id);
uint8_t at_config_sms_event(void);
uint8_t at_config_sms_port(void);
uint8_t at_query_urc_port_state(void);
uint8_t at_config_sms_fromat(void);
uint8_t at_ctrl_at_syn(void);
uint16_t get_lte_send_num(void);





void thread_entry_at(void *parameter);


#endif



