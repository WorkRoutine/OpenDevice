/*
 * device demo code
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/device.h>

static struct device demo_device = {
    .init_name = "demo_dev",
};

/* initialization entry */
static __init int demo_device_init(void)
{
    int error;

    error = device_register(&demo_device);
    if (error) {
        printk(KERN_ERR "Unable to create new device.\n");
        return error;
    }
    return error;
}
device_initcall(demo_device_init);

/* exit entry */
static __exit void demo_device_exit(void)
{
    device_unregister(&demo_device);
}
