/*
 * Copyright (C) 2010 Lantiq Deutschland GmbH
 * Copyright (C) 2011-2013 Daniel Schwierzeck, daniel.schwierzeck@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/soc.h>
#include <asm/arch/gphy.h>
#include <asm/lantiq/clk.h>
#include <asm/lantiq/io.h>

#define LTQ_CGU_SYS_CLK_SHIFT		8
#define LTQ_CGU_SYS_CLK_MASK		(0x1 << LTQ_CGU_SYS_CLK_SHIFT)
#define LTQ_CGU_SYS_CPU_SHIFT		4
#define LTQ_CGU_SYS_CPU_MASK		(0x7 << LTQ_CGU_SYS_CPU_SHIFT)
#define LTQ_CGU_SYS_IF_SHIFT		25
#define LTQ_CGU_SYS_IF_MASK		(0xF << LTQ_CGU_SYS_IF_SHIFT)

#define LTQ_CGU_UPDATE			1

#define LTQ_CGU_IFCLK_GPHY_SEL_SHIFT	2
#define LTQ_CGU_IFCLK_GPHY_SEL_MASK	(0x7 << LTQ_CGU_IFCLK_GPHY_SEL_SHIFT)

#define LTQ_CGU_GPHY_CFG_MASK		0xFFFFFFFF

#define LTQ_CHIPID_CFG			(LTQ_CHIPID_BASE + 0x10)
#define CHIPID_CFG_IF_CLK_125		(1 << 17)

struct ltq_cgu_regs {
	u32	rsvd0;
	u32	pll0_cfg;	/* PLL0 config */
	u32	pll1_cfg;	/* PLL1 config */
	u32	sys;		/* System clock */
	u32	clk_fsr;	/* Clock frequency select */
	u32	clk_gsr;	/* Clock gating status */
	u32	clk_gcr0;	/* Clock gating control 0 */
	u32	clk_gcr1;	/* Clock gating control 1 */
	u32	update;		/* CGU update control */
	u32	if_clk;		/* Interface clock */
	u32	ddr;		/* DDR memory control */
	u32	ct1_sr;		/* CT status 1 */
	u32	ct_kval;	/* CT K value */
	u32	pcm_cr;		/* PCM control */
	u32	pci_cr;		/* PCI clock control */
	u32	rsvd1;
	u32	gphy1_cfg;	/* GPHY1 config */
	u32	gphy2_cfg;	/* GPHY2 config */
	u32	gphy3_cfg;	/* GPHY3 config */
	u32	gphy0_cfg;	/* GPHY0 config */
};

static struct ltq_cgu_regs *ltq_cgu_regs =
	(struct ltq_cgu_regs *) CKSEG1ADDR(LTQ_CGU_BASE);
static u32 *ltq_chipid_cfg = (void *) CKSEG1ADDR(LTQ_CHIPID_CFG);

static inline u32 ltq_cgu_sys_readl(u32 mask, u32 shift)
{
	return (ltq_readl(&ltq_cgu_regs->sys) & mask) >> shift;
}

static inline u32 ltq_cgu_if_readl(u32 mask, u32 shift)
{
	return (ltq_readl(&ltq_cgu_regs->if_clk) & mask) >> shift;
}

unsigned long ltq_get_io_region_clock(void)
{
	unsigned int io_sel;
	unsigned long clk;

	io_sel = ltq_cgu_if_readl(LTQ_CGU_SYS_IF_MASK,
				   LTQ_CGU_SYS_IF_SHIFT);

	switch (io_sel) {
		case 1:
			clk = CLOCK_300_MHZ;
			break;
		case 2:
			/* check BSP_MPS_ID_CFG, if bit 17 is set,
			 * treat FPI clock as 125 instead of 150 */
			if (*ltq_chipid_cfg & CHIPID_CFG_IF_CLK_125)
				clk = CLOCK_125_MHZ;
			else
				clk = CLOCK_150_MHZ;
			break;
		case 5:
			clk = CLOCK_250_MHZ;
			break;
		case 6:
			clk = CLOCK_125_MHZ;
			break;
		default:
			clk = 0;
			break;
	}

	return clk;
}

unsigned long ltq_get_sys_clock(void)
{
	unsigned long sys_clk;

	if (ltq_cgu_sys_readl(LTQ_CGU_SYS_CLK_MASK, LTQ_CGU_SYS_CLK_SHIFT))
		sys_clk = CLOCK_500_MHZ;
	else
		sys_clk = CLOCK_600_MHZ;

	return sys_clk;
}

unsigned long ltq_get_cpu_clock(void)
{
	unsigned int cpu_sel;
	unsigned long clk, sys_clk;

	cpu_sel = ltq_cgu_sys_readl(LTQ_CGU_SYS_CPU_MASK,
			LTQ_CGU_SYS_CPU_SHIFT);
	sys_clk = ltq_get_sys_clock();

	switch (cpu_sel) {
	case 0:
		clk = sys_clk;
		break;
	case 1:
		clk = sys_clk / 2;
		break;
	case 2:
		clk = sys_clk / 4;
		break;
	default:
		clk = 0;
		break;
	}

	return clk;
}

unsigned long ltq_get_bus_clock(void)
{
	return ltq_get_io_region_clock();
}

void ltq_cgu_gphy_clk_src(enum ltq_gphy_clk clk)
{
	unsigned int clk_val;
	unsigned int gphy_cfg;

	switch (clk) {
	case LTQ_GPHY_CLK_36MHZ_XTAL:
		clk_val = 1;
		gphy_cfg = 0x1fdb20c5;
		break;
	case LTQ_GPHY_CLK_25MHZ_PLL0:
	case LTQ_GPHY_CLK_25MHZ_GPIO3:
		clk_val = 2;
		gphy_cfg = 0x1fe70000;
		break;
	default:
		BUG();
	}

	ltq_clrbits(&ltq_cgu_regs->if_clk, LTQ_CGU_IFCLK_GPHY_SEL_MASK);
	ltq_setbits(&ltq_cgu_regs->if_clk, clk_val << LTQ_CGU_IFCLK_GPHY_SEL_SHIFT);

	// FIXME: UGW does this - do we need to do it as well (seems to work without)?
	ltq_clrsetbits(&ltq_cgu_regs->gphy0_cfg, LTQ_CGU_GPHY_CFG_MASK, gphy_cfg);
	ltq_clrsetbits(&ltq_cgu_regs->gphy1_cfg, LTQ_CGU_GPHY_CFG_MASK, gphy_cfg);
	ltq_clrsetbits(&ltq_cgu_regs->gphy2_cfg, LTQ_CGU_GPHY_CFG_MASK, gphy_cfg);
}
