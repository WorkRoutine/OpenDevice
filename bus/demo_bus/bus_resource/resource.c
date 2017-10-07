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
#include <linux/slab.h>

#include "demo_device.h"

#define DEV_NAME  "demo_bus"

struct demo_priv
{
    struct demo_device *dev;
    struct demo_driver *drv;
    void __iomem *base;
};

/* device resource */
#define DEMO_IO_START     0x01c10000
#define DEMO_IO_SIZE      0x10
#define DEMO_MEM_START    0x01c20000
#define DEMO_MEM_SIZE     0x10


static int demo_probe(struct demo_device *ddev)
{
    struct demo_priv *priv;
    struct resource *res;
    int ret;

    priv = (struct demo_priv *)kmalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        printk(KERN_INFO "Unable to allocate memory for priv\n");
        return -ENOMEM;
    }
    memset(priv, 0, sizeof(*priv));
    demo_set_drvdata(ddev, priv);
    priv->dev = ddev;

    /* get memory resource by name */
    res = demo_get_resource_byname(ddev, IORESOURCE_MEM, "demo_mem");
    if (!res) {
        printk(KERN_ERR "Unable to get MEM resource.\n");
        ret = -ENODEV;
        goto err_free;
    }
    /* request resource */
    if (!request_mem_region(res->start, resource_size(res), ddev->name)) {
        printk(KERN_ERR "Unable to request memory region.\n");
        ret = -EBUSY;
        goto err_free;
    }
    /* IO mapping */
    priv->base = ioremap(res->start, resource_size(res));
    if (!priv->base) {
        printk(KERN_ERR "Unable to mapping memory.\n");
        ret = -ENOMEM;
        goto err_region;
    }

    /* Get resource */
    res = demo_get_resource(ddev, IORESOURCE_IO, 1);
    if (!res) {
        printk(KERN_ERR "No IO resource.\n");
        return -ENXIO;
    }

    printk("Demo probe.\n");
    return 0;

err_region:
    release_mem_region(res->start, resource_size(res));
err_free:
    kfree(priv);
    return ret;
}

static int demo_remove(struct demo_device *ddev)
{
    struct demo_priv *priv = demo_get_drvdata(ddev);
    struct resource *res;

    /* Remove hardware */
    iounmap(priv->base);
    res = demo_get_resource_byname(ddev, IORESOURCE_MEM, "demo_mem");
    release_mem_region(res->start, resource_size(res));

    kfree(priv);
    demo_set_drvdata(ddev, NULL);
    printk("Demo remove.\n");
    return 0;
}

static struct resource demo_resources[] = {
    {
        .name  = "demo_io",
        .start = DEMO_IO_START,
        .end   = DEMO_IO_START + DEMO_IO_SIZE,
        .flags = IORESOURCE_IO,
    },
    {
        .name  = "demo_mem",
        .start = DEMO_MEM_START,
        .end   = DEMO_MEM_START + DEMO_MEM_SIZE,
        .flags = IORESOURCE_MEM,
    },
};

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
    .resource = demo_resources,
    .num_resources = ARRAY_SIZE(demo_resources),
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

