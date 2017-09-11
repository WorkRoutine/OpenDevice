/*
 * klist demo code.
 *
 * (C) 2017.09.06 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>

/* header of klist */
#include <linux/klist.h>

/* klist struct */
static struct klist *klist;

static __init int _hlist_demo_init(void)
{
    /* allocate memory*/
    klist = (struct klist *)kmalloc(sizeof(*klist), GFP_KERNEL);
    if (!klist)
        return -ENOMEM;

    /* Initialize klist, non get() and put() */
    klist_init(klist, NULL, NULL);

    return 0;
}
device_initcall(_hlist_demo_init);
