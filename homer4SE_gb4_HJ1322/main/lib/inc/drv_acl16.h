



#ifndef _DRV_ACL16_H
#define _DRV_ACL16_H


/****************************************************************************
** File Name:      bc_acl16.h                                               *
** Description:    bsp acl16 Operations										*
** Author:         WXY                                                  	*
** DATE:           2022/02/21                                               *	
** Copyright:      All Rights Reserved.  									*
*****************************************************************************/

/* IP_CODE */
#define IPCODE_GETID	            0x08
#define IPCONDE_GETVER  			0x0A
#define IPCODE_REBOOT               0X11
/* CMD_CODE*/
#define CMDCODE_ID_GET		        0x01	//获取chipid/Reboot

/* IP_CODE */
#define IPCODE_SM2			        0x05
/* CMD_CODE*/
#define CMDCODE_SM2_GENSECKEY		0x01	//生成密钥操作
#define CMDCODE_SM2_SIGN			0x02	//签名
#define CMDCODE_SM2_ENCRY			0x04	//加密

/* IP_CODE */
#define IPCODE_LP			        0x10
/* CMD_CODE*/
#define CMDCODE_LOWP_GOLOWP			0x01

typedef enum 
{
	CRY_GETCHIPID = 0,
	CRY_GENKEY ,
	CRY_SIGN,
	CRY_ENCRY,
	CRY_GOLOWP,
	CRY_GETDATA,
	CRY_WEAKUP,
	CRY_RST,
	CRY_GETAPPVER,
	CRY_REBOOT
}EVENTTYPE;


#pragma pack(1)






struct acl16_pro_str
{
	uint8_t     ip_code;     /*本次命令对象*/
	uint8_t     cmd_code;    /*操作*/
	uint16_t    key_len;     /*密钥相关的数据长度*/
	uint32_t    crypt_len;   /*要运算的数据长度*/
	uint8_t*    key_data;
	uint8_t*    cry_data;
};





#pragma pack()





uint8_t rt_hw_init_acl16(void);

uint16_t rt_write_data_to_acl16(uint8_t *buf, uint16_t len);
uint16_t rt_read_data_from_acl16(uint8_t *buf, uint16_t len);


#endif


