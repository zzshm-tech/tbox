

#include <stdio.h>
#include <string.h>


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "board.h"
#include "drv_rtc.h"

#include "common.h"
#include "pro_data.h"

#include "app_gb4.h"
#include "app_products.h"
#include "app_at.h"
#include "app_fifo.h"
#include "app_can_send.h"
#include "app_iap.h"







#define GB4_SOCKET_ID   					1



#define GB4_DATA_LEN   						256

#define GB4_FIFO_NUM   						5


#define GB4_BLIND_DATA_MAX_INDEX			0x200000 

#define GB4_BLIND_DATA_MAX_CNT  			(GB4_BLIND_DATA_MAX_INDEX / GB4_DATA_LEN)



#define QB4_DATA_LEN   						512

#define QB4_FIFO_NUM   						5


#define QB4_BLIND_DATA_MAX_INDEX			0x200000 

#define QB4_BLIND_DATA_MAX_CNT  			(QB4_BLIND_DATA_MAX_INDEX / QB4_DATA_LEN)



/************************** 本地全局变量 ******************************/

static struct encryption_chip_str              				encrypt_chip = {0};    		//

static struct gb4_str                          				gb4 = {0};					//

static struct serial_num_str                  				gb4_serial_num = {0};	    //

static QueueHandle_t                           				gb4_down_qh = NULL; 	    // 

static struct socket_down_str                  				gb_down_data = {0};			//

static uint8_t 												local_buff[1460] = {0};		//发送缓冲区

static struct gb4_alarm_str									gb4_alarm;					//




static uint8_t 												gb4_blind_save_log_buff[15] = {0};    //

static struct blind_s 										*const gb4_blind_log = (struct blind_s *)gb4_blind_save_log_buff;    //




/************************ GB4 发送缓冲区  *********************/

static struct send_fifo 									gb4_class = {GB4_DATA_LEN,GB4_FIFO_NUM ,0,0,0,0,0,NULL};   //

static uint8_t 												gb4_fifo_buff[GB4_DATA_LEN * GB4_FIFO_NUM] = {0};          //

static  SemaphoreHandle_t  									gb4_fifo_sem = NULL;                                       //



/************************ QB4 发送缓冲区  *********************/

static struct send_fifo 									qb4_class = {QB4_DATA_LEN,QB4_FIFO_NUM ,0,0,0,0,0,NULL};     //

static uint8_t 												qb4_fifo_buff[QB4_DATA_LEN * QB4_FIFO_NUM] = {0};            //

static  SemaphoreHandle_t  									qb4_fifo_sem = NULL;                                         //


static uint8_t 												qb4_blind_save_log_buff[15] = {0};    //

static struct blind_s 										*const qb4_blind_log = (struct blind_s *)qb4_blind_save_log_buff;    //






/***********************
**	系统时间(调试使用)
************************/

void build_rtc_time(uint8_t *buf,uint16_t size)
{
	struct rt_tm tmp;

	if(size < 6 || buf == NULL)
		return;

	get_rtc_time(&tmp);          //获取系统时间

	*(buf + 0) = (uint8_t)tmp.year;
	*(buf + 1) = (uint8_t)tmp.mon;
	*(buf + 2) = (uint8_t)tmp.day;
	*(buf + 3) = (uint8_t)tmp.hour;
	*(buf + 4) = (uint8_t)tmp.min;
	*(buf + 5) = (uint8_t)tmp.sec;
}







/*******************************
**	国四企业平台通用应答
********************************/

uint16_t build_gb4_response_packets(uint8_t *buf,uint16_t size,struct down_cmd_res_str *res)
{
	struct start_str    		*p_start;
	uint16_t 					len;
	uint32_to_byte   			tmp_data; 
	uint8_t 					array[50];
	
	len = sizeof(struct start_str);
	
	/* 数据打包时间 */
	build_rtc_time(array,sizeof(array));
	memcpy(buf + len,array,6);
	len += 6;
	/* 信息流水号 */
	tmp_data.value = swap_uint32_t(res->ser_num[0]); 
	memcpy(buf + len,&tmp_data.byte[0],4);
	len += 4;
	
	tmp_data.value = swap_uint32_t(res->ser_num[1]); 
	memcpy(buf + len,&tmp_data.byte[0],4);
	len += 4;
	
	*(buf + len++) = res->msg_type;    //消息类型
	*(uint16_t *)(buf + len) = swap_uint16_t(1);
	len += 2;
	*(buf + len++) = res->res;

	p_start = (struct start_str *)array;
	p_start->head[0] = 0x23;
	p_start->head[1] = 0x23;
	p_start->cmd = res->cmd_id;
	
	memset(p_start->vin,0,17);
	read_config_terminal_id(p_start->vin,17);
	
	p_start->soft_ver = SOFEWARE_VER;
	p_start->encrypt = SM2;
	p_start->len = swap_uint16_t(len - sizeof(struct start_str));

	memcpy(buf, array, sizeof(struct start_str));
	*(buf + len) = calc_xor_verify(buf + 2,len - 2);
	len++;
	
	return len;
}






/************************************************
**	下行数据解析
************************************************/
uint16_t gb4_platform_down_parse(uint8_t *data, uint16_t len,uint8_t *buf,uint16_t size)
{
	struct start_str    		    *p_start;
	struct down_cmd_con_str         *p_down_cmd;
	uint16_t 	                    offset = 0; 
	uint8_t 				        *index_p = NULL;
	uint8_t     	                array[100];
	struct down_cmd_res_str 		*p_res;

	//printf("\r\n-- Platform  receive GB4 Platform:");
   	//mem_printf(LOG_ERROR, PRINT_HEX, data, len);

    if(data == NULL || len == 0)
	{
		return 0;
	}
        

	if(len < sizeof(struct start_str))
		return 0;	
   	
	p_start = (struct start_str *)data;
	
	if((p_start->head[0] != 0x23) && (p_start->head[1] != 0x23))
	{
		printf("-- Down Cmd Header error.......\r\n");
		return 0;
	}
	
	//设备编号编号判断
	memset(array,0,sizeof(array));
	read_config_terminal_id(array,sizeof(array));  //
	//printf("-- %s\r\n",array);
	if(str_compare(p_start->vin,array,17) == 0)
	{
		//printf("-- Down Cmd Dev Id error\r\n");
		//return 0;
	}
	
	if((*(data + len - 1)) != (calc_xor_verify(data + 2,len - 3)))
	{
		printf("-- Down Cmd Check the error.......\r\n");
		return 0;
	}
	
	memset(array,0,sizeof(array));
	
	switch(p_start->cmd)
	{
		case 0x01: //国标登录  登入回应

			index_p = data + sizeof(struct start_str) + 8;	

			if(*index_p == 1)   //判断登入状态
			{
				index_p++;
				if(swap_uint16_t(*(uint16_t *)index_p) == 1)    //解析环保代码长度，不是17 退出
				{
					index_p += 2;
					if(*index_p == 0)
						gb4.login_out_state = 1;
					else
						gb4.login_out_state = 0;

					//printf("-- the gb4 login out state.....%d,%d\r\n",gb4.login_out_state,*index_p);
				}
			} 
			index_p += 6;

			if(*index_p == 3)
			{
				struct products_mq_str mq;
				QueueHandle_t t_q = NULL;

				index_p++;
				if(swap_uint16_t(*(uint16_t *)index_p) != 17)    //解析环保代码长度，不是17 退出
					break;

				index_p += 2;   //跳过长度

				mq.cmd = 3;
				mq.len = 18;
				mq.data[0] = 29;
				memset(&mq.data[1],'\0',20);
				memcpy(&mq.data[1],index_p,17);   //拷贝机械环保代码
				//printf("-- the vin:%s\r\n",mq.data);
				t_q = get_products_queue();
        		if(t_q != NULL)
            		xQueueSend(t_q,&mq,sizeof(struct products_mq_str));	

				#if H4DEV_TYPE == 0
//				if(read_config_archival_state() != 1 && read_config_archival_state() == 2)
//					xTaskCreate(thread_entry_archivel,          "thread_entry_archivel",        4096,       NULL,  15,  NULL);  //创建生产处理任务
				#endif
			}
			
			break;
		case 0x04:
			gb4.login_out_state = 0;
			printf("-- LogOut packet response... ok\r\n");
			break;
		case 0x81:    //设置  VIN 
			{
				p_down_cmd = (struct down_cmd_con_str *)(data + sizeof(struct start_str));
				//rt_kprintf("-- recv cmd.....%d,%d\r\n",Cmd_Type,p_down_cmd->cmd);
				switch(p_down_cmd->cmd)
				{
					case 0x80:               //设置VIN
						{
							struct products_mq_str 		mq;
							QueueHandle_t 				t_q = NULL;
							uint32_t   					tmp;
							uint8_t						m_array[50];

							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							mq.cmd = 3;

							mq.len = 50;

							memset(m_array,'\0',sizeof(m_array));
							tmp = swap_uint16_t(p_down_cmd->len);
							if(tmp > 50)
								tmp = 50;
							
							memset(mq.data,'\0',sizeof(m_array));
							mq.data[0] = 5;
							memcpy(m_array,index_p,tmp);
							sscanf((const char *)m_array,"%[^:]s",&mq.data[1]);
							mq.len = (uint8_t)strlen((const char *)&mq.data[1]);
							sscanf((const char *)m_array,"%*[^:]:%d",(uint32_t *)&mq.data[50]);

							printf("-- Set GatWay:%s:%d\r\n",&mq.data[1],*(uint32_t *)&mq.data[50]);
							
							t_q = get_products_queue();
        					if(t_q != NULL)
            					xQueueSend(t_q,&mq,sizeof(struct products_mq_str));
							
							vTaskDelay(300);
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x80;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
						}
						
					case 0x89:    //设置VIN
						{
							struct products_mq_str mq;
							QueueHandle_t t_q = NULL;

							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							mq.cmd = 3;

							mq.len = 18;
							mq.data[0] = 29;
							memset(&mq.data[1],'\0',20);
							memcpy(&mq.data[1],index_p,17);
							//printf("-- the vin:%s\r\n",mq.data);
							t_q = get_products_queue();
        					if(t_q != NULL)
            					xQueueSend(t_q,&mq,sizeof(struct products_mq_str));
							
							vTaskDelay(300);
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x89;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;
								
							#if H4DEV_TYPE == 0
								//xTaskCreate(thread_entry_archivel,          "thread_entry_archivel",        4096,       NULL,  15,  NULL);  //创建生产处理任务
							#endif

							return build_gb4_response_packets(buf,size,p_res);
						}
						break;
					case 0x92:           //设置机械环保代码 
						{
							struct products_mq_str mq;
							QueueHandle_t t_q = NULL;

							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							mq.cmd = 3;
							mq.len = 18;
							mq.data[0] = 29;
							memset(&mq.data[1],'\0',20);
							memcpy(&mq.data[1],index_p,17);
							//printf("-- the vin:%s\r\n",mq.data);
							t_q = get_products_queue();
        					if(t_q != NULL)
            					xQueueSend(t_q,&mq,sizeof(struct products_mq_str));
							
							vTaskDelay(300);
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x92;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;


							#if H4DEV_TYPE == 0
							//创建一个备案任务
								//xTaskCreate(thread_entry_archivel,          "thread_entry_archivel",        4096,       NULL,  15,  NULL);  //创建生产处理任务
							#endif

							return build_gb4_response_packets(buf,size,p_res);
						}
						break;
					case 0x94:          //设置三合一状态
						{
							struct products_mq_str mq;
							QueueHandle_t t_q = NULL;

							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							mq.cmd = 3;
							mq.len = 3;
							mq.data[0] = 36;     //

							mq.data[1] = *index_p;

							printf("-- set nj state:%d\r\n",mq.data[1]);

							t_q = get_products_queue();
        					if(t_q != NULL)
            					xQueueSend(t_q,&mq,sizeof(struct products_mq_str));
							
							vTaskDelay(300);
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x94;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
						}
						break;
					case 0x96:          // 设置国标数据上传时间间隔
						{
							struct products_mq_str mq;
							QueueHandle_t t_q = NULL;

							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							mq.cmd = 3;
							mq.len = 3;
							mq.data[0] = 34;

							*(uint16_t *)&mq.data[1] = swap_uint16_t(*(uint16_t *)index_p);

							//printf("-- set gb4 upload cycle:%d\r\n",*(uint16_t *)&mq.data[1]);

							t_q = get_products_queue();
        					if(t_q != NULL)
            					xQueueSend(t_q,&mq,sizeof(struct products_mq_str));
							
							vTaskDelay(300);
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x96;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
							
						}
						break;
					case 0x81:
						{
							struct products_mq_str mq;
							QueueHandle_t t_q = NULL;

							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							mq.cmd = 3;
							mq.len = 3;
							mq.data[0] = 9;

							*(uint16_t *)&mq.data[1] = swap_uint16_t(*(uint16_t *)index_p);

							//printf("-- set en upload cycle:%d\r\n",*(uint16_t *)&mq.data[1]);

							t_q = get_products_queue();
        					if(t_q != NULL)
            					xQueueSend(t_q,&mq,sizeof(struct products_mq_str));
							
							vTaskDelay(300);
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x81;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
							
						}
					default:
						
						break;
				}
			}
			break;
		case 0x82: //终端控制
			{
				p_down_cmd = (struct down_cmd_con_str *)(data + sizeof(struct start_str));
				//rt_kprintf("-- recv cmd.....%d,%d\r\n",Cmd_Type,p_down_cmd->cmd);
				switch(p_down_cmd->cmd)
				{
					case 0x88:
					{
						struct can_send_mq_t tmp_mq;
						QueueHandle_t t_q = NULL;

						tmp_mq.cmd = 4;
						tmp_mq.len = 2;
						tmp_mq.data[0] = 0;
						tmp_mq.data[1] = *((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));

						if(tmp_mq.data[1] == 0 || tmp_mq.data[1]  == 2)
							tmp_mq.data[1] = 1;
						else
							tmp_mq.data[1] = 0;

						t_q = get_can_send_queue();
						if(t_q != NULL)
							xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
				
				
						printf("-- recv mon cmd...Down..%d\r\n",tmp_mq.data[0]);

						p_res = (struct down_cmd_res_str *)array;
						p_res->cmd_id = 0x81;
						p_res->msg_type = 0x80;
						p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
						p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
						p_res->res = 0;

						return build_gb4_response_packets(buf,size,p_res);
					}
					break;
					case 0x86:    //锁车命令
						{
							struct can_send_mq_t tmp_mq;
							QueueHandle_t t_q = NULL;

							tmp_mq.cmd = 4;
							tmp_mq.len = 2;
							tmp_mq.data[0] = 1;
							tmp_mq.data[1] = *((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							t_q = get_can_send_queue();
							if(t_q != NULL)
								xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
								
							printf("-- recv lock cmd...Down..%d\r\n",tmp_mq.data[1]);
						
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x80;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
						}
						break;
					case 0x87:    //解锁命令
						{
							struct can_send_mq_t tmp_mq;
							QueueHandle_t t_q = NULL;

							tmp_mq.cmd = 4;
							tmp_mq.len = 2;
							tmp_mq.data[0] = 1;
							tmp_mq.data[1] = *((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							t_q = get_can_send_queue();
							if(t_q != NULL)
								xQueueSend(t_q,&tmp_mq,sizeof(struct can_send_mq_t));	
							 printf("-- recv unlock cmd...Down..%d\r\n",tmp_mq.data[1]);
							//rt_mq_send(get_can_lock_mq(),&event,sizeof(event));
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x80;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
						}
						break;
					case 0x01:     //远程升级命令
						{
							struct iap_ftp_str ftp_info;
							uint16_t 	tmp_len;
							
							tmp_len = swap_uint16_t(p_start->len);
							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							//printf("-- the ftp update..  %d ...%s\r\n",tmp_len,index_p);
							memset((uint8_t *)&ftp_info,'\0',sizeof(struct iap_ftp_str));
							if(get_buf_str(8,9,';',index_p,array,tmp_len) > 0)      
							{
								index_p = array + 6;
								offset += get_given_string((uint8_t *)index_p, ':',ftp_info.user);
								offset += get_given_string((uint8_t *)index_p + offset, '@', ftp_info.passwd);
								offset += get_given_string((uint8_t *)index_p + offset, ':', ftp_info.host);
								offset += get_given_number((uint8_t *)index_p + offset, '/', &ftp_info.port);
								offset += get_given_string((uint8_t *)index_p + offset, ':', ftp_info.files);
								
								printf("\r\n-- FTP FtpUserName:%s\r\n",ftp_info.user);
								printf("-- FTP FtpUserPassd:%s\r\n",ftp_info.passwd);
								printf("-- FTP Server Addr:%s\r\n",ftp_info.host);
								printf("-- FTP Server Port:%d\r\n",ftp_info.port);
								printf("-- FTP File Name:%s\r\n",ftp_info.files);	

								if(read_iap_state() == 0)
								{
									xTaskCreate(thread_entry_iap, "thread_entry_iap",        4096,&ftp_info,  14,  NULL);  //创建FTP升级任务
									vTaskDelay(100);
								}
								
								
								p_res = (struct down_cmd_res_str *)array;
								p_res->cmd_id = 0x82;
								p_res->msg_type = 0x01;
								p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
								p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
								p_res->res = 0;

								return build_gb4_response_packets(buf,size,p_res);
							}
							break;
						}
					}
					break;
				}
				default:
					break;
		}

		return 0;
}







/*****************************
**  处理国四的任务
*******************************/

void thread_entry_gb4(void *parameter)
{
  parameter = parameter;
	
	for(;;)
	{
		
	}
}

