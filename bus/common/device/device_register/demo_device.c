/*
 * Registe and unregister an demo device into/from system..
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "device.h"

/* demo device register */
static __init int demo_a_device_init(void)
{
    struct demo_device *dev;
    char *init_name = NULL;
    int ret;

    /* allocate a new demo device */
    dev = (struct demo_device *)kmalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev) {
        printk(KERN_ERR "Unable allocate memory to demo device.\n");
        ret = -ENOMEM;
        goto err_alloc;
    }
    /* Note must clear demo device, clear device->p */
    memset(dev, 0, sizeof(*dev));

    /* Note! if demo device doesn't belong to any demo bus,
     * Must set init_name before register. */
    init_name = (char *)kzalloc(sizeof(char) * 20, GFP_KERNEL);
    if (!init_name) {
        printk(KERN_ERR "Unbale allocate memory to init_name.\n");
        ret = -ENOMEM;
        goto err_alloc2;
    }
    strcpy(init_name, "BiscuitOS-dev");
    dev->init_name = init_name;

    /* register demo device into system */
    ret = demo_device_register(dev);
    if (ret < 0) {
        printk(KERN_ERR "Unable register demo device into system.\n");
        goto err_register;
    }
   
    /* Emulate unregister routine */
    demo_device_unregister(dev);
    return ret;

err_register:
    kfree(init_name);
err_alloc2:
    kfree(dev);
err_alloc:
    return ret;
}
device_initcall_sync(demo_a_device_init);
