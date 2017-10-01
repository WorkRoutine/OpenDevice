/*
 * Create kobject and add into sys
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

int __init _init_kobject_demo(void)
{
    struct kobject *kobj;

    /* create a kobject and add into sys/ */
    kobj = kobject_create_and_add("kobj_demo", NULL);
    if (!kobj) {
        printk("failed to create a kobject.\n");
        return -EINVAL;
    }
    return 0;
}
device_initcall(_init_kobject_demo);
