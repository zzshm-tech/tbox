





#include <stdio.h>
#include <string.h>



#include <time.h>


#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_task_wdt.h"
#include <esp_event.h>

#include "pro_data.h"
#include "common.h"
#include "drv_uart.h"
#include "drv_rtc.h"

#include "app_sms.h"

#include "ringbuffer.h"
#include "app_at.h"



#define NTP_SERVER_ADDR 			    "ntp1.aliyun.com"    /** LTE模块类型 **/

#define AT_BUFF_SIZE                    4224      /** AT命令接收缓冲区大小 **/

#define SOCKET_NUM                      4         /** 最大Socket链接数量 **/


static uint8_t                          lte_type = 0;

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




/*************************
 * 返回ELTE模块类型
 **************************/

uint8_t read_lte_type(void)
{
    uint8_t rv = 0;

    rv = lte_type;

    return rv;
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



/********************************************************
 ** 检测Socket状态 ，完全分析（暂时这么使用）
*********************************************************/

uint8_t analysis_socket_state(uint8_t *source,uint16_t len,struct socket_state_t *sc)
{
    uint8_t 		*p = source;
    uint8_t			tmp_s[60] = {0};
    uint16_t 		i = 0;
    if(p == NULL)
        return 0;

    if(lte_type == 0)
    {
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
    }
    else
    {
        sprintf((char *)tmp_s,"%d",sc->connect_id);
        if(list_for_str(source,tmp_s,len) >= 0)
        {
            sc->socket_state = 0;
        }
        else
        {
            sc->socket_state = 2;
        }
    }

    return 0;
}



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

    pitch_time = 1000 / CONFIG_FREERTOS_HZ;
    timeout_cnt = timeout_s * CONFIG_FREERTOS_HZ;
    xSemaphoreTake(at_mutex.cmd_mutex,portMAX_DELAY);
    
    if (buff != NULL)
    {
        w_len = write_data_to_uart1(buff,len);
        if (w_len != 0)
            printf("-- (1) gprs uart devic write error. w_len:%d, len:%d\r\n", (int)w_len, (int)len);
        vTaskDelay(10 / pitch_time);// 10ms
    }
    
    do
    {
        rb_return_len = rt_ringbuffer_data_len(&rb);
        if (tmp_offset + rb_return_len > LTE_BUFF_SIZE)  
        {
            tmp_offset = 0;
            memset(tmp_buf, 0, AT_BUFF_SIZE);
        }
        w_len = rt_ringbuffer_get(&rb, tmp_buf + tmp_offset, rb_return_len);
        
        if (w_len > 0)
        {
            at_cmd_res = func((const char *)(tmp_buf + tmp_offset));	
            tmp_offset += w_len;
            if (at_cmd_res.res == RES_OK)
            {
                tmp_offset = 0;
                memset(tmp_buf, 0, AT_BUFF_SIZE);
                xSemaphoreGive(at_mutex.cmd_mutex);								
                return at_cmd_res;
            }
            else
            {
                if (flag) 
                {
                    if (buff != NULL)
					{
                        write_data_to_uart1(buff,len);
					}
                    tmp_offset -= w_len;
                    vTaskDelay(1000/pitch_time); 
                    if (timeout_cnt > CONFIG_FREERTOS_HZ)
                        timeout_cnt -= (CONFIG_FREERTOS_HZ - 1);
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



/*********************************
**	EC600S  RDY标志
**********************************/

static struct at_res_str at_cmd_ready(const char *data)
{
    struct at_res_str at = {0};
    char *p = NULL;

    p = strstr(data, "RDY");
    if (p != NULL)
    {
        at.res = RES_OK;
        lte_type = 0;
        return at;
    }

    p = strstr(data, "AT command ready");
    if (p != NULL)
    {
        at.res = RES_OK;
        lte_type = 1;
        return at;
    }

    at.res = RES_TIMEOUT;
    
    return at;
}





/******************************
**	解析IMEI
*******************************/

static struct at_res_str at_cmd_parse_imei(const char *data)
{
    struct at_res_str at = {0};

	char *p_start =_NULL;
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
**	获取ccid
*******************************/

static struct at_res_str at_cmd_match_ok(const char *data)
{
    struct at_res_str at = {0};

	char *p = NULL;
  
    p = strstr(data, "OK");

    if (p != NULL)
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
        at.res = RES_OK;

		return at;
    }

    at.res = RES_TIMEOUT;

	return at;
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



/****************************
**	解析获取的ICCID
*****************************/

static struct at_res_str at_cmd_parse_iccid(const char *data)
{
    struct at_res_str at = {0};
    char *p = NULL;

    p = strstr(data, "+QCCID:");
    if (p != NULL && lte_type == 0)
    {
        sscanf(p, "+QCCID: %s", at.data);
        at.res = RES_OK;
        at.len = 20;
   
        return at;
    }
   
    p = strstr(data, "+CCID:");
    if (p != NULL && lte_type == 1)
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

    if(lte_type  == 0)
	    at = at_cmd_send_syn("AT+QCCID\r\n", strlen("AT+QCCID\r\n"), at_cmd_parse_iccid, 30, NOT_RETRY);
    else
        at = at_cmd_send_syn("AT+CCID\r\n", strlen("AT+CCID\r\n"), at_cmd_parse_iccid, 30, NOT_RETRY);

    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
    {
        memcpy(source,at.data,at.len);
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
   
    if(lte_type == 0)
	{
        at = at_cmd_send_syn("AT+QICSGP=1,1,\"CMNET\",\"\",\"\",1\r\n", strlen("AT+QICSGP=1,1,\"CMNET\",\"\",\"\",1\r\n"), at_cmd_match_ok, 30, NOT_RETRY);
    }   
    else 
    {
        sprintf((char *)at.data, "AT+MIPCALL=1,\"%s\"\r\n", "CMNET");
        at = at_cmd_send_syn(at.data, strlen(at.data), at_cmd_match_ok, 30, NOT_RETRY);
    }    

    xSemaphoreGive(at_mutex.send_sem);

    if(at.res == RES_OK)
    {
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
        if(lte_type == 0)
            tm.year = rt_mktime(&tm) + 28800;         //GNSS
        else 
            tm.year = rt_mktime(&tm);         //GNSS
        
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



/***************************************************************************************
** 
***************************************************************************************/
/* 回显控制指令  0 回显方式关闭;  1 回显方式打开 */
uint8_t at_ctrl_at_syn(void)
{
    struct at_res_str at = {0}; 
    
    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	at = at_cmd_send_syn("AT\r\n", strlen("AT\r\n"), at_cmd_match_ok, 2, NOT_RETRY);
    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
        return 0;
    else 
        return 1;
}


/*******************************
**
**********************************/

uint8_t  at_wait_cmd_ok_syn(void)
{
    struct at_res_str at;
    
    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
    at = at_cmd_send_syn(NULL, 0, at_cmd_ready, 60, NOT_RETRY);
    xSemaphoreGive(at_mutex.send_sem);
	if(at.res == RES_OK)
    {
        return at.data[0];
    }
    else 
    {
        return 2;
    }

}





/*****************************
** 设置LTE模块波特率
******************************/

uint8_t at_set_lte_ipr(void)
{
    struct at_res_str at = {0};

    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
    printf("-- The run is ........  (1) \r\n");
    at = at_cmd_send_syn("AT+IPR=115200\r\n", strlen("AT+IPR=115200\r\n"), at_cmd_match_ok, 5, NOT_RETRY);
    xSemaphoreGive(at_mutex.send_sem);
    
    printf("-- The run is ........  (3) \r\n");
    if(at.res == RES_OK)
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

  xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
  at = at_cmd_send_syn("AT&W\r\n", strlen("AT&W\r\n"), at_cmd_match_ok, 5, NOT_RETRY);
  xSemaphoreGive(at_mutex.send_sem);
    
  if(at.res == RES_OK)
  {
    return 0;
  }

  return 1;
}


/***************************************************************************************
*                                  AT命令发送
***************************************************************************************/
/* 回显控制指令  0 回显方式关闭;  1 回显方式打开 */
uint8_t at_ctrl_echo_syn(void)
{
    struct at_res_str at;

    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	at = at_cmd_send_syn("ATE0\r\n", strlen("ATE0\r\n"), at_cmd_match_ok, 2, NOT_RETRY);
    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
		return  0;
	else
		return  1;
       
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




/*****************************
**
*******************************/

uint8_t at_get_sim_syn(uint8_t *data)
{
    struct at_res_str at;
    
    *data = 0;

    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	at = at_cmd_send_syn("AT+CPIN?\r\n", strlen("AT+CPIN?\r\n"), at_cmd_sim_state, 60, RETRY);
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

uint8_t at_set_cereg_syn(void)
{
    struct at_res_str at;
    
    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	at = at_cmd_send_syn("AT+CEREG=1\r\n", strlen("AT+CEREG=1\r\n"), at_cmd_match_ok, 30, NOT_RETRY);
    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
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

    if(lte_type == 0)
    {
        p = strstr(data, "+QIACT: 1,1,1");
        if (p != NULL)
        {
		    sscanf(p,"+QIACT: 1,1,1,\"%d.%d.%d.%d\"",&tmp[0],&tmp[1],&tmp[2],&tmp[3]);
            at.res = RES_OK;
		    at.data[0] = tmp[0];
            at.data[1] = tmp[1];
            at.data[2] = tmp[2];
            at.data[3] = tmp[3];
        
            return at;
        }
    }
    else 
    {
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







/******************************
**	+QNTP: 0,"2023/09/23,06:52:18+32"
*******************************/

struct at_res_str at_cmd_ntp_res(const char *data)
{
	struct at_res_str at = {0};
	char *p = NULL;
    struct rt_tm tm;
    struct rt_tm *pt;


    //printf("-- ntp ntp ntp ntp ntp ntp ntp ::::::::  %s\r\n",data);
	if(lte_type == 0)
	{
        p = strstr(data, "+QNTP: 0");
        if (p != NULL)
        {
            sscanf(p, "+QNTP: 0,\"%d/%d/%d,%d:%d:%d+",&tm.year,&tm.mon,&tm.day,&tm.hour,&tm.min,&tm.sec);

            tm.year -= 2000;
            tm.year = rt_mktime(&tm) + 28800;         //GNSS
        
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
    }
    else
    {
        p = strstr(data, "+MIPNTP: 1");
        if (p != NULL)
        {
            at.res = RES_OK;
            return at;
        }
    }
    
	at.res = RES_TIMEOUT;
	
	return at;
}




/******************************
**  通过NPT获取时间
******************************/

uint8_t at_set_ntp_server(void)
{
    struct at_res_str at = {0};

    memset(at.data,'\0',sizeof(at.data));
    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);

    if(lte_type == 0)
    {
        sprintf((char *)at.data, "AT+QNTP=%d,\"%s\",%d\r\n",1,NTP_SERVER_ADDR,123);
        at = at_cmd_send_syn((char *)at.data, strlen(at.data), at_cmd_match_ok, 30, NOT_RETRY);
    }
    else
    {
        sprintf((char *)at.data, "AT+MIPNTP=\"%s\",%d\r\n", NTP_SERVER_ADDR,123);
	    //rt_kprintf("-- %s\r\n",at.data);
	    at = at_cmd_send_syn(at.data, strlen(at.data), at_cmd_match_ok, 30, NOT_RETRY);
    }

    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
    {
        return 0;
    }
    return 1;
}





/**************************************
**  获取本地IP (激活pdp之后可执行)
***************************************/

uint8_t at_get_local_ip_syn(uint8_t *ip)
{
    struct at_res_str at = {0};

    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);

    if(lte_type == 0)
        at = at_cmd_send_syn("AT+QIACT?\r\n", strlen("AT+QIACT?\r\n"), at_cmd_local_ip, 5, NOT_RETRY);
    else
        at = at_cmd_send_syn("AT+MIPCALL?\r\n", strlen("AT+MIPCALL?\r\n"), at_cmd_local_ip, 5, NOT_RETRY);

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


/****************************************
**	解析获取的网络注册状态
****************************************/
static struct at_res_str at_cmd_parse_cgreg(const char *data)
{
    struct at_res_str at = {0};
    char *p = NULL;
	uint32_t m;
	
    p = strstr(data, "+CEREG:");
    if(p != NULL)
    {
        sscanf(p, "+CEREG: %*d,%d", &m);
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

uint8_t at_get_cereg_syn(uint8_t *data)
{
    struct at_res_str at;
    
    *data = 0;

    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	at = at_cmd_send_syn("AT+CEREG?\r\n", strlen("AT+CEREG?\r\n"), at_cmd_parse_cgreg,60, RETRY);
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
	at = at_cmd_send_syn("AT+CGATT?\r\n", strlen("AT+CGATT?\r\n"), at_cmd_parse_cgatt, 60, RETRY);
    xSemaphoreGive(at_mutex.send_sem);

    if(at.res == RES_OK)
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

    if(lte_type == 0)
    {
        xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
	    at = at_cmd_send_syn("AT+QIACT=1\r\n", strlen("AT+QIACT=1\r\n"),at_cmd_match_ok, 30, NOT_RETRY);
        xSemaphoreGive(at_mutex.send_sem);

        if(at.res == RES_OK)
        {
            return 0;
        }
        return 1;
    }
    else 
    {
        return 0;
    }
}




/******************************************
**
**	解析创建socket时返回的状态
*******************************************/

static struct at_res_str at_cmd_parse_create_socket(const char *data)
{
    char *p = NULL;
	uint32_t tmp_id = 0;
	uint32_t tmp_err = 0;
	struct at_res_str at = {0}; 

    //printf("-- -- -- %s\r\n",data);
    if(lte_type == 0)
        p = strstr(data, "+QIOPEN:");
	else
        p = strstr(data, "+MIPOPEN:");
    

    if(p != NULL)
    {
        if(lte_type == 0)
	    {
            sscanf(p,"+QIOPEN: %d,%d\r\n",&tmp_id,&tmp_err);
            if(tmp_err == 0)
            {
                at.res = RES_OK;
                return at;
            }
        }
        else
        {
            sscanf(p,"+MIPOPEN: %d,%d\r\n",&tmp_id,&tmp_err);
            if(tmp_err == 1)
            {
                at.res = RES_OK;
                return at;
            }
        }
    }
    at.res = RES_TIMEOUT;
    
    return at;
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

    if(lte_type == 0)
        p = strstr(data, "+QFTPOPEN:");
	else
        p = strstr(data, "+FTPOPEN:");

    if(p != NULL)
    {
        if(lte_type == 0)
	    {
            sscanf(p,"+QFTPOPEN: %d,%d\r\n",&tmp_id,&tmp_err);
            if(tmp_err == 0)
            {
                at.res = RES_OK;
                return at;
            }
        }
        else
        {
            sscanf(p,"+FTPOPEN: %d\r\n",&tmp_err);
            if(tmp_err == 1)
            {
                at.res = RES_OK;
                return at;
            }
        }
    }
    at.res = RES_TIMEOUT;
    
    return at;
}



/******************************************
**
**	解析创建socket时返回的状态
*******************************************/

static struct at_res_str at_cmd_parse_list_files(const char *data)
{
    char *p = NULL;
	struct at_res_str at = {0}; 

    p = strstr(data, "+QFTPLIST: 0");
	
    if(p != NULL)
    {
       
        at.res = RES_OK;
        return at;
    }
    at.res = RES_TIMEOUT;
    
    return at;
}



/******************************************
**
**	解析创建socket时返回的状态
*******************************************/

static struct at_res_str at_cmd_parse_files_size(const char *data)
{
    char *p = NULL;
    uint32_t len;
    uint32_t tmp;

	struct at_res_str at = {0}; 

    p = strstr(data, "+QFTPSIZE:");
	
    if(p != NULL)
    {
        sscanf(p, "+QFTPSIZE: 0,%d", &tmp);
        *(uint32_t *)at.data = tmp;
        at.res = RES_OK;
        return at;
    }

    p = strstr(data, "PM");
        
    if(p != NULL)
    {
            sscanf(p,"PM               %d ",&len);
        *(uint32_t *)at.data = len;
        at.res = RES_OK;
        return at;
    }
	
	p = strstr(data, "AM");
	
    if(p != NULL)
    {
        sscanf(p,"AM               %d ",&len);
        *(uint32_t *)at.data = len;
        at.res = RES_OK;
        return at;
    }

    p = strstr(data, "ftp ftp");
    if(p != NULL)
    {
        sscanf(p,"ftp ftp         %d ",&len);
        *(uint32_t *)at.data = len;
        at.res = RES_OK;
        return at;
    }

    at.res = RES_TIMEOUT;
    
    return at;
}




/******************************************
**  关闭FTP链接
*******************************************/

static struct at_res_str at_cmd_parse_close_ftp(const char *data)
{
    char *p = NULL;
	struct at_res_str at = {0}; 

    if(lte_type == 0)
        p = strstr(data, "+QFTPCLOSE: 0,0");
	else
        p = strstr(data, "+FTPCLOSE: 1");

    if(p != NULL)
    {
        at.res = RES_OK;
        return at;
    }
    at.res = RES_TIMEOUT;
    
    return at;
}





/*************************************
**  创建Socket链接
**************************************/

uint8_t at_creat_socket_connect(uint8_t id,struct socket_addr_str *sa,QueueHandle_t qh)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    if(id == 0 || id > SOCKET_NUM)
        return 1;

    if(sa == NULL)
        return 1;
    
    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
    if(lte_type == 0)
    {
        id -= 1;
        sprintf((char *)tmp_buf, "AT+QIOPEN=%d,%d,\"TCP\",\"%s\",%d,%d,%d\r\n", 1,id,sa->addr,sa->port,0,1);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_parse_create_socket, 40, NOT_RETRY);
    }
    else 
    {
        sprintf((char *)tmp_buf, "AT+MIPOPEN=%d,,\"%s\",%d,0\r\n\r\n",id,sa->addr,sa->port);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_parse_create_socket, 40, NOT_RETRY);
    }
    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
    {
        if(lte_type == 0)
            socket_qh[id] = qh;
        else
            socket_qh[id - 1] = qh;

        return 0;
    }


    return 1;
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

    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
    if(lte_type == 0)
    {
        id -= 1;
        sprintf((char *)tmp_buf, "AT+QICLOSE=%d\r\n",id);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok, 40, NOT_RETRY);
    }
    else
    {
        sprintf((char *)tmp_buf, "AT+MIPCLOSE=%d\r\n",id);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok, 40, NOT_RETRY);
    }

    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
    {
        socket_qh[id] = NULL;
        return 0;
    }


    return 1;
}



/*************************************
**  查询Socket链接状态
**  MC660
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

    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
    xSemaphoreTake(at_mutex.cmd_mutex,portMAX_DELAY);
    if(lte_type == 0)
    {
        id -= 1;
        data_len = sprintf((char *)array, "AT+QISTATE=1,%d\r\n",id);
    }
    else
    {
        data_len = sprintf((char *)array, "AT+MIPOPEN?\r\n");
    }
    
    w_len = write_data_to_uart1(array,data_len);
   

    if(w_len != 0)
        printf("-- (2) gprs uart devic write error. w_len:%d\r\n", (int)w_len);
    vTaskDelay(10);// 10ms

    do
    {
        rb_return_len = rt_ringbuffer_data_len(&rb);
        //printf("-- the run is 22  %d,%d \r\n",rb_return_len,tmp_offset);
        w_len = rt_ringbuffer_get(&rb, tmp_buf, rb_return_len);

       // printf("-- the run is 22  %d,%d \r\n",rb_return_len,w_len);

        if(w_len > 0)
        {
            struct socket_state_t sct;

            if(lte_type == 0)
            {
                data_len = list_for_str(tmp_buf,(uint8_t *)"+QISTATE:",w_len);
            }    
            else
            {
                data_len = list_for_str(tmp_buf,(uint8_t *)"+MIPOPEN:",w_len);
                sct.connect_id = id;
            }
            
			if(data_len >= 0)
			{
               
				analysis_socket_state(tmp_buf + data_len,w_len - data_len,&sct);
                //printf("-- socket state dfaf:%d\r\n",sct.socket_state);
                if(sct.socket_state == 1 || sct.socket_state == 2 || sct.socket_state == 4)
                {
                    rv = 1;
                    break;
                }
                   
			} 
        }
        vTaskDelay(100);// 10ms
        timeout_cnt--;
    }while(timeout_cnt > 0);
    
    xSemaphoreGive(at_mutex.cmd_mutex);
    xSemaphoreGive(at_mutex.send_sem);

    //printf("-- the get ftp file :%d\r\n",timeout_cnt);
    if(timeout_cnt == 0)
        return 0;

    return rv;
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
        at.res = RES_OK;
        return at;
    }

    at.res = RES_OK;

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
    
    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
    if(lte_type == 0)
    {
        id -= 1;
        sprintf((char *)tmp_buf, "AT+QISEND=%d,%d\r\n",id,len);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf),at_cmd_parse_send_flag, 40, NOT_RETRY);
    }
    else
    {
        sprintf((char *)tmp_buf, "AT+MIPSEND=%d,%d\r\n",id,len);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf),at_cmd_parse_send_flag, 40, NOT_RETRY);
    }

    if(at.res == RES_OK)
    {
        at = at_cmd_send_syn(data, len, at_cmd_match_ok, 40, NOT_RETRY);
    }
    xSemaphoreGive(at_mutex.send_sem);
    
    //printf("-- run is llllll (2) %d\r\n",at.res);
    if(at.res == RES_OK)
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

    if(lte_type == 0)
    {
        xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
        sprintf((char *)tmp_buf, "AT+QFTPCFG=\"account\",\"%s\",\"%s\"\r\n", sa->user,sa->passwd);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok, 40, NOT_RETRY);
        xSemaphoreGive(at_mutex.send_sem);
        if(at.res == RES_OK)
        {
            return 0;
        }
    }
    else
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
    
    if(lte_type == 0)
    {
        xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
        sprintf((char *)tmp_buf, "AT+QFTPCFG=\"filetype\",%d\r\n",n);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok, 40, NOT_RETRY);
        xSemaphoreGive(at_mutex.send_sem);
    
        if(at.res == RES_OK)
        {
            return 0;
        }
    }
    else
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

    if(lte_type == 0)
    {
        xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
        sprintf((char *)tmp_buf, "AT+QFTPCFG=\"transmode\",%d\r\n",n);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok, 40, NOT_RETRY);
        xSemaphoreGive(at_mutex.send_sem);
    
        if(at.res == RES_OK)
        {
            return 0;
        }
    }
    else
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
    if(lte_type == 0)
    {
        xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
        sprintf((char *)tmp_buf, "AT+QFTPCFG=\"rsptimeout\",%d\r\n",n);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok, 40, NOT_RETRY);
        xSemaphoreGive(at_mutex.send_sem);
    
        if(at.res == RES_OK)
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }

    return 1;
}




/*************************************
**  创建FTP Socket链接
**************************************/

uint8_t at_creat_ftp_connect(struct ftp_info_str *sa)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[256] = {0};

    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);

    if(lte_type == 0)
    {
        sprintf((char *)tmp_buf, "AT+QFTPOPEN=\"%s\",%d\r\n",sa->host,sa->port);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_parse_create_ftp,5, NOT_RETRY);
    }
    else
    {
        sprintf((char *)tmp_buf, "AT+FTPOPEN=\"%s\",\"%s\",\"%s\",,,%d,0\r\n",sa->host,sa->user,sa->passwd,sa->port);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_parse_create_ftp,5, NOT_RETRY);
    }

    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
    {
        return 0;
    }

    return 1;
}




/*************************************
**  创建FTP Socket链接
**************************************/

uint8_t at_list_ftp_files(uint8_t *fp)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    if(fp == NULL)
        return 1;
    
    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
    sprintf((char *)tmp_buf, "AT+QFTPLIST=\"%s\",\"COM:\"\r\n",fp);
    //printf("%s\r\n",tmp_buf);
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_parse_list_files, 5, NOT_RETRY);
    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
    {
        return 0;
    }


    return 1;
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
    
    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
    
    if(lte_type == 0)
    {
        sprintf((char *)tmp_buf, "AT+QFTPSIZE=\"%s\"\r\n",fp);
        //printf("%s\r\n",tmp_buf);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_parse_files_size, 10, NOT_RETRY);
    }
    else
    {
        sprintf((char *)tmp_buf, "AT+FTPLIST=\"%s\"\r\n",fp);
        //printf("%s\r\n",tmp_buf);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_parse_files_size, 10, NOT_RETRY);
    }

    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
    {
        return *(uint32_t *)at.data;
    }


    return 0;
}





/******************************************
**
**	解析创建socket时返回的状态
*******************************************/

static struct at_res_str at_cmd_download_files_size(const char *data)
{
	char *p = NULL;
	uint32_t  m = 0;
	
	struct at_res_str at = {0}; 

    p = strstr(data, "+FTPGET:");
	//rt_kprintf("-- %s\r\n",data);
	if(p != NULL)
    {
		sscanf((char *)p,"+FTPGET:%d\r\n",&m);
		if(m == 2)
			at.res = RES_OK;
		
		return at;
    }
  
	at.res = RES_TIMEOUT;
    
    return at;
}



/****************************
**	
******************************/

uint8_t at_download_ftp_files(uint8_t *fp,uint32_t offset)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    if(lte_type == 0) 
    {
        return 0;
    }
    else
    {
        if(fp == NULL)
	        return 1;
        
        xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
        sprintf((char *)tmp_buf, "AT+FTPGET=\"%s\",1,%d\r\n",fp,offset);
        //rt_kprintf("%s\r\n",tmp_buf);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_download_files_size,60, NOT_RETRY);
        xSemaphoreGive(at_mutex.send_sem);
    
        if(at.res == RES_OK)
        {
            return 0;
        } 
    }
    
	return 1;
}



/*************************************
**  MC669读取数据
**************************************/

uint32_t at_read_ftp_files_size(uint8_t *fp,uint8_t *buf,uint32_t offset,uint16_t len)
{
    uint8_t                 array[128] = {0};
    uint16_t                w_len = 0;
    int32_t                 data_len = 0;
    int32_t                 rv = 0;
    uint16_t                timeout_cnt = 100;  
    char                    *s = NULL,*e = NULL;
    uint16_t                rb_return_len = 0;

    if(fp == NULL || buf == NULL)
        return 0;
    
    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
    
    memset(array,'\0',sizeof(array));
    if(lte_type == 0)
        data_len = sprintf((char *)array, "AT+QFTPGET=\"%s\",\"COM:\",%d,%d\r\n",fp,offset,len);
    else
        data_len = sprintf((char *)array, "AT+FTPRECV=%d\r\n",len);

    xSemaphoreTake(at_mutex.cmd_mutex,portMAX_DELAY);
    //printf("-- ftp files size .... %s\r\n",array);
    w_len = write_data_to_uart1(array,data_len);
    if(w_len != 0)
        printf("-- (3) gprs uart devic write error. w_len:%d, len:%d\r\n", (int)w_len, (int)len);
    vTaskDelay(10);// 10ms
    
    tmp_offset = 0;
    do
    {
        rb_return_len = rt_ringbuffer_data_len(&rb);
        //printf("-- the run is 22  %d,%d \r\n",rb_return_len,tmp_offset);
        w_len = rt_ringbuffer_get(&rb, tmp_buf + tmp_offset, rb_return_len);
        
        if(w_len > 0)
        {
            tmp_offset += w_len;
            //printf("-- the run is 33  %d,%d \r\n",rb_return_len,w_len);
            if(lte_type == 0)
                w_len = list_for_str(tmp_buf,(unsigned char *)"+QFTPGET: 0",tmp_offset);
            else
                w_len = list_for_str_b(tmp_buf,(unsigned char *)"+FTPRECV:",w_len);
            //printf("-- the run is 44  %d,%d \r\n",rb_return_len,w_len);
            //if(w_len > 2)
            //mem_printf(LOG_ERROR, PRINT_HEX,tmp_buf,rb_return_len);
			if(w_len > 0)
			{
                if(lte_type == 0)
				{
                    sscanf((const char *)(tmp_buf + w_len),"+QFTPGET: 0,%d\r\n",&data_len);
				    s = strstr((const char *)tmp_buf,"CONNECT") + 9;
				    if(s != NULL && *(s + data_len) == 0x0D && *(s + data_len + 1) == 0x0A)
				    {
					    if(data_len > 0 && data_len <= 4096)
					    {
						    memcpy(buf,s,data_len);
                            rv = data_len;
                            //printf("-- run is 55 %d,%d\r\n",data_len,tmp_offset);
						    break;
					    }
					    else 
					    {
                            rv = 0;
						    break;
					    }
				    }
                }
                else
                {
                    sscanf((const char *)(tmp_buf + w_len),"+FTPRECV: %d\r\n",&data_len); 
				    s = strstr((const char *)(tmp_buf + w_len),"\r\n") + 2;
				    e = strstr(s + data_len,"\r\nOK\r\n");
				    //printf("-- run is 55 %d,0x%X,0x%X\r\n",data_len,(uint32_t)s,(uint32_t)e);
				    if(e != NULL)
				    {
					    if(data_len > 0 && data_len <= 2560)
					    {
						    memcpy(buf,s,data_len);
                            rv = data_len;
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
        }
        vTaskDelay(10);// 10ms
        timeout_cnt--;
    }while(timeout_cnt > 0);
    
    xSemaphoreGive(at_mutex.cmd_mutex);
    xSemaphoreGive(at_mutex.send_sem);

    //printf("-- the get ftp file :%d\r\n",timeout_cnt);
    if(timeout_cnt == 0)
        return 0;

    return rv;
}






/*************************************
**  EC200 EC600读取数据
**************************************/

uint32_t at_read_ftp_files_size_back(uint8_t *fp,uint8_t *buf,uint32_t offset,uint16_t len)
{
    uint8_t                 array[128] = {0};
    uint16_t                w_len = 0;
    int32_t                 data_len = 0;
    int32_t                 rv = 0;
    uint16_t                timeout_cnt = 100;  
    char                    *s = NULL;
    uint16_t                rb_return_len = 0;

    if(fp == NULL || buf == NULL)
        return 0;
    
    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);

    data_len = sprintf((char *)array, "AT+QFTPGET=\"%s\",\"COM:\",%d,%d\r\n",fp,offset,len);

    //printf("\r\n-- %s\r\n",array);
    xSemaphoreTake(at_mutex.cmd_mutex,portMAX_DELAY);
    //printf("\r\n-- Send IAP AT CMD ... %s\r\n",array);
    w_len = write_data_to_uart1(array,data_len);
    if(w_len != 0)
        printf("-- (3) gprs uart devic write error. w_len:%d, len:%d\r\n", (int)w_len, (int)len);
    vTaskDelay(10);// 10ms
    tmp_offset = 0;
    do
    {
        rb_return_len = rt_ringbuffer_data_len(&rb);
        //printf("-- the run is 22  %d,%d \r\n",rb_return_len,tmp_offset);
        w_len = rt_ringbuffer_get(&rb, tmp_buf + tmp_offset, rb_return_len);

        //printf("-- the run is 22  %d,%d \r\n",rb_return_len,w_len);

        if(w_len > 0)
        {
            tmp_offset += w_len;
            data_len = list_for_str(tmp_buf,(unsigned char *)"+QFTPGET: 0",tmp_offset);
            
			if(data_len > 0)
			{
				sscanf((const char *)(tmp_buf + data_len),"+QFTPGET: 0,%d\r\n",&data_len);
                
				s = strstr((const char *)tmp_buf,"CONNECT") + 9;

				if(s != NULL && *(s + data_len) == 0x0D && *(s + data_len + 1) == 0x0A)
				{
					if(data_len > 0 && data_len <= 4096)
					{
						memcpy(buf,s,data_len);
                        rv = data_len;
                        //printf("-- run is 33 %d,%d\r\n",data_len,tmp_offset);
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
        vTaskDelay(10);// 10ms
        timeout_cnt--;
    }while(timeout_cnt > 0);
    
    xSemaphoreGive(at_mutex.cmd_mutex);
    xSemaphoreGive(at_mutex.send_sem);

    //printf("-- the get ftp file :%d\r\n",timeout_cnt);
    if(timeout_cnt == 0)
        return 0;

    return rv;
}





/******************************************
**
**	解析创建socket时返回的状态
*******************************************/

static struct at_res_str at_cmd_parse_link_state(const char *data)
{
	char *p = NULL;
	struct at_res_str at = {0}; 
	
	//memset((uint8_t *)&at,0,sizeof(struct at_res_str));
  p = strstr(data, "+FTPOPEN:");
	//rt_kprintf("-- %s\r\n",data);
  if(p != NULL)
  {
		uint32_t m;
		
		sscanf(p,"+FTPOPEN: %d\r\n",&m);
		at.data[0] = m;
		at.res = RES_OK;
		return at;
  }
    
	at.res = RES_TIMEOUT;
    
  return at;
}


/*************************************
**  查看FTP连接状态
**************************************/

uint8_t at_get_ftp_link_state(void)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

   
    if(lte_type == 0)
    {
        return 0;
    }
    else
    {
        xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
        sprintf((char *)tmp_buf, "AT+FTPOPEN?\r\n");
        //rt_kprintf("%s\r\n",tmp_buf);
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_parse_link_state, 30, NOT_RETRY);
        xSemaphoreGive(at_mutex.send_sem);
    
        if(at.res == RES_OK)
        {   
            return at.data[0];
        }
    }

    return 0;
}



/*************************************
**  关闭FTP链接
**************************************/

uint8_t at_close_ftp_connect(void)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);

    if(lte_type == 0)
        sprintf((char *)tmp_buf, "AT+QFTPCLOSE\r\n");
    else
        sprintf((char *)tmp_buf, "AT+FTPCLOSE\r\n");

    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_parse_close_ftp,60, NOT_RETRY);
    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
    {
        return 0;
    }


    return 1;
}



/*************************************
**  查询URC Port配置状态
**************************************/

uint8_t at_query_urc_port_state(void)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    if(lte_type == 0)
    {
        xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
        sprintf((char *)tmp_buf, "AT+QURCCFG?\r\n");
        at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_query_urc_port,6, NOT_RETRY);
        xSemaphoreGive(at_mutex.send_sem);
    
        if(at.res == RES_OK)
        {
            return 0;
        }
        return 1;
    }
    else
    {
        return 0;
    }    
}


/*************************************
**  设置SMS
**************************************/

uint8_t at_config_sms_port(void)
{
    struct at_res_str at = {0};
    uint8_t tmp_buf[128] = {0};

    
    xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
    sprintf((char *)tmp_buf, "AT+QURCCFG=\"urcport\",\"uart1\"\r\n");
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok,60, NOT_RETRY);
    xSemaphoreGive(at_mutex.send_sem);
    
    if(at.res == RES_OK)
    {
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
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok,60, NOT_RETRY);
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
    at = at_cmd_send_syn(tmp_buf, strlen((char *)tmp_buf), at_cmd_match_ok,60, NOT_RETRY);
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

uint8_t at_config_hex_recv(void)
{
    struct at_res_str at = {0};

    if(lte_type == 0)
    {
        return 0;
    }
    else 
	{
        xSemaphoreTake(at_mutex.send_sem,portMAX_DELAY);
        at = at_cmd_send_syn("AT+GTSET=\"IPRFMT\",2\r\n", strlen((char *)"AT+GTSET=\"IPRFMT\",2\r\n"), at_cmd_match_ok,5, NOT_RETRY);
        xSemaphoreGive(at_mutex.send_sem);
    
        if(at.res == RES_OK)      
		    return 0;
    }
    
	return 1;
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
    uint8_t buf[64] = {0};
    //int offset = 0;
  
    index = (char *)data;
    do
    {
        if(lte_type == 0)
        {
		    p = strstr(index, "+QIURC: \"closed\"");
		    if (p != NULL)
		    {
                struct socket_down_str tmp;

                sscanf(p, "+QIURC: \"closed\",%d", &id);
                //socket_status[id -1] = 0;
                //printf("-- platform logout!!!! %d\r\n",id);
                if(socket_qh[id] != NULL)
                {
                    //printf("-- Socket Send Queue......%d\r\n",id);
                    memcpy(tmp.data,"+QIURC:",sizeof("+QIURC:"));
                    tmp.len = sizeof("+QIURC:");
                    xQueueSend(socket_qh[id],&tmp,sizeof(struct socket_down_str));  //发送队列
                }
		    }
            p = strstr(index, "+QIURC: \"recv\"");
            if (p != NULL)
            {
                struct socket_down_str tmp;

			    p_end = p; 
			    if (p_end > index)
			    {
				    save_len = p_end - index;
				    rt_ringbuffer_put(&rb, (uint8_t *)index, save_len);														 
			    }	
			    sscanf(p, "+QIURC: \"recv\",%d,%d", &id, &len);
			    sprintf((char *)buf, "+QIURC: \"recv\",%d,%d\r\n",id,len);
			    p = p + strlen((const char *)buf);
                tmp.len = len;
                memcpy(tmp.data,p,len);
                if(socket_qh[id] != NULL)
                {
                    //printf("-- Socket Send Queue......%d\r\n",id);
                    xQueueSend(socket_qh[id],&tmp,sizeof(struct socket_down_str));  //发送队列
                }
			    index = p + len;
            }
            p = strstr(index, "+QMTRECV:");
            if (p != NULL)
            {
                p_end = p; 
                if (p_end > index)
                {
                    save_len = p_end - index;
                    rt_ringbuffer_put(&rb, (uint8_t *)index, save_len);														 
                }	
                sscanf(p, "+QMTRECV:%d,%d", &id, &len);
                sprintf((char *)buf, "+QMTRECV:%d,%d\r\n",id,len);
                p = p + strlen((const char *)buf);
                index = p + len;
            }

            p = strstr(index, "+QMTSTAT:");
            if (p != NULL)
            { 
                sscanf(p, "+QMTSTAT:%d\r\n", &id);
                printf("-- platform logout!!!!\r\n");
            }

            p = strstr(index, "+QMTPING:");
            if (p != NULL)
            { 
                sscanf(p, "+QMTPING:%d\r\n", &id);
                printf("-- platform logout!!!!\r\n");
            }  
        }
        else
        {
            p = strstr(index, "+MIPRTCP:");   //接收到数据
            if(p != NULL)
            {
                struct socket_down_str tmp;
                int offset = 0;
                
                sscanf(p, "+MIPRTCP: %d,%d",&id,&len);
                if(len < 10)
                    offset = 1;
                else if (len < 100)
                    offset = 2;
                else if (len < 1000)
                    offset = 3;
                else
                    offset = 4;
                p += (13 + offset); 			//跳过关键字,指定数据指针
			
                //printf("-- recv data .......  %d,,,,  %d\r\n",len,id);
                tmp.len = len;
                memcpy(tmp.data,p,len);
                if(id == 1 || id == 2)
                {
                    if(socket_qh[id - 1] != NULL)
                    {
                        //printf("-- recv data .......OK OK OK OK OK.... \r\n");
                        xQueueSend(socket_qh[id - 1],&tmp,sizeof(struct socket_down_str));  //发送队列           //下行命令解析
                    }
                }
                        
                index += (len + 16); 														//保证下次检索位置是新的
            }
							
            p = strstr(data, "+MIPSTAT:");  //服务器断开链接
            if(p != NULL)
            {
                struct socket_down_str tmp;
                    
                sscanf(p, "+MIPSTAT: %d,%d,", &id, &len);
                if(id == 1 || id == 2)
                {
                    if(socket_qh[id - 1] != NULL)
                    {
                        memcpy(tmp.data,"+QIURC:",sizeof("+QIURC:"));
                        tmp.len = sizeof("+QIURC:");
                        xQueueSend(socket_qh[id - 1],&tmp,sizeof(struct socket_down_str));  //发送队列//发送队列
                    }
                }
            }
        }

        /* 查短信索引 : +CMTI: ME",4*/
        p = strstr(index, "+CMT:");
        if (p != NULL)
        {
            struct sms_down_str         tmp;
            uint16_t                    t_len;
            QueueHandle_t               t_q;

            p_end = p; 
			if (p_end > index)
			{
				save_len = p_end - index;
				rt_ringbuffer_put(&rb, (uint8_t *)index, save_len);														 
			}	
            
            if((p - index) > 0)
                t_len = rev_len -  (p - index);              //??????????????? 问题
            else
                t_len = 0;

            //printf("-- t len :%d\r\n",t_len);
            if(t_len > 0 && t_len < 1024)
            {
                tmp.len = (uint16_t)t_len;
                memcpy(tmp.data,p,t_len);
                t_q = get_sms_queue();
                if(t_q != NULL)
                    xQueueSend(t_q,&tmp,sizeof(struct sms_down_str));   
            }

            index = p + t_len;
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

void thread_entry_at_parse(void *parameter)
{
    parameter = parameter;

    rt_ringbuffer_init(&rb, &rb_pool[0], sizeof(rb_pool)); 
    at_mutex.cmd_mutex = xSemaphoreCreateMutex();
    xSemaphoreGive(at_mutex.cmd_mutex);
    at_mutex.send_sem = xSemaphoreCreateMutex();
    xSemaphoreGive(at_mutex.send_sem);

    
	for(;;)
	{
        memset(at_buf,0,sizeof(at_buf));
        at_len = read_data_from_uart1(at_buf,AT_BUFF_SIZE,1000,50);
        //printf("-- run at parse...%d\r\n",at_len);
        if(at_len > 0)
        {
            //printf("-- run at len:%s\r\n",at_buf);
            at_recv_data((const char *)at_buf, at_len);
        }

	}
}







