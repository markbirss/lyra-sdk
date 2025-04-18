/**
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 ******************************************************************************
 * @file    rga_drv.h
 * @author  cerf yu
 * @version V0.1
 * @date    23-Jul-2024
 * @brief   rga driver
 *
 ******************************************************************************
 */

#ifndef __DRV_RGA_H__
#define __DRV_RGA_H__

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <rtdevice.h>
#include <rtthread.h>
#include <rthw.h>

#include "rga.h"

#include "rtos_adapter.h"

/* Driver information */
#define DRIVER_DESC     "RGA multicore Device Driver for RT-thread"
#define DRIVER_NAME     "rga_multicore"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define DRIVER_MAJOR_VERISON        1
#define DRIVER_MINOR_VERSION        3
#define DRIVER_REVISION_VERSION     5
#define DRIVER_PATCH_VERSION        _for_RT-Thread

#define DRIVER_VERSION (STR(DRIVER_MAJOR_VERISON) "." STR(DRIVER_MINOR_VERSION) \
            "." STR(DRIVER_REVISION_VERSION) STR(DRIVER_PATCH_VERSION))

/* time limit */
#define RGA_JOB_TIMEOUT_DELAY       1000
#define RGA_RESET_TIMEOUT           1000

#define RGA_MAX_SCHEDULER           RGA_HW_SIZE
#define RGA_MAX_BUS_CLK             10

extern struct rga_drvdata *g_rga_drv_data;

enum
{
    RGA_CMD_SLAVE       = 1,
    RGA_CMD_MASTER      = 2,
};

enum rga_scheduler_status
{
    RGA_SCHEDULER_IDLE = 0,
    RGA_SCHEDULER_WORKING,
    RGA_SCHEDULER_ABORT,
};

enum rga_job_state
{
    RGA_JOB_STATE_PENDING = 0,
    RGA_JOB_STATE_PREPARE,
    RGA_JOB_STATE_RUNNING,
    RGA_JOB_STATE_FINISH,
    RGA_JOB_STATE_DONE,
    RGA_JOB_STATE_INTR_ERR,
    RGA_JOB_STATE_HW_TIMEOUT,
    RGA_JOB_STATE_ABORT,
};

enum RGA_DEVICE_TYPE
{
    RGA_DEVICE_RGA2 = 0,
    RGA_DEVICE_RGA3,
    RGA_DEVICE_BUTT,
};

struct rga_session
{
    int id;

    pid_t tgid;

    char *pname;
};

struct rga_dma_buffer
{
    // /* DMABUF information */
    // struct dma_buf *dma_buf;
    // struct dma_buf_attachment *attach;
    // struct sg_table *sgt;
    void *vaddr;

    // struct iommu_domain *domain;

    // enum dma_data_direction dir;

    dma_addr_t iova;
    dma_addr_t dma_addr;
    unsigned long size;
    /*
     * The offset of the first page of the sgt.
     * Since alloc iova must be page aligned, the offset of the first page is
     * identified separately.
     */
    size_t offset;

    /* The scheduler of the mapping */
    struct rga_scheduler *scheduler;
};

struct rga_internal_buffer
{
    // /* DMA buffer */
    struct rga_dma_buffer *dma_buffer;

    // /* virtual address */
    // struct rga_virt_addr *virt_addr;

    /* physical address */
    uint64_t phys_addr;

    /* buffer size */
    unsigned long size;

    struct rga_memory_parm memory_parm;


    // struct mm_struct *current_mm;

    /* memory type. */
    uint32_t type;

    uint32_t handle;

    uint32_t mm_flag;

    // struct kref refcount;
    struct rga_session *session;
};

struct rga_job_buffer
{
    union
    {
        struct
        {
            struct rga_external_buffer *ex_y_addr;
            struct rga_external_buffer *ex_uv_addr;
            struct rga_external_buffer *ex_v_addr;
        };
        struct rga_external_buffer *ex_addr;
    };

    union
    {
        struct
        {
            struct rga_internal_buffer *y_addr;
            struct rga_internal_buffer *uv_addr;
            struct rga_internal_buffer *v_addr;
        };
        struct rga_internal_buffer *addr;
    };

    uint32_t *page_table;
    int order;
    int page_count;
};

struct rga_job
{
    // struct list_head head;
    rt_list_t head;

    struct rga_scheduler *scheduler;
    struct rga_session *session;

    struct rga_req rga_command_base;
    struct rga_dma_buffer *cmd_buf;
    struct rga_full_csc full_csc;
    struct rga_csc_clip full_csc_clip;
    struct rga_pre_intr_info pre_intr_info;

    struct rga_job_buffer src_buffer;
    struct rga_job_buffer src1_buffer;
    struct rga_job_buffer dst_buffer;
    /* used by rga2 */
    struct rga_job_buffer els_buffer;

    /* for rga2 virtual_address */
    struct mm_struct *mm;

    /* job time stamp */
    ktime_t timestamp;
    /* The time when the job is actually executed on the hardware */
    ktime_t hw_running_time;
    /* The time only for hrtimer to calculate the load */
    ktime_t hw_recoder_time;
    unsigned int flags;
    int request_id;
    int priority;
    int core;
    int ret;
    pid_t pid;
    bool use_batch_mode;

    // struct kref refcount;
    unsigned long state;
    uint32_t intr_status;
    uint32_t hw_status;
    uint32_t cmd_status;

    uint32_t work_cycle;

    //for rtt
    struct rt_completion completion;
};

struct rga_timer
{
    u32 busy_time;
    u32 busy_time_record;
};

struct rga_scheduler
{
    struct rt_device *dev;
    void __iomem *rga_base;
//  struct rga_iommu_info *iommu_info;

    struct clk_gate *aclk;
    struct clk_gate *hclk;
    struct clk_gate *core_clk;
//  struct clk_bulk_data *clks;
    int num_clks;


    enum rga_scheduler_status status;
//  int pd_refcount;

    struct rga_job *running_job;
    //struct list_head todo_list;
    spinlock_t irq_lock;
    wait_queue_head_t job_done_wq;

    const struct rga_backend_ops *ops;
    const struct rga_hw_data *data;
    unsigned long hw_issues_mask;

    int job_count;
    //int irq;
    struct rga_version_t version;
    int core;

    struct rga_timer timer;
};

struct rga_request
{
    struct rga_req *task_list;
    int task_count;
    uint32_t finished_task_count;
    uint32_t failed_task_count;

    bool use_batch_mode;
    bool is_running;
    bool is_done;
    int ret;
    uint32_t sync_mode;

    int32_t acquire_fence_fd;
    int32_t release_fence_fd;
    struct dma_fence *release_fence;
    // spinlock_t fence_lock;
    // struct work_struct fence_work;

    wait_queue_head_t finished_wq;

    int flags;
    uint8_t mpi_config_flags;
    int id;
    struct rga_session *session;

    spinlock_t lock;
    // struct kref refcount;

    pid_t pid;

    /*
     * The mapping of virtual addresses to obtain physical addresses requires
     * the memory mapping information of the current process.
     */
    struct mm_struct *current_mm;

    struct rga_feature feature;
    /* TODO: add some common work */
};

struct rga_pending_request_manager
{
    struct mutex lock;

    /*
     * @request_idr:
     *
     * Mapping of request id to object pointers. Used by the GEM
     * subsystem. Protected by @lock.
     */
    // struct idr request_idr;

    int request_count;
};

struct rga_session_manager
{
    struct mutex lock;

    // struct idr ctx_id_idr;

    int session_cnt;
};

struct rga_drvdata
{
    // /* used by rga2's mmu lock */
    struct mutex lock;

    struct rt_device *dev;

    struct rga_scheduler *scheduler[RGA_MAX_SCHEDULER];
    int num_of_scheduler;
    int device_count[RGA_DEVICE_BUTT];
    /* The scheduler_index used by default for memory mapping. */
    int map_scheduler_index;
    // struct rga_mmu_base *mmu_base;

    // struct delayed_work power_off_work;

    // struct rga_mm *mm;

    /* rga_job pending manager, import by RGA_START_CONFIG */
    struct rga_pending_request_manager *pend_request_manager;

    struct rga_session_manager *session_manager;

// #ifdef CONFIG_ROCKCHIP_RGA_ASYNC
//  struct rga_fence_context *fence_ctx;
// #endif

// #ifdef CONFIG_ROCKCHIP_RGA_DEBUGGER
//  struct rga_debugger *debugger;
// #endif
};

struct rga_backend_ops
{
    int (*get_version)(struct rga_scheduler *scheduler);
    int (*set_reg)(struct rga_job *job, struct rga_scheduler *scheduler);
    int (*init_reg)(struct rga_job *job);
    void (*soft_reset)(struct rga_scheduler *scheduler);
    int (*read_back_reg)(struct rga_job *job, struct rga_scheduler *scheduler);
    int (*read_status)(struct rga_job *job, struct rga_scheduler *scheduler);
    int (*irq)(struct rga_scheduler *scheduler);
    int (*isr_thread)(struct rga_job *job, struct rga_scheduler *scheduler);
};

static inline void writel(uint32_t value, uint32_t address)
{
    *((volatile uint32_t *)address) = value;
}

static inline uint32_t readl(uint32_t address)
{
    return *((volatile uint32_t *)address);
}

static inline int rga_read(int offset, struct rga_scheduler *scheduler)
{
    return readl((uint32_t)(scheduler->rga_base + offset));
}

static inline void rga_write(int value, int offset, struct rga_scheduler *scheduler)
{
    writel(value, (uint32_t)(scheduler->rga_base + offset));
}

#endif /* #ifndef __DRV_RGA_H__ */

