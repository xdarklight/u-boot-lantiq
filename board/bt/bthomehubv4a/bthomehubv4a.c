/*
 * Copyright (C) 2015 Martin Blumenstingl <martin.blumenstingl@googlemail.com>
 * Based on p2812hnufx.c: (C) 2013 Luka Perkov <luka@openwrt.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/gpio.h>
#include <asm/lantiq/eth.h>
#include <asm/lantiq/chipid.h>
#include <asm/lantiq/cpu.h>
#include <asm/lantiq/mem.h>
#include <asm/arch/gphy.h>

#if defined(CONFIG_SPL_BUILD)
#define do_gpio_init	1
#elif defined(CONFIG_SYS_BOOT_RAM)
#define do_gpio_init	1
#else
#define do_gpio_init	0
#endif

#define GPIO_POWER_GREEN	6

static void gpio_init(void)
{
	/* Configure GPIO3 as 25Mhz clock output for the external PHYs */
	gpio_set_altfunc(3, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);

	/* EBU.FL_CS1 as output for NAND CE */
	gpio_set_altfunc(23, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	/* EBU.FL_A23 as output for NAND CLE */
	gpio_set_altfunc(24, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	/* EBU.FL_A24 as output for NAND ALE */
	gpio_set_altfunc(13, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	/* GPIO 3.0 as input for NAND Ready Busy */
	gpio_set_altfunc(48, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_IN);
	/* GPIO 3.1..12 as output for NAND Read */
	gpio_set_altfunc(49, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	gpio_set_altfunc(50, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	gpio_set_altfunc(51, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	gpio_set_altfunc(52, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	gpio_set_altfunc(53, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	gpio_set_altfunc(54, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	gpio_set_altfunc(55, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	gpio_set_altfunc(56, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	gpio_set_altfunc(57, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	gpio_set_altfunc(58, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	gpio_set_altfunc(59, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);
	gpio_set_altfunc(60, GPIO_ALTSEL_SET, GPIO_ALTSEL_CLR, GPIO_DIR_OUT);

	/* Turn on the green power LED */
	gpio_direction_output(GPIO_POWER_GREEN, 0);
	gpio_set_value(GPIO_POWER_GREEN, 0);
}

int board_early_init_f(void)
{
	if (do_gpio_init)
		gpio_init();

	return 0;
}

int checkboard(void)
{
	puts("Board: " CONFIG_BOARD_NAME "\n");
	ltq_chip_print_info();

	return 0;
}

int misc_init_r(void)
{
#if 0
	return mc_tune_store_flash();
#endif
}

static const struct ltq_eth_port_config eth_port_config[] = {
	/* GMAC0: external Lantiq PEF7071 10/100/1000 PHY for WANoE port */
	{ 0, 0x0, LTQ_ETH_PORT_PHY, PHY_INTERFACE_MODE_RGMII },
	/* GMAC1: internal 10/100 GPHY2 for LAN port 1 */
	{ 1, 0x1, LTQ_ETH_PORT_PHY, PHY_INTERFACE_MODE_RGMII },
	/* GMAC2: internal 10/100/1000 GPHY0 for LAN port 4 */
	{ 2, 0x2, LTQ_ETH_PORT_PHY, PHY_INTERFACE_MODE_GMII },
	/* GMAC3: unused */
	{ 3, 0x0, LTQ_ETH_PORT_NONE, PHY_INTERFACE_MODE_NONE },
	/* GMAC4: internal 10/100 GPHY1 for LAN port 2 */
	{ 4, 0x4, LTQ_ETH_PORT_PHY, PHY_INTERFACE_MODE_GMII },
	/* GMAC5: internal 10/100 GPHY1 for LAN port 3 */
	{ 5, 0x5, LTQ_ETH_PORT_PHY, PHY_INTERFACE_MODE_MII },
	/* GMAC6: unused */
	{ 6, 0x0, LTQ_ETH_PORT_NONE, PHY_INTERFACE_MODE_NONE },
};

static const struct ltq_eth_board_config eth_board_config = {
	.ports = eth_port_config,
	.num_ports = ARRAY_SIZE(eth_port_config),
};

int board_eth_init(bd_t * bis)
{
	const enum ltq_gphy_clk clk = LTQ_GPHY_CLK_25MHZ_GPIO3;
	const ulong fw_addr_phy11g = 0x80FE0000;
	const ulong fw_addr_phy22f = 0x80FF0000;

	ltq_gphy_phy11g_a2x_load(fw_addr_phy11g);
	ltq_gphy_phy22f_a2x_load(fw_addr_phy22f);

	ltq_cgu_gphy_clk_src(clk);

	ltq_rcu_gphy_boot(0, fw_addr_phy11g);
	ltq_rcu_gphy_boot(1, fw_addr_phy22f);
	ltq_rcu_gphy_boot(2, fw_addr_phy22f);

	return ltq_eth_initialize(&eth_board_config);
}
