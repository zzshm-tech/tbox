
/***********************************
**	File Name:drv_watchdog.c
**	2020.09.12
************************************/

#include <rtthread.h>
#include "board.h"
#include "drv_watchdog.h"


static struct rt_device watchdog; 


static IWDG_HandleTypeDef hiwdg;


static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}



/**********************************************
**	控制watchdog
*********************************************/
static rt_err_t rt_watchdog_control(rt_device_t dev, int cmd, void *args)
{
    RT_ASSERT(dev != RT_NULL);

    switch (cmd)
    {
			case RT_DEVICE_CTRL_WDT_GET_TIMEOUT:   	/* 获取溢出时间 */
        break;

			case RT_DEVICE_CTRL_WDT_SET_TIMEOUT:		/* 设置溢出时间 */
				{
					float tmp;
					rt_uint16_t sec;
					
					sec = *(rt_uint16_t *)args;
					if(sec > 25)
						sec = 0;
		
					sec *= 1000;
					
					hiwdg.Instance = IWDG;
					hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
					
					tmp = (sec - 6.4) / 6.4;
		
					sec = (unsigned short int) tmp;
					
					hiwdg.Init.Reload = sec;
					if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
					{
						//Error_Handler();
					}
				}
				break;
			case RT_DEVICE_CTRL_WDT_GET_TIMELEFT:		/* 获取剩余时间 */
				
				break;
			case RT_DEVICE_CTRL_WDT_KEEPALIVE:		 /* 喂狗 */
				HAL_IWDG_Refresh(&hiwdg);
				break;
			case RT_DEVICE_CTRL_WDT_START:          /* 启动看门狗 */
				break;
			case RT_DEVICE_CTRL_WDT_STOP:						 /* 停止看门狗 */
				break;
    }

    return RT_EOK;
}



/*******************************************************
**	初始化watchdog
********************************************************/
static rt_err_t rt_watchdog_init(struct rt_device *dev)
{
	MX_IWDG_Init();
  return RT_EOK;
}


/*******************************************************
**	打开watchdog
********************************************************/
static rt_err_t rt_watchdog_open(rt_device_t dev, rt_uint16_t oflag)
{
	return RT_EOK;
}



/**************************************
**	看门狗
***************************************/

int rt_hw_watchdog_init(void)
{
    watchdog.type	= RT_Device_Class_Miscellaneous;
    /* register rtc device */
		watchdog.rx_indicate = RT_NULL;
		watchdog.rx_indicate = RT_NULL;
    watchdog.init 	= rt_watchdog_init;
    watchdog.open 	= rt_watchdog_open;
    watchdog.close	= RT_NULL;
    watchdog.read 	= RT_NULL;
    watchdog.write	= RT_NULL;
    watchdog.control = rt_watchdog_control;
    watchdog.user_data = RT_NULL;

    return rt_device_register(&watchdog, "watchdog", RT_DEVICE_FLAG_RDWR);
}

INIT_BOARD_EXPORT(rt_hw_watchdog_init);



