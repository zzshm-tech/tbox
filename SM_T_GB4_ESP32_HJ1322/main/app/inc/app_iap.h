


#ifndef _APP_IAP_H
#define _APP_IAP_H



struct iap_ftp_str
{
    uint8_t 							cmd;                        //参数类型
	uint16_t 							len;						//
	uint8_t							    files[50];			    //文件名称
	uint8_t         			        host[50];				//FTP地址
	uint16_t 						    port;					//FTP端口
	uint8_t							    user[30];				//FTP用户名
	uint8_t 							passwd[30];                	//FTP密码
};



void thread_entry_iap(void *parameter);

uint8_t read_iap_state(void);

#endif

