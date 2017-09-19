/*
 * mdio bus demo code
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
#include <linux/mii.h>
#include <linux/phy.h>

struct demo_mdio_priv 
{
    struct device *dev;
    void __iomem  *base;
};

/* define name for device and driver */
#define DEV_NAME "demo_mdio"

/* Eth base io address */
#define GETH_BASE 0x01c30000

extern int sunxi_mdio_read(void *iobase, int phyaddr, int phyreg);
extern int sunxi_mdio_write(void *iobase, int phyaddr, int phyreg, unsigned short data);

/* mdio read */
static int demo_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
    struct demo_mdio_priv *priv = bus->priv;
    
    return (int)sunxi_mdio_read(priv->base, phyaddr, phyreg);
}

/* mdio write */
static int demo_mdio_write(struct mii_bus *bus, int phyaddr,
                           int phyreg, u16 data)
{
    struct demo_mdio_priv *priv = bus->priv;

    sunxi_mdio_write(priv->base, phyaddr, phyreg, data);
    return 0;
}

static int mdio_hardware_init(struct platform_device *pdev)
{
    struct mii_bus *bus;
    struct demo_mdio_priv *priv = platform_get_drvdata(pdev);
    int ret;

    /* allocate mii bus */
    bus = mdiobus_alloc();
    if (!bus) {
        printk(KERN_ERR "Unable to allocate mdio bus.\n");
        return -ENOMEM;
    }
    bus->name   = DEV_NAME;
    bus->read   = &demo_mdio_read;
    bus->write  = &demo_mdio_write;
    snprintf(bus->id, MII_BUS_ID_SIZE, "%s-%x",
             bus->name, 8);
    bus->parent = priv->dev;
    bus->priv   = priv;

    if (mdiobus_register(bus)) {
        printk(KERN_ERR "%s cann't register MDIO bus\n", bus->name);
        ret = -EINVAL;
        goto err_alloc;
    }
    return 0;
err_alloc:
    mdiobus_free(bus);
    return ret;
}

/* probe platform driver */
static int demo_probe(struct platform_device *pdev)
{
    struct demo_mdio_priv *priv = NULL;
    struct resource *res;
    int ret;

    /* Setup mdio private data */
    priv = (struct demo_mdio_priv *)kmalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;
    memset(priv, 0, sizeof(*priv));
    platform_set_drvdata(pdev, priv);
    priv->dev = &pdev->dev;

    /* request resource */
    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "demo_mdio_io");
    if (!res) {
        printk(KERN_ERR "Unable to get IO resource.\n");
        ret = -ENODEV;
        goto err_free;
    }
    if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
        printk(KERN_ERR "Unable to request memory region.\n");
        ret = -EBUSY;
        goto err_free;
    }

    /* mapping memory */
    priv->base = ioremap(res->start, resource_size(res));
    if (!priv->base) {
        printk(KERN_ERR "Unable to mapping memory.\n");
        ret = -ENOMEM;
        goto err_region;
    }

    /* Initialize mdio  hardware. */
    ret = mdio_hardware_init(pdev);
    if (ret < 0) {
        printk(KERN_ERR "Initialize mdio hardware error.\n");
        goto err_io;
    }
    printk("probe done.\n");
    return 0;

err_io:
    iounmap(priv->base);
err_region:
    release_mem_region(res->start, resource_size(res));
err_free:
    kfree(priv);
    return ret;
}

/* remove platform driver */
static int demo_remove(struct platform_device *pdev)
{
    /* Remove hardware */
    return 0;
}


/* mdio resource */
static struct resource demo_mdio_res[] = {
    {
        .name  = "demo_mdio_io",
        .start = GETH_BASE,
        .end   = GETH_BASE + 0x1054,
        .flags = IORESOURCE_MEM,
    },
};

/* platform device information */
static struct platform_device demo_device = {
    .name = DEV_NAME,  /* Same as driver name */
    .id   = 2014,
    .resource = demo_mdio_res,
    .num_resources = ARRAY_SIZE(demo_mdio_res),
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
static __exit void demo_mdio_exit(void)
{
    platform_driver_unregister(&demo_driver);
    platform_device_unregister(&demo_device);
}

/* init entry .. */
static __init int demo_mdio_init(void)
{
    int ret;

    /* register device */
    ret = platform_device_register(&demo_device);
    if (ret)
        return ret;    
    
    /* register driver */
    return platform_driver_register(&demo_driver);
}
device_initcall(demo_mdio_init); /* last invoke */
