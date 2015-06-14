/*
 * Copyright (C) 2007-2010 Lantiq Deutschland GmbH
 * Copyright (C) 2012 Daniel Schwierzeck, daniel.schwierzeck@gmail.com
 * Copyright (C) 2015 Martin Blumenstingl <martin.blumenstingl@googlemail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ARX300_SOC_H__
#define __ARX300_SOC_H__

#define LTQ_ASC0_BASE			0x1E100400
#define LTQ_SPI_BASE			0x1E100800 // TODO: unknown - assumed ???
#define LTQ_GPIO_BASE			0x1E100B00
#define LTQ_SSIO_BASE			0x1E100B00 // TODO: 0x1E100BB0 - typo in UGW u-boot???
#define LTQ_ASC1_BASE			0x1E100C00
#define LTQ_DMA_BASE			0x1E104100

#define LTQ_EBU_BASE			0x16000000
#define LTQ_EBU_REGION0_BASE		0x10000000
#define LTQ_EBU_REGION1_BASE		0x14000000
#define LTQ_EBU_NAND_BASE		(LTQ_EBU_BASE + 0xB0)

#define LTQ_PPE_BASE			0x1E180000
#define LTQ_SWITCH_BASE			0x1E108000

#define LTQ_BOOTROM_BASE		0x1F000000 // TODO: unknown - assumed ???
#define LTQ_PMU_BASE			0x1F102000
#define LTQ_DCDC_BASE			0x1F106A00 // TODO: does this really exist on xrx300?
#define LTQ_CGU_BASE			0x1F103000
#define LTQ_MPS_BASE			0x1F107000
#define LTQ_CHIPID_BASE			(LTQ_MPS_BASE + 0x340)
#define LTQ_RCU_BASE			0x1F203000

#define LTQ_MC_GLOBAL_BASE		0xBF800000
#define LTQ_MC_DDR_BASE			0xBF801000
#define LTQ_MC_DDR_CCR_OFFSET(x)	(x * 0x10)

#endif /* __ARX300_SOC_H__ */
