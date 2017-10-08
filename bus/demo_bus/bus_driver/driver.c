/*
 * demo bus device register
 * 
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>

#include "demo_device.h"

#define DEV_NAME  "demo_bus"

static int demo_probe(struct demo_device *ddev)
{
    printk("Demo probe.\n");
    return 0;
}

static int demo_remove(struct demo_device *ddev)
{
    printk("Demo remove.\n");
    return 0;
}

static struct demo_driver demo_bus_driver = {
    .driver = {
        .name = DEV_NAME,
    },
    .probe   = demo_probe,
    .remove  = demo_remove,
};

static struct demo_device demo_bus_device = {
    .name = DEV_NAME,
    .id   = 3,
};

/* initialization entry */
static __init int demo_bus_device_init(void)
{
    demo_device_register(&demo_bus_device);

    return demo_driver_register(&demo_bus_driver);
}
device_initcall_sync(demo_bus_device_init);

/* exit entry */
static __exit void demo_bus_device_exit(void)
{
    demo_driver_unregister(&demo_bus_driver);
    demo_device_unregister(&demo_bus_device);
}

