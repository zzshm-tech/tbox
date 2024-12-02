





#ifndef _APP_IAP_H
#define _APP_IAP_H

#include <rtthread.h>
#include <rtdevice.h>


#pragma pack(1)



struct iap_ftp_str
{
	uint8_t							  files[50];			    //文件名称
	uint8_t         			host[50];				//FTP地址
	uint16_t 						  port;					//FTP端口
	uint8_t							  user[30];				//FTP用户名
	uint8_t 							passwd[30];                	//FTP密码
	uint8_t 							cmd;                        //参数类型
};

#pragma pack()



rt_mq_t get_iap_mq(void);
uint8_t erase_back_app_setcors(void);
uint32_t read_vector_table_flag(void);
void thread_entry_iap(void *parameter);
void thread_entry_boot_iap(void *parameter);
uint8_t read_iap_state(void);



#endif




