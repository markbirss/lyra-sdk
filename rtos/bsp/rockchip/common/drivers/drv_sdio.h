/*
 * Copyright (c) 2019, Rockchip
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-1-16     Shawn Lin      first implementation
 */

#ifndef DRV_SDIO_H_
#define DRV_SDIO_H_

#include "hal_sdio.h"
#include <rtdevice.h>
#include <rthw.h>

#define MMC_FEQ_MIN 100000
#define MMC_FEQ_MAX 50000000

#define CARD_UNPLUGED   1
#define CARD_PLUGED     0

struct mmc_driver
{
    MMC_DMA_DESCRIPTORS *dma_descriptors;
    rt_uint32_t max_desc;
    struct rt_mmcsd_host *host;
    struct rt_mmcsd_req *req;
    struct rt_mmcsd_data *data;
    struct rt_mmcsd_cmd *cmd;
    struct rt_completion transfer_completion;
    struct rk_mmc_platform_data *pdata;
    void  *priv;
    rt_uint32_t transfer_state;
    rt_uint32_t *mmc_dma_buf;
    bool is_write_emergency;
};

struct rk_mmc_platform_data
{
    rt_uint32_t flags; /* define device capabilities */
    rt_uint32_t irq;
    rt_uint32_t base;
    rt_uint64_t clk_id; /* clock for devices */
    rt_uint32_t hclk_gate;  /* clock gate for registers */
    rt_uint32_t clk_gate;  /* clock gate for device */
    rt_uint32_t freq_min;
    rt_uint32_t freq_max;
    bool is_pwr_gpio;
    struct GPIO_REG *pwr_gpio;
    eGPIO_bankId pwr_gpio_bank;
#ifdef HAL_GPIO_MODULE_ENABLED
    ePINCTRL_GPIO_PINS pwr_gpio_pin;
#endif
    struct mmc_driver *drv;
    rt_uint32_t control_id;
    rt_bool_t keep_power_in_suspend;
};

void rt_hw_mmc_init(void);

#endif /* DRV_SDIO_H_ */
