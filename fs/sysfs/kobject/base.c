/*
 * kobject demo code
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/kobject.h>

static struct kobject *root;

/* initialization entry */
static __init int demo_kobject_init(void)
{
    /* create a node on /sys/ */
    root = kobject_create_and_add("demo", NULL);
    return 0;
}
device_initcall(demo_kobject_init);

/* exit entry */
static __exit void demo_kobject_exit(void)
{
    kobject_put(root);
}
