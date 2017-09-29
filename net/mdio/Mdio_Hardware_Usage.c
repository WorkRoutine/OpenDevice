/*
 * mdio bus running on sunxi
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
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/clk.h>

/* demo core private data */
struct demo_mdio_priv 
{
    struct device *dev;
    void __iomem  *base;
    struct mii_bus *mdio;
    char name[20];

    struct clk    *clk;
    void __iomem  *extclk;
    struct pinctrl *pinctrl;
};

/* mdio priv */
struct demo_base {
    void *iobase;
    unsigned int ver;
    unsigned int mdc_div;
};

static struct demo_base hwdev;
static unsigned long tx_delay = 0;
static unsigned long rx_delay = 0;

/* define name for device and driver */
#define DEV_NAME            "gmac0"
#define DEMO_CLKBASE        "gmac"

/* Eth base io address */
#define GETH_BASE               0x01c30000
#define GETH_MDIO_ADDR          0x48
#define GETH_MDIO_DATA          0x4C

#define SYS_CTL_BASE            0x01c00000
#define GETH_CLK_REG            0x0030

#define HW_VERSION              1

#define MII_BUSY                0x00000001
#define MII_WRITE               0x00000002

/* dumplicate from sunxi */
int legacy_mdio_read(void *iobase, int phyaddr, int phyreg)
{
    unsigned int value = 0;

    /* Mask the MDC_DIV_RATIO */
    value |= ((hwdev.mdc_div & 0x07) << 20);
    value |= (((phyaddr << 12) & (0x0001F000)) |
             ((phyreg << 4) & (0x000007F0)) | MII_BUSY);

    while (((readl(iobase + GETH_MDIO_ADDR)) & MII_BUSY) == 1);
    writel(value, iobase + GETH_MDIO_ADDR);
    while (((readl(iobase + GETH_MDIO_ADDR)) & MII_BUSY) == 1);
 
    return (int)readl(iobase + GETH_MDIO_DATA);
}

/* mdio read */
static int demo_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
    struct demo_mdio_priv *priv = bus->priv;
    
    return (int)legacy_mdio_read(priv->base, phyaddr, phyreg); 
}

/* dumplicate from sunxi */
int legacy_mdio_write(void *iobase, int phyaddr, int phyreg, unsigned short data)
{
    unsigned int value;

    value = ((0x07 << 20) & readl(iobase + GETH_MDIO_ADDR)) | (hwdev.mdc_div << 20);
    value |= (((phyaddr << 12) & (0x0001F000)) |
             ((phyreg << 4) & (0x000007F0))) | MII_WRITE | MII_BUSY;

    /* Wait until any existing MII operation is complete */
    while (((readl(iobase + GETH_MDIO_ADDR)) & MII_BUSY) == 1);

    /* Set the MII address register to write */
    writel(data, iobase + GETH_MDIO_DATA);
    writel(value, iobase + GETH_MDIO_ADDR);

    /* Wait until any existing MII operation is complete */
    while (((readl(iobase + GETH_MDIO_ADDR)) & MII_BUSY) == 1);

    return 0;
}

/* mdio write */
static int demo_mdio_write(struct mii_bus *bus, int phyaddr,
                           int phyreg, u16 data)
{
    struct demo_mdio_priv *priv = bus->priv;

    legacy_mdio_write(priv->base, phyaddr, phyreg, data);
    return 0;
}

/* mdc ration */
static void legacy_mdc_clock(void *iobase, int version, unsigned int div)
{
    hwdev.ver     = version;
    hwdev.iobase  = iobase;
    hwdev.mdc_div = div;

    return 0;
}

/* Initialize hardware mdio */
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

    /* register mdio */
    if (mdiobus_register(bus)) {
        printk(KERN_ERR "%s cann't register MDIO bus\n", bus->name);
        ret = -EINVAL;
        goto err_alloc;
    }
    priv->mdio = bus;
    return 0;
err_alloc:
    mdiobus_free(bus);
    return ret;
}

/* Soc clock initialize */
static int legacy_clk_enable(struct platform_device *pdev)
{
    struct demo_mdio_priv *priv = platform_get_drvdata(pdev);
    int clkval;

    clk_prepare_enable(priv->clk);

    /* get current clkval */
    clkval = readl(priv->extclk + GETH_CLK_REG);
    printk("PreVale %#x\n", clkval);
    /* Soc setup */
    clkval |= 0x00000004;

    clkval &= (~0x00000003);
    clkval |= 0x00000002;
    /* Adjust Tx/Rx clock delay */
    clkval &= ~(0x07 << 10);
    clkval |= ((tx_delay & 0x07) << 10);
    clkval &= ~(0x1f << 5);
    clkval |= ((rx_delay &0x1f) << 5);

    writel(clkval, priv->extclk + GETH_CLK_REG);
    return 0;
}

static int legacy_power_on(struct platform_device *pdev)
{
    struct demo_mdio_priv *priv = platform_get_drvdata(pdev);
    int value;

    value = readl(priv->extclk + GETH_CLK_REG);
    value &= ~(1 << 15);
    value |= (1 << 16);
    writel(value, priv->extclk + GETH_CLK_REG);
    return 0;
}

static int demo_platform_resource_request(struct platform_device *pdev)
{
    struct demo_mdio_priv *priv = platform_get_drvdata(pdev);
    struct resource *res;
    int ret;

    /* request resource */
    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "demo_mdio_io");
    if (!res) {
        printk(KERN_ERR "Unable to get IO resource.\n");
        ret = -ENODEV;
        goto out;
    }
    if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
        printk(KERN_ERR "Unable to request memory region.\n");
        ret = -EBUSY;
        goto out;
    }

    /* mapping memory */
    priv->base = ioremap(res->start, resource_size(res));
    if (!priv->base) {
        printk(KERN_ERR "Unable to mapping memory.\n");
        ret = -ENOMEM;
        goto out_mem;
    }

    /* request clk resource */
    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "demo_extclk");
    if (unlikely(!res)) {
        printk(KERN_ERR "Unable to get CLK resource.\n");
        ret = -ENODEV;
        goto out_io;
    }

    /* mapping memory */
    priv->extclk = ioremap(res->start, resource_size(res));
    if (unlikely(!priv->extclk)) {
        printk(KERN_ERR "Unable to mapping memory.\n");
        ret = -ENOMEM;
        goto out_io;
    }

    /* pre-request clock */
    priv->clk = clk_get(&pdev->dev, DEMO_CLKBASE);
    if (unlikely(!priv->clk || IS_ERR(priv->clk))) {
        printk(KERN_ERR "Get clock is failed!\n");
        ret = -EINVAL;
        goto out_extclk;
    }

    /* request pinctl */
    priv->pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
    if (IS_ERR_OR_NULL(priv->pinctrl)) {
        printk(KERN_ERR "devm_pinctrl is failed.\n");
        ret = -EINVAL;
        goto out_clk;
    }
    return 0;

/* error area */
out_clk:
    clk_put(priv->clk);
out_extclk:
    iounmap(priv->extclk);
out_io:
    iounmap(priv->base);
out_mem:
    release_mem_region(res->start, resource_size(res));
out:
    return ret;
}

/* probe platform driver */
static int demo_probe(struct platform_device *pdev)
{
    struct demo_mdio_priv *priv = NULL;
    struct phy_device *phydev = NULL;
    struct resource *res;
    int ret;
    int reg;

    /* Setup mdio private data */
    priv = (struct demo_mdio_priv *)kmalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;
    memset(priv, 0, sizeof(*priv));
    platform_set_drvdata(pdev, priv);
    priv->dev = &pdev->dev;
    strcpy(priv->name, "USAB");

    /* request resource */
    ret = demo_platform_resource_request(pdev); 
    if (ret < 0) {
        printk(KERN_ERR "Unable to request resource.\n");
        goto out_free;
    }

    /* Soc initialize */
    legacy_power_on(pdev);
    legacy_clk_enable(pdev);

    legacy_mdc_clock(priv->base, HW_VERSION, 0x03);

    /* Initialize mdio  hardware. */
    ret = mdio_hardware_init(pdev);
    if (ret < 0) {
        printk(KERN_ERR "Initialize mdio hardware error.\n");
        goto err_free;
    }

    phydev = phy_find_first(priv->mdio);
    if (!phydev) {
        printk(KERN_INFO "NO PHY found!\n");
        goto err_free;
    }
    
    return 0;

err_free:
    kfree(priv);
    return ret;
}

/* remove platform driver */
static int demo_remove(struct platform_device *pdev)
{
    struct demo_mdio_priv *priv = platform_get_drvdata(pdev);
    /* Remove hardware */
    clk_put(priv->clk);
    iounmap(priv->extclk);
    iounmap(priv->base);
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
    {
        .name  = "demo_extclk",
        .start = SYS_CTL_BASE,
        .end   = SYS_CTL_BASE + GETH_CLK_REG,
        .flags = IORESOURCE_MEM,
    }
};

/* platform device information */
static struct platform_device demo_device = {
    .name = DEV_NAME,  /* Same as driver name */
    .id = -1,
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
