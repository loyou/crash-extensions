/* zdm.c - dump zfs dbgmsgs extension for crash
 *
 * Author: loyou85@gmail.com
 *
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
 */

#include "defs.h"      /* From the crash source top-level directory */

void zdm_init(void);
void zdm_fini(void);

void cmd_zdm(void);

char *help_zdm[] = {
        "zdm",
        "dump zfs dbgmsgs for zfsonlinux",
	"",

	"run without any args.",
	"",
	"author: loyou85@gmail.com",
        NULL
};

static struct command_table_entry command_table[] = {
        { "zdm", cmd_zdm, help_zdm, 0},
        { NULL },
};


void __attribute__((constructor)) zdm_init(void)
{
        register_extension(command_table);
}

void __attribute__((destructor)) zdm_fini(void) { }

static unsigned long get_next_from_list(unsigned long addr)
{
	unsigned long ret;
	readmem(addr + MEMBER_OFFSET("struct list_head", "next"), KVADDR, &ret, sizeof(void *), "get_next_from_list:list_head.next", FAULT_ON_ERROR);
	return ret;
}

#define list_for_each_zdm(next, head) \
	for (next = get_next_from_list(head); next != head; next = get_next_from_list(next))

static void dump_zdm(unsigned long zdm)
{
	char *zdm_msg = NULL;
	int zdm_size;

	readmem(zdm + MEMBER_OFFSET("struct zfs_dbgmsg", "zdm_size"), KVADDR, &zdm_size, sizeof(int), "zfs_dbgmsg->zdm_size", FAULT_ON_ERROR);
	zdm_msg = malloc(sizeof(char) * zdm_size);
	if (zdm_msg == NULL) {
		fprintf(stderr, "oom");
		return;
	}
	readmem(zdm + MEMBER_OFFSET("struct zfs_dbgmsg", "zdm_msg"), KVADDR, zdm_msg, zdm_size, "zfs_dbgmsg->zdm_msg", FAULT_ON_ERROR);

	// need timestamp ???
	fprintf(fp, "%s\n", zdm_msg);
	free(zdm_msg);
}

static void dump_zdms()
{
	unsigned long zfs_dbgmsgs;
	int zfs_dbgmsg_enable;
	unsigned long zdm_head, zdm_next;
	unsigned long offset;

	if (symbol_exists("zfs_dbgmsg_enable")) {
		readmem(symbol_value("zfs_dbgmsg_enable"), KVADDR, &zfs_dbgmsg_enable, sizeof(int), "zfs_dbgmsg_enable", FAULT_ON_ERROR);
		if (!zfs_dbgmsg_enable) {
			fprintf(fp, "zfs_dbgmsg_enable is disabled, cannot get zfs_dbgmsgs\n");
			return;
		}
	}
	if (!symbol_exists("zfs_dbgmsgs")) {
		fprintf(fp, "cannot find symbol zfs_dbgmsgs");
		return;
	}
	zfs_dbgmsgs = symbol_value("zfs_dbgmsgs");
	zdm_head = zfs_dbgmsgs + MEMBER_OFFSET("struct list", "list_head");

	//fprintf(fp, "timestamp\tdbgmsgs\n");
	list_for_each_zdm(zdm_next, zdm_head) {
		offset = MEMBER_OFFSET("struct zfs_dbgmsg", "zdm_node");
		dump_zdm(zdm_next - offset);
	}
}

void cmd_zdm(void)
{
        int c;

	if (!is_module_name("zfs", NULL, NULL)) {
		fprintf(fp, "zfs module is not loaded\n");
		argerrs++;
		goto out;
	}
	if (argcnt != 1) {
		argerrs++;
		goto out;
	}

	dump_zdms();

out:
        if (argerrs)
                cmd_usage(pc->curcmd, SYNOPSIS);

        fprintf(fp, "\n");
}
