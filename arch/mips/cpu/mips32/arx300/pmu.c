/*
 * Copyright (C) 2015 Martin Blumenstingl <martin.blumenstingl@googlemail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/lantiq/io.h>
#include <asm/lantiq/pm.h>
#include <asm/arch/soc.h>

typedef enum {
	LTQ_PMU_BIT_DFEV0	= 2,
	LTQ_PMU_BIT_DFEV1	= 3,
	LTQ_PMU_BIT_DMA		= 5,
	LTQ_PMU_BIT_USB0_CTRL	= 6,
	LTQ_PMU_BIT_USIF	= 7,
	LTQ_PMU_BIT_SPI		= 8,
	LTQ_PMU_BIT_DSL_DFE	= 9,
	LTQ_PMU_BIT_EBU		= 10,
	LTQ_PMU_BIT_LEDC	= 11,
	LTQ_PMU_BIT_GPTC	= 12,
	LTQ_PMU_BIT_UART1	= 17,
	LTQ_PMU_BIT_DEU		= 20,
	LTQ_PMU_BIT_PPE_TC	= 21,
	LTQ_PMU_BIT_PPE_EMA	= 22,
	LTQ_PMU_BIT_PPE_DPLUS	= 24,
	LTQ_PMU_BIT_TDM		= 25,
	LTQ_PMU_BIT_USB1_CTRL	= 27,
	LTQ_PMU_BIT_SWITCH	= 28,
	LTQ_PMU_BIT_PCIE0_CTRL	= 33,
	LTQ_PMU_BIT_PDI0	= 36,
	LTQ_PMU_BIT_MSI0	= 37,
	LTQ_PMU_BIT_DDR_CKE	= 38,
	LTQ_PMU_BIT_PCIE1_CTRL	= 49,
	LTQ_PMU_BIT_PDI1	= 52,
	LTQ_PMU_BIT_MSI1	= 53,
	LTQ_PMU_BIT_PCIE2_CTRL	= 57, /* GRX390 only */
	LTQ_PMU_BIT_PCIE2_PDI2	= 57, /* GRX390 only */
	LTQ_PMU_BIT_MSI2	= 59, /* GRX390 only */
	LTQ_PMU_BIT_USB0_PHY	= 64,
	LTQ_PMU_BIT_USB1_PHY	= 64,
	LTQ_PMU_BIT_PCIE1_PHY	= 73,
	LTQ_PMU_BIT_PCIE2_PHY	= 74, /* GRX390 only */
	LTQ_PMU_BIT_ASDL_AFE	= 80,
	LTQ_PMU_BIT_DCDC_2V5	= 81, /* always on */
	LTQ_PMU_BIT_DCDC_1VX	= 82, /* always on */
	LTQ_PMU_BIT_DCDC_1V0	= 83, /* always on */
	LTQ_PMU_BIT_PCIE0_PHY	= 72,
} pmu_bit;

struct ltq_pmu_regs {
	u32	rsvd0[8];
	u32	sr0;		/* module 0 status register (IFX_PMU_CLKGSR1) */
	u32	pwdcr0;		/* module 0 power up control (IFX_PMU_CLKGCR1_A) */
	u32	pwdcrb0;	/* module 0 power down control (IFX_PMU_CLKGCR1_B) */
	u32	sr1;		/* module 1 status register (IFX_PMU_CLKGSR2) */
	u32	pwdcr1;		/* module 1 power up control (IFX_PMU_CLKGCR2_A) */
	u32	pwdcrb1;	/* module 1 power down control (IFX_PMU_CLKGCR2_B) */
	u32	sr2;		/* analog module status register (IFX_PMU_ANALOG_SR) */
	u32	pwdcr2;		/* analog module power up control (IFX_PMU_ANALOGCR_A) */
	u32	pwdcrb2;	/* analog module power down control (IFX_PMU_ANALOGCR_B) */
};

static struct ltq_pmu_regs *ltq_pmu_regs =
	(struct ltq_pmu_regs *) CKSEG1ADDR(LTQ_PMU_BASE);

int ltq_pm_enable_function_bit(pmu_bit bit)
{
	u32 *pwdcr;
	u32 *sr;
	u32 module;
	u32 val;
	const unsigned long timeout = 1000;
	unsigned long timebase;

	module = bit / 32;
	val = 1 << (bit % 32);

	switch (module) {
	case 0:
		pwdcr = &ltq_pmu_regs->pwdcr0;
		sr = &ltq_pmu_regs->sr0;
		break;
	case 1:
		pwdcr = &ltq_pmu_regs->pwdcr1;
		sr = &ltq_pmu_regs->sr1;
		break;
	case 2:
		pwdcr = &ltq_pmu_regs->pwdcr2;
		sr = &ltq_pmu_regs->sr2;
		break;
	default:
		return 1;
	}

	/* enable the function */
	ltq_writel(pwdcr, val);

	timebase = get_timer(0);

	do {
		if (ltq_readl(sr) & val)
			return 0;
	} while (get_timer(timebase) < timeout);

	return 1;
}

int ltq_pm_disable_function_bit(pmu_bit bit)
{
	u32 *pwdcrb;
	u32 *sr;
	u32 module;
	u32 val;
	const unsigned long timeout = 1000;
	unsigned long timebase;

	module = bit / 32;
	val = 1 << (bit % 32);

	switch (module) {
	case 0:
		pwdcrb = &ltq_pmu_regs->pwdcrb0;
		sr = &ltq_pmu_regs->sr0;
		break;
	case 1:
		pwdcrb = &ltq_pmu_regs->pwdcrb1;
		sr = &ltq_pmu_regs->sr1;
		break;
	case 2:
		pwdcrb = &ltq_pmu_regs->pwdcrb2;
		sr = &ltq_pmu_regs->sr2;
		break;
	default:
		return 1;
	}

	/* disable the function */
	ltq_writel(pwdcrb, val);

	timebase = get_timer(0);

	do {
		if ((ltq_readl(sr) & val) == 0)
			return 0;
	} while (get_timer(timebase) < timeout);

	return 1;
}

int ltq_pm_apply(enum ltq_pm_modules module, int (*apply_pmu_bit_func)(pmu_bit))
{
	int ret;

	switch (module) {
	case LTQ_PM_CORE:
		ret = 0;
		ret |= apply_pmu_bit_func(LTQ_PMU_BIT_UART1);
		ret |= apply_pmu_bit_func(LTQ_PMU_BIT_LEDC);
		ret |= apply_pmu_bit_func(LTQ_PMU_BIT_EBU);
		break;
	case LTQ_PM_DMA:
		ret = apply_pmu_bit_func(LTQ_PMU_BIT_DMA);
		break;
	case LTQ_PM_ETH:
		ret = 0;
		ret |= apply_pmu_bit_func(LTQ_PMU_BIT_SWITCH);
		ret |= apply_pmu_bit_func(LTQ_PMU_BIT_PPE_TC);
		ret |= apply_pmu_bit_func(LTQ_PMU_BIT_PPE_EMA);
		ret |= apply_pmu_bit_func(LTQ_PMU_BIT_PPE_DPLUS);
		break;
	case LTQ_PM_SPI:
		ret = apply_pmu_bit_func(LTQ_PMU_BIT_SPI);
		break;
	default:
		ret = 1;
		break;
	}

	return ret;
}

int ltq_pm_enable(enum ltq_pm_modules module)
{
	return ltq_pm_apply(module, ltq_pm_enable_function_bit);
}

int ltq_pm_disable(enum ltq_pm_modules module)
{
	return ltq_pm_apply(module, ltq_pm_disable_function_bit);
}

void ltq_pmu_init(void)
{
	ltq_pm_apply(LTQ_PM_CORE, ltq_pm_enable_function_bit);
}
