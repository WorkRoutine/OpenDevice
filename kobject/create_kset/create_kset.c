/*
 * Create kset and add into sys
 *
 * (C) 2017.09.06 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>

int __init _init_kset_demo(void)
{
    struct kset *kset;

    /* Create a kset without parent and uevent_ops. */
    kset = kset_create_and_add("frank", NULL, NULL);
    if (!kset) {
        printk("failed to create a kset.\n");
        return -EINVAL;
    }
    return 0;
}
device_initcall(_init_kset_demo);
