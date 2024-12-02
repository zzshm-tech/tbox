

#include <rtthread.h>
#include <rtdevice.h>
 
#ifndef EEP_I2CBUS_NAME
#define EEP_I2CBUS_NAME          "i2c2"  /* 连接的I2C总线设备名称 */
#endif
 
#define EEP_ADDR             0xA0  //从设备芯片地址
 
rt_uint8_t wr_data = 0x15;

#define SIZE sizeof(wr_data)	
 
 
static struct rt_i2c_bus_device *eep_i2c_bus = RT_NULL;
 
//写EEPROM
//write_addr:开始写入的地址 对24c02为0~255
//data:写入的数据
//number：数据个数
rt_err_t eeprom_iic_write(rt_uint8_t write_addr, rt_uint8_t data, rt_uint32_t number)
{
   rt_uint8_t buf[2];
 
    buf[0] = write_addr;
    buf[1] = data;
    rt_size_t result;
  
    result = rt_i2c_master_send(eep_i2c_bus, EEP_ADDR, RT_I2C_WR, buf, 2);
    rt_thread_mdelay(10);  //必须延时
    if (result == 2)
    {
        rt_kprintf("EEP write ok \r\n");
        return RT_EOK;       
    }
    else
    {
        rt_kprintf("EEP write failed ,ERR is: %d \r\n",result);
        return -RT_ERROR;       
    }
}
 
//读EEPROM
//read_addr:开始读出的地址 对24c02为0~255
//number：数据个数
rt_err_t eeprom_iic_read(rt_uint8_t read_addr, rt_uint32_t len, rt_uint8_t *buf)
{   
    //通知要读哪个设备的哪个内存地址的内容，（告知是需要读read_addr）
    rt_i2c_master_send(eep_i2c_bus, EEP_ADDR, RT_I2C_WR, &read_addr, 1);
    
    //读取到的内容存入buf
    rt_i2c_master_recv(eep_i2c_bus, EEP_ADDR, RT_I2C_RD, buf, len);//地址读数据
	
	return RT_EOK;
}
 
int i2c_eeprom_sample(void)
{
    rt_uint8_t buf;
    //寻找设备
     eep_i2c_bus = rt_i2c_bus_device_find(EEP_I2CBUS_NAME);
     rt_kprintf("EEP set i2c bus to %s\r\n", EEP_I2CBUS_NAME);
    
     eeprom_iic_write(0x00,wr_data,SIZE);//0x00地址写数据wr_data
     
     eeprom_iic_read(0x00, 1, &buf);//0x00地址读数据
     if(buf == wr_data)
     {
        rt_kprintf("EEP read ok ,data is: 0x%02x \r\n",buf);
     }
		 
		 return 0;
}
 
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(i2c_eeprom_sample, i2c_eeprom sample);

