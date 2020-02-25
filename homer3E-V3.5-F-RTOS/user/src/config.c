
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "FreeRTOS.h"

#include "data_type.h"
#include "gsm.h"
#include "pro_data.h"
#include "gnss.h"
#include "version.h"
#include "mon.h"
#include "fr_drv_mem.h"
#include "can.h"



static struct config_str	config_info;    //设备配置信息



/***************************************
**	配置信息保存在外部EEPROM内部
****************************************/

unsigned char read_config_info(void)
{
	unsigned char rv;
	
	rv = fr_fram_read(0,(unsigned char *)&config_info, sizeof(config_info));
	
	if(config_info.flag != 0x55)
	{
		config_info.flag = 0x56;
		
		memset(config_info.terminal_id,'\0',sizeof(config_info.terminal_id));          //设备号
		memcpy(config_info.terminal_id,"YMJ41219606100001",sizeof("YMJ41219606100001"));          //调试使用
		config_info.terminal_type = 0;		 							//设备类型
		config_info.fir_ver = 12;			 									//固件版本号
		memcpy(config_info.gateway_addr1,"hdyx-2013.xicp.net",sizeof("hdyx-2013.xicp.net"));      					//网关地址
		config_info.gateway_port1 = 13011;     	 						//网关端口 
		config_info.socket1 = 1;
		
		
		
		config_info.run_time = 300;            	 					//断开ACC后运行时间(单位：秒，例如：300S，5分钟)
		config_info.sleep_time = 21600;          					//睡眠时间(单位：秒)
		config_info.travel_upload_cycle = 60;    				//行驶上传周期(单位：秒)
		config_info.work_upload_cycle = 1;      				//作业上传周期(单位：秒)
		config_info.distance_upload = 200;     	 					//定距离报位 (单位：米)
		config_info.azimuth_upload = 30;       					//航向角报位(单位：度)
		config_info.car_type = 3;             					//安装车辆类型
		config_info.user_code = 3;            					//用户编码
		config_info.can_num = 22;												//CAN协议号 					
		memcpy(config_info.apn,"CMNET",sizeof("CMNET"));    //APN
		memcpy(config_info.user,"",sizeof(""));												//GSM模块APN用户名
		memcpy(config_info.password,"",sizeof(""));										//GSM模块APN密码
		memset(config_info.icc_id,'\0',sizeof(config_info.icc_id));											//SIM卡ICC-ID号
		config_info.hard_ware = 35;									//硬件版本
		memset(config_info.dev_id,0,8);
		memset(config_info.dev_secret,0,8);
	}
	config_info.gateway_port1 = 27000;
	config_info.travel_upload_cycle = 20;
//	config_info.run_time = 10;
//	memcpy(config_info.gateway_addr1,"123.127.244.154",sizeof("123.127.244.154"));      					//网关地址
//	config_info.gateway_port1 = 13012;     	 						//网关端口 
	
	return rv;
}


/***************************************
**	配置信息保存在外部EEPROM内部
****************************************/
unsigned char read_config_info_hard_ware(void)
{
	return config_info.hard_ware;
}



/***************************************
**	配置信息保存在外部EEPROM内部
****************************************/
unsigned short int read_config_info_run_time(void)
{
	return config_info.run_time;
}


unsigned short int read_config_info_sleep_time(void)
{
	return config_info.sleep_time;
}

/***************************************
**	配置信息保存在外部EEPROM内部
****************************************/

unsigned char save_config_info(void)
{
	unsigned char rv;
	
	config_info.flag = 0x55;
	
	rv = fr_fram_write(0,(unsigned char *)&config_info, sizeof(config_info));
	
	return rv;
}

/***************************************
**	返回设备号
****************************************/

unsigned char read_terminal_id(unsigned char *buf,unsigned char buf_size)
{
	if(buf_size < 16)
		return 0;
	
	memcpy(buf,config_info.terminal_id,16);
	
	return 16;
}

unsigned char read_gateway_addr1(unsigned char *buf,unsigned char buf_size)
{
	if(buf_size < 50)
		return 0;
	
	memcpy(buf,config_info.gateway_addr1,50);
	
	return 50;
}

unsigned short int read_gateway_port1(void)
{
	return config_info.gateway_port1;
}


/************************
**	读取车辆类型
*******************/

unsigned char read_car_type(void)
{
	return config_info.car_type;
}


/*****************************
**	返回配置状态
******************************/

unsigned char read_config_state(void)
{
	return config_info.flag;
}


/*****************************
**	返回配置-上传时间周期
******************************/

unsigned int read_config_travel_upload_cycle(void)
{
	return config_info.travel_upload_cycle;
}

/**********************
**	返回
***********************/

unsigned int read_config_work_upload_cycle(void)
{
	return config_info.work_upload_cycle;
}


/**********************
**	返回
***********************/

void read_config_apn(unsigned char *buf,unsigned char buf_size)
{
	if(buf_size < 20)
		return;
	memcpy(buf,config_info.apn,20);
}


void read_config_dev_id(unsigned char *buf,unsigned char buf_size)
{
	if(buf_size < 3)
		return;
	memcpy(buf,config_info.dev_id,3);
}


void read_config_dev_secret(unsigned char *buf,unsigned char buf_size)
{
	if(buf_size < 3)
		return;
	memcpy(buf,config_info.dev_secret,3);
}


/**************************************************
**	函数名称:
***************************************************/

unsigned char analysis_config_info(unsigned char *source,unsigned short len)
{
	unsigned char *p = source;
	unsigned char i;
	unsigned char 			tmp_s[40];
	
	if(*(p) != ':')
		return 0;
	
	p += 1;
	memset(config_info.terminal_id,'\0',sizeof(config_info.terminal_id));    //解析设备号
	for(i = 0;i < 16;i++)                                  			//
		config_info.terminal_id[i] = *(p + i);
	
	p += i;
	if(*p != ',')
		return 0;
				
	i = get_data_str(1,2,p,tmp_s,len);    //网关地址
	if(i > 0)
	{
		tmp_s[i] = '\0';
		memcpy(config_info.gateway_addr1,tmp_s,i + 1);          	//				
	}
				
	i = get_data_str(2,3,p,tmp_s,len);              //端口号
	if(i > 0)
	{	
		tmp_s[i] = '\0';
		config_info.gateway_port1 = fr_atof((const char *)tmp_s);         			//
	}
				
	i = get_data_str(3,4,p,tmp_s,len);  
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.sleep_time = fr_atof((const char *)tmp_s);         			//休眠时间
	}
				
	i = get_data_str(4,5,p,tmp_s,len);  
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.distance_upload = fr_atof((const char *)tmp_s);   			//定距上传
	}


	i = get_data_str(5,6,p,tmp_s,len);  
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.travel_upload_cycle = fr_atof((const char *)tmp_s);  			//
	}

	i = get_data_str(6,7,p,tmp_s,len);  
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.hard_ware = fr_atof((const char *)tmp_s);      			//
	}
				
	i = get_data_str(7,8,p,tmp_s,len);  
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.can_num = fr_atof((const char *)tmp_s);   			//解析CAN协议号
					
		if(config_info.can_num == 0x40 )                       //
		{
			config_info.car_type = 0x0A; 
		}
		else
		{
			config_info.can_num = 0x40;                       	//
			config_info.car_type = 0x0A;												//
		}
	}
	i = get_data_str(8,9,p,tmp_s,len);  
	if(i > 0)
	{
		memcpy(config_info.apn,tmp_s,i);           			//设备APN
		config_info.apn[i] = '\0';
	}
	
	i = get_data_str(9,10,p,tmp_s,len);              
	if(i > 0)
	{
		if(tmp_s[0] == '0')
		{
			
		}
		else if(tmp_s[0] == '1')
		{
			
		}
	}
	
	for(i = 0;i < 3;i++)
	{
		len = 0;
		len = tmp_s[i * 2 + 14] - 0x30;
		len <<= 4;
		len += (tmp_s[i * 2 + 15] - 0x30);
		config_info.dev_secret[i] = len;
	}
	
	for(i = 0;i < 3;i++)
	{
		len = 0;
		len = config_info.terminal_id[i * 2 + 10] - 0x30;
		len <<= 4;
		len += (config_info.terminal_id[i * 2 + 11] - 0x30);
		config_info.dev_id[i] = len;
	}
	
	read_icc_id(config_info.icc_id,20);
	
	return 1;
}



/**************************************************
**	函数名称:
**	功能描述:发送配置信息
***************************************************/
unsigned short int build_config_info(char *buf,unsigned short int buf_size,unsigned char flag)
{
	unsigned short int 			len;
	unsigned int 						tmp;
	int 										i;
	unsigned char 					tmp_c[20];
	struct time_str 				*ptime;
	
	if(buf_size < 200)
		return 0;
	
	if(flag == 0)
		config_info.flag = 0x56;
	
	len = 0; 
	memcpy((char *)buf,(char *)"homer3x:",sizeof("homer3x:") - 1);
	len += sizeof("homer3x:") - 1;				
	for(i = 0;i < 16;i++)
		*(buf + len + i) = config_info.terminal_id[i];   			//
	len += i;
	*(buf + len) = ',';                               		//
	len++;
	
	if(flag == 0)
		read_icc_id((unsigned char *)buf + len,20);
	else
		memcpy(buf + len,config_info.icc_id,20);
	
	len += 20;
	buf[len++] = ',';                                		//
	
	
	tmp = strlen((const char *)config_info.gateway_addr1);
	memcpy(buf + len,config_info.gateway_addr1,tmp);     //
	len += tmp;
	buf[len++] = ',';                                //

	tmp = int_to_str(config_info.gateway_port1,(char *)tmp_c,sizeof(tmp_c));
	for(i = 0;i < tmp;i++)            //网关端口号
		buf[len++] = tmp_c[i];
	buf[len++] = ',';

	tmp = strlen((const char *)config_info.apn);
	memcpy(buf + len,config_info.apn,tmp);        // GSM连接 
	len += tmp;
	buf[len++] = ',';                             //
					
	
	if(read_gnss_state() == 'A')                    //定位状态
	{
		buf[len] = '8';
		buf[len + 1] = '0';
	}
	else
	{
		buf[len] = '0';
		buf[len + 1] = '0';
	}
	len += 2;
					
	tmp = read_gnss_latitude() * 100000;                   //维度(经纬度)
	
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));
	memset(buf + len,'0',9);
	memcpy(buf + len,tmp_c,tmp);              								//
	len += 9;
	tmp = read_gnss_longitude() * 10000;                  //经度
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));                       
	memset(buf + len,'0',9);
	memcpy(buf + len,tmp_c,tmp);              //
	len += 9;

	tmp = sprintf((char *)&buf[len],(char *)"%04d%04d",read_gnss_speed(),read_gnss_heading());
	
	len += 8;

	ptime = (struct time_str *)tmp_c;
	read_gnss_utc_time(ptime);
	
	tmp = sprintf((char *)&buf[len],(char *)"%02d%02d%02d%02d%02d%02d",ptime->year,ptime->mon,ptime->day,ptime->hour,ptime->min,ptime->sec);
	len += tmp;
	buf[len++] = ',';
	
	tmp = read_satellite_num();			
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));
	memset(buf + len,'0',9);
	memcpy(buf + len,tmp_c,tmp);
	len += 2;
	buf[len++] = ',';
	
	tmp = read_gsm_signal();                              //信号值
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));
	memset(buf + len,'0',9);
	memcpy(buf + len,tmp_c,tmp);
	len += 2;
	buf[len++] = ',';
					
	tmp = read_power_vol();    //外部供电电压
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));          //
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';
					
	tmp = read_batter_vol();
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));          //电池电压
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';
					
	buf[len++] = read_acc_state() + 0x30;   				// ACC状态
	buf[len++] = ',';
						
	buf[len++] = read_net_state() + 0x30;   //网络连接状态 0：未注册网络 1：已经附着网络	2：已经连接到服务器
	buf[len++] = ',';
					
	tmp = read_fir_ver();
	buf[len++] = tmp / 10 + 0x30;
	buf[len++] = '.';
	buf[len++] = tmp % 10 + 0x30;
					
	buf[len++] = ',';

	
	tmp = int_to_str(config_info.sleep_time,(char *)tmp_c,sizeof(tmp_c));      		//
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';

	
	tmp = int_to_str(config_info.distance_upload,(char *)tmp_c,sizeof(tmp_c));      		//   //
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';

	tmp = int_to_str(config_info.travel_upload_cycle,(char *)tmp_c,sizeof(tmp_c));  
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';

	buf[len++] = config_info.hard_ware / 10 + 0x30;      //
	buf[len++] = '.';
	buf[len++] = config_info.hard_ware % 10 + 0x30;
	buf[len++] = ',';
					
	buf[len++] = '1';	   							//
	buf[len++] = ',';

	buf[len++] = '1';									//
	buf[len++] = ',';

	buf[len++] = '1';								//
	buf[len++] = ',';
					
	buf[len++] = read_ant_state() + 0x30;    //天线状态
	buf[len++] = ',';
			
	buf[len++] = read_shell_state() + 0x30; 	//外壳状态
	buf[len++] = ',';

	buf[len++] = '1';                                //
	buf[len++] = ',';

	buf[len++] = '1';								//
	buf[len++] = ',';

	
	tmp = sizeof("0102") - 1;                            	//
	
	memcpy(buf + len,"0102",sizeof("0102") - 1);
	len += tmp;
	tmp = read_fir_ver();

	buf[len++] = tmp / 10 + 0x30;     
	buf[len++] = tmp % 10 + 0x30;
	buf[len++] = tmp / 10 + 0x30;   //
	buf[len++] = tmp % 10 + 0x30;   //
	buf[len++] = ',';
	
	tmp = int_to_str(config_info.can_num,(char *)tmp_c,sizeof(tmp_c));      //CAN协议号
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';

	//
					
	if(read_can_connect_state() == 0)						  //CAN连接状态
		buf[len++] = '0';                              //
	else
		buf[len++] = '1';                              //
					
	buf[len++] = ',';
	
	tmp = read_fir_ver();
	buf[len++] = tmp / 10 + 0x30; ;   //单片机版本号
	buf[len++] = '.';
	buf[len++] = tmp % 10 + 0x30; ;   //
	buf[len++] = ',';
	
	buf[len++] = read_mon_expect_state() + 0x30;   //激活期望状态
	buf[len++] = ',';
	
	buf[len++] = read_lock_expect_state() + 0x30;   //锁车期望状态
	buf[len++] = ',';
	
	buf[len++] = 0x0d;
	buf[len++] = 0x0a;   //回车换行符
	
	return len;
}






