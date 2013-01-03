/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <common.h>
#include <command.h>
#include <image.h>
#include <u-boot/zlib.h>
#include <asm/byteorder.h>
#include <asm/addrspace.h>
#include <fdt.h>
#include <libfdt.h>
#include <fdt_support.h>

DECLARE_GLOBAL_DATA_PTR;

#define	LINUX_MAX_ENVS		256
#define	LINUX_MAX_ARGS		256

static int linux_argc;
static char **linux_argv;

static char **linux_env;
static char *linux_env_p;
static int linux_env_idx;

static void linux_params_init(ulong start, char *commandline);
static void linux_env_set(char *env_name, char *env_val);

static ulong arch_get_sp(void)
{
	ulong ret;

	asm("move %0, $sp" : "=r"(ret) : );
	return ret;
}

void arch_lmb_reserve(struct lmb *lmb)
{
	ulong sp;

	/*
	 * Booting a (Linux) kernel image
	 *
	 * Allocate space for command line and board info - the
	 * address should be as high as possible within the reach of
	 * the kernel (see CONFIG_SYS_BOOTMAPSZ settings), but in unused
	 * memory, which means far enough below the current stack
	 * pointer.
	 */
	sp = arch_get_sp();
	debug("## Current stack ends at 0x%08lx\n", sp);

	/* adjust sp by 4K to be safe */
	sp -= 4096;
	lmb_reserve(lmb, sp, CONFIG_SYS_SDRAM_BASE + gd->ram_size - sp);
}

static void boot_prep_linux_legacy(bootm_headers_t *images)
{
	char *commandline = getenv("bootargs");
	char env_buf[12];
	char *cp;

	linux_params_init(UNCACHED_SDRAM(gd->bd->bi_boot_params), commandline);

#ifdef CONFIG_MEMSIZE_IN_BYTES
	sprintf(env_buf, "%lu", (ulong)gd->ram_size);
	debug("## Giving linux memsize in bytes, %lu\n", (ulong)gd->ram_size);
#else
	sprintf(env_buf, "%lu", (ulong)(gd->ram_size >> 20));
	debug("## Giving linux memsize in MB, %lu\n",
		(ulong)(gd->ram_size >> 20));
#endif /* CONFIG_MEMSIZE_IN_BYTES */

	linux_env_set("memsize", env_buf);

	sprintf(env_buf, "0x%08X", (uint) UNCACHED_SDRAM(images->rd_start));
	linux_env_set("initrd_start", env_buf);

	sprintf(env_buf, "0x%X", (uint) (images->rd_end - images->rd_start));
	linux_env_set("initrd_size", env_buf);

	sprintf(env_buf, "0x%08X", (uint) (gd->bd->bi_flashstart));
	linux_env_set("flash_start", env_buf);

	sprintf(env_buf, "0x%X", (uint) (gd->bd->bi_flashsize));
	linux_env_set("flash_size", env_buf);

	cp = getenv("ethaddr");
	if (cp)
		linux_env_set("ethaddr", cp);

	cp = getenv("eth1addr");
	if (cp)
		linux_env_set("eth1addr", cp);
}

#ifdef CONFIG_OF_LIBFDT
static ulong boot_get_ft_len(bootm_headers_t *images)
{
	return images->ft_len;
}

static ulong boot_get_ft_addr(bootm_headers_t *images)
{
	return (ulong)images->ft_addr;
}

static int create_fdt(bootm_headers_t *images)
{
	ulong of_size = images->ft_len;
	char **of_flat_tree = &images->ft_addr;
	ulong *initrd_start = &images->initrd_start;
	ulong *initrd_end = &images->initrd_end;
	struct lmb *lmb = &images->lmb;
	int ret;

	boot_fdt_add_mem_rsv_regions(lmb, *of_flat_tree);

	ret = boot_relocate_fdt(lmb, of_flat_tree, &of_size);
	if (ret)
		return ret;

	fdt_chosen(*of_flat_tree, 1);
	fdt_fixup_memory(*of_flat_tree, 0, gd->ram_size);
	fdt_fixup_ethernet(*of_flat_tree);
	fdt_initrd(*of_flat_tree, *initrd_start, *initrd_end, 1);

#ifdef CONFIG_OF_BOARD_SETUP
	ft_board_setup(*of_flat_tree, gd->bd);
#endif

	return 0;
}

static int boot_prep_linux_fdt(bootm_headers_t *images)
{
	if (!images->ft_len)
		return -1;

	debug("using: FDT\n");
	if (create_fdt(images)) {
		printf("FDT creation failed! hanging...");
		hang();
	}

	return 0;
}
#else
static inline ulong boot_get_ft_len(bootm_headers_t *images)
{
	return 0;
}

static inline ulong boot_get_ft_addr(bootm_headers_t *images)
{
	return 0;
}

static inline int boot_prep_linux_fdt(bootm_headers_t *images)
{
	return -1;
}
#endif /* CONFIG_OF_LIBFDT */

static void boot_prep_linux(bootm_headers_t *images)
{
	int ret;

	ret = boot_prep_linux_fdt(images);
	if (!ret)
		return;

	boot_prep_linux_legacy(images);
}

static void boot_jump_linux(bootm_headers_t *images)
{
	typedef void __noreturn (*kernel_entry_t)(ulong, ulong, ulong, ulong);
	kernel_entry_t kernel = (kernel_entry_t) images->ep;
	ulong ft_len;

	debug("## Transferring control to Linux (at address %p) ...\n", kernel);

	bootstage_mark(BOOTSTAGE_ID_RUN_OS);

	/* we assume that the kernel is in place */
	printf("\nStarting kernel ...\n\n");

	ft_len = boot_get_ft_len(images);
	if (ft_len)
		kernel(FDT_MAGIC, boot_get_ft_addr(images), 0, 0);
	else
		kernel(linux_argc, (ulong)linux_argv, (ulong)linux_env, 0);
}

int do_bootm_linux(int flag, int argc, char * const argv[],
			bootm_headers_t *images)
{
	/* No need for those on MIPS */
	if (flag & BOOTM_STATE_OS_BD_T || flag & BOOTM_STATE_OS_CMDLINE)
		return -1;

	if (flag & BOOTM_STATE_OS_PREP) {
		boot_prep_linux(images);
		return 0;
	}

	if (flag & BOOTM_STATE_OS_GO) {
		boot_jump_linux(images);
		return 0;
	}

	boot_prep_linux(images);
	boot_jump_linux(images);

	/* does not return */
	return 1;
}

static void linux_params_init(ulong start, char *line)
{
	char *next, *quote, *argp;

	linux_argc = 1;
	linux_argv = (char **) start;
	linux_argv[0] = 0;
	argp = (char *) (linux_argv + LINUX_MAX_ARGS);

	next = line;

	while (line && *line && linux_argc < LINUX_MAX_ARGS) {
		quote = strchr(line, '"');
		next = strchr(line, ' ');

		while (next && quote && quote < next) {
			/* we found a left quote before the next blank
			 * now we have to find the matching right quote
			 */
			next = strchr(quote + 1, '"');
			if (next) {
				quote = strchr(next + 1, '"');
				next = strchr(next + 1, ' ');
			}
		}

		if (!next)
			next = line + strlen(line);

		linux_argv[linux_argc] = argp;
		memcpy(argp, line, next - line);
		argp[next - line] = 0;

		argp += next - line + 1;
		linux_argc++;

		if (*next)
			next++;

		line = next;
	}

	linux_env = (char **) (((ulong) argp + 15) & ~15);
	linux_env[0] = 0;
	linux_env_p = (char *) (linux_env + LINUX_MAX_ENVS);
	linux_env_idx = 0;
}

static void linux_env_set(char *env_name, char *env_val)
{
	if (linux_env_idx < LINUX_MAX_ENVS - 1) {
		linux_env[linux_env_idx] = linux_env_p;

		strcpy(linux_env_p, env_name);
		linux_env_p += strlen(env_name);

		strcpy(linux_env_p, "=");
		linux_env_p += 1;

		strcpy(linux_env_p, env_val);
		linux_env_p += strlen(env_val);

		linux_env_p++;
		linux_env[++linux_env_idx] = 0;
	}
}
