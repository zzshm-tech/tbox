


#include <rtthread.h>
#include "drv_rtc.h"
#include "board.h"
#include "drv_mem.h"





static struct inner_mem_device inner_mem;     //内部存储器



/****************************
**
*****************************/

uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;
 
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_Sector_0; 
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_Sector_1; 
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_Sector_2; 
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_Sector_3; 
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_Sector_4; 
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_Sector_5; 
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_Sector_6; 
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_Sector_7; 
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_Sector_8; 
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_Sector_9; 
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_Sector_10; 
  }
  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/
  {
    sector = FLASH_Sector_11; 
  }

  return sector;
}



/*************************************
** 这两个块是
**************************************/

rt_size_t inner_mem_block_read(rt_device_t dev, rt_off_t ReadAddr, void *pBuffer, rt_size_t NumToRead)
{
	rt_uint32_t mem_addr;
	rt_uint16_t i;
	rt_uint16_t *pt;
	
	pt = (rt_uint16_t *)pBuffer;
	NumToRead /= 2;
	mem_addr = ReadAddr;
	for(i = 0;i < NumToRead;i++)
	{
		*(pt) = *(__IO rt_uint16_t *)(mem_addr + i * 2);
		pt++;
	}
	//rt_kprintf("Read inner ....\r\n");
	return 1;
}



/*************************************
**	保存数据
**************************************/

rt_size_t inner_mem_block_write(rt_device_t dev, rt_off_t WriteAddr, const void *pBuffer, rt_size_t NumToWrite)
{	
	int i;
	rt_uint32_t Addr;
	HAL_StatusTypeDef res = HAL_OK;
	
	rt_uint32_t *pt = (rt_uint32_t *)pBuffer;
	
	Addr = WriteAddr;
	
	rt_enter_critical();   //关闭全局中断
  HAL_FLASH_Unlock(); 
   __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                          FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
 
	NumToWrite /= 4;
	
	
	for(i = 0;i < NumToWrite;i++)
	{
		res = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,Addr+ i * 4,*pt);
		if(res != HAL_OK)
		{
			return 0;
		}
		pt++;
	}
  HAL_FLASH_Lock();
  rt_exit_critical();

	return NumToWrite * 4;
}


/*************************************
**	必须是块写，保存数据
**************************************/

static rt_err_t rt_inner_mem_control(rt_device_t dev, int cmd, void *args)
{
    RT_ASSERT(dev != RT_NULL);
		
    switch (cmd)
    {
			case RT_DEVICE_CTRL_INNER_MEM_ERASE:               //擦除单片机内部闪存
			{
				FLASH_EraseInitTypeDef pEraseInit;
				uint32_t SectorError;
				uint32_t UserStartSector;
				
				rt_enter_critical();
				UserStartSector = *(uint32_t *)args;
				
				UserStartSector = GetSector(UserStartSector);
				
				pEraseInit.TypeErase = TYPEERASE_SECTORS;
				pEraseInit.Sector = UserStartSector;
				pEraseInit.NbSectors = 1 ;
				pEraseInit.VoltageRange = VOLTAGE_RANGE_3;

				
				HAL_FLASH_Unlock(); 
				__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                          FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
				
	
				if(HAL_FLASHEx_Erase(&pEraseInit, &SectorError) != HAL_OK)
				{
					rt_exit_critical();
					rt_kprintf("FLASH_ErasePage is failed, error_num = %d\r\n", SectorError);
					return RT_ERROR;
				}
		
				HAL_FLASH_Lock();
				
				rt_exit_critical();
			}
      break;
			case 2:
				break;
		}	
		
		return RT_EOK;
}



/*************************************
**	信号量
**	注意信号量
**************************************/

int rt_hw_inner_mem_init(void)
{
	inner_mem.lock = rt_mutex_create("mutex_inner_mem", RT_IPC_FLAG_FIFO);
    
	if (inner_mem.lock == RT_NULL)
  {
		rt_kprintf("Can't create mutex for inner device on \r\n");

    return RT_ENOSYS;
  }
	
	inner_mem.parent.type = RT_Device_Class_Block;
  inner_mem.parent.init = RT_NULL;
  inner_mem.parent.open = RT_NULL;
  inner_mem.parent.close = RT_NULL;
  inner_mem.parent.read = inner_mem_block_read;
  inner_mem.parent.write = inner_mem_block_write;
  inner_mem.parent.control = rt_inner_mem_control;
  inner_mem.parent.user_data = RT_NULL;

  rt_device_register(&inner_mem.parent, "inner_mem", RT_DEVICE_FLAG_RDWR);
	
	return 0;
}


INIT_BOARD_EXPORT(rt_hw_inner_mem_init);






