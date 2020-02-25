




#ifndef _CONFIG_H
#define _CONFIG_H


unsigned char read_config_state(void);
void send_config_info(void);
unsigned char read_terminal_id(unsigned char *buf,unsigned char buf_size);
unsigned char read_car_type(void);
unsigned short int build_config_info(char *buf,unsigned short int buf_size,unsigned char flag);
unsigned char analysis_config_info(unsigned char *source,unsigned short len);
unsigned char read_config_info(void);
unsigned char save_config_info(void);
unsigned int read_config_travel_upload_cycle(void);
unsigned int read_config_work_upload_cycle(void);
void read_config_apn(unsigned char *buf,unsigned char buf_size);
void read_config_dev_id(unsigned char *buf,unsigned char buf_size);
void read_config_dev_secret(unsigned char *buf,unsigned char buf_size);
unsigned char read_gateway_addr1(unsigned char *buf,unsigned char buf_size);
unsigned short int read_gateway_port1(void);
unsigned short int read_config_info_run_time(void);
unsigned char read_config_info_hard_ware(void);
unsigned short int read_config_info_sleep_time(void);
#endif




