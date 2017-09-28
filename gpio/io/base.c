/*
 * I/O demo code.
 *
 * (C) 2017.09 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <linux/platform_device.h>

/* define name for device and driver */
#define DEV_NAME            "demo_io"
/* Test region */
#define DEMO_BASE           0x01c30000
#define DEMO_SIZE           0x1054

/* Ethernet Registe map */
#define DEMO_CONTROL                      0x00

/* platform private data */ 
struct demo_driv_priv
{
    struct device *dev;
    void __iomem *base;
    int irq;
};

/* I/O hardware read */
static int demo_io_hardware_read(struct demo_driv_priv *priv)
{
    int reg;

    reg = readl(priv->base + DEMO_CONTROL);

    return reg;
}

/* probe platform driver */
static int demo_probe(struct platform_device *pdev)
{
    struct resource *res;
    struct demo_driv_priv *priv;

    /* set driver private data */
    priv = (struct demo_driv_priv *)kmalloc(sizeof(*priv), GFP_KERNEL);
    memset(priv, 0, sizeof(*priv));
    platform_set_drvdata(pdev, priv);
    priv->dev = &pdev->dev; 

    /* get memory resource by name  */
    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "demo_mem");
    if (!res) {
        printk(KERN_ERR "Unable to get IO resource.\n");
        return -ENODEV;
    }

    /* request resource */
    if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
        printk(KERN_ERR "Unable to request memory region.\n");
        return -EBUSY;
    }

    /* Io mapping */
    priv->base = ioremap(res->start, resource_size(res));
    if (!priv->base) {
        printk(KERN_ERR "Unable to mapping memory.\n");
        return -ENOMEM;
    }
    printk(KERN_INFO "Mapping address %#x\n", priv->base);

    /* I/O read */
    demo_io_hardware_read(priv);
    return 0;
}

/* remove platform driver */
static int demo_remove(struct platform_device *pdev)
{
    struct demo_driv_priv *priv = platform_get_drvdata(pdev);
    struct resource *res;

    /* Remove hardware */
    iounmap(priv->base);
    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "demo_mem");
    release_mem_region(res->start, resource_size(res));

    /* clear driv private data */
    kfree(priv);
    platform_set_drvdata(pdev, NULL);
    return 0;
}

/* platform resource */
static struct resource demo_resources[] = {
    {
        .name  = "demo_mem",
        .start = DEMO_BASE,
        .end   = DEMO_BASE + DEMO_SIZE,
        .flags = IORESOURCE_MEM,
    },
};

/* platform device information */
static struct platform_device demo_device = {
    .name = DEV_NAME,  /* Same as driver name */
    .id   = 2014,      /* device ID */
    .resource = demo_resources,
    .num_resources = ARRAY_SIZE(demo_resources),
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
