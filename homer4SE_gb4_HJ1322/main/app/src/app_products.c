


#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_partition.h"


#include "pro_data.h"
#include "common.h"
#include "version.h"

#include "board.h"
#include "drv_rtc.h"
#include "drv_uart.h"
#include "drv_in.h"



#include "app_products.h"
#include "app_lte.h"
#include "app_gnss.h"
#include "app_in.h"
#include "app_files.h"
#include "app_gb4.h"
#include "app_can_recv.h"
#include "app_can_send.h"
#include "app_lte.h"
#include "app_main.h"
#include "app_archive.h"

#include "mbedtls/aes.h"



/*************************************************/





/************** 本地全局变量 ******************/

static QueueHandle_t  			products_queue = NULL;   //生产队列

static struct config_str        config_info = {0};    //？配置信息读写要增加  信号保护？？？？？

static uint8_t 					products_port = 0;    //默认的生产端口CAN  0  1：通过SHELL口

/*************************************************
**
************************************************/

QueueHandle_t get_products_queue(void)
{
	return products_queue;
}

	



/*****************************************
**	返回网关地址
******************************************/

uint8_t read_enterprise_gw_addr(uint8_t *buf,uint8_t buf_size)
{
	if(buf_size < 50)
		return 0;
	
	memcpy(buf,config_info.enterprise_gw_addr,50);
	
	return 50;
}


/************************************
**	返回网关地址
*************************************/

uint16_t read_enterprise_gw_port(void)
{
	uint16_t rv;
	
	rv = config_info.enterprise_gw_port;

	return rv;
}




/************************************
**	返回网关地址
*************************************/

uint8_t read_config_archivel_gw_addr(uint8_t *buf,uint8_t buf_size)
{
	if(buf_size < 50)
		return 0;
	
	memcpy(buf,config_info.archivel_gw_addr,50);
	
	return 50;
}


/************************************
**	返回网关地址
*************************************/

uint16_t read_config_archivel_gw_port(void)
{
	uint16_t rv;
	
	rv = config_info.archivel_gw_port;

	return rv;
}




/************************************
**	返回网关地址
*************************************/

uint8_t read_config_public_key(uint8_t *buf,uint8_t buf_size)
{
	if(buf_size < 64)
		return 0;
	if(config_info.public_key[0] == 0x5A)
		memcpy(buf,&config_info.public_key[1],64);
	else
		memset(buf,'\0',64);
	
	return 64;
}


/**********************************
**  返回生产模式
**  0x56：生产模式
**  0x55：正常模式
***********************************/

uint8_t get_products_cfg_model(void)
{
    uint8_t rv;

    rv = config_info.product_mode;

    return rv;
}


/**********************************
** 	返回农机补贴状态
**	
***********************************/

uint8_t read_config_nj_state(void)
{
	uint8_t rv;

	rv = config_info.nj_state;
	//rv = 2;
	return rv;
}





/*****************************
**	返回配置-上传时间周期
******************************/

uint32_t  read_config_qb4_upload_cycle(void)
{
	uint32_t rv;
	
	rv = config_info.travel_upload_cycle;
	//rv = 60;
	return rv;
}



/*****************************
**	返回配置-上传时间周期
******************************/

uint32_t read_config_gb4_upload_cycle(void)
{
	uint32_t rv;
	
	rv = config_info.gb_four_upload_cycle;
	//rv = 20;
	return rv;
}




/*******************************
**
********************************/

uint16_t read_config_farm_manu(void)
{
	uint16_t rv;

	rv = config_info.farm_manu;
	//rv = 276;
	return rv;
}


/*******************************
**	返回token
********************************/

uint8_t read_config_token(uint8_t *data,uint8_t size)
{
	if(data == NULL || size < 32)
		return 0;
	memcpy(data,config_info.token,32);

	return  32;
}



/*******************************
**	返回用户编码
********************************/

uint8_t read_config_user_code(void)
{
	uint8_t rv ;

	rv = config_info.user_code;

	return rv;
}



/*******************************
**	返回用户编码
********************************/

uint8_t read_config_archival_state(void)
{
	uint8_t rv ;

	rv = config_info.archival_state;

	return rv;
}




/*******************************
**	返回硬件版本号
********************************/

uint8_t read_config_hard_ware(void)
{
	uint8_t rv = 0;

	rv = config_info.hard_ware;

	return rv;
}

/*******************************
**	返回用户编码
********************************/

uint8_t read_config_eco_mark(uint8_t *buf,uint8_t size)
{
	if(size < 4 || buf == NULL)
		return 0;
	memcpy(buf,config_info.eco_mark,4);

	return 4;
}


/**********************************
**	保存配置信息
***********************************/

uint8_t save_products_cfg_info(void)
{
	const esp_partition_t *partition = NULL;
    uint8_t read_data[1024] = {0};

    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY,"product");
   
    if(partition == NULL)
    {
        printf("-- esp partition find first fail...\r\n");
        return 1;
    }
	printf("-- The Products Area:%d\r\n",partition->size);
	esp_partition_read(partition, 0, read_data,sizeof(struct config_str));
	
	config_info.product_mode = 0x55;
                                          //计算CRC32值
	if (ESP_OK != esp_partition_erase_range(partition, 0, partition->size))
    {
        printf("-- erase product faile \r\n");
        return 1;
    }

    if (ESP_OK != esp_partition_write(partition, 0, (uint8_t *)&config_info,sizeof(struct config_str)))
    {
        printf("-- write product faile \r\n");
        if(ESP_OK != esp_partition_write(partition, 0, read_data, sizeof(struct config_str)))
        {
            return 1;
        }
    }

	
	return 0;
}





/****************************************
**  加载生产配置信息
*****************************************/

uint8_t load_products_cfg_info(void)
{
    const esp_partition_t *partition = NULL;
    
    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY,"product");
   
    if(partition == NULL)
    {
        printf("-- esp partition find first fail...\r\n");
        return 1;
    }
    // Read back the data, checking that read data and written data match
    if(ESP_OK == esp_partition_read(partition, 0,(uint8_t *)&config_info,sizeof(struct config_str)))
	{
		if(config_info.product_mode != 0x55)
		{
			memcpy(config_info.terminal_id,"0000000000000000",sizeof("0000000000000000"));
			memcpy(config_info.enterprise_gw_addr,(char *)"gateway.bcnyyun.com",sizeof("gateway.bcnyyun.com"));              //调试使用
			config_info.enterprise_gw_port = 19003;     	 				//网关端口 
		
			memcpy(config_info.archivel_gw_addr,"fdlpfjk.vecc.org.cn",sizeof("fdlpfjk.vecc.org.cn")); 
			config_info.archivel_gw_port = 61003;
		
			config_info.terminal_type = 0;		 							//设备类型
			config_info.fir_ver = 10;			 							//固件版本号
			config_info.run_time = 300;            	 						//断开ACC后运行时间(单位：秒，例如：300S，5分钟)
			config_info.sleep_time = 21600;          						//睡眠时间(单位：秒)
			config_info.travel_upload_cycle = 60;    						//行驶上传周期(单位：秒)
			config_info.gb_four_upload_cycle = 300;         				//国标数据上传周期
			config_info.work_upload_cycle = 1;      						//作业上传周期(单位：秒)
			config_info.distance_upload = 200;     	 						//定距离报位 (单位：米)
			config_info.azimuth_upload = 300;       						//航向角报位(单位：度)
			config_info.car_type = 0;             						//安装车辆类型
			config_info.user_code = 72;            							//用户编码
			config_info.delay_shutdown_time = 300;     						//唤醒后的工作时间
			config_info.nj_state = 0;						 				//农业3和1补贴状态
			config_info.farm_manu = 0; 										//农机补贴 厂商编码
			config_info.waring_time_ms = 50; 			 					//报警上传间隔 单位ms
			config_info.heartbeat_time_sec = 30; 	 						//心跳上传间隔 单位秒
			config_info.blind_time_sec = 10; 	 							//盲区上传间隔 单位秒
			config_info.can_num = 0x02;										//CAN协议号 					
			memcpy(config_info.apn,"CMNET",sizeof("CMNET"));    			//APN
			memset(config_info.user,'\0',sizeof(config_info.user));							//GSM模块APN用户名
			memset(config_info.password,'\0',sizeof(config_info.password));						//GSM模块APN密码						
			config_info.hard_ware = 0;										//硬件版本
			memset(config_info.dev_id,0,3);									//潍柴锁车使用
			memset(config_info.dev_secret,0,3);								//潍柴锁车使用
			config_info.archival_state = 0;									//备案状态（国四激活状态）
			config_info.product_mode = 0x56;
		}
	}
	else
	{
		printf("-- load config info error....\r\n");
	}

	config_info.user_code = 72;  //山东肯石客户编号
	config_info.enterprise_gw_port = 19003;


	memcpy(config_info.enterprise_gw_addr,(char *)"124.222.139.175",sizeof("124.222.139.175"));              //调试使用
	config_info.enterprise_gw_port = 19003;     	 				//网关端口 

	memcpy(config_info.archivel_gw_addr,"fdlpfjk.vecc.org.cn",sizeof("fdlpfjk.vecc.org.cn")); 
	config_info.archivel_gw_port = 61003;
	
	printf("-- the vin:");
	for(int i = 0;i < 17;i++)
		printf("%c",config_info.vin[i+ 1]);
	printf("\r\n-- the archival state:%d\r\n",config_info.archival_state);

	printf("-- the dev id:%s\r\n",config_info.terminal_id);
	
    return 0;
}



/****************************************
**	擦除配置信息
*****************************************/

uint8_t erase_products_cfg_info(void)
{
	const esp_partition_t *partition = NULL;
    
    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY,"product");
   
    if(partition == NULL)
    {
        printf("-- esp partition find first fail...\r\n");
        return 1;
    }

	if(ESP_OK != esp_partition_erase_range(partition, 0, partition->size))
    {
        printf("-- erase product faile \r\n");
        return 1;
    }
	
	vTaskDelay(10);
	load_products_cfg_info();
	
	return 0;
}


/***********************************
**	修改配置信息
************************************/

static uint8_t modification_config_info(uint8_t *data,uint16_t len)
{
	uint8_t index = 0;
	uint16_t tmp;

	if(len == 0 || data == NULL ||  read_in_acc_state() == 0)
		return 1;
	len -= 1;

	index = *data;
	//printf("-- modification_config_info:%d\r\n",index);

	switch(index)
	{
		case 5:                  //修改企标网关地址
			memset(config_info.enterprise_gw_addr,'\0',sizeof(config_info.enterprise_gw_addr));
			memcpy(config_info.enterprise_gw_addr,data + 1,50);    //50
			config_info.enterprise_gw_port = *(uint16_t *)(data + 50);

			printf("-- Products enterprise_gw_addr:%s:%d\r\n",config_info.enterprise_gw_addr,config_info.enterprise_gw_port);

			save_products_cfg_info();
			vTaskDelay(1000);
			rt_reboot_sys();           //暂时这样使用
			break; 
		case 29:            //修改VIN  (需要增加其他稳妥方法)
			if(str_compare(&config_info.vin[1],data + 1,17) == 1)   //比较机械环保代码
				break;
			memset(config_info.vin,'\0',sizeof(config_info.vin));
			config_info.vin[0] = 0x5A;
			memcpy(&config_info.vin[1],data + 1,17);
			printf("-- modification vin...%s\r\n",&config_info.vin[0]);
			save_products_cfg_info();
			xTaskCreate(thread_entry_archivel,          "thread_entry_archivel",        4096,       NULL,  15,  NULL);  //创建生产处理任务
			break;
		case 35:    			//设置Token
			memset(config_info.token,'\0',32);
			memcpy(config_info.token,data + 1,32);    //50
			printf("-- token :::::: \r\n");
			mem_printf(LOG_ERROR,PRINT_HEX,config_info.token,32);
			save_products_cfg_info();
			break;
		case 9:					//设置企标上传周期
			tmp = *(uint16_t *)(data + 1);
			if(tmp > 600)
				tmp = 10;
			config_info.travel_upload_cycle = tmp;
			printf("-- Products QB4 upload cycle:%d\r\n",config_info.travel_upload_cycle);
			save_products_cfg_info();
			break;
		case 34:            	//设置国标相关数据
			tmp = *(uint16_t *)(data + 1);
			if(tmp > 600)
				tmp = 10;
			config_info.gb_four_upload_cycle = tmp;
			printf("-- Products gb 4 upload cycle:%d\r\n",config_info.gb_four_upload_cycle);
			save_products_cfg_info();
			break;
		case 36:
			tmp = *(data + 1);
			if(tmp == 1 || tmp == 2)
			{
				 QueueHandle_t  	            qu_t = NULL;          //生产队列
       			struct app_main_mq_str      a_mq = {0};

				config_info.nj_state = tmp;
				printf("-- Products nj_state:%d\r\n",config_info.nj_state);
				save_products_cfg_info();
				printf("-- Reset be from shell cmd ...\r\n");
        		a_mq.state = 1;

        		qu_t =  get_app_main_queue();
        		if(qu_t != NULL)
        		{
            		xQueueSend(qu_t,&a_mq,sizeof(struct app_main_mq_str));
        		}
				vTaskDelay(100);
    
			}
			break;
		case 32:  //
			config_info.archival_state = *(data + 1);
			save_products_cfg_info();
			printf("-- Save archival state:%d\r\n",config_info.archival_state);
			break;
		default:
			break;
	}

	return 1;
}



/**************************************************
**	函数名称: NJ车型 解析配置信息
***************************************************/

uint8_t analysis_products_cfg_info(uint8_t *source,uint16_t len)
{	
    uint8_t 		*p = source;
	uint8_t 		i = 0;
	uint8_t			tmp_s[60] = {0};
	
	if(source == NULL)
		return 0;

	if(*(p) != ':')
		return 0;
	
	//printf("-- analysis_products_cfg_info %d\r\n",len);
	p += 1;
	memset(config_info.terminal_id,'\0',sizeof(config_info.terminal_id));    //解析设备号
	for(i = 0;i < 16;i++)  
	{	
		config_info.terminal_id[i] = *(p + i);             //
	}
	
	p += i;

	if(*p != ',')
		return 0;
	
	i = get_data_str(1,2,p,tmp_s,len);    //网关地址
	//printf("-- run is...... %d\r\n",i);
	if(i > 0)
	{
		tmp_s[i] = '\0';
		memcpy(config_info.enterprise_gw_addr,tmp_s,i + 1);          	//				
	}
				
	i = get_data_str(2,3,p,tmp_s,len);              //端口号
	if(i > 0)
	{	
		tmp_s[i] = '\0';
		config_info.enterprise_gw_port = fr_atof((const char *)tmp_s);         			//
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
		config_info.gb_four_upload_cycle = fr_atof((const char *)tmp_s);   			//定距上传
	}


	i = get_data_str(5,6,p,tmp_s,len); 

	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.travel_upload_cycle = fr_atof((const char *)tmp_s);  			//数据上传周期
	}

	i = get_data_str(6,7,p,tmp_s,len);  
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.hard_ware = fr_atof((const char *)tmp_s);      					//硬件版本号
	}
				
	i = get_data_str(7,8,p,tmp_s,len);  
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.can_num = fr_atof((const char *)tmp_s);   			//解析CAN协议号
		config_info.car_type = 	config_info.can_num;
	}

	
	i = get_data_str(8,9,p,tmp_s,len);  
	if(i > 0)
	{
		memcpy(config_info.apn,tmp_s,i);           			//设备APN
		config_info.apn[i] = '\0';
	}
	
	i = get_data_str(10,11,p,tmp_s,len);               //工作唤醒时间间隔   
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.delay_shutdown_time = fr_atof((const char *)tmp_s);
	}
	
	i = get_data_str(11,12,p,tmp_s,len);               //报警间隔     
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.waring_time_ms = fr_atof((const char *)tmp_s);
	}
	
	i = get_data_str(12,13,p,tmp_s,len);               //设置心跳间隔          
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.user_code = 72;
		memcpy(config_info.eco_mark,"FDUK",sizeof("FDUK"));   //
	}
	
	i = get_data_str(13,14,p,tmp_s,len);               //盲区发送间隔      
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.blind_time_sec = fr_atof((const char *)tmp_s);
	}
	

	i = get_data_str(17,18,p,tmp_s,len);               //农机三合一补贴状态
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.nj_state = fr_atof((const char *)tmp_s);
	}

	i = get_data_str(19,20,p,tmp_s,len);               //厂家编号 
	if(i > 0)
	{
		tmp_s[i] = '\0';
		config_info.farm_manu = fr_atof((const char *)tmp_s);   //用户编码
	}

	i = get_data_str(20,21,p,tmp_s,len);    //网关地址
	if(i > 0)
	{
		tmp_s[i] = '\0';
		memcpy(config_info.archivel_gw_addr,tmp_s,i + 1);          	//				
	}
				
	i = get_data_str(21,22,p,tmp_s,len);              //端口号
	if(i > 0)
	{	
		tmp_s[i] = '\0';
		config_info.archivel_gw_port = fr_atof((const char *)tmp_s);         			//
	}
	
	return 1;
}



/***************************************************** 
 * 农机配置：
 *****************************************************/

uint16_t build_config_info(uint8_t *buf,uint16_t size,uint8_t flag)
{
	uint16_t 						len;
	uint32_t 						tmp;
	uint8_t							i;
	uint8_t							tmp_c[50];
	//uint8_t 						tmp_buf[20];
	
	uint32_to_byte				    m_tmp;
	struct rt_tm			        *pt;
	
	if(size < 300 && buf == NULL)
		return 0;
	
	len = 0; 
	
	memcpy((char *)buf,(char *)"\r\nhomer4c:",sizeof("\r\nhomer4c:") - 1);
	if(flag == 2)
		memcpy((char *)buf,(char *)"\r\nhomer3t:",sizeof("\r\nhomer3t:") - 1);
	len += sizeof("\r\nhomer3t:") - 1;	
	
	for(i = 0;i < 16;i++)
		*(buf + len + i) = config_info.terminal_id[i];   			//
	len += i;
	*(buf + len) = ',';                               		//设备号  第0段
	len++;
	
	read_lte_icc_id((unsigned char *)buf + len,20);      

	len += 20;
	buf[len++] = ',';                                		//ICCID号 第1段
	
	
	tmp = strlen((const char *)config_info.enterprise_gw_addr);
	memcpy(buf + len,config_info.enterprise_gw_addr,tmp);     //网关地址
	len += tmp;
	buf[len++] = ',';                                //网关地址 第2段

	tmp = int_to_str(config_info.enterprise_gw_port,(char *)tmp_c,sizeof(tmp_c));
	for(i = 0;i < tmp;i++)            //网关端口号
		buf[len++] = tmp_c[i];
	buf[len++] = ',';                        //第3段  网关端口

	tmp = strlen((const char *)config_info.apn);
	memcpy(buf + len,config_info.apn,tmp);        // APN 
	len += tmp;
	buf[len++] = ',';                             //第4段  APN
					
	
	if(read_gnss_positing_state() == 'A')                    //定位状态
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
					
	tmp = read_gnss_latitude(1);                   //维度(经纬度)
	
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));
	memset(buf + len,'0',9);
	memcpy(buf + len,tmp_c,tmp);              								//
	len += 9;
	tmp = read_gnss_longitude(1);                  //经度
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));                       
	memset(buf + len,'0',9);
	memcpy(buf + len,tmp_c,tmp);              //
	len += 9;

	tmp = sprintf((char *)&buf[len],(char *)"%04d%04d",read_gnss_speed(),read_gnss_heading());
	
	len += 8;

	pt = (struct rt_tm *)tmp_c;
	read_gnss_utc_time(pt);
	
	tmp = sprintf((char *)&buf[len],(char *)"%02d%02d%02d%02d%02d%02d",pt->year,pt->mon,pt->day,pt->hour,pt->min,pt->sec);
	len += tmp;
	buf[len++] = ',';                             //第5段，GNSS定位数据，此段长度为40字节。
	
	tmp = read_gnss_satellite_num();		        //使用卫星的数量	
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));
	memset(buf + len,'0',9);
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';                //第6段  使用卫星的数量
	
	tmp = read_lte_csq();                              				//信号值
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));
	memset(buf + len,'0',9);
	memcpy(buf + len,tmp_c,tmp);
	len += 2;
	buf[len++] = ',';                          //第7段 LTE信号值
					
	tmp = read_in_power_vol();   
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));          //外部供电电压
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';               //第8段  当前设备的供电电压
					
	tmp = read_in_board_vol();
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));          //板卡电压
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';                        //第9段 板卡内部电池电压
					
	buf[len++] = read_in_acc_state() + 0x30;   										// ACC状态
	buf[len++] = ',';                          //第10段  ACC状态
		

	if(read_lte_init_state() == 1 && read_lte_sim_state() == 1)
		buf[len] = '0';   										//网络连接状态 0：未注册网络 1：已经附着网络	2：已经连接到服务器
	else
		buf[len] = '1';
	
	if(read_gb4_socket_state() == 1)           //链网状态
		buf[len] = '2';
	
	len++;
	
	buf[len++] = ',';                          //第11段
					
	tmp = read_app_version();                    //单片机版本号
	buf[len++] = tmp / 10 + 0x30;
	buf[len++] = '.';
	buf[len++] = tmp % 10 + 0x30;
					
	buf[len++] = ',';                         //第12段 研发自己定义的设备程序版本识别信息

	
	tmp = int_to_str(config_info.sleep_time,(char *)tmp_c,sizeof(tmp_c));      		//睡眠时间
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';                 //第13段 设备休眠时间

	
	tmp = int_to_str(config_info.gb_four_upload_cycle,(char *)tmp_c,sizeof(tmp_c));      		//定距离报位时间
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';            //第14段   距离报位间隔

	tmp = int_to_str(config_info.travel_upload_cycle,(char *)tmp_c,sizeof(tmp_c));      //数据推送周期
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';         //第15段   时间报位间隔

	buf[len++] = config_info.hard_ware / 10 + 0x30;      //硬件版本号
	buf[len++] = '.';
	buf[len++] = config_info.hard_ware % 10 + 0x30;
	buf[len++] = ',';                                  //第16段 硬件版本信息
					
	buf[len++] = ((read_files_sys_state() > 0) ? 0 : 1 ) + 0x30;   							//SD卡状态
	buf[len++] = ',';  //第17段  SD卡状态    1：功能正常   0：异常

	buf[len++] = '1';									
	buf[len++] = ',';                              //第18段  EEPROM状态  1：功能正常   0：异常

	buf[len++] = '1';								 
	buf[len++] = ',';							   //第19段, Ex_RTC状态  1：功能正常   0：异常
					
	buf[len++] = read_gnss_ant_state() + 0x30;    //
	buf[len++] = ',';                //第20段，天线检测    状态值：1/0   0：天线正常 1:天线开路 2:天线短路
			 
	buf[len++] = read_in_shell_state() + 0x30; 	//
	//printf("-- the read in shell state:%d\r\n",read_in_shell_state());
	buf[len++] = ',';                     //  第21段，光敏状态    状态值：1/0  0,1有变化为正常

	buf[len++] = '0';                               
	buf[len++] = ',';    //第22段，key_1状态   状态值：1/0  1：KEY_1按下 0：KEY_1释放

	buf[len++] = '1';								
	buf[len++] = ',';           //第23段，key_2状态   状态值：1/0  1：KEY_2按下 0：KEY_2释放 

	//read_boot_loader_version((uint8_t *)&m_tmp.value,4);              //BOOTLoader版本号
    m_tmp.byte[0] = '4';
    m_tmp.byte[1] = 'S';
    m_tmp.byte[2] = 'E';
    m_tmp.byte[3] = 0;

	tmp = hex_to_str(&m_tmp.byte[0],3,(unsigned char *)tmp_c,sizeof(tmp_c));      //"C"
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;

	buf[len++] = config_info.user_code / 10 + 0x30;      //硬件版本号
	buf[len++] = config_info.user_code % 10 + 0x30;

	buf[len++] = config_info.hard_ware / 10 + 0x30;      //硬件版本号
	buf[len++] = config_info.hard_ware % 10 + 0x30;
	
	tmp = read_app_version();                //单片机版本
	buf[len++] = tmp / 10 + 0x30;     
	buf[len++] = tmp % 10 + 0x30;
	
	tmp = read_acl16_app_version();               //嵌入式系统软件版本号
	buf[len++] = tmp / 10 + 0x30;   //
	buf[len++] = tmp % 10 + 0x30;   //
	
	tmp = read_user_version();                 //嵌入式系统软件版本号
	buf[len++] = tmp / 10 + 0x30;   //
	buf[len++] = tmp % 10 + 0x30;   //
	
	buf[len++] = ',';                 // 第24段，验证识别码
	
	tmp = int_to_str(config_info.can_num,(char *)tmp_c,sizeof(tmp_c));      //CAN协议号  25
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';                 //    第25段，CAN协议版本

	buf[len++] = read_can_connect_state() + 0x30;  
	buf[len++] = ',';                  //   第26段，CAN状态   0 :正常  1：异常  (注意和其他相反)
	
	tmp = 10;
	tmp = int_to_str(config_info.travel_upload_cycle,(char *)tmp_c,sizeof(tmp_c));      //数据推送周期
	memcpy(buf + len,tmp_c,tmp);
	len += tmp; 
	buf[len++] = ',';  // 第27段，盲区间隔   10：代表10秒
	
	buf[len++] = ',';   //第28段，格式化计数  1-100， 101：格式化关闭 102：格式化失败 协议头为"SDFormat"
	buf[len++] = ',';                //第29段,终检错误用  1：设备不存在 2：CAN线有问题。 协议头为"SetCheck"

	buf[len++] = '4';
	buf[len++] = ',';  //第30段,GSM模块类型  "2" 为2G模块，  "3" 为3G模块，  "4" 为4G模块， "5" 为5G模块 …

	buf[len++] = '4';	//第31段,GSM当前状态  "0"无网络， "2" 为2G状态，  "3" 为3G状态，  "4" 为4G状态， "5" 为5G状态
	buf[len++] = ',';
	
	buf[len++] = read_acl16_work_state() + 0x30;
  	buf[len++] = ','; //第32段,加密芯片  0 异常 1正常
		
	buf[len++] = ',';//	第33段,1路数字  0 异常 1正常
	buf[len++] = ',';//	第34段,2路数字  0 异常 1正常
	buf[len++] = ',';//	第35段,3路数字  0 异常 1正常

	buf[len++] = ',';//第36段,1路电压  数值，单位：V
	buf[len++] = ',';//第37段,2路电压  数值，单位：V
	buf[len++] = ',';//第38段,雅迪第二IP
	buf[len++] = ',';//第39段,雅迪第二端口
	buf[len++] = ',';//第40段,雅迪时区(单位:秒)
	buf[len++] = ',';//第41段,4S增加 MOTO控制线  0 异常 1正常
	buf[len++] = ',';//第42段 4S增加 NORFLASH    0 异常 1正常

	buf[len++] = '1';
	buf[len++] = ',';//第43段 CAN2状态  0 正常  1 异常  (注意和其他相反)

	tmp = read_lte_imei_id((unsigned char *)tmp_c,sizeof(tmp_c));       //LTE模块IMEI号   36
	memcpy(buf + len,tmp_c,tmp);

	len += tmp;
	buf[len++] = ',';  // 第44段,设备IMEI号

	tmp = int_to_str(config_info.delay_shutdown_time,(char *)tmp_c,sizeof(tmp_c));      		//睡眠时间
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';                 //第45段,唤醒工时 单位：秒

	tmp = int_to_str(config_info.waring_time_ms,(char *)tmp_c,sizeof(tmp_c));      		//睡眠时间
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';//第46段,报警间隔 单位：秒

	tmp = int_to_str(config_info.user_code,(char *)tmp_c,sizeof(tmp_c));      		//睡眠时间
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';//第47段,心跳间隔 单位：秒

	tmp = int_to_str(config_info.blind_time_sec,(char *)tmp_c,sizeof(tmp_c));      		//睡眠时间
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';//第48段,盲区间隔 单位：秒


	buf[len++] = ',';//第49段,雅迪自动时区用户名
	buf[len++] = ',';//第50段,千寻差分Ak
	buf[len++] = ',';//第51段,千寻差分As
	buf[len++] = ',';//第52段,千寻差分开关(0 关闭  1开启)
	buf[len++] = ',';//第53段,应用程序识别
	buf[len++] = ',';//第54段,三合一  1:关闭   2:开启
	buf[len++] = ',';//第55段,存储选择 0:eMMC 1:NORFLASH 2:ALL

	tmp = int_to_str(config_info.farm_manu,(char *)tmp_c,sizeof(tmp_c));      		//用户编号
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';//第56段,厂商编号

	tmp = strlen((const char *)config_info.archivel_gw_addr);
	memcpy(buf + len,config_info.archivel_gw_addr,tmp);     //网关地址
	len += tmp;
	buf[len++] = ',';                   //第57段,从IP或域名

	tmp = int_to_str(config_info.archivel_gw_port,(char *)tmp_c,sizeof(tmp_c));
	for(i = 0;i < tmp;i++)            //网关端口号
		buf[len++] = tmp_c[i];
	buf[len++] = ',';				//第58段,从端口号

	buf[len++] = ','; //第59段,锁车选择参数
	buf[len++] = ','; //第60段, 485通讯 0 异常 1正常
	buf[len++] = 0x30;        
	buf[len++] = ',';       //第61段, 蓝牙通讯 0 异常 1正常

	tmp = 0;                        //第三十八段 蓝牙强度
	tmp = int_to_str(tmp,(char *)tmp_c,sizeof(tmp_c));          //
	memcpy(buf + len,tmp_c,tmp);
	len += tmp;
	buf[len++] = ',';   //第62段, 蓝牙信号强度 小于50  不为0
	
	buf[len++] = ',';		//第63段, 单位秒，默认小于25秒，可编辑。
	buf[len++] = ',';       //第64段, 单位秒，默认小于60秒 可编辑
	buf[len++] = ',';       //第65段，返回配置的SIM卡号
	buf[len++] = ',';		//第66段，和设备号相同为正常，不相同异常(数据空不判断)

	buf[len++] = ',';//第67段,设备12V电压检测  0 异常 1正常(数据空不判断)

	buf[len++] = 0x0d;
	buf[len++] = 0x0a;   //回车换行符
	
	return len;
}

//#endif


/***************************
**	返回设备内部VIN状态
**	0x5A标识已经正确读取到VIN
**	
****************************/

uint8_t read_config_vin_state(void)
{
	uint8_t rv;
	
	rv = config_info.vin[0];
	
	return rv;
}


/***************************************
**	返回设备号
****************************************/

uint8_t read_config_terminal_id(uint8_t *buf,uint8_t size)
{
	if(size < 16)
		return 0;
	memcpy(buf,config_info.terminal_id,16);

	return 16;
}


/***************************
**	返回设备内部VIN状态
****************************/

uint8_t read_config_vin_info(uint8_t *buf,uint8_t size_buf)
{
	if(size_buf < 17)
		return 0;
	
	memcpy(buf,(uint8_t *)&config_info.vin[1],17);
	
	return 17;
	
}



/**********************************
**	返回车辆类型
************************************/

uint8_t read_config_car_type(void)
{
	uint8_t rv;

	rv = config_info.car_type;

	return rv;
}



/***********************************
**	
***********************************/

uint32_t read_config_sleep_cycle(void)
{
	uint32_t rv;

	rv = config_info.sleep_time;

	return rv;
}




/************************************
**	
*************************************/

void read_config_dev_id(uint8_t *buf,uint8_t buf_size)
{
	if(buf_size < 3)
		return;

	memcpy(buf,"DKS",sizeof("DKS"));
}



/************************************************
 *  玉柴使用
************************************************/

void read_config_gps_id(uint8_t *buf,uint8_t buf_size)
{
	if(buf_size < 4)
		return;
	
	buf[0] = 'S';
	buf[1] = 'D';
	buf[2] = 'K';
	buf[3] = 'S';
}



/************************************************
 *  玉柴使用
************************************************/

void read_config_mask(uint8_t *buf,uint8_t buf_size,uint8_t n)
{
	if(buf_size < 4)
		return;
	
	if(n == 0)  //3010W8 
	{
		config_info.mask[0] = 0x37;
		config_info.mask[1] = 0xFC;
		config_info.mask[2] = 0x7E;
		config_info.mask[3] = 0x23;
		memcpy(buf,config_info.mask,4);
	}
	else  //（3010W9）
	{
		config_info.mask[0] = 0xD6;
		config_info.mask[1] = 0x8B;
		config_info.mask[2] = 0x23;
		config_info.mask[3] = 0xCA;
		memcpy(buf,config_info.mask,4);
	}
}




/**************************************
**	返回延时关机时间  （S）
***************************************/

uint16_t read_config_delay_shutdown_time(void)
{
	uint16_t rv;

	rv = config_info.delay_shutdown_time;
	//rv = 30;   //调试使用

	return rv;
}


/****************************
 ** 处理生产配置
 ****************************/

void thread_entry_products(void *parameter)
{
	//mbedtls_aes_context aes_ctx;

	struct products_mq_str	products_mq;

    parameter = parameter;
	
	products_queue = xQueueCreate(1,sizeof(struct products_mq_str));

    for (;;)
    {
       	if(xQueueReceive(products_queue,&products_mq,100) == pdTRUE)
	   	{
			//mbedtls_aes_init(&aes_ctx);
			//printf("-- recv products mq.....%d,%d\r\n",products_mq.cmd,products_mq.len);
			switch(products_mq.cmd)
			{
				case 0:                                                    //解析生产信息
					if(config_info.product_mode != 0x55)
					{
						analysis_products_cfg_info(products_mq.data,products_mq.len);
						printf("-- Recv products Info:%d,%s\r\n",products_mq.len,products_mq.data);
					}
					
					break;
				case 1:                                                    //结束配置
					if(config_info.product_mode != 0x55)
					{
						if(save_products_cfg_info() == 0)
						{
							vTaskDelay(50);
							load_products_cfg_info();
							products_mq.len = build_config_info(products_mq.data,sizeof(products_mq.data),2);
						
							if(products_mq.len > 0)
							{
								if(products_port == 1)
								{
									write_data_to_uart0(products_mq.data,products_mq.len);
								}	
								else if(products_port == 2)
								{
									write_data_to_can(products_mq.data,products_mq.len);    //通过CAN接口发送
								}
								else
								{
									write_data_to_uart0(products_mq.data,products_mq.len);    //通过串口发送出去
									//write_data_to_can(products_mq.data,products_mq.len); 
								}
							}
						}
 						
						//printf("\r\n");
						//rintf("-- Recv Products Cmd:%s\r\n",products_mq.data);
					}
					
					break;
				case 2:                //进入生产模式  通过SHell进入配置模式
					config_info.product_mode = 0x56;
					products_port = 1;
					//printf("-- the this is...... 2 \r\n");
					break;
				case 3:                 //网络设置，来保存数据  （）
					modification_config_info(products_mq.data,products_mq.len);
					break;
				case 4:                //进入生产模式  通过SHell进入配置模式
					config_info.product_mode = 0x56;
					products_port = 2;
					//printf("-- the this is...... 4  %d,%x \r\n",products_port,config_info.product_mode);
					break;
				case 5:
		
					{
						uint16_t 		total_len = 0;
						uint8_t 		array[16] = {0};
						read_config_terminal_id(array,16);
						if(*(uint32_t *)(array + 8) != *(uint32_t *)&products_mq.data[0] || *(uint32_t *)(array + 12) != *(uint32_t *)&products_mq.data[4])
							break;
						
						total_len = build_config_info(products_mq.data,320,1);
						write_data_to_can(products_mq.data,total_len);
					}
					break;
					
			}

		}
		

		//printf("-- config_info.product_mode 0x%X,%d\r\n",config_info.product_mode,config_info.product_mode);
		
		
		if(config_info.product_mode == 0x56)
		{
			products_mq.len = build_config_info(products_mq.data,sizeof(products_mq.data),1);

			if(products_mq.len > 0)
			{
				if(products_port == 1)
				{
					write_data_to_uart0(products_mq.data,products_mq.len);   //通过串口发送
				} 
				else if(products_port == 2)
				{
					write_data_to_can(products_mq.data,products_mq.len);    //通过CAN接口发送
				}
				else
				{
					write_data_to_uart0(products_mq.data,products_mq.len);   //通过串口发送
					//vTaskDelay(10);
					//write_data_to_can(products_mq.data,products_mq.len);    //通过CAN接口发送
				}
			}
		}
    } 
}










