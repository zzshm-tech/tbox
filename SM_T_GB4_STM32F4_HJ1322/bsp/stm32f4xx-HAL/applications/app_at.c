


#include <stdio.h>
#include <string.h>


#include <rtthread.h>
#include <rtdevice.h>

#include "board.h"
#include "drv_gpio.h"
#include "drv_timer.h"
#include "drv_rtc.h"

#include "common.h"

#include "app_lte.h"
#include "app_can_send.h"
#include "pro_data.h"
#include "app_shell.h"
#include "app_iap.h"
#include "app_sms.h"
#include "app_at.h"


#define NTP_SERVER_ADDR 							"ntp1.aliyun.com"

#define TCP_DATA_LEN 									 1440 			/** 模块一次性上限1460byte **/

#define AT_BUFF_SIZE                   4224     /** AT命令接收缓冲区大小 **/

#define SOCKET_NUM                     2        /** 最大Socket链接数量 **/


/***********************************/

struct at_info 							at_info = {0};

static rt_device_t 					gprs_uart_dev = RT_NULL;

struct rt_ringbuffer 				rb = {0};

static uint8_t 							rb_pool[AT_BUFF_SIZE] = {0};

static uint8_t 							tmp_buf[AT_BUFF_SIZE] = {0};

static rt_size_t 						tmp_offset = 0;

static uint8_t 							at_buf[AT_BUFF_SIZE] = {0};

static uint32_t 						lte_send_num = 0;

/************** AT命令返回相关变量 ********************/

static rt_mq_t   			       socket_qh[2] = {NULL,NULL};


/****************************************************
**  
*****************************************************/

static struct at_res_str at_cmd_send_syn(void *buff, uint16_t len, struct at_res_str (*func)(const char *),uint16_t timeout_s, uint16_t retry)
{
    struct at_res_str 		at_cmd_res = {0};
    uint16_t 							pitch_time = 0;
    rt_size_t 						rb_return_len = 0, w_len = 0;
    int 									timeout_cnt = 0;
    
    pitch_time = 1000 / RT_TICK_PER_SECOND;       /* 根据系统节拍,获得系统心跳间隔*/
    timeout_cnt = timeout_s * RT_TICK_PER_SECOND; /* 根据超时时间，获得出需要循环等待的次数*/
		
    if(buff != RT_NULL)
    {
        w_len = rt_device_write(gprs_uart_dev, 0, buff, len);
        if (w_len != len)
            rt_kprintf("-- gprs uart devic write error. w_len:%d, len:%d\r\n", (int)w_len, (int)len);
        rt_thread_delay(AT_CMD_WAIT_TIME_MS / pitch_time);
    }
		
		if(func == NULL)
		{
			at_cmd_res.res = RT_EOK;
			return at_cmd_res;
		}
		
    do
    {
        rb_return_len = rt_ringbuffer_data_len(&rb);
        if (tmp_offset + rb_return_len > RING_BUFF_SIZE)
        {
            tmp_offset = 0;
            rt_memset(tmp_buf, 0, RING_BUFF_SIZE);
        }
        w_len = rt_ringbuffer_get(&rb, tmp_buf + tmp_offset, rb_return_len); /* 读取环形区的数据*/
        if (w_len != rb_return_len)
            rt_kprintf("-- read ringbuffer data error:rb_rerurn_len %d w_len: %d\r\n", (int)rb_return_len, (int)w_len);
        if (w_len > 0)
        {
					at_cmd_res = func((const char *)(tmp_buf + tmp_offset));
					tmp_offset += w_len;
					if(at_cmd_res.res == RT_EOK)
					{
						tmp_offset = 0;
						rt_memset(tmp_buf, 0, RING_BUFF_SIZE);
						return at_cmd_res;
					}
					else
					{
						if(retry) /* 需要循环发送*/
            {
							rt_kprintf("-- at cmd retry:%s\r\n",buff);
              if (buff != RT_NULL)
                 rt_device_write(gprs_uart_dev, 0, buff, len);

              tmp_offset -= w_len;                /* 循环发送时禁止指针偏移*/
              rt_thread_delay(1000 / pitch_time); /* 循环查询 1秒1次*/

              if(timeout_cnt > RT_TICK_PER_SECOND)
                 timeout_cnt -= (RT_TICK_PER_SECOND - 1);
              else
                 timeout_cnt = 0;
              continue;
             }
					}
				}
        rt_thread_delay(AT_CMD_WAIT_TIME_MS / pitch_time); /* 没有等到串口数据时线程延迟指定时间后查询*/
        timeout_cnt--;
    }
    while (timeout_cnt > 0);
    tmp_offset = 0;
    rt_memset(tmp_buf, 0, RING_BUFF_SIZE);
    rt_kprintf("-- the at cmd timeout ... %s\r\n",buff);
		at_cmd_res.res = RT_ETIMEOUT;	
    return at_cmd_res;
}


/*********************
**	记录Socket发送次数
**************************/

uint32_t read_lte_send_num(void)
{
	uint32_t rv = 0;
	
	rv = lte_send_num;
	
	return rv;
}




/************************************************
**	名称：
**	功能: AT指令接收数据 (GPRS/SMS)
**	输入：pdata GSM模块输出的AT指令响应
**	输出: None
*********************************************/

static rt_err_t at_recv_data(const char *data, rt_uint16_t rev_len)
{
	int id = 0, len = 0;
  int offset = 0;
  char *p = RT_NULL;
  char *index = RT_NULL;
  rt_size_t rb_write_len = 0;

  index = (char *)data;
    
  do                           //循环解析接收数据 防止多条数据10ms内同时到达
  {
      p = rt_strstr(index, "+QIURC: \"recv\"");
      if(p != RT_NULL)
      {
				 struct socket_down_str tmp;
				 
         sscanf(p, "+QIURC: \"recv\",%d,%d", &id, &len);
				
				 //rt_kprintf("-- +QIURC: recv  %d,%d\r\n",id,len);
         
				 if (len < 10)
            offset = 1;
          else if (len < 100)
            offset = 2;
          else if (len < 1000)
            offset = 3;
          else
            offset = 4;
           p += (19 + offset); 			//跳过关键字,指定数据指针
					
					memcpy(tmp.data,p,len);
					tmp.len = len;
          if(socket_qh[id] != NULL)
          {
						rt_mq_send(socket_qh[id],&tmp,sizeof(struct socket_down_str));            //下行命令解析
					}
					
					index += (len + 16); 														//保证下次检索位置是新的
      }
							
			p = strstr(index, "+QIURC: \"closed\"");
			if(p != NULL)
			{
				struct socket_down_str tmp;

				sscanf(p, "+QIURC: \"closed\",%d", &id);
				//rt_kprintf("-- platform logout!!!! %d\r\n",id);
        if(socket_qh[id] != NULL)
        {
            //printf("-- Socket Send Queue......%d\r\n",id);
            memcpy(tmp.data,"+QIURC:",sizeof("+QIURC:"));
            tmp.len = sizeof("+QIURC:");
            rt_mq_send(socket_qh[id],&tmp,sizeof(struct socket_down_str));  //发送队列
        }
			}
			
			p = strstr(index, "+CMTI:");											//查短信索引: +CMTI: "SM",4
			if(p != NULL)
			{
				struct sms_down_str         tmp;
				rt_mq_t                			t_q;
				
        sscanf(p, "+CMTI: \"ME\",%d\r\n", &id);
				
				tmp.cmd = 0;
        tmp.data[0] = id;
				rt_kprintf("-- Recv SMS Event.....%d\r\n",id);
        t_q = get_sms_queue();
        if(t_q != NULL)
					rt_mq_send(t_q,&tmp,sizeof(struct sms_down_str));
			}
			
			p = strstr(index, "+CMGR:");                      //读短信内容
      if(p != RT_NULL)
      {
				struct sms_down_str         tmp;
        uint16_t                    t_len;
        rt_mq_t                			t_q;
					
        if((p - index) > 0)
           t_len = rev_len -  (p - index);              //??????????????? 问题
        else
           t_len = 0;
				
        rt_kprintf("-- t len :%d\r\n",t_len);
        if(t_len > 0 && t_len < 1024)
        {
					 tmp.len = (uint16_t)t_len;
           rt_memcpy(tmp.data,p,t_len);
					 tmp.cmd = 2;
           t_q = get_sms_queue();
           if(t_q != NULL)
							rt_mq_send(t_q,&tmp,sizeof(struct sms_down_str));
					 //rt_kprintf("-- Process SMS CMD ... \r\n %s\r\n",p);
        }
				//rt_kprintf("-- +CMGR: ... %d \r\n %s\r\n",p);
        index = p + t_len;
      }
			
			p = strstr(index, "+CMT:");                      //读短信内容
      if(p != RT_NULL)
      {
				struct sms_down_str         tmp;
        uint16_t                    t_len;
        rt_mq_t                			t_q;
					
        if((p - index) > 0)
           t_len = rev_len -  (p - index);              //??????????????? 问题
        else
           t_len = 0;
				
        rt_kprintf("-- t len :%d\r\n",t_len);
        if(t_len > 0 && t_len < 1024)
        {
					 tmp.len = (uint16_t)t_len;
           rt_memcpy(tmp.data,p,t_len);
					 tmp.cmd = 2;
           t_q = get_sms_queue();
           if(t_q != NULL)
							rt_mq_send(t_q,&tmp,sizeof(struct sms_down_str));
					 //rt_kprintf("-- Process SMS CMD ... \r\n %s\r\n",p);
        }
				//rt_kprintf("-- +CMT: ... \r\n %s\r\n",p);
        index = p + t_len;
       }
    }
    while (p != RT_NULL);
    //不缓存已经解析过的数据
    rb_write_len = rt_ringbuffer_put(&rb, (uint8_t *)index, rev_len - (index - data)); 
    if ((rb_write_len != (rev_len - (index - data))))
    {
       rt_kprintf("\r\n- write ringbuffer data error: at recv %d  rb_write_len: %d\r\n", (int)(rev_len - (index - data)), (int)rb_write_len);
    }
    return RT_EOK;
}






/******************************
**	
*******************************/

static struct at_res_str at_cmd_match_ok(const char *data)
{
	struct at_res_str at = {0};
	char *p = RT_NULL;
  
	p = rt_strstr(data, "OK");
  if (p != RT_NULL)
  {
		at.res = RT_EOK;
		return at;
  }

  p = rt_strstr(data, "ERROR");
  if (p != RT_NULL)
  {
		at.res = RT_ERROR;
		return at;
  }
    
	at.res = RT_ETIMEOUT;
	
	return at;
}


/******************************
**	
*******************************/

struct at_res_str at_cmd_ntp_res(const char *data)
{
	struct at_res_str at = {0};
	char *p = RT_NULL;
	struct rt_tm tm;
  struct rt_tm *pt;
  
	//rt_kprintf("-- AT %s\r\n",data);
	p = strstr(data, "+QNTP: 0");
  if (p != RT_NULL)
  {
		sscanf(p, "+QNTP: 0,\"%d/%d/%d,%d:%d:%d+",&tm.year,&tm.mon,&tm.day,&tm.hour,&tm.min,&tm.sec);
		
		//rt_kprintf("-- run this is. 2 ....%u\r\n",tm.year);
    tm.year -= 2000;
    tm.year = rt_mktime(&tm) + 28800;         //GNSS
        
		pt = rt_localtime(&tm.year);
		
    at.data[0] = (uint8_t)pt->year;
    at.data[1] = (uint8_t)pt->mon;
    at.data[2] = (uint8_t)pt->day;
    at.data[3] = (uint8_t)pt->hour;
    at.data[4] = (uint8_t)pt->min;
    at.data[5] = (uint8_t)pt->sec;
		
		at.res = RT_EOK;
		return at;
  }
    
	at.res = RT_ETIMEOUT;
	
	return at;
}



/*******************************************
**	设置NTP同步服务器
*******************************************/
uint8_t at_set_ntp_server(void)
{
	struct at_res_str at = {0};

	rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	memset(at.data,'\0',sizeof(at.data));
	sprintf((char *)at.data, "AT+QNTP=%d,\"%s\",%d\r\n",1,NTP_SERVER_ADDR,123);
	//rt_kprintf("-- %s\r\n",at.data);
	at = at_cmd_send_syn(at.data, strlen(at.data), at_cmd_match_ok, 30, NOT_RETRY);
	rt_sem_release(&at_info.send_sem);
	
	if(at.res == RT_EOK)
  {
		return 0;
  }
	
	return 1;
}




/*********************************
**	解析pdp激活返回的状态
**********************************/

static struct at_res_str at_cmd_ready(const char *data)
{
	struct at_res_str at = {0};
  char *p = RT_NULL;
	
	at.res = RT_ETIMEOUT;
  p = rt_strstr(data, "RDY");
  if (p != RT_NULL)
  {
    at.res = RT_EOK;
        
		return at;
  }
  
	return at;
}


/*******************************************
**	终端链路连接 (设置apn 激活pdp)
*******************************************/
uint8_t at_set_apn_syn(void)
{
	struct at_res_str at = {0};
	uint8_t tmp_buf[50] = {0};
	
	rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	sprintf((char *)tmp_buf, "AT+QICSGP=1,1,\"CMNET\",\"\",\"\",1\r\n"); 
	at = at_cmd_send_syn(tmp_buf, rt_strlen((char *)tmp_buf),at_cmd_match_ok, 30, NOT_RETRY);
	rt_sem_release(&at_info.send_sem);
	
	if(at.res == RT_EOK)
  {
		return 0;
  }
	
	return 1;
}




/*************************************
**  SMS Reporting 
**************************************/

uint8_t at_config_sms_event(void)
{
    struct at_res_str at = {0};
 
		at.res = RT_ERROR;
    rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
    at = at_cmd_send_syn("AT+CNMI=2,2,0,0,0\r\n", strlen("AT+CNMI=2,2,0,0,0\r\n"), at_cmd_match_ok,30, NOT_RETRY);
    rt_sem_release(&at_info.send_sem);
    
    if(at.res == RT_EOK)
    {
			return 0;
    }

    return 1;
}




/*****************************
** 设置LTE模块波特率
******************************/

uint8_t at_set_lte_ipr(uint32_t band)
{
	struct at_res_str at = {0};
	uint8_t array[32];

  rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	memset(array,0,sizeof(array));
	sprintf((char *)array,"AT+IPR=%d\r\n",band);
  at = at_cmd_send_syn(array, strlen((const char *)array), at_cmd_match_ok, 5, NOT_RETRY);
  rt_sem_release(&at_info.send_sem);
    
  if(at.res == RT_EOK)
  {
    return 0;
  }

  return 1;
}



/*****************************
** 设置LTE模块波特率
******************************/

uint8_t at_save_lte_arg(void)
{
	struct at_res_str at = {0};

  rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
  at = at_cmd_send_syn("AT&W\r\n", strlen("AT&W\r\n"), at_cmd_match_ok, 5, NOT_RETRY);
  rt_sem_release(&at_info.send_sem);
    
  if(at.res == RT_EOK)
  {
    return 0;
  }

  return 1;
}


/**********************
**	登录LTE启动
***********************/

uint8_t at_wait_cmd_ok_syn(void)
{
	struct at_res_str at;
	
	rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	at = at_cmd_send_syn(RT_NULL, 0, at_cmd_ready, 40, NOT_RETRY);
	rt_sem_release(&at_info.send_sem);
	
	if(at.res == RT_EOK)
		return 0;
	else
		return 1;
}


/*************************************
**  设置SMS  AT+CMGF=1
**************************************/

uint8_t at_config_sms_fromat(void)
{
	struct at_res_str at;

  rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
  at = at_cmd_send_syn("AT+CMGF=1\r\n", rt_strlen("AT+CMGF=1\r\n"), at_cmd_match_ok,5, NOT_RETRY);
  rt_sem_release(&at_info.send_sem);
    
  if(at.res == RT_EOK)
		return 0;
  else
		return 1;
}


/****************************
**	解析获取的ICCID
*****************************/

static struct at_res_str at_cmd_parse_iccid(const char *data)
{
	struct at_res_str at = {0};
	char *p = RT_NULL;

  p = rt_strstr(data, "+QCCID:");
  if(p != RT_NULL)
  {
		sscanf(p, "+QCCID: %s", at.data);
    at.res = RT_EOK;
    at.len = 20;
   
    return at;
  }
 
	at.res = RT_ETIMEOUT;
    
	return at;
}



/******************************
**	解析IMEI
*******************************/

static struct at_res_str at_cmd_parse_imei(const char *data)
{
	struct at_res_str at = {0};

	char *p_start = NULL;
	char *p_end = NULL;
	uint8_t len;
    
  p_start = strstr(data, "\r\n");
    
	if(p_start != NULL)
  {
		p_start += 2;
    p_end = strstr(p_start,"\r\n");
		if(p_end != NULL)
		{
			len = p_end - p_start;
			memcpy((char *)at.data,p_start,len);
      at.len = len;
      at.res = RT_EOK;
      //printf("--IMEI:%s  %d\r\n",data,len);
      return at;
		}
  }

	at.res = RT_ETIMEOUT;
   
	return at;
}


/****************************************
**	解析获取的网络注册状态
****************************************/
static struct at_res_str at_cmd_parse_cgreg(const char *data)
{
	struct at_res_str at = {0};
  char *p = RT_NULL;
	
	uint32_t m;

  p = strstr(data, "+CEREG:");
  if(p != RT_NULL)
  {
		sscanf(p, "+CEREG: %*d,%d", &m);
    if((m == 1) || (m == 5))
    {
			at.data[0] = m;
			at.res = RT_EOK;
			return at;
    }
  }
	
	at.res = RT_ETIMEOUT;
  return at;
}


/****************************************
**	解析GPRS网络附着状态
*****************************************/
static struct at_res_str at_cmd_parse_cgatt(const char *data)
{
    struct at_res_str at = {0};
		char *p = NULL;
    uint32_t m = 0;
    
		p = strstr(data, "+CGATT:");
    if(p != NULL)
    {
        sscanf(p, "+CGATT: %d", &m);
        if(m == 1)
        {
            at.res = RT_EOK;
            at.data[0] = 1;
            return at;
        }
    }
	
    at.res = RT_ETIMEOUT;

    return at;
}


/****************************************
**	解析GPRS网络附着状态
*****************************************/
static struct at_res_str at_cmd_parse_cclk(const char *data)
{
	struct at_res_str at = {0};
	char *p = NULL;

	struct rt_tm tm;
  struct rt_tm *pt;

	p = strstr(data, "+CCLK:");
  if(p != NULL)
  {
		sscanf(p, "+CCLK: \"%d/%d/%d,%d:%d:%d+",&tm.year,&tm.mon,&tm.day,&tm.hour,&tm.min,&tm.sec);
		tm.year = rt_mktime(&tm) + 28800;         //GNSS
        //printf("-- run this is. 2 ....%u\r\n",tm.year);
		pt = rt_localtime(&tm.year);
		
    at.data[0] = (uint8_t)pt->year;
    at.data[1] = (uint8_t)pt->mon;
    at.data[2] = (uint8_t)pt->day;
    at.data[3] = (uint8_t)pt->hour;
    at.data[4] = (uint8_t)pt->min;
    at.data[5] = (uint8_t)pt->sec;

    at.res = RT_EOK;
		
		return at;
    }
	
    at.res = RT_ETIMEOUT;
    return at;
}


/*****************************
**  读取网络时间
******************************/

uint8_t at_get_lte_cclk(struct rt_tm *tt)
{
	struct at_res_str at = {0};

  rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
  at = at_cmd_send_syn("AT+CCLK?\r\n", strlen("AT+CCLK?\r\n"), at_cmd_parse_cclk, 60, RETRY);
  rt_sem_release(&at_info.send_sem);
    
  if(at.res == RT_EOK)
  {
		tt->year = at.data[0];
    tt->mon = at.data[1];
    tt->day = at.data[2];
    tt->hour = at.data[3];
    tt->min = at.data[4];
    tt->sec = at.data[5];

    return 0;
  }

  return 1;
}


/******************************************
** 回显控制指令  0 回显方式关闭;  1 回显方式打开
******************************************/

uint8_t at_ctrl_echo_syn(void)
{
	struct at_res_str at;

	rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	at = at_cmd_send_syn("ATE0\r\n", rt_strlen("ATE0\r\n"), at_cmd_match_ok, 2, NOT_RETRY);
	rt_sem_release(&at_info.send_sem);
	
  if(at.res == RT_EOK)
		return 0;
	else
		return 1;   
}



/******************************
**	获取ccid
*******************************/

uint8_t at_get_ccid_syn(uint8_t *source,uint8_t size)
{
	struct at_res_str at;
	
	rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	at = at_cmd_send_syn("AT+QCCID\r\n", rt_strlen("AT+QCCID\r\n"), at_cmd_parse_iccid, 30, NOT_RETRY);
	rt_sem_release(&at_info.send_sem);
	
	if(at.res == RT_EOK)
  {
		memcpy(source,at.data,at.len);
        
		return 0;
  }

  return 1;
}




/***************************************
**	解析SIM卡状态
****************************************/

static struct at_res_str at_cmd_sim_state(const char *data)
{
	struct at_res_str at = {0};
  char *p = NULL;

  p = strstr(data, "+CPIN: READY");
  if (p != NULL)
  {
		at.res = RT_EOK;
		return at;
  }
		
	at.res = RT_ETIMEOUT;
		
  return at;
}


/*****************************
**	获取SIM卡号
*******************************/

uint8_t at_get_sim_syn(uint8_t *data)
{
	struct at_res_str at;
    
  *data = 0;

  rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	at = at_cmd_send_syn("AT+CPIN?\r\n", strlen("AT+CPIN?\r\n"), at_cmd_sim_state, 30, RETRY);
  rt_sem_release(&at_info.send_sem);
    
	if(at.res == RT_EOK)
  {
		*data = 1;
    return 0;
  }

  return 1;
}


/******************************
**	获取IMEI
*******************************/

uint8_t at_get_imei_syn(uint8_t *source,uint8_t len)
{
	struct at_res_str at = {0};

	rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	at = at_cmd_send_syn("AT+GSN\r\n", rt_strlen("AT+GSN\r\n"), at_cmd_parse_imei, 30, NOT_RETRY);
	rt_sem_release(&at_info.send_sem);
	
	if(at.res == RT_EOK)
	{
		memcpy(source,at.data,at.len);
		return 0;
	}
	else
	{
		return 1;
	}
}



/*****************************
**	设置CGREG状态
*******************************/

uint8_t at_set_cgreg_syn(void)
{
	struct at_res_str at;
    
  rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	at = at_cmd_send_syn("AT+CGREG=1\r\n", strlen("AT+CGREG=1\r\n"), at_cmd_match_ok, 30, NOT_RETRY);
  rt_sem_release(&at_info.send_sem);
    
  if(at.res == RT_EOK)
  {
		return 0;
  }

  return 1;
}


/******************************
**	获取LTE模块信号值
*******************************/

struct at_res_str at_cmd_get_csq(const char *data)
{
	struct at_res_str at = {0};
	char *p = NULL;
	uint32_t csq = 0;
  uint32_t dbm = 0;
	
	//rt_kprintf("-- The csq cmd....%s\r\n",data);
	p = rt_strstr(data, "+CSQ:");
      
	if (p != RT_NULL)
  {
		sscanf(p, "+CSQ:%d,%d",&csq,&dbm); 
    at.data[0] = (uint8_t)csq;
    at.data[1] = (uint8_t)dbm;
    at.res = RT_EOK;
		
		return at;			
  }
	at.res = RT_ETIMEOUT;
	return at;
}




/*****************************
**	测试AT命令
******************************/

uint8_t at_check_cmd_syn(void)
{
	struct at_res_str at = {0};
	
	rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	at = at_cmd_send_syn("AT\r\n", rt_strlen("AT\r\n"),at_cmd_match_ok, 30, NOT_RETRY);
	rt_sem_release(&at_info.send_sem);
	
	if(at.res == RT_EOK)
		return 0;
	else
		return 1;
}




/******************************
**  
*******************************/

static struct at_res_str at_cmd_query_urc_port(const char *data)
{
    struct at_res_str at = {0};

		char *p = NULL;
  
    p = strstr(data, "uart1");
  
    if (p != NULL)
    {
        at.res = RT_EOK;

		return at;
    }

    at.res = RT_ETIMEOUT;

	return at;
}




/*************************************
**  查询URC Port配置状态
**************************************/

uint8_t at_query_urc_port_state(void)
{
	struct at_res_str at = {0};

  rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
  at = at_cmd_send_syn("AT+QURCCFG?\r\n", strlen("AT+QURCCFG?\r\n"), at_cmd_query_urc_port,6, NOT_RETRY);
  rt_sem_release(&at_info.send_sem);
    
  if(at.res == RT_EOK)
  {
     return 0;
  }
	
  return 1;
}


/*************************************
**  读取短信内容
**************************************/

uint8_t at_read_sms_text(uint16_t id)
{
	struct at_res_str at = {0};
	uint8_t tmp_buf[50] = {0};
	
  rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	sprintf((char *)tmp_buf, "AT+CMGR=%d\r\n", id);
  at = at_cmd_send_syn(tmp_buf, strlen((const char *)tmp_buf),NULL,0, NOT_RETRY);
  rt_sem_release(&at_info.send_sem);
    
  if(at.res == RT_EOK)
  {
     return 0;
  }
	
  return 1;
}




/*************************************
**  设置SMS
**************************************/

uint8_t at_config_sms_port(void)
{
	struct at_res_str at = {0};

	rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
  at = at_cmd_send_syn("AT+QURCCFG=\"urcport\",\"uart1\"\r\n", strlen("AT+QURCCFG=\"urcport\",\"uart1\"\r\n"), at_cmd_match_ok,5, NOT_RETRY);
  rt_sem_release(&at_info.send_sem);
    
  if(at.res == RT_EOK)
  {
		return 0;
  }
	
	return 1;
}


/**********************************
**	获取查看信号强度CSQ
***********************************/
uint8_t at_get_csq_syn(uint8_t *data)
{
	struct at_res_str at = {0};
	
	
	rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	//rt_kprintf("-- start get csq value\r\n");
	at = at_cmd_send_syn("AT+CSQ\r\n", rt_strlen("AT+CSQ\r\n"),at_cmd_get_csq,5,NOT_RETRY);
	
	rt_sem_release(&at_info.send_sem);
	
	if(at.res == RT_EOK)
	{
		*data = at.data[0];
		return 0;
	}
	else
	{
		return 1;
	}
}





/*****************************
**
*******************************/

uint8_t at_get_cgreg_syn(uint8_t *data)
{
	struct at_res_str at;
    
  *data = 0;

  rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	at = at_cmd_send_syn("AT+CEREG?\r\n", strlen("AT+CEREG?\r\n"), at_cmd_parse_cgreg,60, RETRY);
  rt_sem_release(&at_info.send_sem);

  if(at.res == RT_EOK)
  {
		*data = at.data[0];
    return 0;
  }

  return 1;
}




/*****************************
**获取GPRS网络附着状态
*******************************/

uint8_t at_get_cgatt_syn(uint8_t *data)
{
	struct at_res_str at;
    
  *data = 0;

  rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);

	at = at_cmd_send_syn("AT+CGATT?\r\n", strlen("AT+CGATT?\r\n"), at_cmd_parse_cgatt, 60, RETRY);
  rt_sem_release(&at_info.send_sem);
	
	if(at.res == RT_EOK)
  {
		*data = at.data[0];
        
		return 0;
  }

  return 1;
}




/*****************************
**
*******************************/

uint8_t at_activate_pdp(void)
{
	struct at_res_str at;
    
  rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	at = at_cmd_send_syn("AT+QIACT=1\r\n", strlen("AT+QIACT=1\r\n"),at_cmd_match_ok, 30, NOT_RETRY);
  rt_sem_release(&at_info.send_sem);

	if(at.res == RT_EOK)
  {
		return 0;
  }

  return 1;
}



/**************************
**	EC200S 获取本地IP地址
****************************/
static struct at_res_str  at_cmd_local_ip(const char *data)
{
	struct at_res_str at = {0};
  char *p = NULL;
  uint32_t tmp[4];

  p = strstr(data, "+QIACT: 1,1,1");
  if(p != NULL)
  {
			sscanf(p,"+QIACT: 1,1,1,\"%d.%d.%d.%d\"",&tmp[0],&tmp[1],&tmp[2],&tmp[3]);
      at.res = RT_EOK;
			at.data[0] = tmp[0];
      at.data[1] = tmp[1];
      at.data[2] = tmp[2];
      at.data[3] = tmp[3];
        
      return at;
   }

   at.res = RT_ETIMEOUT;
   
	 return at;
}

/**************************************
**  获取本地IP (激活pdp之后可执行)
***************************************/

uint8_t at_get_local_ip_syn(uint8_t *ip)
{
	struct at_res_str at = {0};

  rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
  at = at_cmd_send_syn("AT+QIACT?\r\n", strlen("AT+QIACT?\r\n"), at_cmd_local_ip, 5, NOT_RETRY);
  rt_sem_release(&at_info.send_sem);
    
  if(at.res == RT_EOK)
  {
		*(ip + 0) = at.data[0];
    *(ip + 1) = at.data[1];
    *(ip + 2) = at.data[2];
    *(ip + 3) = at.data[3];

    return 0;
   }

   return 1;
}






/******************************
**	解析创建socket时返回的状态
******************************/

static struct at_res_str at_cmd_parse_create_socket(const char *data)
{
  char *p = NULL;
	uint32_t tmp_id = 0;
	uint32_t tmp_err = 0;
	struct at_res_str at = {0}; 

  p = strstr(data, "+QIOPEN:");
	
  if(p != NULL)
  {
	    sscanf(p,"+QIOPEN: %d,%d\r\n",&tmp_id,&tmp_err);
        if(tmp_err == 0)
        {
            at.res = RT_EOK;
            return at;
        }
  }
  at.res = RT_ETIMEOUT;
    
  return at;
}




/********************************************************
 ** 检测Socket状态 ，完全分析（暂时这么使用）
*********************************************************/

uint8_t analysis_socket_state(uint8_t *source,uint16_t len,struct socket_state_t *sc)
{
    uint8_t 		*p = source;
    uint16_t 		i = 0;
    uint8_t			tmp_s[60] = {0};
		
    if(p == NULL)
        return 0;

    sscanf((char *)p,"+QISTATE: %d,",&sc->connect_id);
    i = get_data_str(1,2,p,tmp_s,len);    //网关地址
    if(i > 0)
    {
        tmp_s[i] = '\0';
        memcpy(sc->service_type,tmp_s + 1, i - 2);          	//
    }

    i = get_data_str(2,3,p,tmp_s,len);              //IP地址
    if(i > 0)
    {
        tmp_s[i] = '\0';
        memcpy(sc->ip_addr,tmp_s +1,i - 2);         			//
    }

    i = get_data_str(3,4,p,tmp_s,len);
    if(i > 0)
    {
        tmp_s[i] = '\0';
        sc->remote_port = (uint32_t)(fr_atof((const char *)tmp_s) *1);         			//休眠时间
    }

    i = get_data_str(4,5,p,tmp_s,len);
    if(i > 0)
    {
        tmp_s[i] = '\0';
        sc->local_prot = (uint32_t)(fr_atof((const char *)tmp_s) * 1);   			//定距上传
    }


    i = get_data_str(5,6,p,tmp_s,len);
    if(i > 0)
    {
        tmp_s[i] = '\0';
        sc->socket_state = (uint32_t)(fr_atof((const char *)tmp_s) * 1);  			//数据上传周期
    }
    return 0;
}


/*************************************
**  查询Socket链接状态
**************************************/

uint8_t at_query_socket_state(uint8_t id)
{
	 uint8_t                 array[128] = {0};
   uint16_t                w_len = 0;
   int32_t                 data_len = 0;
   int32_t                 rv = 0;
   uint8_t                 timeout_cnt = 5;  
   uint16_t                rb_return_len = 0;
	
  if(id == 0 || id > SOCKET_NUM)
		return 0;

  id -= 1;

	rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
	data_len = sprintf((char *)array, "AT+QISTATE=1,%d\r\n",id);
	
  w_len = rt_device_write(gprs_uart_dev, 0, array, data_len);
  if(w_len != data_len)
    rt_kprintf("-- gprs uart devic write error. w_len:%d, len:%d\r\n", (int)w_len, (int)data_len);
  rt_thread_delay(10);
    
	do
  {
		rb_return_len = rt_ringbuffer_data_len(&rb);
		if(tmp_offset + rb_return_len > RING_BUFF_SIZE)
    {
        tmp_offset = 0;
        rt_memset(tmp_buf, 0, RING_BUFF_SIZE);
    }
				
    w_len = rt_ringbuffer_get(&rb, tmp_buf, rb_return_len); /* 读取环形区的数据*/
    if(w_len != rb_return_len)
      rt_kprintf("- read ringbuffer data error:rb_rerurn_len %d w_len: %d\r\n", (int)rb_return_len, (int)w_len);
    if(w_len > 0)
    {
			data_len = list_for_str(tmp_buf,(unsigned char *)"+QISTATE:",w_len);
            
			if(data_len >= 0)
			{
        struct socket_state_t sct;
				analysis_socket_state(tmp_buf + data_len,w_len - data_len,&sct);
                //printf("-- socket state dfaf:%d\r\n",sct.socket_state);
        if(sct.socket_state == 1 || sct.socket_state == 2 || sct.socket_state == 4)
        {
					rv = 1;
          break;
        }           
			} 
     }
     rt_thread_delay(10); /* 没有等到串口数据时线程延迟指定时间后查询*/
     timeout_cnt--;
    }
    while (timeout_cnt > 0);
		
		rt_sem_release(&at_info.send_sem);
    rt_memset(tmp_buf, 0, sizeof(tmp_buf));
    if(timeout_cnt == 0)
        return 0;
		return rv;
}




/*************************************
**  创建Socket链接
**************************************/

uint8_t at_close_socket_connect(uint8_t id)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    if(id == 0 || id > SOCKET_NUM)
        return 1;

    id -= 1;

    rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
    sprintf((char *)tmp_buf, "AT+QICLOSE=%d\r\n",id);
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok, 40, NOT_RETRY);
    rt_sem_release(&at_info.send_sem);
    
    if(at.res == RT_EOK)
    {
        socket_qh[id] = NULL;
        return 0;
    }

    return 1;
}






/*************************************
**  创建Socket链接
**************************************/

uint8_t at_creat_socket_connect(uint8_t id,struct socket_addr_str *sa,rt_mq_t qh)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    if(id == 0 || id > SOCKET_NUM)
        return 1;

    if(sa == NULL)
        return 1;
    
    id -= 1;

    rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
    sprintf((char *)tmp_buf, "AT+QIOPEN=%d,%d,\"TCP\",\"%s\",%d,%d,%d\r\n", 1,id,sa->addr,sa->port,0,1);
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_parse_create_socket, 40, NOT_RETRY);
    rt_sem_release(&at_info.send_sem);
    
    if(at.res == RT_EOK)
    {
      socket_qh[id] = qh;
			rt_thread_delay(10);
        
			return 0;
    }


    return 1;
}




/***************************************
** 解析允许发送数据的标志
****************************************/

static struct at_res_str at_cmd_parse_send_flag(const char *data)
{
    char *p = NULL;
    struct at_res_str at = {0};

    p = strstr(data, ">");
    if (p != NULL)
    {
        at.res = RT_EOK;
        return at;
    }

    at.res = RT_EOK;

    return at;
}



/*************************************
**  创建Socket链接
**************************************/

uint8_t at_send_socket_data(uint8_t id,uint8_t *data,uint16_t len)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    if(id > SOCKET_NUM || id == 0)
        return 1;
    if(data == NULL || len > 1024)
        return 1;
    
    id -= 1;
    
    rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
    sprintf((char *)tmp_buf, "AT+QISEND=%d,%d\r\n",id,len);
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf),at_cmd_parse_send_flag, 40, NOT_RETRY);
    if(at.res == RT_EOK)
    {
        at = at_cmd_send_syn(data, len, at_cmd_match_ok, 40, NOT_RETRY);
    }
    rt_sem_release(&at_info.send_sem);
    
    if(at.res == RT_EOK)
    {
        lte_send_num++;
        return 0;
    }

    return 1;
}





/************************************
**  配置FTP用户名和地址
*************************************/

uint8_t at_config_ftp_account(struct ftp_info_str *sa)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    if(sa == NULL)
        return 1;
    
    rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
    sprintf((char *)tmp_buf, "AT+QFTPCFG=\"account\",\"%s\",\"%s\"\r\n", sa->user,sa->passwd);
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok, 40, NOT_RETRY);
    rt_sem_release(&at_info.send_sem);
    
    if(at.res == RT_EOK)
    {
        return 0;
    }

    return 1;
}





/************************************
**  配置FTP文件类型
*************************************/

uint8_t at_config_ftp_transmode(uint8_t n)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
    sprintf((char *)tmp_buf, "AT+QFTPCFG=\"transmode\",%d\r\n",n);
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok, 40, NOT_RETRY);
    rt_sem_release(&at_info.send_sem);
    
    if(at.res == RT_EOK)
    {
        return 0;
    }

    return 1;
}



/************************************
**  配置FTP 
*************************************/

uint8_t at_config_ftp_rsptimeout(uint8_t n)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
    sprintf((char *)tmp_buf, "AT+QFTPCFG=\"rsptimeout\",%d\r\n",n);
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok, 40, NOT_RETRY);
    rt_sem_release(&at_info.send_sem);
    
    if(at.res == RT_EOK)
    {
        return 0;
    }

    return 1;
}






/************************************
**  配置FTP文件类型
*************************************/

uint8_t at_config_ftp_file_type(uint8_t n)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};
    
    rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
    sprintf((char *)tmp_buf, "AT+QFTPCFG=\"filetype\",%d\r\n",n);
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok, 40, NOT_RETRY);
    rt_sem_release(&at_info.send_sem);
    
    if(at.res == RT_EOK)
    {
        return 0;
    }

    return 1;
}





/******************************************
**
**	解析创建socket时返回的状态
*******************************************/

static struct at_res_str at_cmd_parse_create_ftp(const char *data)
{
		char *p = NULL;
		uint32_t tmp_id = 0;
		uint32_t tmp_err = 0;
		struct at_res_str at = {0}; 

    p = strstr(data, "+QFTPOPEN:");
	
    if(p != NULL)
    {
	    sscanf(p,"+QFTPOPEN: %d,%d\r\n",&tmp_id,&tmp_err);
        if(tmp_err == 0)
        {
            at.res = RT_EOK;
            return at;
        }
    }
    at.res = RT_ETIMEOUT;
    
    return at;
}




/*************************************
**  创建FTP Socket链接
**************************************/

uint8_t at_creat_ftp_connect(struct ftp_info_str *sa)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
    sprintf((char *)tmp_buf, "AT+QFTPOPEN=\"%s\",%d\r\n",sa->host,sa->port);
    //printf("%s\r\n",tmp_buf);
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_parse_create_ftp,5, NOT_RETRY);
    rt_sem_release(&at_info.send_sem);
    
    if(at.res == RT_EOK)
    {
        return 0;
    }

    return 1;
}




/******************************************
**
**	解析创建socket时返回的状态
*******************************************/

static struct at_res_str at_cmd_parse_files_size(const char *data)
{
    char *p = NULL;
    uint32_t tmp;

		struct at_res_str at = {0}; 

    p = strstr(data, "+QFTPSIZE:");
	
    if(p != NULL)
    {
        sscanf(p, "+QFTPSIZE: 0,%d", &tmp);
        *(uint32_t *)at.data = tmp;
        at.res = RT_EOK;
        return at;
    }
    at.res = RT_ETIMEOUT;
    
    return at;
}



/*************************************
**  获取FTP文件大小
**************************************/

uint32_t at_get_ftp_files_size(uint8_t *fp)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    if(fp == NULL)
        return 1;
    
    rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
    sprintf((char *)tmp_buf, "AT+QFTPSIZE=\"%s\"\r\n",fp);
    //printf("%s\r\n",tmp_buf);
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_parse_files_size, 10, NOT_RETRY);
    rt_sem_release(&at_info.send_sem);
    
    if(at.res == RT_EOK)
    {
        return *(uint32_t *)at.data;
    }


    return 0;
}





/*************************************
**  下载文件数据
**************************************/

uint32_t at_download_ftp_files(uint8_t *fp,uint8_t *buf,uint32_t offset,uint16_t len)
{
    uint8_t                 array[128] = {0};
    uint16_t                w_len = 0;
    int32_t                 data_len = 0;
    int32_t                 rv = 0;
    uint16_t                timeout_cnt = 200;  
    char                    *s = NULL;
    uint16_t                rb_return_len = 0;

    if(fp == NULL || buf == NULL)
        return 0;
    
    
    rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
    data_len = sprintf((char *)array, "AT+QFTPGET=\"%s\",\"COM:\",%d,%d\r\n",fp,offset,len);
    w_len = rt_device_write(gprs_uart_dev, 0, array, data_len);
		if(w_len != data_len)
			rt_kprintf("-- gprs uart devic write error. w_len:%d, len:%d\r\n", (int)w_len, (int)data_len);
		rt_thread_delay(5);
    
		tmp_offset = 0;
    do
    {
			rb_return_len = rt_ringbuffer_data_len(&rb);
      w_len = rt_ringbuffer_get(&rb, tmp_buf + tmp_offset, rb_return_len);
			
			if(w_len > 0)
      {
				tmp_offset += w_len;
        data_len = list_for_str(tmp_buf,(unsigned char *)"+QFTPGET: 0",tmp_offset);
        //rt_kprintf("-- the run is 33  %d,%d\r\n",data_len,tmp_offset);
				if(data_len > 0)
				{
					sscanf((const char *)(tmp_buf + data_len),"+QFTPGET: 0,%d\r\n",&data_len);
         
					s = strstr((const char *)tmp_buf,"CONNECT") + 9;
					//rt_kprintf("-- the run is 44 %u,%d\r\n",(uint32_t)s,data_len);
					if(s != NULL)
					{
						if(data_len > 0 && data_len <= 4096)
						{
							memcpy(buf,s,data_len);
              rv = data_len;
              //rt_kprintf("-- run is 23 %d,%d\r\n",data_len,tmp_offset);
							break;
						}
						else 
						{
              rv = 0;
							break;
						}
					}
				} 
			}
			rt_thread_delay(10); //没有等到串口数据时线程延迟指定时间后查询
			timeout_cnt--;
    }while(timeout_cnt > 0);
    //printf("-- the get ftp file :%d\r\n",timeout_cnt);
    rt_sem_release(&at_info.send_sem);
		rt_memset(tmp_buf, 0, sizeof(tmp_buf));
    
    if(timeout_cnt == 0)
        return 0;

    return rv;
}





/******************************************
**  关闭FTP链接
*******************************************/

static struct at_res_str at_cmd_parse_close_ftp(const char *data)
{	
	char *p = NULL;
	struct at_res_str at = {0}; 

	//rt_kprintf("-- at ftp close parse:%s\r\n",data);
  p = strstr(data, "+QFTPCLOSE: 0,0");
	
  if(p != NULL)
  {
		at.res = RT_EOK;
        
		return at;
  }
    
	at.res = RT_ETIMEOUT;
    
  return at;
}


/*************************************
**  关闭FTP链接
**************************************/

uint8_t at_close_ftp_connect(void)
{
	struct at_res_str at = {0};
	uint8_t tmp_buf[128] = {0};
		
  rt_sem_take(&at_info.send_sem, RT_WAITING_FOREVER);
		
	sprintf((char *)tmp_buf, "AT+QFTPCLOSE\r\n");
  at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_parse_close_ftp,60, NOT_RETRY);  
	rt_sem_release(&at_info.send_sem);
    
  if(at.res == RT_EOK)
  {
     return 0;
  }

  return 1;
}




/******************************
**	关闭LTE模块
*******************************/

void close_lte_module(void)
{
	gprs_uart_dev = rt_device_find(RT_GPRS_DEVICE_NAME); 
	{
		rt_device_close(gprs_uart_dev);     //关闭串口
		gprs_uart_dev->flag &= 0xFFEF;        //重新初始化
	}
	
	rt_lte_pwk_high();
	rt_thread_delay(200);
	rt_lte_pwk_low();
	rt_thread_delay(100);
  rt_lte_power_off();   //关闭LTE电源
}






/*****************************************
**
*******************************************/

rt_err_t at_data_rx_handle(rt_device_t dev, rt_size_t size)
{
    at_info.uart_tick = rt_tick_get();
    at_info.data_len = size;
	
    return RT_EOK;
}


/****************************************
**
**	
*****************************************/
void at_tick_handle(void)
{
	if(at_info.uart_tick == 0xffffffff)
		return;

  if (rt_tick_get() - at_info.uart_tick > 1)
  {
    at_info.uart_tick = 0xffffffff;
    rt_sem_release(&at_info.at_cmd_sem);
  }
}




/******************************
**	设置LTE模块串口
*******************************/

void set_lte_com(void)
{
	struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
	
	gprs_uart_dev = rt_device_find(RT_GPRS_DEVICE_NAME); 
	config.baud_rate = BAUD_RATE_230400;
	config.bufsz = 4608;
	rt_device_control(gprs_uart_dev,RT_DEVICE_CTRL_CONFIG,&config); 					//RT_CAN_CMD_SET_BAUD
	rt_device_set_rx_indicate(gprs_uart_dev, at_data_rx_handle); 									// 添加回掉函数
  rt_irq_timer3_sethook(at_tick_handle);         
	rt_device_init(gprs_uart_dev);
  rt_device_open(gprs_uart_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);		//
}
	



/********************************************
**	AT命令解析任务
********************************************/
void thread_entry_at_parse(void *parameter)
{
		parameter = parameter;
		
    at_info.uart_tick = 0xffffffff;
		
    rt_ringbuffer_init(&rb, &rb_pool[0], sizeof(rb_pool)); 													// 初始化 ringbuffer
    gprs_uart_dev = rt_device_find(RT_GPRS_DEVICE_NAME);        										// 获取uart4句柄
    rt_device_set_rx_indicate(gprs_uart_dev, at_data_rx_handle); 									// 添加回掉函数
    rt_irq_timer3_sethook(at_tick_handle);                    										// 添加回掉函数
		
		rt_sem_init(&at_info.send_sem, "send_sem", 1, 0); 			 //初始化 gprs 发送数据的信号量
		rt_sem_init(&at_info.at_cmd_sem, "at_sem",0, 0); 																// 初始化AT命令信号量
		
		rt_device_init(gprs_uart_dev);
    rt_device_open(gprs_uart_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);		//
    

    for(;;)
    {
        if(rt_sem_take(&at_info.at_cmd_sem, RT_WAITING_FOREVER) != RT_EOK)
            continue;
				memset(at_buf, 0, sizeof(at_buf)); 
        at_info.data_len = rt_device_read(gprs_uart_dev,0,at_buf,sizeof(at_buf));
        at_recv_data((char *)at_buf,at_info.data_len);
				//rt_kprintf("-- %s\r\n",at_buf);
    }
}





