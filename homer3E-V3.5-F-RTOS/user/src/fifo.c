
/**************************************************
**	FileName:FiFoData.c
**	Time:
***************************************************/


#include <stdio.h>
#include <string.h>

#include "pro_data.h"
#include "data_type.h"

#include "FreeRTOS.h"

#include "fr_drv_mem.h"


static SendQueueStr 		SendQueue;               //

static FiFoStr				  CmdData;                 //

static unsigned char 		fif_buf[4096];					//

SemaphoreHandle_t sema_upload_queue = NULL;  

SemaphoreHandle_t sema_upload_cmd = NULL;  


/**************************************************
**	函数名称:
**	功能描述:
**************************************************/

SendQueueStr *GetSendQueueSpace(void)
{
	return &SendQueue;
}



void init_fifo_sema(void)
{
	if(sema_upload_queue == NULL)
		sema_upload_queue = xSemaphoreCreateMutex();   //创建信号量
	if(sema_upload_cmd == NULL)
		sema_upload_cmd = xSemaphoreCreateMutex();    
}




/***************************************************
**	函数名称:
**	功能描述:
***************************************************/

void ClearSendQueue(void)
{
	SendQueue.QData[SendQueue.QRead].DataLen = 0;
	SendQueue.QRead++;
	
	if(SendQueue.QRead >= BLIND_NUM)
		SendQueue.QRead = 0;
	
	if(SendQueue.QNum != 0)
		SendQueue.QNum--;
	//fr_printf("SendQueue Num:%d\r\n",SendQueue.QNum);
	
}


/***************************************************
**	函数名称:
**	功能描述:
***************************************************/

unsigned short int ReadSendQueue(unsigned char *Source)
{
	int i;
	unsigned char tmp;
	
	if(SendQueue.QNum == 0)
		return 0;
	
	//fr_printf("Builid Num:%d,%d \r\n",SendQueue.QNum,SendQueue.QRead);

	if(SendQueue.QData[SendQueue.QRead].DataBig > 0)     //
	{
		//fr_printf("Ready Blind Data\r\n");
		tmp = CalcCrc8(SendQueue.QData[SendQueue.QRead].DataBuf,SendQueue.QData[SendQueue.QRead].DataLen);
		if(tmp != SendQueue.QData[SendQueue.QRead].DataCrc)
		{
			ClearSendQueue();
			//fr_printf("The SendQueue \r\n");
			return 0;
		}
		for(i = 0; i < SendQueue.QData[SendQueue.QRead].DataLen;i++)
		{
			*(Source + i) = SendQueue.QData[SendQueue.QRead].DataBuf[i];
		}
		//fr_printf("Return Blind Data\r\n");    //
		return SendQueue.QData[SendQueue.QRead].DataLen;
			
	}
	else
	{
		if(SendQueue.QWrite > SendQueue.QRead)
		{
			if(SendQueue.QNum != (SendQueue.QWrite - SendQueue.QRead))
			{
				//fr_printf("The Queue Reset(1) %d,%d,%d\r\n",SendQueue.QNum,SendQueue.QWrite,SendQueue.QRead);
				SendQueue.QWrite = 0;
				SendQueue.QRead = 0;
				SendQueue.QNum = 0;
				
				return 0;
			}
		}

		if(SendQueue.QWrite <= SendQueue.QRead)
		{
			if(SendQueue.QNum != (SendQueue.QWrite + BLIND_NUM - SendQueue.QRead))
			{
				//fr_printf("The Queue Reset(2) %d,%d,%d\r\n",SendQueue.QNum,SendQueue.QWrite,SendQueue.QRead);
				SendQueue.QWrite = 0;
				SendQueue.QRead = 0;
				SendQueue.QNum = 0;
				
				return 0;
			}
		}
			
		for(i = 0;i < SendQueue.QData[SendQueue.QRead].DataLen;i++)
		{

			*(Source + i) = SendQueue.QData[SendQueue.QRead].DataBuf[i];
		}
		fr_printf(">App:SendQueue Read:%d,%d\r\n",SendQueue.QRead,SendQueue.QNum);
		return SendQueue.QData[SendQueue.QRead].DataLen;
	}
}


/***************************************************
**	函数名称:
**	功能描述:
***************************************************/

unsigned char WriteSendQueue(unsigned char *Buf,unsigned short int DataLen)
{
	int i;
	//unsigned short int Read;
	MsgHeadStr *p;
	unsigned short int len;
	
	unsigned char tmp = 0;
	
	if(DataLen > BLIND_BUF)    //
		return 0;
	
	if(SendQueue.QWrite != SendQueue.QRead || SendQueue.QNum == 0)
	{
		for(i = 0;i < DataLen;i++)
		{
			SendQueue.QData[SendQueue.QWrite].DataBuf[i] = *(Buf + i);
		}
		SendQueue.QData[SendQueue.QWrite].DataLen = DataLen;
		SendQueue.QData[SendQueue.QWrite].DataBig = 0;
		SendQueue.QNum++;
		SendQueue.QWrite++;
		if(SendQueue.QWrite >= BLIND_NUM)
			SendQueue.QWrite = 0;
	}
	else
	{
		len = 0;
		for(i = 0;i < 10;i++)
		{
			p = (MsgHeadStr *)SendQueue.QData[SendQueue.QRead].DataBuf;
			p->msg_id = 0x46;            //
			SendQueue.QData[SendQueue.QRead].DataBig = 1;
			tmp = SendQueue.QData[SendQueue.QRead].DataLen;
			SendQueue.QData[SendQueue.QRead].DataBuf[tmp - 2] = BccVerify(SendQueue.QData[SendQueue.QRead].DataBuf + 3,tmp - 4);
			SendQueue.QData[SendQueue.QRead].DataCrc = CalcCrc8(SendQueue.QData[SendQueue.QRead].DataBuf, tmp);
			
			memcpy(fif_buf + len,(unsigned char *)&SendQueue.QData[SendQueue.QRead],sizeof(FiFoStr));
			len += sizeof(FiFoStr);
			SendQueue.QNum--;
			SendQueue.QRead++;
			if(SendQueue.QRead >= BLIND_NUM)     //
				SendQueue.QRead = 0;
			fr_printf(">App:SendQueue Blind Read:%d,%d,%d,%d\r\n",SendQueue.QWrite,SendQueue.QRead,SendQueue.QNum,DataLen);
//			len += ReadSendQueue(fif_buf + len);
//			ClearSendQueue();
		}
		
		//Write EEPROM
		//w25qxx_flash_write(fif_buf,0,1);
		for(i = 0;i < DataLen;i++)
		{
			SendQueue.QData[SendQueue.QWrite].DataBuf[i] = *(Buf + i);
		}
		SendQueue.QData[SendQueue.QWrite].DataLen = DataLen;
		SendQueue.QData[SendQueue.QWrite].DataBig = 0;
		SendQueue.QNum++;
		SendQueue.QWrite++;
		if(SendQueue.QWrite >= BLIND_NUM)
			SendQueue.QWrite = 0;
	}
	fr_printf(">App:SendQueue Write:%d,%d,%d,%d\r\n",SendQueue.QWrite,SendQueue.QRead,SendQueue.QNum,DataLen);
	return 1;
}





/************************************************
**	函数名称:
**	功能描述:
************************************************/

void WriteCmdDataBuf(unsigned char *Buf,unsigned short int DataLen)
{
	if(DataLen > BLIND_BUF)
		return;
	memcpy(CmdData.DataBuf,Buf,DataLen);
	CmdData.DataLen = DataLen;
}


/************************************************
**	函数名称:
**	功能描述:
************************************************/

unsigned short int ReadCmdDataBuf(unsigned char *buf)
{
	xSemaphoreTake(sema_upload_cmd,portMAX_DELAY );
	
	if(CmdData.DataLen > 0)
	{
		memcpy(buf,CmdData.DataBuf,CmdData.DataLen);
		xSemaphoreGive(sema_upload_cmd); 
		return CmdData.DataLen;
	}
	
	xSemaphoreGive(sema_upload_cmd); 
	return 0;
}

/************************************************
**	函数名称:
**	功能描述:
************************************************/

void ClearCmdData(void)
{
	xSemaphoreTake(sema_upload_cmd,portMAX_DELAY );     //获取信号量
	CmdData.DataLen = 0;
	xSemaphoreGive(sema_upload_cmd);      					//归还信号量
	
}

/***************************************************
**	函数名称:
**	功能描述:
***************************************************/

void SaveSendQueue(void)
{	
	int i;
	unsigned char tmp = 0;
	unsigned short int Read;
	MsgHeadStr *p;
	
	if(SendQueue.QNum > 0)
	{
		Read = SendQueue.QRead;
		for(i = 0;i < SendQueue.QNum;i++)              //
		{
			p = (MsgHeadStr *)SendQueue.QData[Read].DataBuf;
			p->msg_id = 0x46;            //
			SendQueue.QData[Read].DataBig = 1;
			tmp = SendQueue.QData[Read].DataLen;
			SendQueue.QData[Read].DataBuf[tmp - 2] = BccVerify(SendQueue.QData[Read].DataBuf + 3,tmp - 4);
			SendQueue.QData[Read].DataCrc = CalcCrc8(SendQueue.QData[Read].DataBuf, tmp);
			Read++;
			if(Read >= BLIND_NUM)
				Read = 0;
		}
	}
	
}



/*************************************************
**	函数名称:
**	功能描述:
*************************************************/

void LoadSendQueue(void)
{
	
}



/*********************************************
**	函数名称:
**	功能描述:
**********************************************/

void ClearBindData(void)
{
	SendQueue.QNum = 0;
	SendQueue.QRead = 0;
	SendQueue.QWrite = 0;
	fr_printf("Claer Bind Data Finsh..............\r\n");
		
}


/*************************
**	测试使用
**************************/

void test_SendQueue(void)
{
	int rv = 0;
	
	rv = sizeof(SendQueue);
	fr_printf("SendQueue:%d\r\n",rv);
}


/*****************************File End************************/

