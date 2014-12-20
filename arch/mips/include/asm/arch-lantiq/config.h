/*
 * Copyright (C) 2014 Daniel Schwierzeck, daniel.schwierzeck@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ARCH_LANTIQ_CONFIG_H
#define __ARCH_LANTIQ_CONFIG_H

#define CONFIG_SYS_GENERIC_BOARD
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_BAUDRATE			115200

/* Memory usage */
#define CONFIG_SYS_MAXARGS		24
#define CONFIG_SYS_MALLOC_LEN		(1024 * 1024)
#define CONFIG_SYS_MALLOC_F_LEN		(1 * 1024)
#define CONFIG_SYS_BOOTPARAMS_LEN	(128 * 1024)
#define CONFIG_SYS_CBSIZE		512

#define CONFIG_DM_STDIO

#endif /* __ARCH_LANTIQ_CONFIG_H */
