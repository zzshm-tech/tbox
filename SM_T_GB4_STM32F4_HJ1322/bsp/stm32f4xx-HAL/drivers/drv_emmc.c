/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author         Notes
 * 2018-01-13     Liu2guang      the first version.
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "drv_emmc.h"

//#include "stm32f4xx_hal_sd.h"
//#include "stm32f4xx_hal_mmc.h"
//#include "stm32f4xx_ll_sdmmc.h"

#ifndef SDIO_CLK_DIV
    #define SDIO_CLK_DIV 2
#endif
#define SDIO_TIMEOUT ((uint32_t)0x100000)
static MMC_HandleTypeDef hsdemmc;
static DMA_HandleTypeDef hdma;
static struct rt_semaphore emmc_lock;

void SDIO_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_MMC_IRQHandler(&hsdemmc);
    rt_interrupt_leave();
}

#if defined(USING_EMMC_RX_DMA) || defined(USING_EMMC_TX_DMA)
void DMA2_Stream6_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_DMA_IRQHandler(&hdma);
    rt_interrupt_leave();
}
#endif

rt_err_t stm32_read_blocks(uint32_t *data, uint32_t addr, uint32_t num)
{
    uint32_t timeout = 0;
    HAL_MMC_StateTypeDef state_return;
    HAL_MMC_CardStateTypeDef emmc_card_state_return;
#if defined(USING_EMMC_RX_DMA) && defined(USING_EMMC_TX_DMA)
    hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma.Init.MemInc    = DMA_MINC_ENABLE;
    HAL_DMA_DeInit(&hdma);
    HAL_DMA_Init(&hdma);
#endif
#if defined(USING_EMMC_RX_DMA)
    if (HAL_MMC_ReadBlocks_DMA(&hsdemmc, (uint8_t *)data, addr, num) != HAL_OK)
#else
    if (HAL_MMC_ReadBlocks(&hsdemmc, (uint8_t *)data, addr, num, SDIO_TIMEOUT) != HAL_OK)
#endif
    {
        return RT_EIO;
    }
    do
    {
        state_return = HAL_MMC_GetState(&hsdemmc);
        timeout++;
    }
    while ((HAL_MMC_STATE_BUSY == state_return) && (SDIO_TIMEOUT > timeout));
    if (HAL_MMC_STATE_READY != state_return)
    {
        return RT_ERROR;
    }
    do
    {
        emmc_card_state_return = HAL_MMC_GetCardState(&hsdemmc);
        timeout++;
    }
    while ((HAL_MMC_CARD_TRANSFER != emmc_card_state_return) && (SDIO_TIMEOUT > timeout));
    if (SDIO_TIMEOUT <= timeout)
    {
        return RT_ETIMEOUT;
    }
    return RT_EOK;
}

rt_err_t stm32_write_blocks(uint32_t *data, uint32_t addr, uint32_t num)
{
    uint32_t timeout = 0;
    HAL_MMC_StateTypeDef state_return;
    HAL_MMC_CardStateTypeDef emmc_card_state_return;
#if defined(USING_EMMC_RX_DMA) && defined(USING_EMMC_TX_DMA)
    hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma.Init.PeriphInc = DMA_MINC_ENABLE;
    hdma.Init.MemInc    = DMA_PINC_DISABLE;
    HAL_DMA_DeInit(&hdma);
    HAL_DMA_Init(&hdma);
#endif
#if defined(USING_EMMC_TX_DMA)
    if (HAL_MMC_WriteBlocks_DMA(&hsdemmc, (uint8_t *)data, addr, num) != HAL_OK)
#else
    if (HAL_MMC_WriteBlocks(&hsdemmc, (uint8_t *)data, addr, num, SDIO_TIMEOUT) != HAL_OK)
#endif
    {
        return RT_ERROR;
    }
    do
    {
        state_return = HAL_MMC_GetState(&hsdemmc);
        timeout++;
    }
    while ((HAL_MMC_STATE_BUSY == state_return) && (SDIO_TIMEOUT > timeout));
    if (HAL_MMC_STATE_READY != state_return)
    {
        return RT_ERROR;
    }
    do
    {
        emmc_card_state_return = HAL_MMC_GetCardState(&hsdemmc);
        timeout++;
    }
    while ((HAL_MMC_CARD_TRANSFER != emmc_card_state_return) && (SDIO_TIMEOUT > timeout));
    if (SDIO_TIMEOUT <= timeout)
    {
        return RT_ETIMEOUT;
    }
    return RT_EOK;
}

static rt_err_t stm32_emmc_init(rt_device_t dev)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    if (rt_sem_init(&emmc_lock, "emmclock", 1, RT_IPC_FLAG_FIFO) != RT_EOK)
    {
        return RT_ERROR;
    }
		
		//rt_kprintf("-- Debug Init Emmc.....1 \r\n");
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStruct.Pin   = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
                            GPIO_PIN_12;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull  = GPIO_PULLUP;          
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
		
    GPIO_InitStruct.Pin   = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
#if defined(USING_EMMC_RX_DMA) || defined(USING_EMMC_TX_DMA)
    __HAL_RCC_DMA2_CLK_ENABLE();
    hdma.Instance                 = DMA2_Stream6_IRQn;
    hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hdma.Init.Mode                = DMA_NORMAL;
    hdma.Init.Priority            = DMA_PRIORITY_HIGH;
#if defined(USING_EMMC_RX_DMA)
    hdma.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma.Init.MemInc              = DMA_MINC_ENABLE;
    __HAL_LINKDMA(&hsdemmc, hdmarx, hdma);
#endif
#if defined(USING_EMMC_TX_DMA)
    hdma.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma.Init.PeriphInc           = DMA_MINC_ENABLE;
    hdma.Init.MemInc              = DMA_PINC_DISABLE;
    __HAL_LINKDMA(&hsdemmc, hdmatx, hdma);
#endif
    HAL_DMA_DeInit(&hdma);
    if (HAL_DMA_Init(&hdma) != HAL_OK)
    {
        rt_kprintf("HAL_DMA_Init error\n");
        return RT_EIO;
    }
#endif
    HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
    __HAL_RCC_SDIO_CLK_ENABLE();
    hsdemmc.Instance                 = SDIO;
    hsdemmc.Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
    hsdemmc.Init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
    hsdemmc.Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
    hsdemmc.Init.BusWide             = SDIO_BUS_WIDE_1B;
    hsdemmc.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_ENABLE;
    hsdemmc.Init.ClockDiv            = SDIO_CLK_DIV;
    HAL_MMC_DeInit(&hsdemmc);
		//rt_kprintf("-- Debug Init Emmc..... 2\r\n");
    if (HAL_MMC_Init(&hsdemmc) != HAL_OK)
    {
        rt_kprintf("HAL_SD_Init error\n");
        return RT_EIO;
    }
		
    HAL_NVIC_SetPriority(SDIO_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);
		//rt_kprintf("-- Debug Init Emmc.....3\r\n");
    if (HAL_MMC_ConfigWideBusOperation(&hsdemmc, SDIO_BUS_WIDE_4B) != HAL_OK)
    {
        rt_kprintf("HAL_EMMC_ConfigWideBusOperation error\n");
        return RT_EIO;
    }
		//hsdemmc.MmcCard.BlockNbr  = 0x800000;   
		rt_kprintf("-- Init EMMC OK ...0x%x\r\n",hsdemmc.MmcCard.BlockNbr);
    return RT_EOK;
}

static rt_err_t stm32_emmc_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t stm32_emmc_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t stm32_emmc_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    int ret = RT_EOK;
    rt_sem_take(&emmc_lock, RT_WAITING_FOREVER);
    ret = stm32_read_blocks((uint32_t *)buffer, pos, size);
    rt_sem_release(&emmc_lock);
    if (ret != RT_EOK)
    {
        return 0;
    }
    return size;
}

static rt_size_t stm32_emmc_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    int ret = RT_EOK;
    rt_sem_take(&emmc_lock, RT_WAITING_FOREVER);
    ret = stm32_write_blocks((uint32_t *)buffer, pos, size);
    rt_sem_release(&emmc_lock);
    if (ret != RT_EOK)
    {
        return 0;
    }
    return size;
}

uint32_t emmc_flash_read(uint32_t blk_addr,void *buf,uint32_t blk_len)
{
	int ret = RT_EOK;
	ret = stm32_read_blocks((uint32_t *)buf, blk_addr, blk_len);
	if (ret != RT_EOK)
	{
	    return 0;
	}
	return blk_len;
}
uint32_t emmc_flash_write(uint32_t blk_addr,void *buf,uint32_t blk_len)
{
	int ret = RT_EOK;
	ret = stm32_write_blocks((uint32_t *)buf, blk_addr, blk_len);
	if (ret != RT_EOK)
	{
	    return 0;
	}
	return blk_len;
}


static rt_err_t stm32_emmc_control(rt_device_t dev, int cmd, void *args)
{
    RT_ASSERT(dev != RT_NULL);
    // RT_DEVICE_CTRL_BLK_GETGEOME
    if (cmd == RT_DEVICE_CTRL_BLK_GETGEOME)
    {
        HAL_MMC_CardInfoTypeDef emmc_info;
        struct rt_device_blk_geometry *geometry;
        HAL_MMC_GetCardInfo(&hsdemmc, &emmc_info);
        geometry = (struct rt_device_blk_geometry *)args;
        geometry->bytes_per_sector = emmc_info.BlockSize;
        geometry->block_size       = emmc_info.BlockSize;
        geometry->sector_count     = emmc_info.BlockNbr;
    }
    return RT_EOK;
}

static struct rt_device device;

int rt_hw_emmc_init(void)
{
    rt_err_t ret = RT_EOK;
    device.type    = RT_Device_Class_Block;
    device.init    = stm32_emmc_init;
    device.open    = stm32_emmc_open;
    device.read    = stm32_emmc_read;
    device.write   = stm32_emmc_write;
    device.control = stm32_emmc_control;
    device.close   = stm32_emmc_close;
    ret = rt_device_register(&device, "sd0",
                             RT_DEVICE_FLAG_REMOVABLE |
                             RT_DEVICE_FLAG_RDWR      |
                             RT_DEVICE_FLAG_STANDALONE);
    if (ret != RT_EOK)
    {
        return ret;
    }
    return RT_EOK;
}


INIT_DEVICE_EXPORT(rt_hw_emmc_init);
