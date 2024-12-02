
/*******************************************************/

#include <rtthread.h>
#include <rtdevice.h>
#include <stdio.h>
#include <string.h>

#include "drv_gpio.h"
#include "drv_spi.h"
#include "drv_rtc.h"
#include "spi_acl16.h"

#include "pro_data.h"
#include "common.h"


#include "app_gb4.h"
#include "app_shell.h"
#include "app_lte.h"
#include "app_gnss.h"
#include "app_can_recv.h"
#include "app_mon.h"
#include "app_fifo.h"
#include "pro_data.h"
#include "app_can_send.h"
#include "app_iap.h"
#include "app_ver.h"
#include "app_lte.h"
#include "app_files.h"
#include "app_products.h"
#include "app_edge.h"
#include "app_at.h"

#include "main.h"



#ifdef RT_USING_ACL16

#define TCP_DATA_LEN       1024


static SIGNOREN	gSignOrEncryData;

static uint8_t ACL_SendFrameBuf[1024] = {0};



/********************** 本地国四全局变量  *************************/

static struct gb4_str       						gb4 = {0};					//

static rt_mq_t  												gb4_down_mq = NULL;

static struct socket_down_str   				gb4_down_data = {0};			//

static uint8_t 													local_buff[1460] = {0};		//发送缓冲区

static struct serial_num_str    				gb4_serial_num = {0};	    //

static struct encryption_chip_str 			encrypt_chip;



/****************************
**	返回GB4链接状态
****************************/

uint8_t read_gb4_socket_state(void)
{
	uint8_t rv = 0;
	
	rv = gb4.socket_state;

	return rv;
}


/***********************
**	系统时间(调试使用)
************************/

void build_rtc_time(uint8_t *buf,uint16_t size)
{
	struct rt_tm tmp;

	if(size < 6 || buf == NULL)
		return;

	rt_get_rtc(&tmp);          //获取系统时间

	*(buf + 0) = (uint8_t)tmp.year;
	*(buf + 1) = (uint8_t)tmp.mon;
	*(buf + 2) = (uint8_t)tmp.day;
	*(buf + 3) = (uint8_t)tmp.hour;
	*(buf + 4) = (uint8_t)tmp.min;
	*(buf + 5) = (uint8_t)tmp.sec;
}





/***********************
**	复位
************************/

uint8_t reset_serial_num(void)
{
	struct rt_tm         tm;
	time_t          t_t;
	
	rt_get_rtc(&tm);
	t_t = rt_mktime(&tm);
 
	if(tm.day != gb4_serial_num.day)
	{
		if(t_t >= gb4_serial_num.unix_time)
		{
			gb4_serial_num.login_num = 1;
			gb4_serial_num.serial_num = 1;
			gb4_serial_num.serial_gb = 1;
			gb4_serial_num.alarm_num = 1;
			gb4_serial_num.unix_time = t_t;
			gb4_serial_num.day = tm.day;
			printf("-- 改变流水号\r\n");
		}
	}

	return 0;
}


/******************************
**	保存ECU运行数据
*******************************/

void write_login_out_num(void)
{
	rt_device_t 								bsram_dev = RT_NULL;       //
	
	bsram_dev = rt_device_find("back_sram");
	rt_device_open(bsram_dev, RT_DEVICE_OFLAG_RDWR);
	gb4_serial_num.verfy = 0x5A5A;
	//rt_kprintf("-- write login out num\r\n");
	rt_device_write(bsram_dev,3072, (uint8_t *)&gb4_serial_num,sizeof(struct serial_num_str));
	rt_device_close(bsram_dev);
}


/******************************
**	复位
*******************************/

uint8_t load_login_out_num(void)
{
	rt_device_t 								bsram_dev = RT_NULL;       //
	
	bsram_dev = rt_device_find("back_sram");
	rt_device_open(bsram_dev, RT_DEVICE_OFLAG_RDWR);
	rt_device_read(bsram_dev,3072, (uint8_t *)&gb4_serial_num,sizeof(struct serial_num_str));
	if(gb4_serial_num.verfy != 0x5A5A)
	{
		memset((uint8_t *)&gb4_serial_num,0,sizeof(struct serial_num_str));
		rt_kprintf("-- Reset serial num...\r\n");
	}
	rt_device_close(bsram_dev);
	
	return 1;
}


/******************************************************
 * @desc  : acl16 写数据
 * @param : buf  数据
            len 数据长度
 * @return: 成功写入的长度
 *****************************************************/
uint16_t ACL16_write(struct rt_spi_device *device,rt_uint8_t  *buf, uint16_t len)
{
    rt_spi_send(device, buf,len);
    return len;
}

/******************************************************
 * @desc  : acl16 读数据
 * @param : buf  数据缓冲区
            len 数据缓冲区长度
 * @return: 读到的数据长度
 *****************************************************/
uint16_t ACL16_read(struct rt_spi_device *device,rt_uint8_t  *buf, uint16_t len)
{
    return rt_spi_recv(device,buf,len);
}




int checkData(unsigned char *buf, int len, int event)
{
	int ret = 0;
	unsigned int keyLen = 0;
	unsigned int cryLen = 0;
	
	if(buf[0] != 0xaa)
		return ret;
	
	keyLen = *((unsigned short *)&buf[3]);
	cryLen = *((unsigned int *)&buf[5]);
	
	if(buf[9+keyLen+cryLen] != 0x55){
		printf("-- function: %s - Line : %d\n", __func__, __LINE__);
		printf("-- keyLen : %d ,  cryLen : %d \n", keyLen, cryLen);
		return ret;
	}
	
	if((event == CRY_GENKEY) && keyLen <= 0){
		printf("-- function: %s - Line : %d\n", __func__, __LINE__);
		return ret;
	}
	
	if((event == CRY_SIGN || event == CRY_ENCRY || event == CRY_GETCHIPID) && cryLen <= 0){
		printf("-- function: %s - Line : %d\n", __func__, __LINE__);
		return ret;
	}
	
	ret = keyLen ? keyLen : (cryLen ? cryLen : 0);	
	return ret;
}



/************************************************
**
**
*************************************************/


static uint16_t Make_FrameBuf(ACL_ProtocolStr Data)
{
	int data_len = 0;
	
	memset(ACL_SendFrameBuf, 0, sizeof(ACL_SendFrameBuf));
	
	ACL_SendFrameBuf[data_len++] = 0XAA;
	ACL_SendFrameBuf[data_len++] = Data.ip_code;
	ACL_SendFrameBuf[data_len++] = Data.cmd_code;
	*((unsigned short *)(&ACL_SendFrameBuf[data_len])) = Data.key_len;
	data_len += sizeof(short);
	*((unsigned int *)(&ACL_SendFrameBuf[data_len])) = Data.crypt_len;
	data_len += sizeof(int);
	
	memcpy(&ACL_SendFrameBuf[data_len], Data.keyDataSeg, Data.key_len);
	data_len += Data.key_len;
	
	memcpy(&ACL_SendFrameBuf[data_len], Data.cryDataSeg, Data.crypt_len);
	data_len += Data.crypt_len;
	
	ACL_SendFrameBuf[data_len++] = 0X55;
	
	return data_len;
}

static int ACL_Make_Protocol_Frame(uint8_t event)
{
	int iFrame = 0;
	ACL_ProtocolStr ACL_Protocol;
	
	memset(&ACL_Protocol, 0, sizeof(ACL_ProtocolStr));
	
	switch(event)
	{
		case CRY_GETREBOOT:
			ACL_Protocol.ip_code = IPCONDE_REBOOT;
			ACL_Protocol.cmd_code = CMDCODE_ID_GET;
			ACL_Protocol.key_len = 0;
			ACL_Protocol.crypt_len = gSignOrEncryData.dataLen;
			ACL_Protocol.keyDataSeg = (unsigned char *)gSignOrEncryData.keyData;
			ACL_Protocol.cryDataSeg = (unsigned char *)gSignOrEncryData.signOrEncryData;
			break;
		case CRY_GETAPPVER:
			ACL_Protocol.ip_code = IPCONDE_GETVER;
			ACL_Protocol.cmd_code = CMDCODE_ID_GET;
			ACL_Protocol.key_len = 0;
			ACL_Protocol.crypt_len = gSignOrEncryData.dataLen;
			ACL_Protocol.keyDataSeg = (unsigned char *)gSignOrEncryData.keyData;
			ACL_Protocol.cryDataSeg = (unsigned char *)gSignOrEncryData.signOrEncryData;
			break;
		case CRY_GETCHIPID:
			ACL_Protocol.ip_code = IPCODE_GETID;
			ACL_Protocol.cmd_code = CMDCODE_ID_GET;
			ACL_Protocol.key_len = 0;
			ACL_Protocol.crypt_len = gSignOrEncryData.dataLen;
			ACL_Protocol.keyDataSeg = (unsigned char *)gSignOrEncryData.keyData;
			ACL_Protocol.cryDataSeg = (unsigned char *)gSignOrEncryData.signOrEncryData;
			break;
		
		case CRY_GENKEY:
			ACL_Protocol.ip_code = IPCODE_SM2;
			ACL_Protocol.cmd_code = CMDCODE_SM2_GENSECKEY;
			ACL_Protocol.key_len = 64;
			ACL_Protocol.crypt_len = 0;
			ACL_Protocol.keyDataSeg = (unsigned char *)gSignOrEncryData.keyData;
			ACL_Protocol.cryDataSeg = (unsigned char *)gSignOrEncryData.signOrEncryData;
			break;
		
		case CRY_SIGN:
			ACL_Protocol.ip_code = IPCODE_SM2;
			ACL_Protocol.cmd_code = CMDCODE_SM2_SIGN;
			ACL_Protocol.key_len = 0;
			ACL_Protocol.crypt_len = gSignOrEncryData.dataLen;
			ACL_Protocol.keyDataSeg = (unsigned char *)gSignOrEncryData.keyData;
			ACL_Protocol.cryDataSeg = (unsigned char *)gSignOrEncryData.signOrEncryData;
			break;
		case CRY_ENCRY:
			ACL_Protocol.ip_code = IPCODE_SM2;
			ACL_Protocol.cmd_code = CMDCODE_SM2_ENCRY;
			ACL_Protocol.key_len = 64;
			ACL_Protocol.crypt_len = gSignOrEncryData.dataLen;
			ACL_Protocol.keyDataSeg = (unsigned char *)gSignOrEncryData.keyData;
			ACL_Protocol.cryDataSeg = (unsigned char *)gSignOrEncryData.signOrEncryData;
			break;
		case CRY_GOLOWP:
			ACL_Protocol.ip_code = IPCODE_LP;
			ACL_Protocol.cmd_code = CMDCODE_LOWP_GOLOWP;
			ACL_Protocol.key_len = 0;
			ACL_Protocol.crypt_len = gSignOrEncryData.dataLen;
			ACL_Protocol.keyDataSeg = (unsigned char *)gSignOrEncryData.keyData;
			ACL_Protocol.cryDataSeg = (unsigned char *)gSignOrEncryData.signOrEncryData;
			break;
		
		default:
			break;
	}
	iFrame = Make_FrameBuf(ACL_Protocol);
	return iFrame;
}




/************************************************
**
***************************************************/

int acl_write_then_read(uint8_t eventType,void *inData,void * outData,int len)
{
	ACL_ProtocolStr     ACL_Protocol;
	uint16_t WriteLen = 0;
	uint16_t ReadLen = 0;
	uint8_t loopCount = 0;
	
	memset(ACL_SendFrameBuf, 0, sizeof(ACL_SendFrameBuf));
	
	memset(&gSignOrEncryData, 0, sizeof(SIGNOREN));
	gSignOrEncryData.dataLen = len;
	memcpy(gSignOrEncryData.signOrEncryData, inData, len);
	memcpy(gSignOrEncryData.keyData, outData, 64);
	
	memset(&ACL_Protocol, 0, sizeof(ACL_ProtocolStr));
	
	switch(eventType)
	{
		case CRY_GETREBOOT:
			WriteLen = ACL_Make_Protocol_Frame(CRY_GETREBOOT);
			ACL16_write(acl_device,ACL_SendFrameBuf,WriteLen);
			rt_thread_mdelay(60);
			return 0;
		case CRY_GETAPPVER:
			WriteLen = ACL_Make_Protocol_Frame(CRY_GETAPPVER);
			ACL16_write(acl_device,ACL_SendFrameBuf,WriteLen);
			while(1)
			{
				rt_thread_mdelay(60);
				ACL16_read(acl_device,ACL_SendFrameBuf,16 + 10);
				if((ReadLen = checkData(ACL_SendFrameBuf, 16 + 10, CRY_GETAPPVER)) > 0)
				{
					memcpy(outData, (&ACL_SendFrameBuf[9]), ReadLen);
					return ReadLen;
				}
				if(loopCount++ > 15)
				{
					loopCount = 0;
					return RT_ERROR;
				}
			}
		case CRY_GETCHIPID:
			WriteLen = ACL_Make_Protocol_Frame(CRY_GETCHIPID);
			ACL16_write(acl_device,ACL_SendFrameBuf,WriteLen);
			while(1)
			{
				rt_thread_mdelay(60);
				ACL16_read(acl_device,ACL_SendFrameBuf,16 + 10);
				if((ReadLen = checkData(ACL_SendFrameBuf, 16+10, CRY_GETCHIPID)) > 0)
				{
					memcpy(outData, (&ACL_SendFrameBuf[9]), ReadLen);
					return ReadLen;
				}
				if(loopCount++ > 15)
				{
					loopCount = 0;
					return RT_ERROR;
				}
			}
			
		case CRY_GENKEY :
			WriteLen = ACL_Make_Protocol_Frame(CRY_GENKEY);
			ACL16_write(acl_device,ACL_SendFrameBuf,WriteLen);
			while(1)
			{
				rt_thread_mdelay(60);
				ACL16_read(acl_device,ACL_SendFrameBuf,64 + 10);
				if((ReadLen = checkData(ACL_SendFrameBuf, 64 + 10, CRY_GENKEY)) > 0)
				{
					memcpy(outData, (&ACL_SendFrameBuf[9]), ReadLen);
					return ReadLen;
				}
				if(loopCount++ > 15)
				{
					loopCount = 0;
					return -RT_ERROR;
				}
			}
		
		case CRY_SIGN:
			WriteLen = ACL_Make_Protocol_Frame(CRY_SIGN);
			ACL16_write(acl_device,ACL_SendFrameBuf,WriteLen);
			rt_thread_mdelay(50);
			while(1)
			{
				//rt_kprintf("-- the acl cnt :%d\r\n",loopCount);
				ACL16_read(acl_device,ACL_SendFrameBuf,64 + 10);
				if((ReadLen = checkData(ACL_SendFrameBuf, 64+10, CRY_SIGN)) > 0)
				{
					memcpy(outData, (&ACL_SendFrameBuf[9]), ReadLen);
					return ReadLen;
				}
				if(loopCount++ > 5)
				{
					loopCount = 0;
					return -RT_ERROR;
				}
				rt_thread_mdelay(50);
			}
		
		case CRY_ENCRY:
			WriteLen = ACL_Make_Protocol_Frame(CRY_ENCRY);
			ACL16_write(acl_device,ACL_SendFrameBuf,WriteLen);
			while(1)
			{
				rt_thread_mdelay(20);
				ReadLen = ACL16_read(acl_device,ACL_SendFrameBuf,600);
				rt_thread_mdelay(50);
				if((ReadLen = checkData(ACL_SendFrameBuf, ReadLen, CRY_ENCRY)) > 0)
				{
					memcpy(outData, (&ACL_SendFrameBuf[9]), ReadLen);
					return ReadLen;
				}
				if(loopCount++ > 150)
				{
					loopCount = 0;
					return -RT_ERROR;
				}
			}
			
		case CRY_GOLOWP:
			WriteLen = ACL_Make_Protocol_Frame(CRY_GOLOWP);
			ACL16_write(acl_device,ACL_SendFrameBuf,WriteLen);
			return RT_EOK;
		case CRY_GETDATA:
			return RT_EOK;
		case CRY_WEAKUP:
			rt_pin_mode(PIN_ACL_WAK, PIN_MODE_OUTPUT);
		    rt_acl_wak_high();	 	    
			return RT_EOK;
		case CRY_RST:
			rt_pin_mode(PIN_ACL_RST, PIN_MODE_OUTPUT);
			rt_acl_reset();
			
			return RT_EOK;
		//case 
		default:
			return -RT_ERROR;
	}
	
}




/******************************
**	返回ACL16的工作状态
*******************************/

uint8_t read_acl16_work_state(void)
{
	uint8_t rv;
	
	rv = encrypt_chip.state;

	return rv;
}


/***************
**	返回加密芯片ID
********************/

uint8_t read_encryption_chip_id(uint8_t *buf,uint8_t buf_size)
{
	if(buf_size < 16)
		return 0;
	
	memcpy(buf,encrypt_chip.id_lot,sizeof(encrypt_chip.id_lot));
	
	return 16;
}



/**************************
**	重启AC16芯片
**	
****************************/

void reboot_acl16(void)
{
	uint8_t buff[50];
	
	if(sizeof(encrypt_chip.id_sn) == acl_write_then_read(CRY_GETCHIPID,buff,encrypt_chip.id_sn,sizeof(encrypt_chip.id_sn)))
	{
		//set_info_sys_acl_state(RT_TRUE);
	}
	rt_kprintf("-ACL chip id : ");
	mem_printf(LOG_ERROR, PRINT_HEX,encrypt_chip.id_sn,sizeof(encrypt_chip.id_sn));
}
#endif





/****************************************************
**	
*****************************************************/

uint8_t get_given_string(uint8_t *src, uint8_t ch, uint8_t *des)
{
    uint8_t offset = 0;

    while (*src != ch)
    {
        *des++ = *src++;
        offset++;
    }

    offset++; //跳过 ch

    return offset;
}



/***********************************
**
************************************/

uint8_t get_given_number(uint8_t *src, uint8_t ch, uint16_t *des)
{
    uint8_t offset = 0;
    uint16_t value = 0;

    while (*src != ch)
    {
        value *= 10;
        value += (*src - '0');
        src++;
        offset++;
    }

    offset++; //跳过 ch
    *des = value;
    return offset;
}

/************************
**
*************************/

uint8_t read_acl16_app_version(void)
{
	uint8_t rv;
	
	rv = encrypt_chip.version;
	
	return rv;
}	




/*******************************
**	国四企业平台通用应答
********************************/

uint16_t build_gb4_response_packets(uint8_t *buf,uint16_t size,struct down_cmd_res_str *res)
{
	struct start_str    		*p_start;
	uint16_t 								len;
	uint32_to_byte   				tmp_data; 
	uint8_t 								array[50];
	
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
	
	p_start->soft_ver = (uint8_t)read_user_ver();
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
	struct start_str    		    		*p_start;
	struct down_cmd_con_str         *p_down_cmd;
	uint16_t 	                    	offset = 0; 
	uint8_t 				        				*index_p = NULL;
	uint8_t     	                	array[100];
	struct down_cmd_res_str 				*p_res;

//	rt_kprintf("\r\n-- Platform  receive GB4 Platform:");
//  mem_printf(LOG_ERROR, PRINT_HEX, data, len);

  if(data == NULL || len < 25)
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
	
	//rt_kprintf("-- recv id :%s\r\n",p_start->vin);
	if(str_compare(p_start->vin,array,16) == 0)
	{
		read_config_vin_info(array,sizeof(array));
		if(str_compare(p_start->vin,array,17) == 0)
		{
			rt_kprintf("-- Down Cmd Dev ID error ... \r\n");
			return 0;
		}
	}
	
	if((*(data + len - 1)) != (calc_xor_verify(data + 2,len - 3)))
	{
		rt_kprintf("-- Down Cmd Check the error.......\r\n");
		return 0;
	}
	
	memset(array,0,sizeof(array));
	
	switch(p_start->cmd)
	{
		case 0x01: 				//国标登录  登入回应
			if(len < 33)
				break;
			offset = len - sizeof(struct start_str) - 9;
			index_p = data + sizeof(struct start_str) + 8;	
			
			for(int i = 1;i <= 7;i++)
			{
				if(*index_p == i)
				{
					switch(i)
					{
						case 1:
							index_p++;
							if(swap_uint16_t(*(uint16_t *)index_p) == 1)    //解析环保代码长度，不是17 退出
							{
								index_p += 2;
								if(*index_p == 0)
								{
									gb4.login_out_state = 1;
									rt_kprintf("-- LogIn OK....\r\n");
								}
								else
								{
									gb4.login_out_state = 0;
									rt_kprintf("-- LogIn Fail ....\r\n");
								}
							}
							index_p++;
							break;
						case 0x02:
							index_p += 5;
							//rt_kprintf("-- 收到下发的协议\r\n");
							break;
						case 0x03:
							{
								rt_mq_t 									t_mq = NULL;
								struct products_data_t 		pd = {0};
								
								index_p++;
								if(swap_uint16_t(*(uint16_t *)index_p) != 17)    //解析环保代码长度，不是17 退出
									break;
								
								pd.cmd = 3;
								pd.len = 18;
								pd.data[0] = 28;
								memset(&pd.data[1],'\0',20);
								index_p += 2;
								memcpy(&pd.data[1],index_p,17);   //拷贝机械环保代码
								t_mq = get_products_mq();
								if(t_mq != NULL)
									rt_mq_send(t_mq,&pd,sizeof(struct products_data_t));
										
								rt_kprintf("-- the login set vin (3):%s\r\n",&pd.data[1]);
								index_p += 17;
							}
							break;
						case 0x04:
							
							break;
						case 0x05:
							break;
						case 0x06:
							{
								rt_mq_t 									t_mq = NULL;
								struct products_data_t 		pd = {0};
								
								index_p++;
								if(swap_uint16_t(*(uint16_t *)index_p) != 17)    //解析环保代码长度，不是17 退出
									break;
								
								pd.cmd = 3;
								pd.len = 18;
								pd.data[0] = 28;
								memset(&pd.data[1],'\0',20);
								index_p += 2;
								memcpy(&pd.data[1],index_p,17);   //拷贝机械环保代码
								t_mq = get_products_mq();
								if(t_mq != NULL)
								{
									//rt_mq_send(t_mq,&pd,sizeof(struct products_data_t));
								}
										
								rt_kprintf("-- the login set vin (7):%s\r\n",&pd.data[1]);
								index_p += 17;
							}
							break;
						case 0x07:
							{
								rt_mq_t 									t_mq = NULL;
								struct products_data_t 		pd = {0};
								
								index_p++;
								if(swap_uint16_t(*(uint16_t *)index_p) != 1)    //解析环保代码长度，不是17 退出
									break;
								
								pd.cmd = 3;
								pd.len = 1;
								pd.data[0] = 35;
								index_p += 2;
								if(*index_p == 3 || *index_p == 4)
								{
									pd.data[1] = *index_p;
									t_mq = get_products_mq();
									if(t_mq != NULL && read_config_emission() != pd.data[1])
									{
										rt_mq_send(t_mq,&pd,sizeof(struct products_data_t));
									}
										
									rt_kprintf("-- 平台绑定的是国[%d]\r\n",*index_p);
								}
							}
							break;
					}
				}
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
					case 0x80:               //设置网关地址
						{
							rt_mq_t 									t_mq = NULL;
							struct products_data_t 		pd = {0};
							uint16_t 									i = 0;
							uint32_t   								tmp = 0;
							uint8_t    								m_arry[50] = {0};
							struct lte_mq_t 					event;
							rt_mq_t										tmp_mq;
							
							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							
							pd.cmd = 3;
							pd.len = 50;
	
							memset(m_arry,'\0',sizeof(m_arry));
							tmp = swap_uint16_t(p_down_cmd->len);
							if(tmp > 50)
								tmp = 50;
							
							memcpy(m_arry,index_p,tmp);
							memset(pd.data,'\0',sizeof(pd.data));
							index_p = NULL;
							for(i = 0;i < tmp - 1;i++)
							{
								if(m_arry[i] == ':')
									index_p = &m_arry[i];
							}
							
							if(index_p != NULL)
							{
								pd.data[0] = 5;
								pd.len = index_p - m_arry;
								memcpy((char *)&pd.data[1],m_arry,pd.len);
								index_p++;
								
								*(uint16_t *)&pd.data[50] = (uint16_t)fr_atof((const char *)index_p);
								rt_kprintf("-- the set gatway addr：%s : %d\r\n",(char *)&pd.data[1],*(uint16_t *)&pd.data[50]);
							
								event.cmd = 0;
		
								tmp_mq = get_lte_link_mq();
								rt_kprintf("-- reboot net cmd from en plat ......\r\n");  //
								if(tmp_mq != NULL)
									rt_mq_send(get_lte_link_mq(),&event,sizeof(struct lte_mq_t)); 
							}
							
							t_mq = get_products_mq();
							if(t_mq != NULL)
								rt_mq_send(t_mq,&pd,sizeof(struct products_data_t));
						
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x80;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;
							
							return build_gb4_response_packets(buf,size,p_res);
						}
					case 0x92:           //设置机械环保代码 
						{
							rt_mq_t 									t_mq = NULL;
							struct products_data_t 		pd = {0};
							
							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							
							pd.cmd = 3;
							pd.len = 18;
							pd.data[0] = 28;
							memset(&pd.data[1],'\0',20);
							memcpy(&pd.data[1],index_p,17);
							//printf("-- the vin:%s\r\n",mq.data);
							t_mq = get_products_mq();
							if(t_mq != NULL)
								rt_mq_send(t_mq,&pd,sizeof(struct products_data_t));
							
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x92;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;
							
							rt_kprintf("-- the set vin 0x92：%s\r\n",&pd.data[1]);
							
							return build_gb4_response_packets(buf,size,p_res);
						}
						
					case 0x94:          //设置三合一状态
						{
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x94;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 1;              //
							
							return build_gb4_response_packets(buf,size,p_res);
						}
						
					case 0x96:          // 设置国标数据上传时间间隔
						{
							rt_mq_t 									t_mq = NULL;
							struct products_data_t 		pd = {0};
							
							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							
							pd.cmd = 3;
							pd.len = 4;
							pd.data[0] = 33;

							*(uint16_t *)&pd.data[1] = swap_uint16_t(*(uint16_t *)index_p);

							t_mq = get_products_mq();
							if(t_mq != NULL)
								rt_mq_send(t_mq,&pd,sizeof(struct products_data_t));
							
							rt_kprintf("-- set gb4 upload cycle:%d\r\n",*(uint16_t *)&pd.data[1]);
							
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x96;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
							
						}
						
					case 0x81:   //设置企标数据上传时间
						{
							rt_mq_t 									t_mq = NULL;
							struct products_data_t 		pd = {0};
							
							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							
							pd.cmd = 3;
							pd.len = 4;
							pd.data[0] = 9;

							*(uint16_t *)&pd.data[1] = swap_uint16_t(*(uint16_t *)index_p);

							t_mq = get_products_mq();
							if(t_mq != NULL)
								rt_mq_send(t_mq,&pd,sizeof(struct products_data_t));
							
							rt_kprintf("-- set en upload cycle:%d\r\n",*(uint16_t *)&pd.data[1]);
							
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x81;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
							
						}
					case 0x82:   //设置休眠时间
						{
							rt_mq_t 									t_mq = NULL;
							struct products_data_t 		pd = {0};
							
							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							
							pd.cmd = 3;
							pd.len = 4;
							pd.data[0] = 8;

							*(uint32_t *)&pd.data[1] = swap_uint32_t(*(uint32_t *)index_p);

							t_mq = get_products_mq();
							if(t_mq != NULL)
								rt_mq_send(t_mq,&pd,sizeof(struct products_data_t));
							
							rt_kprintf("-- set sleep time : %d\r\n",*(uint32_t *)&pd.data[1]);
							
							//比较收到的VIN，根据结果应答
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = 0x82;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;

							return build_gb4_response_packets(buf,size,p_res);
						}
					
					case 0x97:   //设置整车工作时间（英轩使用）
					{
						struct edge_mq_t 					tmp = {0};
						rt_mq_t   								tmp_mq = NULL;
							
						index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							
						tmp.cmd = 0;
						memset(tmp.data,0,sizeof(tmp.data));
						*(uint32_t *)tmp.data = swap_uint32_t(*(uint32_t *)index_p);
						tmp_mq = get_edge_cmd_mq();
						if(tmp_mq != NULL)
						{
							rt_mq_send(tmp_mq,&tmp,sizeof(struct edge_mq_t));
						}
							
						rt_kprintf("-- Set vehicle Work Time %d\r\n",*(uint32_t *)tmp.data);
							
						//比较收到的VIN，根据结果应答
						p_res = (struct down_cmd_res_str *)array;
						p_res->cmd_id = 0x81;
						p_res->msg_type = 0x97;
						p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
						p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
						p_res->res = 0;

						return build_gb4_response_packets(buf,size,p_res);
					}
					default:
						{
							p_res = (struct down_cmd_res_str *)array;
							p_res->cmd_id = 0x81;
							p_res->msg_type = p_down_cmd->cmd;
							p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
							p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
							p_res->res = 0;
							return build_gb4_response_packets(buf,size,p_res);
						}
				}
			}
		case 0x82: //终端控制
			{
				p_down_cmd = (struct down_cmd_con_str *)(data + sizeof(struct start_str));
				//rt_kprintf("-- recv cmd.....%d,%d\r\n",Cmd_Type,p_down_cmd->cmd);
				switch(p_down_cmd->cmd)
				{
					case 0x88:
					{
						struct rt_can_event event = {0};
						rt_mq_t t_mq = NULL;
						
						event.cmd = 1;
						event.arg1 = *((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
						event.arg2 = swap_uint32_t(p_down_cmd->seriarl[0]);
						event.arg3 = swap_uint32_t(p_down_cmd->seriarl[1]);
						event.arg4 = 0x82;
						event.arg5 = 0x88;
						
						t_mq = get_can_lock_mq();
						if(t_mq != NULL)
							rt_mq_send(t_mq,&event,sizeof(event));
						
						//rt_kprintf("-- Recv mon cmd...Down..%d\r\n",event.arg1);
						
						return 0;
					}
					
					case 0x86:    //锁车命令
						{
							struct rt_can_event event = {0};
							rt_mq_t tmp_mq = NULL;
							
							event.cmd = 0;    //
							event.arg1 = 1 + *((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));   //解锁车命令
							event.arg2 = swap_uint32_t(p_down_cmd->seriarl[0]);    //下行命令序列号
							event.arg3 = swap_uint32_t(p_down_cmd->seriarl[1]);
							
							//rt_kprintf("-- recv lock cmd %d,0x%X,0x%X\r\n",event.arg1,event.arg2,event.arg3);
							
							event.arg4 = 0x82;
							event.arg5 = 0x86;
							tmp_mq = get_can_lock_mq();
							if(tmp_mq != NULL)
								rt_mq_send(tmp_mq,&event,sizeof(event));
							 
							return 0;
						}
						
					case 0x87:    //解锁命令
						{
							struct rt_can_event event = {0};
							rt_mq_t t_mq = NULL;
							
							event.cmd = 0;    //解锁车
							event.arg1 = *((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));   //解锁车命令
							event.arg2 = swap_uint32_t(p_down_cmd->seriarl[0]);    //下行命令序列号
							event.arg3 = swap_uint32_t(p_down_cmd->seriarl[1]);
							event.arg4 = 0x82;
							event.arg5 = 0x87;
							 
							t_mq = get_can_lock_mq();
							if(t_mq != NULL)
								rt_mq_send(t_mq,&event,sizeof(event));

							return 0;
						}
						
					case 0x01:     //远程升级命令
						{
							struct iap_ftp_str ftp_info;
							uint16_t 	tmp_len;
							
							tmp_len = swap_uint16_t(p_start->len);
							index_p = ((uint8_t *)p_down_cmd + sizeof(struct down_cmd_con_str));
							
							memset((uint8_t *)&ftp_info,'\0',sizeof(struct iap_ftp_str));
							if(get_buf_str(8,9,';',index_p,array,tmp_len) > 0)      
							{
								index_p = array + 6;
								offset += get_given_string((uint8_t *)index_p, ':',ftp_info.user);
								offset += get_given_string((uint8_t *)index_p + offset, '@', ftp_info.passwd);
								offset += get_given_string((uint8_t *)index_p + offset, ':', ftp_info.host);
								offset += get_given_number((uint8_t *)index_p + offset, '/', &ftp_info.port);
								offset += get_given_string((uint8_t *)index_p + offset, ':', ftp_info.files);
								
								rt_kprintf("\r\n-- FTP FtpUserName:%s\r\n",ftp_info.user);
								rt_kprintf("-- FTP FtpUserPassd:%s\r\n",ftp_info.passwd);
								rt_kprintf("-- FTP Server Addr:%s\r\n",ftp_info.host);
								rt_kprintf("-- FTP Server Port:%d\r\n",ftp_info.port);
								rt_kprintf("-- FTP File Name:%s\r\n",ftp_info.files);	

								if(read_iap_state() == 0)
								{
									rt_thread_t	 tid_iap = NULL;
		
									tid_iap =  rt_thread_create("ftp_iap",thread_entry_iap,&ftp_info,2048, 27, 20); 
									if(tid_iap != RT_NULL)
									{
										rt_kprintf("-- Create Ftp iap thread OK ... \r\n");
										rt_thread_startup(tid_iap);
									}
									
									rt_thread_delay(100);
								}
								
								p_res = (struct down_cmd_res_str *)array;
								p_res->cmd_id = 0x82;
								p_res->msg_type = 0x01;
								p_res->ser_num[0] = swap_uint32_t(p_down_cmd->seriarl[0]);
								p_res->ser_num[1] = swap_uint32_t(p_down_cmd->seriarl[1]);
								p_res->res = 0;
								
								return build_gb4_response_packets(buf,size,p_res);
							}
					}
					break;
				}
				default:
					break;
		}
	}
	return 0;
}



/************************************************
**	国标数据登录
**	cmd:登录或者登出
 ************************************************/
uint16_t build_gb_vehicle_login_out(uint16_t cmd,uint8_t *source,uint16_t size)
{
	struct start_str    		    *p_start = NULL;
	struct gb_login_out_str     *p_login = NULL;
	uint16_t 										len = 0;
	uint8_t 										buf[100];
	
	memset(buf,0,sizeof(buf));
	
	if(read_config_product_mode() != 0x55)
		return 0;
	
	p_start = (struct start_str *)buf;
	p_start->head[0] = 0x23;
	p_start->head[1] = 0x23;
	p_start->cmd = cmd;
	
	memset(p_start->vin,0,17);
	if(read_config_vin_state() == 0x5A)
	{
		read_config_vin_info(p_start->vin,17);
	}
	else
	{
		read_config_terminal_id(p_start->vin,16);   //如果没有
	}

	p_start->soft_ver = (uint8_t)read_user_ver();
	p_start->encrypt = SM2;
	
	if(cmd == LOGIN)
		p_start->len = swap_uint16_t(sizeof(struct gb_login_out_str));
	else
		p_start->len = swap_uint16_t(sizeof(struct gb_login_out_str) - 20);
	
	memcpy(source, buf, sizeof(struct start_str));
	len += sizeof(struct start_str);
	
	
	p_login = (struct gb_login_out_str *)buf;
	build_rtc_time((uint8_t *)p_login->time,6);    //数据采集时间
	
	if(cmd == LOGIN)//登入
	{
		p_login->serial_num = swap_uint16_t(gb4_serial_num.login_num);
		//printf("-- GB4 Login serial num:%d\r\n",gb4_serial_num.login_num);
		gb4_serial_num.login_num++;
		write_login_out_num();
		read_lte_icc_id(p_login->iccid,sizeof(p_login->iccid));
		memcpy(source + len,buf,sizeof(struct gb_login_out_str));	
		len +=	sizeof(struct gb_login_out_str);
	}
	else//登出
	{
		p_login->serial_num = swap_uint16_t((gb4_serial_num.login_num - 1));//登出流水号必须和登入流水号一致(登入时流水号+1 存储)
		//printf("-- GB4 LogOut serial num:%d\r\n",gb4_serial_num.login_num - 1);
		memcpy(source + len,buf,sizeof(struct gb_login_out_str) - sizeof(p_login->iccid));	
		len +=	(sizeof(struct gb_login_out_str) - sizeof(p_login->iccid));
	}
	*(source + len) = calc_xor_verify(source + 2, len - 2);
	len++;
	
	return len;
}



/*********************************
**	OBD故障码
**********************************/

uint16_t make_frame_gb4_fc(uint8_t *buf,uint16_t size)
{
	uint16_t 						len = 0;
	uint8_t 						array[256];
	uint16_t 						i = 0;
	struct obd_fc_t		 *p_obd = NULL;

	if(buf == NULL || size < 256)
		return 0;
	
	len += sizeof(struct obd_fc_t);
	
	i =	read_gb4_dm_serial(array,sizeof(array));
	//printf("-- gb1939 :%d\r\n",i);
	memcpy(buf + len,array,i);
	len += i;

	p_obd = (struct obd_fc_t *)array;

	p_obd->type = 0x01;    //
	build_rtc_time(p_obd->rtc,6);
	p_obd->protocol = 2;		//OBD协议
	p_obd->alarm_state = read_mil_light_state();	//报警状态
	p_obd->num = i / 4;

	memcpy(buf,array,sizeof(struct obd_fc_t));
	//printf("-- gb1939 fc code :%d\r\n",len);

	return len;
}



/*********************************
**	OBD故障码
**********************************/

uint16_t make_frame_qb4_fc(uint8_t *buf,uint16_t size)
{
	uint16_t 						len = 0;
	uint8_t 						array[256];
	uint16_t 						i = 0;
	struct qb4_fc_t			*p = NULL;

	if(buf == NULL || size < 256)
		return 0;
	
	len += sizeof(struct qb4_fc_t);
	
	i =	read_qb4_dm_serial(array,sizeof(array));
	//printf("-- gb1939 :%d\r\n",i);
	memcpy(buf + len,array,i);
	len += i;

	p = (struct qb4_fc_t *)array;

	p->type = 0xA2;    	//
	p->len = swap_uint16_t(i + 1);			//长度
	p->num = i / 4;			//

	memcpy(buf,array,sizeof(struct qb4_fc_t));
	//printf("-- QB FC Dode :%d\r\n",len);

	return len;
}





/*********************************
**	故障码 （诊断故障）
**********************************/

uint16_t make_frame_general_fc(uint8_t *buf,uint16_t size)
{
	uint16_t 								len = 0;
	uint8_t 								array[256];
	uint16_t 								i = 0;
	struct general_fc_t			*p = NULL;

	if(buf == NULL || size < 256)
		return 0;
	
	len += sizeof(struct general_fc_t);
	
	i =	read_qb4_dm_serial(array,sizeof(array));
	if(i > 0)
	{
		*(buf + len++) = 1;
		*(buf + len++) = i / 4;
		memcpy(buf + len,array,i);
		len += i;
	}
	
	i =	read_tcu_dm_serial(array,sizeof(array));
	
	if(i > 0)
	{
		*(buf + len++) = 2;
		*(buf + len++) = i / 4;
		memcpy(buf + len,array,i);
		len += i;
	}
	
	if(i == sizeof(struct general_fc_t))
		return 0;

	p = (struct general_fc_t *)array;

	p->type = 0x1F;    	//
	p->total_len = swap_uint16_t(len - sizeof(struct general_fc_t));			//长度
	

	memcpy(buf,array,sizeof(struct general_fc_t));
	//rt_kprintf("-- General FC Dode :%d\r\n",len);

	return len;
}






/***************************************
**	国四排放数据流
***************************************/

uint16_t make_frame_data_stream_dpf_scr_real(uint8_t *buf,uint16_t size)
{
	uint16_t 									len = 0;
	uint8_t 									array[256];
	struct dpf_scr_real_t			dpf_scr_real;   //02
	struct ecu_data_str				*p_ecu;
	uint32_t 						tmp = 0;
	
	if(buf == NULL || size < 120)
		return 0;

	p_ecu = (struct ecu_data_str *)array;
	read_ecu_data(p_ecu,sizeof(array));	
	
	dpf_scr_real.type = 0x02; 
	build_rtc_time(dpf_scr_real.rtc,6);
	tmp = (read_gnss_speed() * 256.0 / 100.0);   //
	dpf_scr_real.speed =  swap_uint16_t(tmp);							//车速  使用GPS速度
	
	dpf_scr_real.air_pressure = p_ecu->air_pressure;													//大气压力
	dpf_scr_real.engine_torque = p_ecu->engine_torque;												//发动机实际扭矩
	dpf_scr_real.friction_torque = p_ecu->friction_torque;										//摩擦扭矩
	dpf_scr_real.engine_rotate = swap_uint16_t(p_ecu->engine_rotate); 				//发动机转速
	dpf_scr_real.fuel_flow = swap_uint16_t(p_ecu->engine_fuel_flow);					//发动机燃料流量
	dpf_scr_real.enter_volume = swap_uint16_t(p_ecu->enter_volume);								//进气量
	dpf_scr_real.dpf_diffPressure = swap_uint16_t(p_ecu->dpf_diffPressure);			//DPF压差
	dpf_scr_real.cooling_temp = p_ecu->coolant_temp;														//冷却液温度
	dpf_scr_real.fuel_position = p_ecu->fuel_percent;										//燃油液位
	
	if(read_ecu_type() == 0x02 || read_ecu_type() == 0x04)    //潍柴 SOC  玉柴
	{
		dpf_scr_real.scr_in_temp = swap_uint16_t(p_ecu->scr_entrance_temp); 			//SCR入口温度
		dpf_scr_real.scr_out_temp = 0xFFFF;							//SCR出口温度
		dpf_scr_real.scr_up_nox = 0xFFFF;				//SCR上游NOx传感器输出值
		dpf_scr_real.scr_down_nox = swap_uint16_t(p_ecu->scr_downstream_nox);			//SCR下游NOx传感器输出值
		
		dpf_scr_real.reactant_allowance = p_ecu->reactant_allowance; 										//反应剂余量  尿素液位
		
		dpf_scr_real.egr_opening = 0xFFFF;				//EGR阀开度
		dpf_scr_real.egr_setting = 0xFFFF;				//EGR设定值
		
	}
	else                 //上柴 EGR
	{
		dpf_scr_real.scr_in_temp = 0xFFFF; 			//SCR入口温度
		dpf_scr_real.scr_out_temp = 0xFFFF;			//SCR出口温度
		
		dpf_scr_real.scr_up_nox = 0xFFFF;				//SCR上游NOx传感器输出值
		dpf_scr_real.scr_down_nox = 0xFFFF;			//SCR下游NOx传感器输出值
		
		dpf_scr_real.reactant_allowance = 0xFF; 										//反应剂余量
		
		dpf_scr_real.egr_opening = swap_uint16_t(p_ecu->egr_opening);				//EGR阀开度
		dpf_scr_real.egr_setting = swap_uint16_t(p_ecu->egr_setting);				//EGR设定值
		//rt_kprintf("-- EGR;%d,%d\r\n",p_ecu->egr_opening,p_ecu->egr_setting);
	}
	
	
	memcpy(buf,(uint8_t *)&dpf_scr_real,sizeof(struct dpf_scr_real_t));	
	len  =	sizeof(struct dpf_scr_real_t);

	return len;
}



/***************************************
**	国四排放数据流
***************************************/

uint16_t make_frame_data_stream_dpf_scr_average(uint8_t *buf,uint16_t size)
{
	uint16_t 										len = 0;
	struct exhaust_data_t 			tmp;
	struct dpf_scr_average_t 		dpf_scr_average;    //
	
	if(buf == NULL || size < 120)
		return 0;

	dpf_scr_average.type = 0x04;  	    		//信息标识
	build_rtc_time(dpf_scr_average.rtc,6);
	dpf_scr_average.ref_torque = swap_uint16_t(read_max_ref_torque());			// 参考扭矩  固定值
	
	read_exhaust_data(&tmp);
	dpf_scr_average.engine_power = swap_uint16_t(tmp.engine_power);		// 发动机平均功率
	//dpf_scr_average.engine_power = swap_uint16_t(50);		// 发动机平均功率
	
	dpf_scr_average.scr_up_nox = swap_uint16_t(tmp.scr_up_nox);			// SCR上游NOx平均浓度
	dpf_scr_average.scr_down_nox = swap_uint16_t(tmp.scr_down_nox);		// SCR下游NOx平均浓度
	dpf_scr_average.scr_up_flow = (uint8_t)tmp.scr_up_flow;		// SCR上游NOx平均质量流量
	dpf_scr_average.scr_down_flow = (uint8_t)tmp.scr_down_flow;     	// SCR下游NOx平均质量流量
	dpf_scr_average.src_in_temp = swap_uint16_t(tmp.src_in_temp);		// SCR入口平均温度
	dpf_scr_average.src_out_temp = swap_uint16_t(tmp.src_out_temp);  		// SCR出口平均温度
	dpf_scr_average.fuel_flow = swap_uint16_t(tmp.fuel_flow);			// 发动机燃料流量平均值
	dpf_scr_average.cycle_calculate = swap_uint16_t(tmp.index);    // 统计周期时长
	dpf_scr_average.cycle_pwm = tmp.cycle_pwm; 		  	// 统计周期内有效时间占比
	
	memcpy(buf + len,(uint8_t *)&dpf_scr_average,sizeof(struct dpf_scr_average_t));	
	len  =	sizeof(struct dpf_scr_average_t);

	return len;
}





/************************************************
**
**
************************************************/

uint16_t build_gb4_platform_data(uint8_t *buf,uint16_t size)
{
	struct start_str					*p_start = NULL;
	struct gnss_msg_str				*p_gnss = NULL;      
	uint8_t 									array[300] = {0};
  uint16_t	                len = 0;
	uint16_to_byte            tmp16_t; 
  uint16_t            			index = 0;

	
  if(buf == NULL || size < 300)
	//if(1)
	{
		return 0;
	}
        
	len = sizeof(struct start_str);
	/* 数据打包时间 */
	
	build_rtc_time(array,sizeof(array));
	memcpy(buf + len,array,6);
	len += 6;
	/* 信息流水号 */
	tmp16_t.value = swap_uint16_t(gb4_serial_num.serial_gb); 
	
	memcpy(buf + len,&tmp16_t.byte[0],2);
	len += 2;
	gb4_serial_num.serial_gb++;
	//printf("-- GB4 Data Serial Num...%d\r\n",login_out_num.serial_gb);
	write_login_out_num();
	
    /*GNSS定位信息*/
  p_gnss = (struct gnss_msg_str *)array;
	
  p_gnss->status = 0;
	if('A' != read_gnss_positing_state())
		p_gnss->status |= 0x01; //  定位状态
	else
		p_gnss->status &= 0xFE;
	
	if('S' == read_gnss_latitude_sn()) 	//南纬
		p_gnss->status |= 0x02;
	
	if('W' == read_gnss_longitude_ew())	//西经
		p_gnss->status |= 0x04;
	
	p_gnss->latitude = swap_uint32_t(read_gnss_latitude(0));	 //纬度
	p_gnss->longitude =swap_uint32_t(read_gnss_longitude(0));	 //经度
	memcpy(buf + len,array,sizeof(struct gnss_msg_str));	
	len += sizeof(struct gnss_msg_str);
	
	index = index;

	/** OBD诊断信息 0x01 **/
	index = make_frame_gb4_fc(array,sizeof(array));	  //J1939故障码
	memcpy(buf + len,array,index);
	len += index;
	//printf("-- gb4 the obd data len:%d\r\n",index);
	/** 数据流信息 0x02 **/
	#if 1  //DPF/SCR排放
	//printf("-- gb4 the obd data len:%d\r\n",index);
	/** 数据流信息 0x02 **/
	index = make_frame_data_stream_dpf_scr_real(array,sizeof(array));
	memcpy(buf + len,array,index);
	len += index;
	//printf("-- gb4 make_frame_data_stream_dfp_scr_real:%d\r\n",index);

	index = make_frame_data_stream_dpf_scr_average(array,sizeof(array));
	memcpy(buf + len,array,index);
	len += index;
	//printf("-- gb4 make_frame_data_stream_dpf_scr_average:%d\r\n",index);
	#else  //TWC排放
	index = make_frame_data_stream_twc_real(array,sizeof(array));
	memcpy(buf + len,array,index);
	len += index;
	//printf("-- gb4 make_frame_data_stream_twc_real:%d\r\n",index);
	#endif
	//printf("-- gb4 make_frame_data_stream:%d\r\n",index);
	//memcpy(buf + len,encrypt_chip.id_lot,16);

	read_encryption_chip_id(array,16);
	memcpy(buf + len,array,16);
	
	memset(array,0,sizeof(array));
	if(64 != acl_write_then_read(CRY_SIGN,buf + sizeof(struct start_str),array,len + 16 - sizeof(struct start_str)))
	{
		//rt_kprintf("-- acl16 eerr....\r\n");
		//signfail_cnt++;
	}
	buf[len++] = 0x20;
	memcpy(buf + len,&array[0],32);
	len += 32;
	buf[len++] = 0x20;
	memcpy(buf + len,&array[32],32);
	len += 32;


	p_start = (struct start_str *)array;
	p_start->head[0] = 0x23;
	p_start->head[1] = 0x23;

	p_start->cmd = 0x02;
	memset(p_start->vin,0,17);

	read_config_vin_info(p_start->vin,17);
	p_start->soft_ver = (uint8_t)read_user_ver();
	p_start->encrypt = SM2;
	p_start->len = swap_uint16_t(len - sizeof(struct start_str));
	
	memcpy(buf, array, sizeof(struct start_str));
	buf[len] = calc_xor_verify(buf + 2,len - 2);
	len++;
	//printf("-- build gb4 packe:%d,%d\r\n",swap_uint16_t(p_start->len),len);
	return len;
}




/**************************************
** 0x80 设备状态信息 
**************************************/

uint16_t make_frame_device_info(uint8_t *buf,uint16_t size)
{
	struct terminal_msg_str			tmp_msg;
	uint16_t	    				len = 0;
	
	tmp_msg.msg_type = 0x80;
	tmp_msg.msg_len = swap_uint16_t(sizeof(struct terminal_msg_str) - 3);

	memset(tmp_msg.vin,0,17);
	if(read_config_vin_state() == 0x5A)
	{
		read_config_vin_info(buf,sizeof(buf));
		memcpy(tmp_msg.vin,buf,17);           		//VIN号
	}
	
	memset(tmp_msg.id,0,17);   //设备编号   
	read_config_terminal_id(buf,sizeof(buf));
	//rt_kprintf("-- the terminal is:%s\r\n",buf);
	memcpy(tmp_msg.id,buf,16);   //设备编号     			
	tmp_msg.manu_num = 0x01;          			//厂家编码  0x01标识博创
	tmp_msg.terminal_type = 0x27;     			//终端型号
	tmp_msg.user_num = read_config_user_code();         //使用方编号
	tmp_msg.car_type = read_config_car_type();     		 	//安装车型 
	
	tmp_msg.app_ver1 = read_app_version();      					//终端上传协议版本(修改为)
	tmp_msg.app_ver2 = read_boot_loaer_ver_num();       	//应用程序版本号    
	tmp_msg.hd_ver = 11;        		 											//硬件版本号
	tmp_msg.io_status = read_in_io_state();								//IO状态
	tmp_msg.acc_status = read_in_acc_state();     				//ACC状态
	//printf("-- acc state:%d\r\n",tmp_msg.acc_status);
	tmp_msg.moto_status = read_in_moto_state();						//MOTO状态
	tmp_msg.input_frq1 = swap_uint16_t((uint16_t)read_user_ver());//外部输入频率
	tmp_msg.input_frq2 = swap_uint16_t((uint16_t)read_files_sys_state());//外输输入频率
	tmp_msg.output_frq3 = swap_uint16_t(read_sd_total_mb());						//PWM1输出频率
	tmp_msg.output_frq4 = swap_uint16_t(read_sd_free_mb());						//PWM2 输出频率
	tmp_msg.input_vol1 = swap_uint16_t(read_config_emission());							//外部输入电压1
	tmp_msg.input_vol2 = swap_uint16_t(read_ecu_type());							//外部输入电压2
	tmp_msg.power_vol = swap_uint16_t(read_in_power_vol() * 100);							//外部输入电压
	tmp_msg.batter_vol = swap_uint16_t(read_in_batter_vol() * 100);							//内部电池输入电压
	//rt_kprintf("-- the vechel data:%d,%d\r\n",tmp_msg.power_vol,tmp_msg.batter_vol);
	tmp_msg.warn_value = swap_uint32_t(read_in_alarm());							//设备报警值
	//rt_kprintf("-- the alarm value:0x%X\r\n",read_in_alarm());
	tmp_msg.gnss_speed = swap_uint16_t(read_gnss_speed() / 10);   					//GNSS 速度
	tmp_msg.gnss_heading = swap_uint16_t(read_gnss_heading());   				//GNSS 方向
	tmp_msg.gnss_altitude = swap_uint16_t(read_gnss_altitude());          //GNSS 海拔高度
	//rt_kprintf("-- the Gnss info:%d,%d,%d\r\n",tmp_msg.gnss_speed,tmp_msg.gnss_heading,tmp_msg.gnss_altitude);
	tmp_msg.gnss_used_satellite = read_gnss_satellite_num();    //GNSS 使用卫星数
	tmp_msg.gnss_view_satellite = read_gnss_bd_sate_num() + read_gnss_gps_sate_num();    //GNSS 可视卫星数
	tmp_msg.gnss_hdop = swap_uint16_t(read_gnss_hdop());             	//GNSS 水平经度因子
	//rt_kprintf("-- 水平精度因子:%d\r\n",tmp_msg.gnss_hdop);
	
	tmp_msg.gnss_mondel_status = 0;
	if('A' == read_gnss_positing_state())
		tmp_msg.gnss_mondel_status |= 0x0A; //  定位状态
	
	tmp_msg.nj_mon_state = 0;    //农机三合一功能状态；0：功能关闭；1：功能开启
	tmp_msg.nj_socket_state = 0;   //农机三合一链接状态
	tmp_msg.data_model = 0;        //数据模式
	tmp_msg.csq = read_lte_csq();    // LTE信号值
	tmp_msg.arch_state = read_config_archival_state();            //备案状态 （最近一次 备案状态）
	
	tmp_msg.ver_security = read_acl16_app_version();
	read_encryption_chip_id(tmp_msg.id_security,sizeof(tmp_msg.id_security));
	tmp_msg.nj_send_num = swap_uint32_t(0);
	//printf("-- the nj send num: %d\r\n",tmp_msg.nj_send_num);

	memcpy(buf,&tmp_msg,sizeof(struct terminal_msg_str));	
	len  +=	sizeof(struct terminal_msg_str);
	
	//printf("-- the device_info_make_frame %d\r\n",len);

	return len;
}




/***********************************
**	组包车身扩展消息
**	
************************************/

uint16_t make_fram_vehicle_info(uint8_t *buf,uint16_t size)
{
	uint16_t 										len = 0;
	struct vehicle_msg_str			tmp_msg;
	struct ecu_data_str					*p_ecu;
	uint8_t 										array[256];

	if(buf == NULL || size < 120)
		return 0;

	tmp_msg.msg_type = 0x10;
	tmp_msg.msg_len = swap_uint16_t(sizeof(struct vehicle_msg_str) - 3);
	
	p_ecu = (struct ecu_data_str *)array;
	read_ecu_data(p_ecu,sizeof(struct ecu_data_str));

	if(read_ecu_type() == 0x02)
		tmp_msg.accumulator_vol = swap_uint16_t(read_in_power_vol());   //电源电压  3026
	else
		tmp_msg.accumulator_vol = swap_uint16_t(p_ecu->accumulator_vol);
	
	tmp_msg.fuel_temp = p_ecu->fuel_temp;								//燃油温度  4106
	tmp_msg.engine_breakdown = swap_uint16_t(p_ecu->engine_breakdown);//发动机舱内温度  4244
	tmp_msg.air_temp = swap_uint16_t(p_ecu->air_temp);									//大气温度  4245
	tmp_msg.oil_water_pilot = swap_uint16_t(p_ecu->max_ref_torque);									//路面温度 修改为
	tmp_msg.vehicle_work_time = swap_uint32_t(read_vehicle_work_time());	//发动机工作时间  4163
	tmp_msg.accelerator_percent = p_ecu->accelerator;			//加速踏板行程值  2212
	tmp_msg.engine_load_percent = p_ecu->engine_load;  		//发动机负荷   4247
	tmp_msg.once_travel = swap_uint32_t(p_ecu->once_travel);							//单次行驶距离  4180
	tmp_msg.total_travel = swap_uint32_t(p_ecu->total_travel);  						//总里程  2205
	tmp_msg.once_fuel = swap_uint32_t(p_ecu->once_fuel);      					//单次油耗  （瞬时油耗）  4022
	
	tmp_msg.total_fuel = swap_uint32_t(read_vehicle_total_fule());  							//累计油耗 4023
	
	tmp_msg.relative_oil_pressure = swap_uint16_t(p_ecu->relative_oil_pressure);		//相对机油压力  4248
	tmp_msg.absolute_oil_pressure = swap_uint16_t(p_ecu->absolute_oil_pressure);		//绝对机油压力  4249
	//rt_kprintf("-- 机油压力：%d,%d\r\n",p_can->relative_oil_pressure,p_can->absolute_oil_pressure);
	tmp_msg.relative_add_pressure = swap_uint16_t(p_ecu->relative_add_pressure);		//相对增压压力  4251
	tmp_msg.absolute_add_pressure = swap_uint16_t(p_ecu->absolute_add_pressure);		//绝对增压压力  4252
	tmp_msg.oil_position = p_ecu->oil_position;							//机油液位  4253
	tmp_msg.oil_temp = swap_uint16_t(p_ecu->oil_temp);         				//机油温度    4243
	//rt_kprintf("-- the oil temp:%d\r\n",p_ecu->oil_temp);
	tmp_msg.engine_air_temp = p_ecu->entered_air_temp;          //发动机支气管温度 4338
	tmp_msg.gearbox_out_rotate = swap_uint16_t(p_ecu->gearbox_out_rotate);   //曲轴箱压力（预留 传送1）    4245
	tmp_msg.cool_pressure = p_ecu->cool_position;  					//冷却液压力  4255
	tmp_msg.cool_position =p_ecu->cool_position;						//冷却液位置      4256
	tmp_msg.lock_preparative_status = read_ecu_lock_state();	//ECU锁车状态    4257
	tmp_msg.mon_status = read_ecu_mon_status();   						//锁车功能状态  4258
	tmp_msg.key_status = read_ecu_key_status();     					//KEY码状态   4259
	tmp_msg.id_status = read_ecu_id_status();								//TBOX ID状态  4260
	tmp_msg.cold_boot_status = p_ecu->cold_boot_status;     		//冷启动加速状态  4261
	
	tmp_msg.drain_off_fault_class = p_ecu->drain_off_fault_class;							//
	
	tmp_msg.dpf_build_light = p_ecu->dpf_build_light;   //DPF再生指示灯
	tmp_msg.dpf_forbid_light = p_ecu->dpf_forbid_light;   		  //DPF再生禁止灯
	tmp_msg.clutch_status = p_ecu->clutch_status;						//离合开关状态    4265
	tmp_msg.fnr_active_light = p_ecu->fnr_active_light;							//FNR激活指示灯    4266
	tmp_msg.auto_model_light = p_ecu->auto_model_light;							  //离合器压力    4696
	tmp_msg.power_dis_lilght = p_ecu->power_dis_lilght;					  //变速箱齿轮油液位  4697
	tmp_msg.gearbox_filter_light = p_ecu->gearbox_filter_light;	  //变速箱齿轮油滤压差   4698
	tmp_msg.gearbox_switch = p_ecu->gearbox_switch;				  //变速箱齿轮油压力  4699
	tmp_msg.gearbox_oil_temp = swap_uint16_t(p_ecu->gearbox_oil_temp);						  //传动系机油温度 4700
	tmp_msg.gearbox_oil_temp_e = swap_uint16_t(0); //变数变速箱齿轮油液位测量状态 4701
	//rt_kprintf("-- gearbox_oil_temp:%d\r\n",p_ecu->gearbox_oil_temp);
	tmp_msg.engine_control_model = p_ecu->engine_control_model;//发动机控制模式
	tmp_msg.gearbox_oil_out_temp = swap_uint16_t(p_ecu->gearbox_oil_out_temp);         //变速器变矩器油出口温度  4704
	tmp_msg.gearbox_pressure_switch = p_ecu->gearbox_pressure_switch;      //变速箱压力开关状态   5017
	tmp_msg.braking_air_pressure = swap_uint16_t(p_ecu->braking_air_pressure);	//制动气压  5094
	tmp_msg.warn_value = 0;       						//车辆报警值 暂时传0  4271
	tmp_msg.daily_work_time =swap_uint16_t((uint16_t)read_day_total_time());    					//TBOX计算 日工作时间  5091
	tmp_msg.tbox_vehicle_work_time = swap_uint32_t(read_engine_total_time());    				//仪表计算行驶速度  预留   5092
	tmp_msg.vehicle_work_offset = swap_uint32_t(read_vehicle_offset_time());    				//仪表计运行时间  预留  5093
	 
	tmp_msg.engine_work_time = swap_uint32_t(read_engine_work_time());							//发动机工作时间原始值
	tmp_msg.fuel_consume_offset = swap_uint32_t(read_vehicle_fuel_offset());			//累计油耗偏移量
	tmp_msg.total_consume = swap_uint32_t(read_total_fuel());       //累计油耗原始值
	 
	memcpy(buf,&tmp_msg,sizeof(struct vehicle_msg_str));	
	len  +=	sizeof(struct vehicle_msg_str);

	return len;
}



/************************************************
**	企标数据
** ()
************************************************/

uint16_t build_qb4_platform_data(uint8_t *buf,uint16_t size)
{      
	uint8_t 							array[300] = {0};
	struct start_str			*p_start;
	struct gnss_msg_str		*p_gnss = NULL;
	uint16_t	            len = 0;
	uint16_t              index = 0;
	uint16_to_byte        tmp16_t;

	if(buf == NULL || size < 300)
		return 0;
    
	len += sizeof(struct start_str);
	
	/* 数据打包时间 */
	build_rtc_time(array,sizeof(array));
	memcpy(buf + len,array,6);
	len += 6;
	
	/* 信息流水号 */
	tmp16_t.value = swap_uint16_t(gb4_serial_num.serial_num); 
	memcpy(buf + len,&tmp16_t.byte[0],2);
	len += 2;
	gb4_serial_num.serial_num++;
	write_login_out_num();
	//printf("-- the qb serial.... %d\r\n",login_out_num.serial_num);
	/*GNSS定位信息*/
	p_gnss = (struct gnss_msg_str *)array;
	
  p_gnss->status = 0;
	if('A' != read_gnss_positing_state())
		p_gnss->status |= 0x01; //  定位状态
	else
		p_gnss->status &= 0xFE;
	
	if('S' == read_gnss_latitude_sn()) 	//南纬
		p_gnss->status |= 0x02;
	
	if('W' == read_gnss_longitude_ew())	//西经
		p_gnss->status |= 0x04;
	
	p_gnss->latitude = swap_uint32_t(read_gnss_latitude(0));	 //纬度
	p_gnss->longitude =swap_uint32_t(read_gnss_longitude(0));	 //经度

	memcpy(buf + len,array,sizeof(struct gnss_msg_str));	
	len += sizeof(struct gnss_msg_str );
	
	index = make_frame_gb4_fc(array,(uint16_t)sizeof(array));        //OBD故障码
	memcpy(buf + len,array,index);
	len += index;
	//rt_kprintf("-- run is make_frame_obd_fc len....%d\r\n",index);

	/** 数据流信息 0x02 **/
	#if 1  //DPF/SCR排放
	/** 数据流信息 0x02 **/
	index = make_frame_data_stream_dpf_scr_real(array,sizeof(array));
	memcpy(buf + len,array,index);
	len += index;
	//printf("-- gb4 make_frame_data_stream_dfp_scr_real:%d\r\n",index);

	index = make_frame_data_stream_dpf_scr_average(array,sizeof(array));
	memcpy(buf + len,array,index);
	len += index;
	//printf("-- gb4 make_frame_data_stream_dpf_scr_average:%d,%d\r\n",len,index);
	#else  //TWC排放
	index = make_frame_data_stream_twc_real(array,sizeof(array));
	memcpy(buf + len,array,index);
	len += index;
	//printf("-- gb4 make_frame_data_stream_twc_real : %d\r\n",index);
	#endif
	//printf("-- run is  make_frame_data_stream....%d\r\n",index);
	/* 0x80 设备状态信息 */
	//printf("-- run is  make_frame_data_stream....%d\r\n",index);
	
	/** 0x80 设备状态信息 **/
	index = make_frame_device_info(array,(uint16_t)sizeof(array));  
	memcpy(buf + len,array,index);     
	len += index;
	//printf("-- the device info len:%d\r\n",index);
	/** 0x10 车身扩展消息 扩展信息 **/
	index = make_fram_vehicle_info(array,(uint16_t)sizeof(array));  
	memcpy(buf + len,array,index);
	len += index;
	//printf("-- make_fram_vehicle_fc:%d\r\n",index);
	
	index = make_frame_general_fc(array,(uint16_t)sizeof(array));
	memcpy(buf + len,array,index);
	len += index;
	//rt_kprintf("-- make_fram TCU fc:%d\r\n",index);
	
	p_start = (struct start_str *)array;
	p_start->head[0] = 0x23;
	p_start->head[1] = 0x23;

	p_start->cmd = 0x70;
	memset(p_start->vin,0,17);
	read_config_terminal_id(p_start->vin,17);  //企标数据报文，直接使用设备号

	p_start->soft_ver = (uint8_t)read_app_version();
	p_start->encrypt = SM2;
	p_start->len = swap_uint16_t(len - sizeof(struct start_str));
	
	memcpy(buf, array, sizeof(struct start_str));
	buf[len] = calc_xor_verify(buf + 2,len - 2);
	len++;

	return len;
}


/****************************
**	拆除报警
**	
****************************/

uint16_t vehicle_dismantle_alarm(uint8_t *buf,uint16_t size)
{
	uint8_t 							array[200] = {0};
	struct start_str			*p_start;
	struct gnss_msg_str		*p_gnss = NULL;
	uint16_t	            len = 0;
	uint16_to_byte				tmp16_t;

	 if(buf == NULL || size < 300)
		return 0;
    
	len += sizeof(struct start_str);
	
	/* 数据打包时间 */
	build_rtc_time(array,sizeof(array));
	memcpy(buf + len,array,6);
	len += 6;
	
	/* 信息流水号 */
	tmp16_t.value = swap_uint16_t(gb4_serial_num.alarm_num);
	memcpy(buf + len,&tmp16_t.byte[0],2);
	len += 2;
	gb4_serial_num.alarm_num++;
	write_login_out_num();
	//rt_kprintf("-- gb4 serial num alarm:%d\r\n",gb4_serial_num.alarm_num);
	p_gnss = (struct gnss_msg_str *)array;
	
   p_gnss->status = 0;
	if('A' != read_gnss_positing_state())
		p_gnss->status |= 0x01; //  定位状态
	else
		p_gnss->status &= 0xFE;
	
	if('S' == read_gnss_latitude_sn()) 	//南纬
		p_gnss->status |= 0x02;
	
	if('W' == read_gnss_longitude_ew())	//西经
		p_gnss->status |= 0x04;
	
	p_gnss->latitude = swap_uint32_t(read_gnss_latitude(0));	 //纬度
	p_gnss->longitude =swap_uint32_t(read_gnss_longitude(0));	 //经度

	memcpy(buf + len,array,sizeof(struct gnss_msg_str));	
	len += sizeof(struct gnss_msg_str );
	
	p_start = (struct start_str *)array;
	p_start->head[0] = 0x23;
	p_start->head[1] = 0x23;
	p_start->cmd = 0x05;
	
	memset(p_start->vin,0,17);
	read_config_vin_info(p_start->vin,17);

	p_start->soft_ver = (uint8_t)read_user_ver();
	p_start->encrypt = SM2;
	p_start->len = swap_uint16_t(len - sizeof(struct start_str));
	
	memcpy(buf, array, sizeof(struct start_str));
	buf[len] = calc_xor_verify(buf + 2,len - 2);
	len++;
	
	return len;
}




/***************************
**	AC16任务

**	备案
**	登录
**	上报数据
**	如果没有机械环保代码，不进行登录
****************************/

void thread_entry_gb4(void *parameter)
{
	uint8_t 		buff[16] = {0} ;
	uint16_t 		len;
	uint16_t 		send_len = 0;
	uint8_t 		acc_state = 0;
	uint8_t 		i = 0;

	
	parameter = parameter;
	rt_thread_delay(100);
	load_login_out_num();
	gb4_down_mq = rt_mq_create("gb4_down_mq",sizeof(gb4_down_data),2,RT_IPC_FLAG_FIFO);
	
	if(read_config_emission() == 4)
	{
		if(sizeof(encrypt_chip.id_sn) == acl_write_then_read(CRY_GETCHIPID,buff,encrypt_chip.id_sn,sizeof(encrypt_chip.id_sn)))
		{
			read_config_terminal_id(encrypt_chip.id_lot,16);
			
			
			encrypt_chip.id_lot[0] = 'A';
			encrypt_chip.id_lot[1] = 'I';
			encrypt_chip.id_lot[2] = 'H';
			encrypt_chip.id_lot[3] = 'R';
			encrypt_chip.version = 13;
			encrypt_chip.state = 0;
			
			
			rt_kprintf("-- the encry chip id lot:%s\r\n",encrypt_chip.id_lot);
			
		 //mem_printf(LOG_ERROR, PRINT_HEX, encrypt_chip.id_sn, 16);
		}
		else
		{
			rt_kprintf("-- Read ACL chip id Fail \r\n");
			encrypt_chip.state = 1;
		}

		rt_thread_delay(10);
		if(4 == acl_write_then_read(CRY_GETAPPVER,buff,(uint8_t *)&encrypt_chip.version,4))
		{
			encrypt_chip.state = 0;
			rt_kprintf("-- the encry chip app version:%d\r\n",encrypt_chip.version);
		}
		else
		{
			encrypt_chip.state = 1;
			//rt_kprintf("-- the encry chip app version fail\r\n");
		}
	}
	
	for(;;)
	{
		if(read_lte_net_init_state() == 0)
    {
			rt_thread_delay(100);
			memset((uint8_t *)&gb4,0,sizeof(gb4));
      continue;
    }

		if(rt_mq_recv(gb4_down_mq,&gb4_down_data,sizeof(gb4_down_data),10) == RT_EOK)
		{
			if(gb4_down_data.len > 0)
			{
				if(strstr((char *)gb4_down_data.data,"+QIURC:") != NULL)
				{
					if(at_close_socket_connect(GB4_SOCKET_ID) == 0)
					{
						rt_kprintf("-- Close gb4 socket ok.......\r\n");
					}

					gb4.socket_state = 0;
					gb4.qb_step = 0;
					gb4.gb_step = 0;

					gb4.gb_cnt = 1;
					gb4.qb_cnt = 1;
					gb4.login_out_state = 0;

					continue;
				}

				send_len = gb4_platform_down_parse(gb4_down_data.data,gb4_down_data.len,local_buff,sizeof(local_buff));
				//mem_printf(LOG_ERROR, PRINT_HEX,local_buff, send_len);
        if(send_len > 0)
				{
					if(at_send_socket_data(GB4_SOCKET_ID,local_buff,send_len) == 0)
          {
             rt_kprintf("-- Send GB4 Down Cmd Res OK......%d\r\n",send_len);
          }
				}
			}  
		}
		
		if(read_sys_run_state() > 1 && gb4.gb_step == 0)
		{
			if(at_query_socket_state(GB4_SOCKET_ID) > 0)
			{
				if(at_close_socket_connect(GB4_SOCKET_ID) == 0)
				{
					gb4.socket_state = 0;
					gb4.qb_step = 0;
					gb4.qb_cnt = 0;
					printf("-- Close gb4 socket onnect ok...\r\n");
				}	
			}
			continue;
		}

		
		switch(gb4.qb_step)    // 企标数据
		{
			case 0:
				if(gb4.qb_cnt++ % 300 == 0)
				{
          struct socket_addr_str tmp;
					if(at_query_socket_state(GB4_SOCKET_ID) > 0)    //查询Socket创建链接的状态
					{
						if(at_close_socket_connect(GB4_SOCKET_ID) == 0)
						{
							gb4.socket_state = 0;
							gb4.gb_step = 0;
							gb4.gb_cnt = 1;
							gb4.qb_cnt = 1;
							gb4.login_out_state = 0;
							rt_kprintf("-- close gb4 socket onnect ok...\r\n");
						}	
					}

					read_enterprise_gw_addr(tmp.addr,sizeof(tmp.addr));
					tmp.port = read_enterprise_gw_port();

					if(at_creat_socket_connect(GB4_SOCKET_ID,&tmp,gb4_down_mq) == 0)
					{
						gb4.socket_state = 1;
						gb4.qb_step++;
						rt_kprintf("-- Creat gb4 socket onnect:%s:%d.....OK\r\n",tmp.addr,tmp.port);  
					}
					else
					{
						rt_kprintf("-- Creat gb4 socket onnect:%s:%d.....Fail\r\n",tmp.addr,tmp.port); 
					}         
        }
				break;
			case 1:
				send_len = 0;
				memset(local_buff,0,sizeof(local_buff));
				for(i = 0;i < 2;i++)
				{
					len = read_qb4_fifo_buff(local_buff + send_len,QB4_DATA_LEN);          //读取发送队列数据
					
					if(len > 0 )
					{
						send_len += len;
					}
				}

				if(send_len > 0 )
				{
					if(at_send_socket_data(GB4_SOCKET_ID,local_buff,send_len) == 0)
					{
						 rt_kprintf("-- Send QB4 Data OK......%d\r\n",send_len);
					}
					else
					{
						rt_kprintf("-- Send QB4 Data Fail......%d\r\n",send_len);
						gb4.qb_step = 0;
						gb4.qb_cnt = 0;
					}
				}
				break;
			default:
				break;
		}
		
		acc_state = read_in_acc_state();
		
		switch(gb4.gb_step)
		{
			case 0:
				if((acc_state > 0 || read_gb4_alarm_state() > 0) && gb4.socket_state > 0)             //如果电锁没开
				{
					gb4.gb_step++;
				}
				break;
			case 1:
				if(gb4.login_out_state == 0)     //登录
				{
					if(gb4.gb_cnt++ % 300 > 0)             //电锁打开
						break;
					len = build_gb_vehicle_login_out(0x01,local_buff,sizeof(local_buff));
					if(len > 0)
					{
						if(at_send_socket_data(GB4_SOCKET_ID,local_buff,len) == 0)
						{
							rt_kprintf("-- Send GB4 Vehicle login IN Data OK 0x01......%d\r\n",len);
							gb4.login_out_state = 1;
						}
						else
						{
							rt_kprintf("-- Send GB4 Vehicle login IN Data Fail 0x01......%d\r\n",len);
							gb4.socket_state = 0;
							gb4.qb_step = 0;
							gb4.gb_step = 0;

							gb4.gb_cnt = 1;
							gb4.qb_cnt = 1;
							gb4.login_out_state = 0;
						}
					}
				}
				else
				{
					gb4.gb_step++;
				}
				break;
			case 2:   //发送国标数据
				send_len = 0;
				memset(local_buff,0,sizeof(local_buff));
				for(i = 0;i < 5;i++)
				{
					len = read_gb4_fifo_buff(local_buff + send_len,GB4_DATA_LEN);
					if(len > 0)
					{
						send_len += len;
					}
				}
				
				if(send_len > 0 )
				{
					if(at_send_socket_data(GB4_SOCKET_ID,local_buff,send_len) == 0)
        	{
             rt_kprintf("-- Send GB4 Data OK ACC ON ...... %d\r\n",send_len);
        	}
					else
					{
						rt_kprintf("-- Send GB4 Data OK ACC Fail......%d\r\n",send_len);
						gb4.socket_state = 0;
						gb4.qb_step = 0;
						gb4.gb_step = 0;

						gb4.gb_cnt = 1;
						gb4.qb_cnt = 1;
						gb4.login_out_state = 0;
					}
				}
				
				//if(read_in_acc_state() == 0 || read_sys_run_state() > 0)
				if(read_in_acc_state() == 0)
				{
					gb4.gb_step++;
					gb4.gb_cnt = 0;	
				}		
				rt_thread_delay(10);
				break;
			case 3:  
				send_len = read_gb4_fifo_buff(local_buff,GB4_DATA_LEN);
				if(send_len > 0)
				{
					if(at_send_socket_data(GB4_SOCKET_ID,local_buff,send_len) == 0)
					{
						rt_kprintf("-- Send GB4 Data OK ACC OFF ...... %d\r\n",send_len);
					}
				}
				
				if(++gb4.gb_cnt % 50 == 0)
					gb4.gb_step++;
				break;
			case 4:           //登出
				if(gb4.login_out_state == 0 || gb4.gb_cnt > 100)
				{
					gb4.gb_step = 0;
					gb4.login_out_state = 0;
					gb4.gb_cnt = 0;
					break;
				}

				if(++gb4.gb_cnt % 50 > 0)
					break;
				
				send_len = build_gb_vehicle_login_out(0x04,local_buff,sizeof(local_buff));
				if(send_len > 0)
				{
					if(at_send_socket_data(GB4_SOCKET_ID,local_buff,send_len) == 0)
					{
						rt_kprintf("-- Send QB4 Vehicle Log Out Data OK 0x04......%d\r\n",send_len);
					}
				}
				break;
      default:
        gb4.gb_step = 0;
				gb4.login_out_state = 0;
        break;
		}	
	}
}





/**************************File End***************************/



