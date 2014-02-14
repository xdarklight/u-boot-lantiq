/*
 * Copyright (C) 2012-2013 Daniel Schwierzeck, daniel.schwierzeck@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <nand.h>
#include <linux/compiler.h>
#include <asm/arch/soc.h>
#include <asm/arch/nand.h>
#include <asm/lantiq/io.h>

#define NAND_CON_ECC_ON		(1 << 31)
#define NAND_CON_LATCH_PRE	(1 << 23)
#define NAND_CON_LATCH_WP	(1 << 22)
#define NAND_CON_LATCH_SE	(1 << 21)
#define NAND_CON_LATCH_CS	(1 << 20)
#define NAND_CON_LATCH_CLE	(1 << 19)
#define NAND_CON_LATCH_ALE	(1 << 18)
#define NAND_CON_OUT_CS1	(1 << 10)
#define NAND_CON_IN_CS1		(1 << 8)
#define NAND_CON_PRE_P		(1 << 7)
#define NAND_CON_WP_P		(1 << 6)
#define NAND_CON_SE_P		(1 << 5)
#define NAND_CON_CS_P		(1 << 4)
#define NAND_CON_CLE_P		(1 << 3)
#define NAND_CON_ALE_P		(1 << 2)
#define NAND_CON_CSMUX		(1 << 1)
#define NAND_CON_NANDM		(1 << 0)
#define NAND_CON_LATCH_SHIFT	18
#define NAND_CON_LATCH_ALL	(NAND_CON_LATCH_PRE | NAND_CON_LATCH_WP | \
				NAND_CON_LATCH_SE | NAND_CON_LATCH_CS | \
				NAND_CON_LATCH_ALE << NAND_CON_LATCH_SHIFT)

#define NAND_WAIT_WR_C		(1 << 3)
#define NAND_WAIT_RD_E		(1 << 2)
#define NAND_WAIT_BY_E		(1 << 1)
#define NAND_WAIT_RDBY		(1 << 0)
#define NAND_WAIT_READY		(NAND_WAIT_RD_E | NAND_WAIT_BY_E | NAND_WAIT_RDBY)

#define NAND_CMD_ALE		(1 << 2)
#define NAND_CMD_CLE		(1 << 3)
#define NAND_CMD_CS		(1 << 4)
#define NAND_CMD_SE		(1 << 5)
#define NAND_CMD_WP		(1 << 6)
#define NAND_CMD_PRE		(1 << 7)

struct ltq_nand_regs {
	__be32	con;		/* NAND controller control */
	__be32	wait;		/* NAND Flash Device RD/BY State */
	__be32	ecc0;		/* NAND Flash ECC Register 0 */
	__be32	ecc_ac;		/* NAND Flash ECC Register address counter */
	__be32	ecc_cr;		/* NAND Flash ECC Comparison */
};

static struct ltq_nand_regs *ltq_nand_regs =
	(struct ltq_nand_regs *) CKSEG1ADDR(LTQ_EBU_NAND_BASE);

static void ltq_nand_wait_ready(void)
{
	while ((ltq_readl(&ltq_nand_regs->wait) & NAND_WAIT_WR_C) == 0)
		;
}

static int ltq_nand_dev_ready(struct mtd_info *mtd)
{
	u32 data = ltq_readl(&ltq_nand_regs->wait);
	return data & NAND_WAIT_RDBY;
}

static void ltq_nand_select_chip(struct mtd_info *mtd, int chip)
{
	if (chip == 0)
		ltq_setbits(&ltq_nand_regs->con,
			NAND_CON_LATCH_ALL | NAND_CON_NANDM);
	else
		ltq_clrbits(&ltq_nand_regs->con,
			NAND_CON_LATCH_ALL | NAND_CON_NANDM);
}

static void ltq_nand_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *chip = mtd->priv;
	unsigned long addr = (unsigned long) chip->IO_ADDR_W;

	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_ALE) {
			addr |= NAND_CMD_ALE;
			ltq_setbits(&ltq_nand_regs->con, NAND_CON_LATCH_ALE);
		} else {
			addr &= ~NAND_CMD_ALE;
			ltq_clrbits(&ltq_nand_regs->con, NAND_CON_LATCH_ALE);
		}

		if (ctrl & NAND_CLE) {
			addr |= NAND_CMD_CLE;
			ltq_setbits(&ltq_nand_regs->con, NAND_CON_LATCH_CLE);
		} else {
			addr &= ~NAND_CMD_CLE;
			ltq_clrbits(&ltq_nand_regs->con, NAND_CON_LATCH_CLE);
		}

		chip->IO_ADDR_W = (void __iomem *) addr;
	}

	if (cmd != NAND_CMD_NONE) {
		writeb(cmd, chip->IO_ADDR_W);
		ltq_nand_wait_ready();
	}
}

static void ltq_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	for (i = 0; i < len; i++)
		buf[i] = readb(chip->IO_ADDR_R);
}

int ltq_nand_init(struct nand_chip *nand)
{
	/* Enable NAND, set NAND CS to EBU CS1, enable EBU CS mux */
	ltq_writel(&ltq_nand_regs->con, NAND_CON_OUT_CS1 | NAND_CON_IN_CS1 |
		NAND_CON_PRE_P | NAND_CON_WP_P | NAND_CON_SE_P |
		NAND_CON_CS_P | NAND_CON_CSMUX);

	nand->dev_ready = ltq_nand_dev_ready;
	nand->select_chip = ltq_nand_select_chip;
	nand->cmd_ctrl = ltq_nand_cmd_ctrl;
	nand->read_buf = ltq_nand_read_buf;

	nand->chip_delay = 30;
	nand->options = NAND_NO_SUBPAGE_WRITE;
	nand->ecc.mode = NAND_ECC_SOFT;

	/* Enable CS bit in address offset */
	nand->IO_ADDR_R = nand->IO_ADDR_R + NAND_CMD_CS;
	nand->IO_ADDR_W = nand->IO_ADDR_W + NAND_CMD_CS;

	return 0;
}

__weak int board_nand_init(struct nand_chip *chip)
{
	return ltq_nand_init(chip);
}

void ltq_nand_spl_read_page(struct mtd_info *mtd, unsigned int page,
				void *dst)
{
	ltq_setbits(&ltq_nand_regs->con, NAND_CON_LATCH_ALL);

	/* Command latch cycle */
	ltq_nand_cmd_ctrl(mtd, NAND_CMD_READ0, NAND_CLE | NAND_CTRL_CHANGE);

	/* Column address */
	ltq_nand_cmd_ctrl(mtd, 0, NAND_CTRL_ALE | NAND_CTRL_CHANGE);
	ltq_nand_cmd_ctrl(mtd, 0, NAND_CTRL_ALE);

	/* Row address */
	ltq_nand_cmd_ctrl(mtd, (page & 0xff), NAND_CTRL_ALE); /* A[19:12] */
	ltq_nand_cmd_ctrl(mtd, ((page >> 8) & 0xff), NAND_CTRL_ALE); /* A[27:20] */
#ifdef CONFIG_SYS_NAND_5_ADDR_CYCLE
	ltq_nand_cmd_ctrl(mtd, ((page >> 16) & 0x0f), NAND_CTRL_ALE); /* A[31:28] */
#endif

	/* Latch in address */
	ltq_nand_cmd_ctrl(mtd, NAND_CMD_READSTART, NAND_CTRL_CLE | NAND_CTRL_CHANGE);
	ltq_nand_cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);

	while ((ltq_readl(&ltq_nand_regs->wait) & NAND_WAIT_READY) != NAND_WAIT_READY)
		;

	ltq_nand_read_buf(mtd, dst, CONFIG_SYS_NAND_PAGE_SIZE);

	ltq_clrbits(&ltq_nand_regs->con, NAND_CON_LATCH_ALL);

	while ((ltq_readl(&ltq_nand_regs->wait) & NAND_WAIT_READY) != NAND_WAIT_READY)
		;
}

void ltq_nand_spl_load(void *dst)
{
	nand_info_t mtd;
	struct nand_chip nand_chip;
	unsigned int page, last_page;

	mtd.priv = &nand_chip;
	nand_chip.IO_ADDR_R = (void  __iomem *)(CONFIG_SYS_NAND_BASE + NAND_CMD_CS);
	nand_chip.IO_ADDR_W = (void  __iomem *)(CONFIG_SYS_NAND_BASE + NAND_CMD_CS);

	/* Enable NAND, set NAND CS to EBU CS1, enable EBU CS mux */
	ltq_writel(&ltq_nand_regs->con, NAND_CON_OUT_CS1 | NAND_CON_IN_CS1 |
		NAND_CON_PRE_P | NAND_CON_WP_P | NAND_CON_SE_P |
		NAND_CON_CS_P | NAND_CON_CSMUX | NAND_CON_NANDM);

	last_page = CONFIG_SPL_MAX_SIZE / CONFIG_SYS_NAND_PAGE_SIZE;
	for (page = 1; page < last_page; page++) {
		ltq_nand_spl_read_page(&mtd, page, dst);
		dst += CONFIG_SYS_NAND_PAGE_SIZE;
	}
}
