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

static struct demo_device demo_bus_device = {
    .name = "demo_device",
    .id   = 3,
};

/* initialization entry */
static __init int demo_bus_device_init(void)
{
    return demo_device_register(&demo_bus_device);
}
device_initcall_sync(demo_bus_device_init);

/* exit entry */
static __exit void demo_bus_device_exit(void)
{
    demo_device_unregister(&demo_bus_device);
}

