


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>


#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_system.h"

#include "common.h"
#include "drv_acl16.h"


/********************************************************
 * @desc: acl16校验接收数据
 * @param:
 * @return:
 * @date:2022/02/21
*********************************************************/
static uint8_t acl16_verify_data(uint8_t *buf, uint16_t len, int event)
{
	uint8_t          ret = 0;
	uint32_t         keyLen = 0;
	uint32_t         cryLen = 0;
	
	if(buf[0] != 0xAA)
		return ret;
	
	keyLen = *((uint16_t *)&buf[3]);
	cryLen = *((uint32_t *)&buf[5]);
	
	if(buf[9 + keyLen + cryLen] != 0x55)
    {
		printf("TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT  function: %s - Line : %d\n", __func__, __LINE__);
		printf("keyLen : %d ,  cryLen : %d \n", keyLen, cryLen);
		return ret;
	}
	
	if(event == CRY_GENKEY && keyLen <= 0)
    {
		printf("TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT  function: %s - Line : %d\n", __func__, __LINE__);
		return ret;
	}
	
	if((event == CRY_SIGN || event == CRY_ENCRY || event == CRY_GETCHIPID) && cryLen <= 0)
    {
		printf("TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT  function: %s - Line : %d\n", __func__, __LINE__);
		return ret;
	}
	
	ret = keyLen ? keyLen : (cryLen ? cryLen : 0);	
    //printf("-- run ting .sdf  %d,%d,%d\r\n",ret,keyLen,cryLen);
	return ret;
}


/********************************************************
 * @desc: acl16组协议帧
 * @param:
 * @return:
 * @date:2022/02/21
*********************************************************/
static uint16_t acl16_make_protocol_frame(uint8_t *buf,struct acl16_pro_str *data)
{
	uint16_t len = 0;

    if(data == NULL || buf == NULL)
        return 0;

    buf[len++] = 0XAA;
	buf[len++] = data->ip_code;
	buf[len++] = data->cmd_code;
	*((uint16_t *)(&buf[len])) = data->key_len;
	len += 2;
	*((uint32_t *)(&buf[len])) = data->crypt_len;
	len += 4;
    
    if(data->key_len > 0)
	    memcpy(buf + len, data->key_data, data->key_len);
	len += data->key_len;
	
    if(data->crypt_len > 0)
	    memcpy(buf + len, data->cry_data, data->crypt_len);
	len += data->crypt_len;
	
	buf[len++] = 0X55;

	return len;
}




/********************************************************
 * 1:超时
 * 0：正确
*********************************************************/
uint16_t acl16_write_then_read(uint8_t event,void *in_data,void * out_data,uint16_t len)
{
	struct acl16_pro_str        acl16_protocol = {0};
    uint16_t                    frame_len = 0;
    uint16_t                    counter = 0;
    uint8_t                     send_buf[128] = {0};
    uint8_t                     recv_buf[128] = {0};
	
	switch(event)
	{
		case CRY_REBOOT:           //恢复BootLoader启动
            acl16_protocol.ip_code = IPCODE_REBOOT;
			acl16_protocol.cmd_code = CMDCODE_ID_GET;
			acl16_protocol.key_len = 0;
			acl16_protocol.crypt_len = 0;
			acl16_protocol.key_data = (uint8_t *)out_data;
			acl16_protocol.cry_data = (uint8_t *)in_data;

			frame_len = acl16_make_protocol_frame(send_buf,&acl16_protocol);
             
            if(rt_write_data_to_acl16(send_buf,frame_len) != frame_len)
			    return 0;
            return len;
        case CRY_GETAPPVER:       //获取加密芯片应用程序版本号
            acl16_protocol.ip_code = IPCONDE_GETVER;
			acl16_protocol.cmd_code = CMDCODE_ID_GET;
			acl16_protocol.key_len = 0;
			acl16_protocol.crypt_len = len;
			acl16_protocol.key_data = (uint8_t *)out_data;
			acl16_protocol.cry_data =  (uint8_t *)in_data;
           
			frame_len = acl16_make_protocol_frame(send_buf,&acl16_protocol);
            
            //mem_printf(LOG_ERROR,PRINT_HEX,send_buf,frame_len);
            if(rt_write_data_to_acl16(send_buf,frame_len) != frame_len)
                return 0;
            
            counter = 5;
            //printf("-- run this..... :%d\r\n",counter);
			while(counter)
			{
                vTaskDelay(20);
				frame_len = rt_read_data_from_acl16(recv_buf,14);
                //printf("-- recv acl16 data len :%d\r\n",frame_len);
                //mem_printf(LOG_ERROR,PRINT_HEX,recv_buf,frame_len);

				if((len = acl16_verify_data(recv_buf, 4 + 10, CRY_GETAPPVER)) > 0)
			 	{
			 		memcpy(out_data, (&recv_buf[9]), len);
					return len;
			 	}
				else
				{
					counter--;
				}
			}
            return 0;
        case CRY_GETCHIPID:       //读取加密芯片ID
            acl16_protocol.ip_code = IPCODE_GETID;
			acl16_protocol.cmd_code = CMDCODE_ID_GET;
			acl16_protocol.key_len = 0;
			acl16_protocol.crypt_len = len;
			acl16_protocol.key_data = (uint8_t *)out_data;
			acl16_protocol.cry_data =  (uint8_t *)in_data;
           
			frame_len = acl16_make_protocol_frame(send_buf,&acl16_protocol);
            //mem_printf(LOG_ERROR,PRINT_HEX,send_buf,frame_len);
            if(rt_write_data_to_acl16(send_buf,frame_len) != frame_len)
                return 0;
            
            counter = 5;
            //printf("-- run this..... :%d\r\n",counter);
			while(counter)
			{
                vTaskDelay(20);
				frame_len = rt_read_data_from_acl16(recv_buf,26);
                //printf("-- recv acl16 data len :%d\r\n",frame_len);
                //mem_printf(LOG_ERROR,PRINT_HEX,recv_buf,frame_len);

				if((len = acl16_verify_data(recv_buf, 26, CRY_GETCHIPID)) > 0)
			 	{
			 		memcpy(out_data, (&recv_buf[9]), len);
					return len;
			 	}
				else
				{
					counter--;
				}
			}
			return 0;
		case CRY_GENKEY:
			acl16_protocol.ip_code = IPCODE_SM2;
			acl16_protocol.cmd_code = CMDCODE_SM2_GENSECKEY;
			acl16_protocol.key_len = 64;
			acl16_protocol.crypt_len = len;
			acl16_protocol.key_data = (uint8_t *)out_data;
			acl16_protocol.cry_data =  (uint8_t *)in_data;
           
			frame_len = acl16_make_protocol_frame(send_buf,&acl16_protocol);
            //mem_printf(LOG_ERROR,PRINT_HEX,send_buf,frame_len);
            if(rt_write_data_to_acl16(send_buf,frame_len) != frame_len)
                return 0;
            
            counter = 5;
            //printf("-- run this..... :%d\r\n",counter);
			while(counter)
			{
                vTaskDelay(50);
				frame_len = rt_read_data_from_acl16(recv_buf,74);
               
				if((len = acl16_verify_data(recv_buf, 74, CRY_GENKEY)) > 0)
			 	{
			 		memcpy(out_data, (&recv_buf[9]), len);
					return len;
			 	}
				else
				{
					counter--;
				}
			}
			return 0;
		case CRY_SIGN:
			acl16_protocol.ip_code = IPCODE_SM2;
			acl16_protocol.cmd_code = CMDCODE_SM2_SIGN;
			acl16_protocol.key_len = 0;
			acl16_protocol.crypt_len = len;
			acl16_protocol.key_data = (uint8_t *)out_data;
			acl16_protocol.cry_data =  (uint8_t *)in_data;
           
			frame_len = acl16_make_protocol_frame(send_buf,&acl16_protocol);
            //mem_printf(LOG_ERROR,PRINT_HEX,send_buf,frame_len);
            if(rt_write_data_to_acl16(send_buf,frame_len) != frame_len)
                return 0;
            
            counter = 5;
            //printf("-- run this..... :%d\r\n",counter);
			while(counter)
			{
                vTaskDelay(50);
				frame_len = rt_read_data_from_acl16(recv_buf,74);
               
				if((len = acl16_verify_data(recv_buf, 74, CRY_SIGN)) > 0)
			 	{
			 		memcpy(out_data, (&recv_buf[9]), len);
					return len;
			 	}
				else
				{
					counter--;
				}
			}
			return 0;	 
		case CRY_GOLOWP:
			return 0;
		case CRY_GETDATA:
			return 0;
		case CRY_WEAKUP:
		 	    
			return 0;
		case CRY_RST:
		
			return 0;
		default:
			return 0;
	}
}


