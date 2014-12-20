/*
 * Copyright (C) 2014 Daniel Schwierzeck, daniel.schwierzeck@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __PLAT_LANTIQ_SERIAL_H
#define __PLAT_LANTIQ_SERIAL_H

struct lantiq_serial_platform_data {
	unsigned long base;
	unsigned long uart_clk;
};

#endif /* __PLAT_LANTIQ_SERIAL_H */
