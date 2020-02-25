

/*****************************
**	
******************************/

#include "stm32f10x.h"

#include "fr_drv_mem.h"
#include "FreeRTOS.h"
#include "board.h"


/*******************************/

static unsigned char  					Update_Flag = 'N';									//升级标志
//static unsigned short int 			gNumofRecPackets = 1; 		 					//当前数据帧号
//static unsigned int 						gFirmwareDataSize;	       					//升级固件总长度
//static unsigned int 						gFirmwareDataPaketNum;		 					//升级固件总帧数
//static unsigned int 						gFirmwareCheckSum;    		 					//升级固件校验和
static unsigned int 						gDataLenSum     = 0;  		 					//接收到的数据字节数和
//static unsigned int 						gCntSend        = 0;  		 					//请求升级包超时计数
static unsigned int 						FlashDestination = ADDR_APP_BKP;   	//


/********************************************
**	计算固件页面大小
*********************************************/
unsigned int FLASH_PagesMask(unsigned int Size)
{
    unsigned int pagenumber = 0x0;
    unsigned int size = Size;

    if((size % PAGE_SIZE) != 0)
    {
        pagenumber = (size / PAGE_SIZE) + 1;
    }
    else
    {
        pagenumber = size / PAGE_SIZE;
    }
    return pagenumber;

}



/*******************************
**	擦除闪存
*******************************/
FLASH_Status  EraseSectors(unsigned int destination_address, unsigned int flashsize)
{
    FLASH_Status FLASHStatus  = FLASH_COMPLETE;
    unsigned int 								NbrOfPage = 0;
    unsigned int EraseCounter = 0x0;
    unsigned short int PageSize = PAGE_SIZE;

    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    NbrOfPage = FLASH_PagesMask(flashsize);						//计算需要擦除Flash的页

    FLASH_Unlock();
    for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
    {
        FLASHStatus = FLASH_ErasePage(destination_address + (PageSize * EraseCounter));
        if(FLASHStatus != FLASH_COMPLETE)
        {
            fr_printf("FLASH_ErasePage is failed, error_num = %d\r\n", FLASHStatus);
        }
    }
    FLASH_Lock();

    return FLASHStatus;
}


/********************************
**	接收到的数据写入闪存
*********************************/
unsigned int Receive_Packet(unsigned char *data, unsigned int length)
{
    unsigned char *pRamSource = 0;
    unsigned short int i;
    FLASH_Status index = FLASH_COMPLETE;
    int datalen;

    datalen = length;
    gDataLenSum += datalen;
    pRamSource = data;
    taskENTER_CRITICAL();   //关闭中断
    FLASH_Unlock();

    for (i = 0; (i < datalen) && (FlashDestination <  ADDR_APP_BKP + APP_DATA_SIZE); i += 4)
    {
        index = FLASH_ProgramWord(FlashDestination, *(unsigned int *)pRamSource);/*把接收到的数据编写到Flash中*/
        if(index != FLASH_COMPLETE)
        {
            fr_printf("* Flash write data error.\r\n");
            return 1;
        }
        if (*(unsigned int *)FlashDestination != *(unsigned int *)pRamSource)
        {
            fr_printf("* Data to check failure.\r\n");
            return 1;/*flash write data not same*/
        }
        FlashDestination += 4;
        pRamSource += 4;
    }
    FLASH_Lock();
    taskEXIT_CRITICAL();   //开启中断
    return 0;
}



/***********************************
**	写升级标志
************************************/
unsigned char WriteUpdateFlag(unsigned int updateFlag)
{
    unsigned int writeData = 0;
    unsigned int readData  = 0;

    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

    writeData = updateFlag;
    writeData = (writeData << 8) & 0xFF00;

    taskENTER_CRITICAL();
    FLASH_Unlock();
    FLASH_ErasePage(ADDR_DATA_FILED);					//擦出对应地址的1页(2K)
    FLASH_ProgramWord(ADDR_DATA_FILED, writeData);
    FLASH_Lock();
    taskEXIT_CRITICAL();

    readData = (*(unsigned int *)(ADDR_DATA_FILED));
    if(readData == writeData)
        return 1;
    else
        return 0;
}
/************************************************
**	读升级标志
*************************************************/
unsigned int ReadUpdateFlag(void)
{
    unsigned int updateFlag = 0;
	
    updateFlag = (*(unsigned int *)(ADDR_DATA_FILED));
    updateFlag = (updateFlag >> 8) & 0xFF;
	
    return updateFlag;
}

/***********************************
**	擦除备份区域  OK    
***********************************/
FLASH_Status BackupEraseHandle(void)
{
    FLASH_Status res = FLASH_COMPLETE;
    res = EraseSectors(ADDR_APP_BKP, APP_DATA_SIZE);
    if(res != FLASH_COMPLETE)     //擦除失败
    {
        Update_Flag = 'R';
        WriteUpdateFlag(Update_Flag);
        fr_printf("Erase the error code :%d \r\n", res); /*Erase operation completed successfully!*/
        __set_FAULTMASK(1);
        NVIC_SystemReset();   //写完标志，重新启动
    }
    return res;
}




/***************************
**	写升级失败标志，并重启
****************************/

void UpgradeFailReset(void)
{
	Update_Flag = 'R';     								//如果升级设备，重启设备
  WriteUpdateFlag(Update_Flag);
  delay_ms(10);         									//这个延时 尽量有
  __set_FAULTMASK(1);										//关闭中断
  NVIC_SystemReset();    								//重启系统  此处不会返回，直接
}



/**************************
**	写升级成功标志，并复位单片机
***************************/

void UpgradeOkReset(void)
{
	Update_Flag = 'Y';     								//如果升级设备，重启设备
  WriteUpdateFlag(Update_Flag);					//写升级成功标志
  delay_ms(10);         									//这个延时 尽量有
  __set_FAULTMASK(1);										//关闭中断
  NVIC_SystemReset();    								//重启系统  此处不会返回，直接
}


