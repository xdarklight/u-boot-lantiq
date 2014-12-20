/*
 * Copyright (C) 2014 Daniel Schwierzeck, daniel.schwierzeck@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ARCH_LANTIQ_IO_H
#define __ARCH_LANTIQ_IO_H

static inline void __iomem *lantiq_ioremap(phys_addr_t offset, size_t size)
{
	return (void __iomem *) CKSEG1ADDR(offset);
}

static inline u8 lantiq_readb(const volatile void __iomem *mem)
{
	return __raw_readb(mem);
}

static inline u32 lantiq_readl(const volatile void __iomem *mem)
{
	return __raw_readl(mem);
}

static inline void lantiq_writeb(u8 val, volatile void __iomem *mem)
{
	__raw_writeb(val, mem);
}

static inline void lantiq_writel(u32 val, volatile void __iomem *mem)
{
	__raw_writel(val, mem);
}

static inline void lantiq_clrbits(u32 clr, volatile void __iomem *mem)
{
	u32 val = __raw_readl(mem);
	val &= ~clr;
	__raw_writel(val, mem);
}

static inline void lantiq_setbits(u32 set, volatile void __iomem *mem)
{
	u32 val = __raw_readl(mem);
	val |= set;
	__raw_writel(val, mem);
}

#endif /* __ARCH_LANTIQ_IO_H */
