/*
 * Platform device demo code
 *
 * (C) 2017.09 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/platform_device.h>

/* define name for device and driver */
#define DEV_NAME "demo_platform"

/* probe platform driver */
static int demo_probe(struct platform_device *pdev)
{
    printk("* demo probe.\n");
    /* Initialize hardware. */
    return 0;
}

/* remove platform driver */
static int demo_remove(struct platform_device *pdev)
{
    /* Remove hardware */
    return 0;
}

/* platform device information */
static struct platform_device demo_device = {
    .name = DEV_NAME,  /* Same as driver name */
    .id   = 2014,
};

/* platform driver information */
static struct platform_driver demo_driver = {
    .probe  = demo_probe,
    .remove = demo_remove,
    .driver = {
        .name = DEV_NAME, /* Same as device name */
    }, 
};

/* exit .. */
static __exit void demo_plat_exit(void)
{
    platform_driver_unregister(&demo_driver);
    platform_device_unregister(&demo_device);
}

/* init entry .. */
static __init int demo_plat_init(void)
{
    int ret;

    /* register device */
    ret = platform_device_register(&demo_device);
    if (ret)
        return ret;    
    
    /* register driver */
    return platform_driver_register(&demo_driver);
}
device_initcall(demo_plat_init); /* last invoke */
