


#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"


#include "FreeRTOS.h"
#include "task.h"

#include "drv_uart.h"
#include "drv_timer.h"

#include "app_at.h"

#include "ringbuffer.h"
#include "common.h"
#include "pro_data.h"


#include "app_at.h"
#include "app_sms.h"





#define AT_BUFF_SIZE                    4224      /** AT命令接收缓冲区大小 **/

#define SOCKET_NUM                      4         /** 最大Socket链接数量 **/



/******************************* 本地全局变量  *******************************/

static struct rt_ringbuffer             rb = {0};

static uint8_t                          rb_pool[AT_BUFF_SIZE] = {0};

static uint8_t                          at_buf[AT_BUFF_SIZE] = {0};

static uint32_t                         at_len = 0;

static struct at_mutex_str              at_mutex = {0};

static QueueHandle_t                    socket_qh[4] = {NULL,NULL,NULL,NULL};


/**************************************************************/

static uint32_t                         tmp_offset = 0;

static uint8_t                          tmp_buf[AT_BUFF_SIZE] = {0};



/********************** LTE发送数据状态 ****************************/

static  uint16_t                         lte_send_num;




/**************  *****************/

struct at_info					at = {0};

static uint8_t 					data_buff[1024] = {0};





/********************************************************
 * @desc: 改为同步方式
 * @param:buff:待发送数据 len:待发送数据长度 func:回调函数 timeout:设置超时时间 retry:是否间隔1S重发 
 * @return:at 执行结果和数据
 * @date:2021/06/12
*********************************************************/
static struct at_res_str at_cmd_send_syn(void *buff, uint16_t len,struct at_res_str (*func)(const char *),uint16_t timeout_s, retry_flag flag)
{
	uint32_t rb_return_len = 0, w_len = 0;
  int timeout_cnt = 0;
  uint16_t pitch_time = 0;
  struct at_res_str  at_cmd_res;

  pitch_time = 1000 / configTICK_RATE_HZ;
  timeout_cnt = timeout_s * configTICK_RATE_HZ;
  xSemaphoreTake(at_mutex.cmd_mutex,portMAX_DELAY);
    
	if(buff != NULL)
  {
      w_len = rt_write_uart_buf(4,buff,len);
      if(w_len != 0)
         printf("-- (1) gprs uart devic write error. w_len:%d, len:%d\r\n", (int)w_len, (int)len);
      vTaskDelay(10 / pitch_time);// 10ms
  }
  
	do
  {
		rb_return_len = rt_ringbuffer_data_len(&rb);
    if(tmp_offset + rb_return_len > LTE_BUFF_SIZE)  
    {
       tmp_offset = 0;
       memset(tmp_buf, 0, AT_BUFF_SIZE);
    }
    w_len = rt_ringbuffer_get(&rb, tmp_buf + tmp_offset, rb_return_len);
    if(w_len > 0)
    {
			at_cmd_res = func((const char *)(tmp_buf + tmp_offset));	
      tmp_offset += w_len;
      if(at_cmd_res.res == RES_OK)
      {
                tmp_offset = 0;
                memset(tmp_buf, 0, AT_BUFF_SIZE);
                xSemaphoreGive(at_mutex.cmd_mutex);								
                return at_cmd_res;
      }
      else
      {
         if(flag) 
         {
            if(buff != NULL)
						{
              w_len = rt_write_uart_buf(4,buff,len);
							if(w_len != 0)
									 printf("-- (2) gprs uart devic write error. w_len:%d, len:%d\r\n", (int)w_len, (int)len);		
						}
            tmp_offset -= w_len;
            vTaskDelay(1000/pitch_time); 
            if(timeout_cnt > configTICK_RATE_HZ)
                timeout_cnt -= (configTICK_RATE_HZ - 1);
            else
                timeout_cnt = 0;
            continue;
          }
        }
      }
      vTaskDelay(10 / pitch_time); 
      timeout_cnt--;
    }
    while (timeout_cnt > 0);
    tmp_offset = 0;
    memset(tmp_buf, 0, AT_BUFF_SIZE);
		xSemaphoreGive(at_mutex.cmd_mutex);	
    at_cmd_res.res = RES_TIMEOUT;	
    return at_cmd_res;
}



/****************************************
**
****************************************/

uint16_t get_lte_send_num(void)
{
    uint16_t rv;

    rv = lte_send_num;

    return rv;
}





/**************************************
**
**************************************/
 
void at_rx_data_handle(uint16_t size)
{
    at.ticks = xTaskGetTickCount(); /* 取收到数据时的tick值*/
    at.len = size;          /* 当前的接收的数据长度*/
}

/********************************************************
**
********************************************************/
void at_ticks_handle(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if(at.ticks == 0xffffffff)
		return;

  if(xTaskGetTickCount() - at.ticks > 0)
  {
		at.ticks = 0xffffffff;     /* tick值复位*/
      
		if(at.semaphore != NULL)
			xSemaphoreGiveFromISR(at.semaphore, &xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}



/*********************************
**	L716  RDY标志
**********************************/

static struct at_res_str at_cmd_ready(const char *data)
{
    struct at_res_str at = {0};
    char *p = NULL;

    p = strstr(data, "AT command ready");
    if (p != NULL)
    {
        at.res = RES_OK;
        return at;
    }
    at.res = RES_TIMEOUT;
    return at;
}




/*******************************
**
**********************************/

int8_t  at_wait_cmd_ok_syn(void)
{
	struct at_res_str at;
    
  xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
  at = at_cmd_send_syn(NULL, 0, at_cmd_ready, 40, NOT_RETRY);
  xSemaphoreGive(at_mutex.send_sem);

	if(at.res == RES_OK)
		return  RES_OK;
	else
		return  RES_ERROR;
}



/******************************
**	获取ccid
*******************************/

static struct at_res_str at_cmd_match_ok(const char *data)
{
	struct at_res_str at = {0};

	char *p = NULL;
  
  p = strstr(data, "OK");
  if(p != NULL)
  {
    at.res = RES_OK;
		return at;
  }

  p = strstr(data, "ERROR");
  if(p != NULL)
  {
		at.res = RES_ERROR;
		return at;
  }
    
  at.res = RES_TIMEOUT;

	 return at;
}




/***********************************

** 回显控制指令  0 回显方式关闭;  1 回显方式打开

************************************/

uint8_t at_ctrl_echo_syn(void)
{
	struct at_res_str at;

  xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	at = at_cmd_send_syn("ATE0\r\n", strlen("ATE0\r\n"), at_cmd_match_ok,30, NOT_RETRY);
  xSemaphoreGive(at_mutex.send_sem);
    
  if(at.res == RES_OK)
		return  RES_OK;
	else
		return  RES_ERROR;
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
        at.res = RES_OK;
        //printf("--IMEI:%s  %d\r\n",data,len);
        return at;
			}
  }

  at.res = RES_TIMEOUT;
   
	return at;
}




/******************************
**	获取IMEI
*******************************/

uint8_t at_get_imei_syn(uint8_t *source,uint8_t len)
{
	struct at_res_str at;

  xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	at = at_cmd_send_syn("AT+GSN\r\n", strlen("AT+GSN\r\n"), at_cmd_parse_imei, 30, NOT_RETRY);
  xSemaphoreGive(at_mutex.send_sem);
    
  if(at.res == RES_OK)
  {
		memcpy(source,at.data,at.len);
    return 0;
  }

  return 1;
}





/****************************
**	解析获取的ICCID
*****************************/

static struct at_res_str at_cmd_parse_iccid(const char *data)
{
    struct at_res_str at = {0};
    char *p = NULL;

    p = strstr(data, "+CCID:");
    if (p != NULL)
    {
        sscanf(p, "+CCID: %s", at.data);
        at.res = RES_OK;
        at.len = 20;
   
        return at;
    }
    
    at.res = RES_TIMEOUT;

    return at;
}



/**************************************
**  获取ICCID
**************************************/

uint8_t  at_get_ccid_syn(uint8_t *source,uint8_t size)
{
	struct at_res_str at;
    
  xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	at = at_cmd_send_syn("AT+CCID\r\n", strlen("AT+CCID\r\n"), at_cmd_parse_iccid, 30, NOT_RETRY);
  xSemaphoreGive(at_mutex.send_sem);
    
  if(at.res == RES_OK)
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
        at.res = RES_OK;
		return at;
    }
		
	at.res = RES_TIMEOUT;
		
    return at;
}


/*****************************
**
*******************************/

uint8_t at_get_sim_syn(uint8_t *data)
{
	struct at_res_str at;
    
  *data = 0;

  xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	at = at_cmd_send_syn("AT+CPIN?\r\n", strlen("AT+CPIN?\r\n"), at_cmd_sim_state, 30, NOT_RETRY);
  xSemaphoreGive(at_mutex.send_sem);
    
  if(at.res == RES_OK)
  {
		*data = 1;
     
		return 0;
  }

  return 1;
}


/*****************************
**
*******************************/

uint8_t at_set_cgreg_syn(void)
{
	struct at_res_str at;
    
  xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	at = at_cmd_send_syn("AT+CGREG=1\r\n", strlen("AT+CGREG=1\r\n"), at_cmd_match_ok, 30, NOT_RETRY);
  xSemaphoreGive(at_mutex.send_sem);
  
	if(at.res == RES_OK)
  {
		return 0;
  }

   return 1;
}



/*************************************
**  
**************************************/

uint8_t at_set_apn_syn(void)
{
	struct at_res_str at;
    
  xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	memset(at.data,'\0',sizeof(at.data));
	sprintf((char *)at.data, "AT+MIPCALL=1,\"%s\"\r\n", "CMNET");
	at = at_cmd_send_syn(at.data, strlen(at.data), at_cmd_match_ok, 30, NOT_RETRY);
  xSemaphoreGive(at_mutex.send_sem);

  if(at.res == RES_OK)
  {
		return 0;
  }

  return 1;
}






/******************************
**	获取LTE模块信号值
*******************************/

static struct at_res_str at_cmd_get_csq(const char *data)
{
	char *p = NULL;
    struct at_res_str at = {0};
	uint32_t csq = 0;
    uint32_t dbm = 0;

	p = strstr(data, "+CSQ:");
      
	if (p != NULL)
    {
		sscanf(p, "+CSQ:%d,%d",&csq,&dbm); 
        at.data[0] = (uint8_t)csq;
        at.data[1] = (uint8_t)dbm;

        at.res = RES_OK;
		return at;		
    }
	
    at.res = RES_TIMEOUT;
	return at;
}


/**************************************
**  获取信号值
***************************************/

uint8_t at_get_csq_syn(uint8_t *data)
{
    struct at_res_str at = {0};

    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
    at = at_cmd_send_syn("AT+CSQ\r\n", strlen("AT+CSQ\r\n"), at_cmd_get_csq, 5, NOT_RETRY);
    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
    {   
        *(data) = at.data[0];
        
        return 0;
    }

    return 1;
}



/*************************************
**  设置SMS  AT+CMGF=1
**************************************/

uint8_t at_config_sms_fromat(void)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    
    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
    sprintf((char *)tmp_buf, "AT+CMGF=1\r\n");
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok,5, NOT_RETRY);
    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
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
    uint8_t tmp_buf[128] = {0};

    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
    sprintf((char *)tmp_buf, "AT+CNMI=1,2,0,1,0\r\n");
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok,5, NOT_RETRY);
    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
    {
        return 0;
    }

    return 1;
}




/****************************************
**	解析获取的网络注册状态
****************************************/
static struct at_res_str at_cmd_parse_cgreg(const char *data)
{
    struct at_res_str at = {0};
    char *p = NULL;
	uint32_t m;
	
    p = strstr(data, "+CGREG:");
    if(p != NULL)
    {
        sscanf(p, "+CGREG: %*d,%d", &m);
        if((m == 1) || (m == 5))
        {
            at.res = RES_OK;
            at.data[0] = m;
			return at;
        }
    }
		
    at.res = RES_TIMEOUT;

    return at;
}





/*****************************
**
*******************************/

uint8_t at_get_cgreg_syn(uint8_t *data)
{
	struct at_res_str at;
    
  *data = 0;

  xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	at = at_cmd_send_syn("AT+CGREG?\r\n", strlen("AT+CGREG?\r\n"), at_cmd_parse_cgreg, 5, NOT_RETRY);
  xSemaphoreGive(at_mutex.send_sem);

  if(at.res == RES_OK)
  {
		*data = at.data[0];
        
		return 0;
  }

  return 1;
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
            at.res = RES_OK;
            at.data[0] = 1;
            return at;
        }
    }
	
    at.res = RES_TIMEOUT;

    return at;
}




/*****************************
**
*******************************/

uint8_t at_get_cgatt_syn(uint8_t *data)
{
	struct at_res_str at;
    
  *data = 0;

  xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	at = at_cmd_send_syn("AT+CGATT?\r\n", strlen("AT+CGATT?\r\n"), at_cmd_parse_cgatt, 30, NOT_RETRY);
  xSemaphoreGive(at_mutex.send_sem);

  if(at.res == RES_OK)
  {
		*data = at.data[0];
        
		return 0;
  }

  return 1;
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

    at.res = RES_OK;
		
		return at;
  }
	
  at.res = RES_TIMEOUT;
    
	return at;
}




/*****************************
**  读取网络时间
******************************/

uint8_t at_get_lte_cclk(struct rt_tm *tt)
{
	struct at_res_str at = {0};

  xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
  at = at_cmd_send_syn("AT+CCLK?\r\n", strlen("AT+CCLK?\r\n"), at_cmd_parse_cclk, 60, RETRY);
  xSemaphoreGive(at_mutex.send_sem);
    
  if(at.res == RES_OK)
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




/**************************
**	L716 获取本地IP地址
****************************/
static struct at_res_str  at_cmd_local_ip(const char *data)
{
	struct at_res_str at = {0};
  char *p = NULL;
  uint32_t tmp[4];

  p = strstr(data, "+MIPCALL: 1");
  if(p != NULL)
  {
		sscanf(p,"+MIPCALL: 1,%d.%d.%d.%d\r\n",&tmp[0],&tmp[1],&tmp[2],&tmp[3]);
    at.res = RES_OK;
		at.data[0] = tmp[0];
    at.data[1] = tmp[1];
    at.data[2] = tmp[2];
    at.data[3] = tmp[3];
        
    return at;
  }

  at.res = RES_TIMEOUT;
    
	return at;
}





/**************************************
**  获取本地IP (激活pdp之后可执行)
***************************************/

uint8_t at_get_local_ip_syn(uint8_t *ip)
{
	struct at_res_str at = {0};

  xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
  at = at_cmd_send_syn("AT+MIPCALL?\r\n", strlen("AT+MIPCALL?\r\n"), at_cmd_local_ip, 10, NOT_RETRY);
  xSemaphoreGive(at_mutex.send_sem);
    
  if(at.res == RES_OK)
  {
		*(ip + 0) = at.data[0];
    *(ip + 1) = at.data[1];
    *(ip + 2) = at.data[2];
    *(ip + 3) = at.data[3];

    return 0;
  }

  return 1;
}



uint8_t at_ctrl_at_syn(void)
{
	struct at_res_str at;

  xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	at = at_cmd_send_syn("AT\r\n", strlen("AT\r\n"), at_cmd_match_ok,30, NOT_RETRY);
  xSemaphoreGive(at_mutex.send_sem);
    
  if(at.res == RES_OK)
		return  0;
	else
		return  1;
}


/********************************************************
 * @desc: lte下行处理
 * @param:
 * @return: 
 * @date:2021/06/13
*********************************************************/
static uint8_t at_recv_data(const char *data, uint16_t rev_len)
{
	int id = 0, len = 0;
  char *p = NULL;
  char *index = NULL;
	char *p_end = NULL;
	uint16_t save_len = 0;
  int offset = 0;
	char buf[32] = {0};
  
  index = (char *)data;
	
  do
  {
    p = strstr(index, "+MIPRTCP: ");   //接收到数据
    if(p != NULL)
    {
			struct socket_down_str tmp;

			p_end = p; 
			if (p_end > index)
			{
				save_len = p_end - index;
				rt_ringbuffer_put(&rb, (uint8_t *)index, save_len);														 
			}	
			sscanf(p, "+MIPRTCP: %*d,%d",&len);
			if(len < 10)
        offset = 1;
      else if (len < 100)
        offset = 2;
      else if (len < 1000)
        offset = 3;
      else
        offset = 4;
      p += (13 + offset); 			//跳过关键字,指定数据指针
      tmp.len = len;
      memcpy(tmp.data,p,len);
      if(socket_qh[id] != NULL)
      {
         //printf("-- Socket Send Queue......%d\r\n",id);
         xQueueSend(socket_qh[id],&tmp,sizeof(struct socket_down_str));  //发送队列
      }
			index = p + len;
    }
		
		p = strstr(data, "+MIPSTAT:");  //服务器断开链接
		if(p != NULL)
		{
			struct socket_down_str tmp;

		 	sscanf(p, "+MIPSTAT: %d,%d,", &id, &len);
      if(socket_qh[id] != NULL)
      {
				memcpy(tmp.data,"+MIPSTAT:",sizeof("+MIPSTAT:"));
        tmp.len = sizeof("+MIPSTAT:");
        xQueueSend(socket_qh[id],&tmp,sizeof(struct socket_down_str));  //发送队列
      }
		}  
  }
  while (p != NULL);
	p_end = (char *)data + rev_len;
  if(p_end > index)
	{
    save_len = p_end - index;
		rt_ringbuffer_put(&rb, (uint8_t *)index, save_len);
	}
    
	return RES_OK;
}



/*********************************
**	AT 命令解析线程
**********************************/

void thread_entry_at(void *parameter)
{
	uint16_t len = 0;
	
  parameter = parameter;
	
	at.semaphore = xSemaphoreCreateBinary();
	
	if(at.semaphore == NULL)
  {
		printf("-- creat packet sph fail....\r\n");
  }
	
	rt_irq_tim3_sethook(at_ticks_handle);
	rt_irq_uart_sethook(4,at_rx_data_handle);
	
	rt_ringbuffer_init(&rb, &rb_pool[0], sizeof(rb_pool)); 
  at_mutex.cmd_mutex = xSemaphoreCreateMutex();
  xSemaphoreGive(at_mutex.cmd_mutex);
  at_mutex.send_sem = xSemaphoreCreateMutex();
  xSemaphoreGive(at_mutex.send_sem);
	
	for(;;)
	{
		if(xSemaphoreTake(at.semaphore, 10000) == pdTRUE)
		{
			memset(data_buff,'\0',sizeof(data_buff));
			len =  rt_read_uart_buf(4, data_buff, at.len);
			if(len > 0)
			{
				//printf("%s",data_buff);
				at_recv_data((const char *)data_buff, len);
			}
		}
	}
}


