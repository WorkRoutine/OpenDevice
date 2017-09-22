/*
 * class basic demo
 *
 * (C) 2017.09 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/module.h>
#include <linux/device.h>

static struct class *demo_class = NULL;
#define CLASS_NAME  "demo_class"

static __init int demo_class_init(void)
{
    /* Create class */
    demo_class = class_create(THIS_MODULE, CLASS_NAME);
    return 0;
}
device_initcall(demo_class_init);

static __exit void demo_class_exit(void)
{
    /* destory class */
    class_destroy(demo_class);
}
