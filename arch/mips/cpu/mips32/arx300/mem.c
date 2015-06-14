/*
 * Copyright (C) 2011-2013 Daniel Schwierzeck, daniel.schwierzeck@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/soc.h>
#include <asm/lantiq/io.h>
#include <asm/lantiq/spl.h>
#include <asm/lantiq/cpu.h>
#include <asm/lantiq/mem.h>

/* Must be configured in BOARDDIR */
#include <ddr_settings.h>

#define CCS_AHBM_CR_BURST_EN		(1 << 2)
#define CCS_FPIM_CR_BURST_EN		(1 << 1)

#define CCR03_EIGHT_BANK_MODE		(1 << 0)
#define CCR07_START			(1 << 8)
#define CCR47_DLL_LOCK_IND		1

struct mc_global_regs {
	u32 rsvd0[0x100];
	u32 ddr_gsr0;		/* DDR SDRAM controller global status 0 */
	u32 rsvd1;
	u32 ddr_gcr0;		/* DDR SDRAM controller global control 0 */
	u32 rsvd2;
	u32 ddr_gcr1;		/* DDR SDRAM controller global control 1 */
	u32 rsvd3;
	u32 ddr_prio_ti;	/* DDR SDRAM controller priority of transaction initiator */
	u32 rsvd4;
	u32 ddr_echo_dll0;	/* DDR SDRAM controller echo gate DLL0 control */
	u32 rsvd5;
	u32 ddr_echo_dll1;	/* DDR SDRAM controller echo gate DLL1 control */
	u32 rsvd6;
	u32 ccs;		/* undocumented */
};

static struct mc_global_regs *mc_global_regs =
	(void *) CKSEG1ADDR(LTQ_MC_GLOBAL_BASE);
static void *mc_ddr_base = (void *) CKSEG1ADDR(LTQ_MC_DDR_BASE);
static void *bootrom_base = (void *) CKSEG1ADDR(LTQ_BOOTROM_BASE);

static __always_inline
u32 mc_ccr_read(u32 index)
{
	return ltq_readl(mc_ddr_base + LTQ_MC_DDR_CCR_OFFSET(index));
}

static __always_inline
void mc_ccr_write(u32 index, u32 val)
{
	ltq_writel(mc_ddr_base + LTQ_MC_DDR_CCR_OFFSET(index), val);
}

static __always_inline
u32 bootrom_readl(u32 offset)
{
	return ltq_readl(bootrom_base + offset);
}

static __always_inline
void bootrom_writel(u32 offset, u32 val)
{
	ltq_writel(bootrom_base + offset, val);
}

static __always_inline
void mc_halt(void)
{
	/*
	 * Create multiple dummy accesses to ROM space to ensure the MIPS to
	 * Xbar path is cleared of pending DDR SDRAM transactions.
	 */
	bootrom_writel(0, bootrom_readl(4));
	bootrom_writel(4, bootrom_readl(8));
	bootrom_writel(8, bootrom_readl(0));

	/* Stop memory controller */
	mc_ccr_write(7, MC_CCR07_VALUE & ~CCR07_START);
}

static __always_inline
void mc_start(void)
{
	/* Start initialization sequence */
	mc_ccr_write(7, MC_CCR07_VALUE | CCR07_START);

	/* Wait until DLL0 has locked */
	while (!(mc_ccr_read(47) & CCR47_DLL_LOCK_IND))
		;

	/* Wait until DLL1 has locked */
	while (!(mc_ccr_read(48) & CCR47_DLL_LOCK_IND))
		;
}

static __always_inline
void mc_init(void)
{
	/* Init MC DDR CCR registers with values from ddr_settings.h */
	mc_ccr_write(0, MC_CCR00_VALUE);
	mc_ccr_write(1, MC_CCR01_VALUE);
	mc_ccr_write(2, MC_CCR02_VALUE);
	mc_ccr_write(3, MC_CCR03_VALUE);
	mc_ccr_write(4, MC_CCR04_VALUE);
	mc_ccr_write(5, MC_CCR05_VALUE);
	mc_ccr_write(6, MC_CCR06_VALUE);
	mc_ccr_write(8, MC_CCR08_VALUE);
	mc_ccr_write(9, MC_CCR09_VALUE);

	mc_ccr_write(10, MC_CCR10_VALUE);
	mc_ccr_write(11, MC_CCR11_VALUE);
	mc_ccr_write(12, MC_CCR12_VALUE);
	mc_ccr_write(13, MC_CCR13_VALUE);
	mc_ccr_write(14, MC_CCR14_VALUE);
	mc_ccr_write(15, MC_CCR15_VALUE);
	mc_ccr_write(16, MC_CCR16_VALUE);
	mc_ccr_write(17, MC_CCR17_VALUE);
	mc_ccr_write(18, MC_CCR18_VALUE);
	mc_ccr_write(19, MC_CCR19_VALUE);

	mc_ccr_write(20, MC_CCR20_VALUE);
	mc_ccr_write(21, MC_CCR21_VALUE);
	mc_ccr_write(22, MC_CCR22_VALUE);
	mc_ccr_write(23, MC_CCR23_VALUE);
	mc_ccr_write(24, MC_CCR24_VALUE);
	mc_ccr_write(25, MC_CCR25_VALUE);
	mc_ccr_write(26, MC_CCR26_VALUE);
	mc_ccr_write(27, MC_CCR27_VALUE);
	mc_ccr_write(28, MC_CCR28_VALUE);
	mc_ccr_write(29, MC_CCR29_VALUE);

	mc_ccr_write(30, MC_CCR30_VALUE);
	mc_ccr_write(31, MC_CCR31_VALUE);
	mc_ccr_write(32, MC_CCR32_VALUE);
	mc_ccr_write(33, MC_CCR33_VALUE);
	mc_ccr_write(34, MC_CCR34_VALUE);
	mc_ccr_write(35, MC_CCR35_VALUE);
	mc_ccr_write(36, MC_CCR36_VALUE);
	mc_ccr_write(37, MC_CCR37_VALUE);
	mc_ccr_write(38, MC_CCR38_VALUE);
	mc_ccr_write(39, MC_CCR39_VALUE);

	mc_ccr_write(40, MC_CCR40_VALUE);
	mc_ccr_write(41, MC_CCR41_VALUE);
	mc_ccr_write(42, MC_CCR42_VALUE);
	mc_ccr_write(43, MC_CCR43_VALUE);
	mc_ccr_write(44, MC_CCR44_VALUE);
	mc_ccr_write(45, MC_CCR45_VALUE);
	mc_ccr_write(46, MC_CCR46_VALUE);

	mc_ccr_write(52, MC_CCR52_VALUE);
	mc_ccr_write(53, MC_CCR53_VALUE);
	mc_ccr_write(54, MC_CCR54_VALUE);
	mc_ccr_write(55, MC_CCR55_VALUE);

	mc_ccr_write(64, MC_PHYR0_VALUE);
	mc_ccr_write(65, MC_PHYR1_VALUE);
	mc_ccr_write(66, MC_PHYR2_VALUE);
	mc_ccr_write(67, MC_PHYR3_VALUE);
	mc_ccr_write(68, MC_PHYR4_VALUE);
	mc_ccr_write(69, MC_PHYR5_VALUE);
	mc_ccr_write(70, MC_PHYR6_VALUE);
	mc_ccr_write(71, MC_PHYR7_VALUE);
	mc_ccr_write(72, MC_PHYR8_VALUE);
	mc_ccr_write(73, MC_PHYR9_VALUE);
}

void ltq_mem_init(void)
{
	mc_halt();
	mc_init();
	mc_start();
	puts("ltq_mem_init\n");
}

static phys_size_t mc_sdram_size(void)
{
	unsigned int colsize, rowsize, banks, cs_map;
	phys_size_t size;

	banks = ((mc_ccr_read(31) >> 16) & 0x1) ? 8 : 4;

	cs_map = mc_ccr_read(35) & 0x3;

	// Col Address[2:0]; "000"=12, "001"=11, "010"=10, "011"=9, 100=8;
	colsize = 12 - (mc_ccr_read(32) & 0x7);

	// Row Address[26:24]; "000"=15, "001"=14, "010"=13, "011"=12, "100"=11 , "101"=10
	rowsize = 15 - ((mc_ccr_read(31) >> 24) & 0x7);

	/*
	 * size (bytes) = (2 ^ rowsize) * (2 ^ colsize)
	 *                 * banks * chipselects
	 *                 * datawidth [bytes]
	 */
	size = (2 << (colsize - 1)) * (2 << (rowsize - 1)) *
		banks * cs_map * 2;

	return size;
}

phys_size_t initdram(int board_type)
{
	return mc_sdram_size();
}
