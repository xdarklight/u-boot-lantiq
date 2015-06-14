/*
 * Copyright (C) 2007-2010 Lantiq Deutschland GmbH
 * Copyright (C) 2012-2013 Daniel Schwierzeck, daniel.schwierzeck@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/addrspace.h>
#include <asm/arch/soc.h>

/* CGU module register */
#define CGU_PLL0_CFG			0x0004	/* PLL0 config */
#define CGU_PLL1_CFG			0x0008	/* PLL1 config */
#define CGU_SYS				0x0010	/* System clock */
#define CGU_UPDATE			0x0014	/* Clock update control */

#define CGU_UPDATE_UPD			0x1


#define LTQ_CLK_CPU_600_DDR_300	0
#define LTQ_CLK_CPU_500_DDR_250	1
#define LTQ_CLK_CPU_300_DDR_300	2
#define LTQ_CLK_CPU_300_DDR_150	3
#define LTQ_CLK_CPU_250_DDR_250	4
#define LTQ_CLK_CPU_250_DDR_125	5
#define LTQ_CLK_CPU_150_DDR_150	6
#define LTQ_CLK_CPU_125_DDR_125	7


#if defined(LTQ_CLK_CPU_500_DDR_250)
#define LTQ_CGU_SYS_VALUE	0x01
#else
#error "Invalid system clock configuration!"
#endif

/* 400M clock */
#define LTQ_CGU_CLK_FSR_VALUE	0x10040000

	.set noreorder

LEAF(ltq_cgu_init)
	/* Load current CGU register values */
	li	t0, (LTQ_CGU_BASE | KSEG1)
	lw	t1, LTQ_CGU_SYS(t0)
	lw	t2, LTQ_CGU_CLK_FSR(t0)

	/* Load target CGU register values */
	li	t3, LTQ_CGU_SYS_VALUE
	li	t4, LTQ_CGU_CLK_FSR_VALUE

	/* Only update registers if values differ */
	bne	t1, t3, update
	 nop
	beq	t2, t4, finished
	 nop

update:
	/* Store target register values */
	sw	t3, LTQ_CGU_SYS(t0)
	sw	t4, LTQ_CGU_CLK_FSR(t0)

	/* Perform software reset to activate new clock config */
	li	t1, 1
	sw	t1, LTQ_CGU_UPDATE(t0)

finished:
	jr	ra
	 nop

	END(ltq_cgu_init)
