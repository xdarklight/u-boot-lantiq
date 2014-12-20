/*
 * Copyright (C) 2014 Daniel Schwierzeck, daniel.schwierzeck@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <dm/platdata.h>
#include <dm/platform_data/serial_lantiq.h>

static const struct lantiq_serial_platform_data uart1_dev = {
	.base = 0x1E100C00,
	.uart_clk = 250000000,
};

U_BOOT_DEVICE(serial1) = {
	.name = "lantiq-uart",
	.platdata = &uart1_dev,
};
