/*
 * Copyright (C) 2010 Thomas Langer <thomas.langer@lantiq.com>
 * Copyright (C) 2011-2014 Daniel Schwierzeck, daniel.schwierzeck@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <serial.h>
#include <asm/errno.h>
#include <dm/device.h>
#include <dm/platform_data/serial_lantiq.h>
#include <asm/arch/soc.h>
#include <asm/arch-lantiq/io.h>

#define MCON_R			(1 << 15)
#define MCON_FDE		(1 << 9)

#define WHBSTATE_SETREN		(1 << 1)
#define WHBSTATE_CLRREN		(1 << 0)

#define RXFCON_RXFITL_S		8
#define RXFCON_RXFITL_M		(0x3F << RXFCON_RXFITL_S)
#define RXFCON_RXFITL_RXFFLU	(1 << 1)
#define RXFCON_RXFITL_RXFEN	(1 << 0)

#define TXFCON_TXFITL_S		8
#define TXFCON_TXFITL_M		(0x3F << TXFCON_TXFITL_S)
#define TXFCON_TXFITL_TXFFLU	(1 << 1)
#define TXFCON_TXFITL_TXFEN	(1 << 0)

#define FSTAT_TXFREE_S		24
#define FSTAT_TXFREE_M		(0x3F << FSTAT_TXFREE_S)
#define FSTAT_RXFREE_S		16
#define FSTAT_RXFREE_M		(0x3F << FSTAT_RXFREE_S)
#define FSTAT_TXFFL_S		8
#define FSTAT_TXFFL_M		(0x3F << FSTAT_TXFFL_S)
#define FSTAT_RXFFL_M		0x3F

#ifdef __BIG_ENDIAN
#define RBUF_OFFSET		3
#define TBUF_OFFSET		3
#else
#define RBUF_OFFSET		0
#define TBUF_OFFSET		0
#endif

struct lantiq_asc {
	u32	clc;
	u32	pisel;
	u32	id;
	u32	rsvd0;
	u32	mcon;
	u32	state;
	u32	whbstate;
	u32	rsvd1;
	u8	tbuf[4];
	u8	rbuf[4];
	u32	rsvd2[2];
	u32	abcon;
	u32	abstat;
	u32	whbabcon;
	u32	whbabstat;
	u32	rxfcon;
	u32	txfcon;
	u32	fstat;
	u32	rsvd3;
	u32	bg;
	u32	bg_timer;
	u32	fdv;
	u32	pmw;
	u32	modcon;
	u32	modstat;
};

struct lantiq_serial_priv {
	void __iomem	*regbase;
	unsigned long	uart_clk;
};

static unsigned int lantiq_serial_tx_free(struct lantiq_asc __iomem *regs)
{
	u32 fstat = lantiq_readl(&regs->fstat);
	return (fstat & FSTAT_TXFREE_M) >> FSTAT_TXFREE_S;
}

static unsigned int lantiq_serial_tx_fill(struct lantiq_asc __iomem *regs)
{
	u32 fstat = lantiq_readl(&regs->fstat);
	return (fstat & FSTAT_TXFFL_M) >> FSTAT_TXFFL_S;
}

static unsigned int lantiq_serial_rx_fill(struct lantiq_asc __iomem *regs)
{
	u32 fstat = lantiq_readl(&regs->fstat);
	return fstat & FSTAT_RXFFL_M;
}

static int lantiq_serial_calc_br_fdv(const struct lantiq_serial_priv *priv,
					int baudrate, unsigned int *fdv,
					unsigned int *bg)
{
	const u32 c = priv->uart_clk / (16 * 512);
	unsigned long diff1, diff2;
	unsigned long bg_calc, br_calc, i;

	/*
	 *             fdv       asc_clk
	 * Baudrate = ----- * -------------
	 *             512    16 * (bg + 1)
	 */

	diff1 = baudrate;
	for (i = 512; i > 0; i--) {
		/* Calc bg for current fdv value */
		bg_calc = i * c / baudrate;

		/* Impossible baudrate */
		if (!bg_calc)
			return -EINVAL;

		/*
		 * Calc diff to target baudrate dependent on current
		 * bg and fdv values
		 */
		br_calc = i * c / bg_calc;
		if (br_calc > baudrate)
			diff2 = br_calc - baudrate;
		else
			diff2 = baudrate - br_calc;

		/* Perfect values found */
		if (diff2 == 0) {
			*fdv = i;
			*bg = bg_calc - 1;
			return 0;
		}

		if (diff2 < diff1) {
			*fdv = i;
			*bg = bg_calc - 1;
			diff1 = diff2;
		}
	}

	return -EINVAL;
}

static void lantiq_serial_hwinit(const struct lantiq_serial_priv *priv)
{
	struct lantiq_asc __iomem *regs = priv->regbase;

	/* Set clock divider for normal run mode to 1 and enable module */
	lantiq_writel(0x100, &regs->clc);

	/* Reset MCON register */
	lantiq_writel(0, &regs->mcon);

	/* Use Port A as receiver input */
	lantiq_writel(0, &regs->pisel);

	/* Enable and flush RX/TX FIFOs */
	lantiq_setbits(RXFCON_RXFITL_RXFFLU | RXFCON_RXFITL_RXFEN,
		&regs->rxfcon);
	lantiq_setbits(TXFCON_TXFITL_TXFFLU | TXFCON_TXFITL_TXFEN,
		&regs->txfcon);

	/* Disable error flags, enable receiver */
	lantiq_writel(WHBSTATE_SETREN, &regs->whbstate);
}

static int lantiq_serial_setbrg(struct udevice *dev, int baudrate)
{
	struct lantiq_serial_priv *priv = dev_get_priv(dev);
	struct lantiq_asc __iomem *regs = priv->regbase;
	unsigned int bg = 0;
	unsigned int fdv = 511;
	int err;

	err = lantiq_serial_calc_br_fdv(priv, baudrate, &fdv, &bg);
	if (err)
		return err;

	/* Disable baudrate generator */
	lantiq_clrbits(MCON_R, &regs->mcon);

	/* Enable fractional divider */
	lantiq_setbits(MCON_FDE, &regs->mcon);

	/* Set fdv and bg values */
	lantiq_writel(fdv, &regs->fdv);
	lantiq_writel(bg, &regs->bg);

	/* Enable baudrate generator */
	lantiq_setbits(MCON_R, &regs->mcon);

	return 0;
}

static int lantiq_serial_getc(struct udevice *dev)
{
	struct lantiq_serial_priv *priv = dev_get_priv(dev);
	struct lantiq_asc __iomem *regs = priv->regbase;

	if (!lantiq_serial_rx_fill(regs))
		return -EAGAIN;

	return lantiq_readb(&regs->rbuf[RBUF_OFFSET]);
}

static int lantiq_serial_putc(struct udevice *dev, const char c)
{
	struct lantiq_serial_priv *priv = dev_get_priv(dev);
	struct lantiq_asc __iomem *regs = priv->regbase;

	if (!lantiq_serial_tx_free(regs))
		return -EAGAIN;

	lantiq_writeb(c, &regs->tbuf[TBUF_OFFSET]);
	return 0;
}

static int lantiq_serial_pending(struct udevice *dev, bool input)
{
	struct lantiq_serial_priv *priv = dev_get_priv(dev);
	struct lantiq_asc __iomem *regs = priv->regbase;

	if (input)
		return lantiq_serial_tx_fill(regs);
	else
		return lantiq_serial_rx_fill(regs);
}

static int lantiq_serial_probe(struct udevice *dev)
{
	struct lantiq_serial_priv *priv = dev_get_priv(dev);
	struct lantiq_serial_platform_data *plat = dev_get_platdata(dev);

	priv->regbase = lantiq_ioremap(plat->base, sizeof(struct lantiq_asc));
	priv->uart_clk = plat->uart_clk;

	lantiq_serial_hwinit(priv);

	return 0;
}

static int lantiq_serial_remove(struct udevice *dev)
{
	return 0;
}

static const struct dm_serial_ops lantiq_serial_ops = {
	.setbrg = lantiq_serial_setbrg,
	.getc = lantiq_serial_getc,
	.putc = lantiq_serial_putc,
	.pending = lantiq_serial_pending,
};

U_BOOT_DRIVER(lantiq_serial) = {
	.name = "lantiq-uart",
	.id = UCLASS_SERIAL,
	.probe = lantiq_serial_probe,
	.remove = lantiq_serial_remove,
	.priv_auto_alloc_size = sizeof(struct lantiq_serial_priv),
	.ops = &lantiq_serial_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
