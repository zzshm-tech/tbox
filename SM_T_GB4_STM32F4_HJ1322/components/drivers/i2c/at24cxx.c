/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-13     XiaojieFan   the first version
 * 2019-12-04     RenMing      ADD PAGE WRITE and input address can be selected 
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "drv_i2c.h"

#include <string.h>
#include <stdlib.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME "at24xx"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#include "at24cxx.h"

//#ifdef PKG_USING_AT24CXX
#define AT24CXX_ADDR (0xA0 >> 1)                      //A0 A1 A2 connect GND

#if (EE_TYPE == AT24C01)
    #define AT24CXX_PAGE_BYTE               8
    #define AT24CXX_MAX_MEM_ADDRESS         128
#elif (EE_TYPE == AT24C02)
    #define AT24CXX_PAGE_BYTE               8
    #define AT24CXX_MAX_MEM_ADDRESS         256
#elif (EE_TYPE == AT24C04)
    #define AT24CXX_PAGE_BYTE               16
    #define AT24CXX_MAX_MEM_ADDRESS         512
#elif (EE_TYPE == AT24C08)
    #define AT24CXX_PAGE_BYTE               16
    #define AT24CXX_MAX_MEM_ADDRESS         1024
#elif (EE_TYPE == AT24C16)
    #define AT24CXX_PAGE_BYTE               16
    #define AT24CXX_MAX_MEM_ADDRESS         2048
#elif (EE_TYPE == AT24C32)
    #define AT24CXX_PAGE_BYTE               32
    #define AT24CXX_MAX_MEM_ADDRESS         4096
#elif (EE_TYPE == AT24C64)
    #define AT24CXX_PAGE_BYTE               32
    #define AT24CXX_MAX_MEM_ADDRESS         8192
#elif (EE_TYPE == AT24C128)
    #define AT24CXX_PAGE_BYTE               64
    #define AT24CXX_MAX_MEM_ADDRESS         16384
#elif (EE_TYPE == AT24C256)
    #define AT24CXX_PAGE_BYTE               64
    #define AT24CXX_MAX_MEM_ADDRESS         32768
#elif (EE_TYPE == AT24C512)
    #define AT24CXX_PAGE_BYTE               128
    #define AT24CXX_MAX_MEM_ADDRESS         65536
#endif

static rt_err_t read_regs(at24cxx_device_t dev, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs;

    msgs.addr = AT24CXX_ADDR;
    msgs.flags = RT_I2C_RD;
    msgs.buf = buf;
    msgs.len = len;

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}
uint8_t at24cxx_read_one_byte(at24cxx_device_t dev, uint16_t readAddr)
{
    rt_uint8_t buf[2];
    rt_uint8_t temp;
#if	(EE_TYPE > AT24C16)  
    buf[0] = (uint8_t)(readAddr>>8);	
    buf[1] = (uint8_t)readAddr;
    if (rt_i2c_master_send(dev->bus, AT24CXX_ADDR, 0, buf, 2) == 0) 
#else
    buf[0] = readAddr;
    if (rt_i2c_master_send(dev->i2c, AT24CXX_ADDR | dev->AddrInput, 0, buf, 1) == 0)
#endif        
    {
        return RT_ERROR;
    }
    read_regs(dev, 1, &temp);
    return temp;
}

rt_err_t at24cxx_write_one_byte(at24cxx_device_t dev, uint16_t writeAddr, uint8_t dataToWrite)
{
    rt_uint8_t buf[3];
#if	(EE_TYPE > AT24C16)      
    buf[0] = (uint8_t)(writeAddr>>8);	
    buf[1] = (uint8_t)writeAddr;
    buf[2] = dataToWrite;
    if (rt_i2c_master_send(dev->bus, AT24CXX_ADDR, 0, buf, 3) == 3)    
#else    
    buf[0] = writeAddr; //cmd
    buf[1] = dataToWrite;
    //buf[2] = data[1];
    

    if (rt_i2c_master_send(dev->i2c, AT24CXX_ADDR | dev->AddrInput, 0, buf, 2) == 2)
#endif        
        return RT_EOK;
    else
        return -RT_ERROR;

}

rt_err_t at24cxx_read_page(at24cxx_device_t dev, uint32_t readAddr, uint8_t *pBuffer, uint16_t numToRead)
{
    struct rt_i2c_msg msgs[2];
    uint8_t AddrBuf[2];
    
    msgs[0].addr = AT24CXX_ADDR;
    msgs[0].flags = RT_I2C_WR ;
 
#if	(EE_TYPE > AT24C16) 
    AddrBuf[0] = readAddr >> 8;
    AddrBuf[1] = readAddr;
	msgs[0].buf = AddrBuf;
    msgs[0].len = 2; 
#else
    AddrBuf[0] = readAddr;
	msgs[0].buf = AddrBuf;
    msgs[0].len = 1;
#endif    
    
    msgs[1].addr = AT24CXX_ADDR;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = pBuffer;
    msgs[1].len = numToRead;
    
    if(rt_i2c_transfer(dev->bus, msgs, 2) == 0) 
    {
        return RT_ERROR;
    }			
	
    return RT_EOK;
}

rt_err_t at24cxx_write_page(at24cxx_device_t dev, uint32_t wirteAddr, uint8_t *pBuffer, uint16_t numToWrite)
{
    struct rt_i2c_msg msgs[2];
    uint8_t AddrBuf[2];

    msgs[0].addr = AT24CXX_ADDR;
    msgs[0].flags = RT_I2C_WR ;
    
#if	(EE_TYPE > AT24C16) 
    AddrBuf[0] = wirteAddr >> 8;
    AddrBuf[1] = wirteAddr;
	msgs[0].buf = AddrBuf;
    msgs[0].len = 2; 
#else
    AddrBuf[0] = wirteAddr;
	msgs[0].buf = AddrBuf;
    msgs[0].len = 1;
#endif    
	  
    msgs[1].addr = AT24CXX_ADDR;
    msgs[1].flags = RT_I2C_WR | RT_I2C_NO_START;
    msgs[1].buf = pBuffer;
    msgs[1].len = numToWrite;
	  
    if(rt_i2c_transfer(dev->bus, msgs, 2) == 0) 
    {
        return RT_ERROR;
    }			
	
    return RT_EOK;	
}

rt_err_t at24cxx_check(at24cxx_device_t dev)
{
    uint8_t temp;
    RT_ASSERT(dev);

    temp = at24cxx_read_one_byte(dev, AT24CXX_MAX_MEM_ADDRESS - 1);
    if (temp == 0x55) return RT_EOK;
    else
    {
        at24cxx_write_one_byte(dev, AT24CXX_MAX_MEM_ADDRESS - 1, 0x55);
        rt_thread_mdelay(EE_TWR);                 // wait 5ms befor next operation
        temp = at24cxx_read_one_byte(dev, AT24CXX_MAX_MEM_ADDRESS - 1);
        if (temp == 0x55) return RT_EOK;
    }
    return RT_ERROR;
}

/**
 * This function read the specific numbers of data to the specific position
 *
 * @param bus the name of at24cxx device
 * @param ReadAddr the start position to read
 * @param pBuffer  the read data store position
 * @param NumToRead
 * @return RT_EOK  write ok.
 */
rt_err_t at24cxx_read(at24cxx_device_t dev, uint32_t ReadAddr, uint8_t *pBuffer, uint16_t NumToRead)
{
    rt_err_t result;
    RT_ASSERT(dev);
    
    if(ReadAddr + NumToRead > AT24CXX_MAX_MEM_ADDRESS)
    {
        return RT_ERROR;
    }
    
    result = rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
        while (NumToRead)
        {
            *pBuffer++ = at24cxx_read_one_byte(dev, ReadAddr++);
            NumToRead--;
        }
    }
    else
    {
        LOG_E("The at24cxx could not respond  at this time. Please try again");
    }
    rt_mutex_release(dev->lock);

    return RT_EOK;
}

/***************************************************************************
 * This function read the specific numbers of data to the specific position
 *
 * @param bus the name of at24cxx device
 * @param ReadAddr the start position to read
 * @param pBuffer  the read data store position
 * @param NumToRead 
 * @return RT_EOK  write ok.
 */
rt_size_t at24cxx_page_read(rt_device_t dev, rt_off_t ReadAddr, void *pBuffer, rt_size_t NumToRead)
{
		struct at24cxx_device *at24cxx;
		rt_uint8_t *p = (rt_uint8_t *)pBuffer;
    rt_err_t result = RT_EOK;
    uint16_t pageReadSize = AT24CXX_PAGE_BYTE - ReadAddr % AT24CXX_PAGE_BYTE;
	  
    RT_ASSERT(dev);
	
		at24cxx = (struct at24cxx_device *)dev;
	
    if(ReadAddr + NumToRead > AT24CXX_MAX_MEM_ADDRESS)
    {
        return RT_ERROR;
    }

    result = rt_mutex_take(at24cxx->lock, RT_WAITING_FOREVER);
    if(result == RT_EOK)
    {
        while (NumToRead)
        {
            if(NumToRead > pageReadSize)
            {
                if(at24cxx_read_page(at24cxx, ReadAddr, p, pageReadSize))
                {
                    result = RT_ERROR;
                }

                ReadAddr += pageReadSize;
                p += pageReadSize;
                NumToRead -= pageReadSize;
                pageReadSize = AT24CXX_PAGE_BYTE;
            }
            else
            {
                if(at24cxx_read_page(at24cxx, ReadAddr, p, NumToRead))
                {
                    result = RT_ERROR;
                }
                NumToRead = 0;
            }
        }
    }
    else
    {
        rt_kprintf("The at24cxx could not respond  at this time. Please try again");
    }
				
    rt_mutex_release(at24cxx->lock);
    return result;
}

/**
 * This function write the specific numbers of data to the specific position
 *
 * @param bus the name of at24cxx device
 * @param WriteAddr the start position to write
 * @param pBuffer  the data need to write
 * @param NumToWrite
 * @return RT_EOK  write ok.at24cxx_device_t dev
 */
rt_err_t at24cxx_write(at24cxx_device_t dev, uint32_t WriteAddr, uint8_t *pBuffer, uint16_t NumToWrite)
{
    uint16_t i = 0;
    rt_err_t result;
    RT_ASSERT(dev);
    
    if(WriteAddr + NumToWrite > AT24CXX_MAX_MEM_ADDRESS)
    {
        return RT_ERROR;
    }
    
    result = rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
        while (1) //NumToWrite--
        {
            if (at24cxx_write_one_byte(dev, WriteAddr, pBuffer[i]) != RT_EOK)
            {
                rt_thread_mdelay(EE_TWR);
            }
            else
            {
                WriteAddr++;
                i++;
            }
            if (i == NumToWrite)
            {
                break;
            }

        }
    }
    else
    {
        LOG_E("The at24cxx could not respond  at this time. Please try again");
    }
    rt_mutex_release(dev->lock);

    return RT_EOK;
}

/**
 * This function write the specific numbers of data to the specific position
 *
 * @param bus the name of at24cxx device
 * @param WriteAddr the start position to write
 * @param pBuffer  the data need to write
 * @param NumToWrite
 * @return RT_EOK  write ok.at24cxx_device_t dev
 */
rt_size_t at24cxx_page_write(rt_device_t dev, rt_off_t WriteAddr, const void *pBuffer, rt_size_t NumToWrite)
{	
		struct at24cxx_device *at24cxx;
		rt_uint8_t *p = (rt_uint8_t *)pBuffer;
    rt_err_t result = RT_EOK;
		rt_size_t 		rv = NumToWrite;
    uint16_t pageWriteSize = AT24CXX_PAGE_BYTE - WriteAddr % AT24CXX_PAGE_BYTE;
	  
    RT_ASSERT(dev);
	
		at24cxx = (struct at24cxx_device *)dev;
	
    if(WriteAddr + NumToWrite > AT24CXX_MAX_MEM_ADDRESS)
    {
        return 0;
    }

    result = rt_mutex_take(at24cxx->lock, RT_WAITING_FOREVER);
    if(result == RT_EOK)
    {
        while (NumToWrite)
        {
            if(NumToWrite > pageWriteSize)
            {
                if(at24cxx_write_page(at24cxx, WriteAddr, p, pageWriteSize))
                {
									rv = 0;
									break;
                }
                rt_thread_mdelay(EE_TWR);    // wait 5ms befor next operation

                WriteAddr += pageWriteSize;
                p += pageWriteSize;
                NumToWrite -= pageWriteSize;
                pageWriteSize = AT24CXX_PAGE_BYTE;
            }
            else
            {
                if(at24cxx_write_page(at24cxx, WriteAddr, p, NumToWrite))
                {
									rv = 0;
                  break;
                }
                rt_thread_mdelay(EE_TWR);   // wait 5ms befor next operation

                NumToWrite = 0;
            }
        }
    }
    else
    {
        rt_kprintf("The at24cxx could not respond  at this time. Please try again");
    }

    rt_mutex_release(at24cxx->lock);
		
		return rv;
}

/**
 * This function initializes at24cxx registered device driver
 *
 * @param dev the name of at24cxx device
 *
 * @return the at24cxx device.
 */


/**
 * This function releases memory and deletes mutex lock
 *
 * @param dev the pointer of device driver structure
 */
void at24cxx_deinit(at24cxx_device_t dev)
{
    RT_ASSERT(dev);

    rt_mutex_delete(dev->lock);

    rt_free(dev);
}


//#endif



/* cat24c64 模块重新封装 */
int rt_hw_at24cxx_init(void)
{
    static struct at24cxx_device at24cxx_drv;
    struct rt_i2c_bus_device *bus;
    bus = rt_i2c_bus_device_find(I2C_BUS_NAME);
	
	
    if (bus == RT_NULL)
    {
        return RT_ENOSYS;
    }

		at24cxx_drv.lock = rt_mutex_create("mutex_at24cxx", RT_IPC_FLAG_FIFO);
    if (at24cxx_drv.lock == RT_NULL)
    {
        rt_kprintf("Can't create mutex for at24cxx device on \r\n");

        return RT_ENOSYS;
    }
		
    at24cxx_drv.bus = bus;
    at24cxx_drv.parent.type = RT_Device_Class_Char;
    at24cxx_drv.parent.init = RT_NULL;
    at24cxx_drv.parent.open = RT_NULL;
    at24cxx_drv.parent.close = RT_NULL;
    at24cxx_drv.parent.read = at24cxx_page_read;
    at24cxx_drv.parent.write = at24cxx_page_write;
    at24cxx_drv.parent.control = RT_NULL;
    at24cxx_drv.parent.user_data = RT_NULL;

    rt_device_register(&at24cxx_drv.parent, "at24cxx", RT_DEVICE_FLAG_RDWR);

    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_at24cxx_init);


