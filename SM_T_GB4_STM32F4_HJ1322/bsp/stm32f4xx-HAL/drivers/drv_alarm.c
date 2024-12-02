

#include "board.h"
#include "drv_alarm.h"



static struct rt_device alarm_a;



/**********************************************
**	控制RTC
*********************************************/
static rt_err_t rt_alarm_a_control(rt_device_t dev, int cmd, void *args)
{
		
    RT_ASSERT(dev != RT_NULL);

    switch (cmd)
    {
			case RT_DEVICE_CTRL_READ_ALARM_TIME:               //读取时间
        break;
			case  RT_DEVICE_CTRL_SET_ALARM_TIME:                 //设置时间
			
			break;
    }

    return RT_EOK;
}



/*******************************************************
**	初始化RTC
********************************************************/
static rt_err_t rt_alarm_a_init(struct rt_device *dev)
{

	return RT_EOK;
}


/*******************************************************
**	打开RTC
********************************************************/
static rt_err_t rt_alarm_a_open(rt_device_t dev, rt_uint16_t oflag)
{ 
	
	return RT_EOK;
}


/*******************************************************
**	关闭RTC()
********************************************************/
static rt_err_t rt_alarm_a_close(struct rt_device *dev)
{
	
	
	return RT_EOK;
}


/*******************************************************
**	RTC始终配置
********************************************************/
static rt_size_t rt_alarm_a_read(struct rt_device *dev,
                                rt_off_t pos,
                                void *buffer,
                                rt_size_t size)
{
    rt_alarm_a_control(dev, RT_DEVICE_CTRL_RTC_GET_TIME, buffer);
	
    return size;
}





/*******************************************************
**	RTC始终配置
********************************************************/
static rt_size_t rt_alarm_a_write(struct rt_device *dev,
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


void rt_hw_alarm_init(void)
{
	alarm_a.type	= RT_Device_Class_Miscellaneous;
    /* register rtc device */
	alarm_a.rx_indicate = RT_NULL;
	alarm_a.rx_indicate = RT_NULL;
  alarm_a.init 	= rt_alarm_a_init;      //初始化RTC
  alarm_a.open 	= rt_alarm_a_open;			//
  alarm_a.close	= rt_alarm_a_close;
  alarm_a.read 	= rt_alarm_a_read;
  alarm_a.write	= rt_alarm_a_write;
	alarm_a.control = rt_alarm_a_control;
  alarm_a.user_data = RT_NULL;

  rt_device_register(&alarm_a, "alarm_a", RT_DEVICE_FLAG_RDWR);
	
	return;
}	


