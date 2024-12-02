


#include <rtthread.h>
#include "board.h"

#include "drv_wkup.h"



static struct rt_device wkup; 

/**********************************************
**	控制RTC
*********************************************/
static rt_err_t rt_wkup_control(rt_device_t dev, int cmd, void *args)
{
		
    RT_ASSERT(dev != RT_NULL);

    switch (cmd)
    {
			case RT_DEVICE_CTRL_READ_WKUP_TIME:               //读取时间
        break;
			case  RT_DEVICE_CTRL_SET_WKUP_TIME:                 //设置时间
			
			break;
    }

    return RT_EOK;
}



/*******************************************************
**	初始化RTC
********************************************************/
static rt_err_t rt_wkup_init(struct rt_device *dev)
{

	return RT_EOK;
}


/*******************************************************
**	打开RTC
********************************************************/
static rt_err_t rt_wkup_open(rt_device_t dev, rt_uint16_t oflag)
{ 
	
	return RT_EOK;
}


/*******************************************************
**	关闭RTC()
********************************************************/
static rt_err_t rt_wkup_close(struct rt_device *dev)
{
	
	
	return RT_EOK;
}


/*******************************************************
**	RTC始终配置
********************************************************/
static rt_size_t rt_wkup_read(struct rt_device *dev,
                                rt_off_t pos,
                                void *buffer,
                                rt_size_t size)
{
    rt_wkup_control(dev, RT_DEVICE_CTRL_RTC_GET_TIME, buffer);
	
    return size;
}





/*******************************************************
**	RTC始终配置
********************************************************/
static rt_size_t rt_wkup_write(struct rt_device *dev,
                                 rt_off_t pos,
                                 const void *buffer,
                                 rt_size_t size)
{
  
    return size;
}



/*****************************************
**	init:初始化
**	open:打开w
*****************************************/


void rt_hw_wkup_init(void)
{
	wkup.type	= RT_Device_Class_Miscellaneous;
    /* register rtc device */
	wkup.rx_indicate = RT_NULL;
	wkup.rx_indicate = RT_NULL;
	
  wkup.init 	= rt_wkup_init;      //初始化RTC
  wkup.open 	= rt_wkup_open;			//
  wkup.close	= rt_wkup_close;
  wkup.read 	= rt_wkup_read;
  wkup.write	= rt_wkup_write;
  wkup.control = rt_wkup_control;
  wkup.user_data = RT_NULL;

  rt_device_register(&wkup, "wkup", RT_DEVICE_FLAG_RDWR);
	
	return;
}







