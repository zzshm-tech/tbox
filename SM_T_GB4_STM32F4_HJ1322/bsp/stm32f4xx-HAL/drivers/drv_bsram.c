



#include "drv_bsram.h"




static struct back_sram_device back_sram;     //内部存储器



/***********************************
**
**************************************/

static rt_err_t rt_back_sram_control(rt_device_t dev, int cmd, void *args)
{
	switch(cmd)
	{
		case RT_DEVICE_CTRL_BACK_SRAM_WRITE:
			break;
		case RT_DEVICE_CTRL_BACK_SRAM_READ:
			break;
	}
	
	return RT_EOK;
}




rt_size_t back_sram_block_read(rt_device_t dev, rt_off_t ReadAddr, void *pBuffer, rt_size_t NumToRead)
{
	uint32_t len;
	
	if(pBuffer == NULL) 
		return 0;											//无效的地址
	
	if(NumToRead==0) 
		return 0;											//无效的数量
	
	if(ReadAddr >= BACKUP_SRAM_SIZE) 
		return 0;						//起始地址有误
	
	len = ReadAddr + NumToRead;	
	
	if(len > BACKUP_SRAM_SIZE) 
		len = BACKUP_SRAM_SIZE;					//限制范围，只有4KB
	
	len -= ReadAddr;													//计算要写入的数据长度
	
	rt_memcpy(pBuffer, (uint8_t *)BKPSRAM_BASE + ReadAddr, NumToRead);
	
	return len;
}



/***********************************
**
**************************************/

rt_size_t back_sram_block_write(rt_device_t dev, rt_off_t WriteAddr, const void *pBuffer, rt_size_t NumToWrite)
{	
	uint32_t len;
	
	if(pBuffer == NULL) 
		return 0;											//无效的地址
	
	if(pBuffer == 0) 
		return 0;											//无效的数量
	
	if(WriteAddr >= BACKUP_SRAM_SIZE) 
		return 0;						//起始地址有误
	
	len = WriteAddr + NumToWrite;	
	if(len > BACKUP_SRAM_SIZE)
		len = BACKUP_SRAM_SIZE;					//限制范围，只有4KB
	
	len -= WriteAddr;													//计算要写入的数据长度
	
	rt_memcpy((uint8_t *)BKPSRAM_BASE + WriteAddr,(char *)pBuffer, NumToWrite);
	
	return len;

}



/** （使用HAL库）备份SRAM初始化
 * 
 * @param[in]   NULL
 * @retval      Null
**/
void BKP_SRAM_Init(void)
{
	/* 电源接口时钟使能 (Power interface clock enable) */
	__HAL_RCC_PWR_CLK_ENABLE();

	/* DBP 位置 1，使能对备份域的访问 */
	HAL_PWR_EnableBkUpAccess();

	/* 通过将 RCC AHB1 外设时钟使能寄存器 (RCC_AHB1ENR) 中的 BKPSRAMEN 位置 1， 使能备份 SRAM 时钟 */
	__HAL_RCC_BKPSRAM_CLK_ENABLE();

	/* 应用程序必须等待备份调压器就绪标志 (BRR) 置 1，指示在待机模式和 VBAT 模式下会保持写入 RAM 中的数据。 */
	HAL_PWREx_EnableBkUpReg();
}




int rt_hw_back_sram_init(void)
{
	back_sram.lock = rt_mutex_create("mutex_back_sram", RT_IPC_FLAG_FIFO);
    
	if (back_sram.lock == RT_NULL)
  {
		rt_kprintf("Can't create mutex for inner device on \r\n");

    return RT_ENOSYS;
  }
	
	back_sram.parent.type = RT_Device_Class_Char;
  back_sram.parent.init = RT_NULL;
  back_sram.parent.open = RT_NULL;
  back_sram.parent.close = RT_NULL;
  back_sram.parent.read = back_sram_block_read;
  back_sram.parent.write = back_sram_block_write;
  back_sram.parent.control = rt_back_sram_control;
  back_sram.parent.user_data = RT_NULL;
	
	BKP_SRAM_Init();
  
	rt_device_register(&back_sram.parent, "back_sram", RT_DEVICE_FLAG_RDWR);
	
	return 0;
}

INIT_BOARD_EXPORT(rt_hw_back_sram_init);

