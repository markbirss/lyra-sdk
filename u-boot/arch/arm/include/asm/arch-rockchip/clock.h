/*
 * (C) Copyright 2015 Google, Inc
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#ifndef _ASM_ARCH_CLOCK_H
#define _ASM_ARCH_CLOCK_H

/* define pll mode */
#define RKCLK_PLL_MODE_SLOW		0
#define RKCLK_PLL_MODE_NORMAL		1
#define RKCLK_PLL_MODE_DEEP		2

/*
 * PLL flags
 */
#define ROCKCHIP_PLL_SYNC_RATE		BIT(0)
/* normal mode only. now only for pll_rk3036, pll_rk3328 type */
#define ROCKCHIP_PLL_FIXED_MODE		BIT(1)

enum {
	ROCKCHIP_SYSCON_NOC,
	ROCKCHIP_SYSCON_GRF,
	ROCKCHIP_SYSCON_SGRF,
	ROCKCHIP_SYSCON_PMU,
	ROCKCHIP_SYSCON_PMUGRF,
	ROCKCHIP_SYSCON_PMUSGRF,
	ROCKCHIP_SYSCON_CIC,
	ROCKCHIP_SYSCON_MSCH,
	ROCKCHIP_SYSCON_USBGRF,
	ROCKCHIP_SYSCON_PCIE30_PHY_GRF,
	ROCKCHIP_SYSCON_PHP_GRF,
	ROCKCHIP_SYSCON_PIPE_PHY0_GRF,
	ROCKCHIP_SYSCON_PIPE_PHY1_GRF,
	ROCKCHIP_SYSCON_PIPE_PHY2_GRF,
	ROCKCHIP_SYSCON_VOP_GRF,
	ROCKCHIP_SYSCON_VO_GRF,
	ROCKCHIP_SYSCON_IOC,
	ROCKCHIP_SYSCON_SDGMAC_GRF,
};

/* Standard Rockchip clock numbers */
enum rk_clk_id {
	CLK_OSC,
	CLK_ARM,
	CLK_DDR,
	CLK_CODEC,
	CLK_GENERAL,
	CLK_NEW,

	CLK_COUNT,
};

#define PLL(_type, _id, _con, _mode, _mshift,			\
		 _lshift, _pflags, _rtable)			\
	{							\
		.id		= _id,				\
		.type		= _type,			\
		.con_offset	= _con,				\
		.mode_offset	= _mode,			\
		.mode_shift	= _mshift,			\
		.lock_shift	= _lshift,			\
		.pll_flags	= _pflags,			\
		.rate_table	= _rtable,			\
	}

#define RK3036_PLL_RATE(_rate, _refdiv, _fbdiv, _postdiv1,	\
			_postdiv2, _dsmpd, _frac)		\
{								\
	.rate	= _rate##U,					\
	.fbdiv = _fbdiv,					\
	.postdiv1 = _postdiv1,					\
	.refdiv = _refdiv,					\
	.postdiv2 = _postdiv2,					\
	.dsmpd = _dsmpd,					\
	.frac = _frac,						\
}

#define RK3588_PLL_RATE(_rate, _p, _m, _s, _k)			\
{								\
	.rate	= _rate##U,					\
	.p = _p,						\
	.m = _m,						\
	.s = _s,						\
	.k = _k,						\
}

struct rockchip_pll_rate_table {
	unsigned long rate;
	unsigned int nr;
	unsigned int nf;
	unsigned int no;
	unsigned int nb;
	/* for RK3036/RK3399 */
	unsigned int fbdiv;
	unsigned int postdiv1;
	unsigned int refdiv;
	unsigned int postdiv2;
	unsigned int dsmpd;
	unsigned int frac;
	/* for RK3588 */
	unsigned int m;
	unsigned int p;
	unsigned int s;
	unsigned int k;
};

enum rockchip_pll_type {
	pll_rk3036,
	pll_rk3066,
	pll_rk3328,
	pll_rk3366,
	pll_rk3399,
	pll_rk3588,
};

struct rockchip_pll_clock {
	unsigned int			id;
	unsigned int			con_offset;
	unsigned int			mode_offset;
	unsigned int			mode_shift;
	unsigned int			lock_shift;
	enum rockchip_pll_type		type;
	unsigned int			pll_flags;
	struct rockchip_pll_rate_table *rate_table;
	unsigned int			mode_mask;
};

struct rockchip_cpu_rate_table {
	unsigned long rate;
	unsigned int aclk_div;
	unsigned int pclk_div;
};

#ifdef CONFIG_ROCKCHIP_IMAGE_TINY
static inline ulong rockchip_pll_get_rate(struct rockchip_pll_clock *pll,
					  void __iomem *base,
					  ulong pll_id)
{
	return 0;
}

static inline int rockchip_pll_set_rate(struct rockchip_pll_clock *pll,
					void __iomem *base, ulong pll_id,
					ulong drate)
{
	return 0;
}

static inline const struct rockchip_cpu_rate_table *
rockchip_get_cpu_settings(struct rockchip_cpu_rate_table *cpu_table,
			  ulong rate)
{
	return NULL;
}
#else
int rockchip_pll_set_rate(struct rockchip_pll_clock *pll,
			  void __iomem *base, ulong clk_id,
			  ulong drate);
ulong rockchip_pll_get_rate(struct rockchip_pll_clock *pll,
			    void __iomem *base, ulong clk_id);
const struct rockchip_cpu_rate_table *
rockchip_get_cpu_settings(struct rockchip_cpu_rate_table *cpu_table,
			  ulong rate);
#endif

static inline int rk_pll_id(enum rk_clk_id clk_id)
{
	return clk_id - 1;
}

struct sysreset_reg {
	unsigned int glb_srst_fst_value;
	unsigned int glb_srst_snd_value;
};

struct softreset_reg {
	void __iomem *base;
	unsigned int sf_reset_offset;
	unsigned int sf_reset_num;
};

/**
 * clk_get_divisor() - Calculate the required clock divisior
 *
 * Given an input rate and a required output_rate, calculate the Rockchip
 * divisor needed to achieve this.
 *
 * @input_rate:		Input clock rate in Hz
 * @output_rate:	Output clock rate in Hz
 * @return divisor register value to use
 */
static inline u32 clk_get_divisor(ulong input_rate, uint output_rate)
{
	uint clk_div;

	clk_div = input_rate / output_rate;
	clk_div = (clk_div + 1) & 0xfffe;

	return clk_div;
}

/**
 * rockchip_get_cru() - get a pointer to the clock/reset unit registers
 *
 * @return pointer to registers, or -ve error on error
 */
void *rockchip_get_cru(void);

/**
 * rockchip_get_pmucru() - get a pointer to the clock/reset unit registers
 *
 * @return pointer to registers, or -ve error on error
 */
void *rockchip_get_pmucru(void);

struct rk3288_cru;
struct rk3288_grf;

void rk3288_clk_configure_cpu(struct rk3288_cru *cru, struct rk3288_grf *grf);

int rockchip_get_clk(struct udevice **devp);

int rockchip_get_scmi_clk(struct udevice **devp);

#endif
