/*
 * Ethernet base demo code
 *
 * (C) 2017.09 <buddy.zhang@aliyun.com> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/etherdevice.h>
#include <linux/mii.h>
#include <linux/slab.h>
#include <linux/phy.h>
#include <linux/io.h>
#include <linux/compiler.h>

#define DEV_NAME  "demo_gmac"

/* Soc Register Mapping */
#define GETH_BASE               0x01c30000
#define GETH_MDIO_ADDR		0x48
#define GETH_MDIO_DATA		0x4C
#define HW_VERSION              1

#define ETHOC_TIMEOUT           (HZ / 2)

/* MDIO */
#define MII_BUSY		0x00000001
#define MII_WRITE		0x00000002

/* interrupt source and mask register */
#define INT_MASK_TXF            (1 << 0)  /* transmit frame */
#define INT_MASK_TXE            (1 << 1)  /* transmit error */
#define INT_MASK_RXF            (1 << 2)  /* receive frame */
#define INT_MASK_RXE            (1 << 3)  /* receive error */
#define INT_MASK_BUSY           (1 << 4)
#define INT_MASK_TXC            (1 << 5)  /* transmit control frame */
#define INT_MASK_RXC            (1 << 6)  /* receive control frame */

#define INT_MASK_TX             (INT_MASK_TXF | INT_MASK_TXE)
#define INT_MASK_RX             (INT_MASK_RXF | INT_MASK_RXE)

/* netdevice private data */
struct demo_priv {
    struct net_device *netdev;
    struct phy_device *phy;
    struct mii_bus *mdio;
    s8 phy_id;
    void __iomem *base_addr;
    char name[20];
    struct napi_struct napi;

    spinlock_t lock;
};

struct demo_hw_gethdev {
    void *iobase;
    unsigned int ver;
    unsigned int mdc_div;
};

static struct demo_hw_gethdev hwdev;

/* networking device open */
static int demo_net_open(struct net_device *dev)
{
    struct demo_priv *priv = netdev_priv(dev);
    int ret;

    printk("Networking open.\n");

    return 0;
}

static const struct net_device_ops demo_netdev_ops = {
    .ndo_open = demo_net_open,
};

/* get MAC address */
static int demo_get_mac_address(struct net_device *dev, void *addr)
{
    return 0;
}

/* 
 * mdio read 
 *
 * Note! Dumplate from Sunxi 
 */
static int sunxi_mdio_read(void *iobase, int phyaddr, int phyreg)
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

static int demo_mdio_read(struct mii_bus *bus, int phy, int reg)
{
    struct net_device *ndev = bus->priv;
    struct demo_priv *priv = netdev_priv(ndev);

    return (int)sunxi_mdio_read(priv->base_addr, phy, reg);
}

/* 
 * mdio write 
 *
 * Note! Dumplicate from sunxi 
 */
int sunxi_mdio_write(void *iobase, int phyaddr, int phyreg, unsigned short data)
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

static int demo_mdio_write(struct mii_bus *bus, int phy, int reg, u16 val)
{
    struct net_device *ndev = bus->priv;
    struct demo_priv *priv = netdev_priv(ndev);

    sunxi_mdio_write(priv->base_addr, phy, reg, val);
    return 0;
}

/*
 * Dumplicate from sunxi
 */
static int sunxi_mdio_reset(void *iobase)
{
    writel((4 << 2), iobase + GETH_MDIO_ADDR);
    return 0;
}

/* mido reset */
static int demo_mdio_reset(struct mii_bus *bus)
{
    struct net_device *ndev = bus->priv;
    struct demo_priv *priv  = netdev_priv(ndev);

    return sunxi_mdio_reset(priv->base_addr);
}

/* Soc specify data register */
static int sunxi_geth_register(void *iobase, int version, unsigned int div)
{
    hwdev.ver = version;
    hwdev.iobase = iobase;
    hwdev.mdc_div = div;

    return 0;
}

static void demo_mdio_poll(struct net_device *dev)
{
}

static int demo_eth_rx(struct net_device *dev, int limit)
{
    printk("* %s\n", __func__);
    return 0;
}

static int demo_eth_tx(struct net_device *dev, int limit)
{
    printk("* %s\n", __func__);
    return 0;
}

static inline void demo_eth_enable_irq(struct demo_priv *dev, u32 mask)
{
    printk("* %s\n", __func__);
}

static int demo_poll(struct napi_struct *napi, int budget)
{
    struct demo_priv *priv = container_of(napi, struct demo_priv, napi);
    int rx_work_done = 0;
    int tx_work_done = 0;

    rx_work_done = demo_eth_rx(priv->netdev, budget);
    tx_work_done = demo_eth_tx(priv->netdev, budget);

    if (rx_work_done < budget && tx_work_done < budget) {
        napi_complete(napi);
        demo_eth_enable_irq(priv, INT_MASK_TX | INT_MASK_RX);
    }
    return rx_work_done;
}

/* probe mdiobus */
static int demo_mdio_probe(struct net_device *net)
{
    struct demo_priv *priv = netdev_priv(net);
    struct phy_device *phy;
    int err;
    int addr;

    if (priv->phy_id != -1) {
        phy = priv->mdio->phy_map[priv->phy_id];
    } else {
        phy =  phy_find_first(priv->mdio);
    }
    if (!phy) {
        printk(KERN_ERR "no PHY found\n");
        return -ENXIO;
    }

    err = phy_connect_direct(net, phy, demo_mdio_poll, 0,
                             PHY_INTERFACE_MODE_GMII);

    if (err) {
        printk(KERN_ERR "could not attach to PHY\n");
        return err;
    }
    priv->phy = phy;
    printk(KERN_INFO "PHY found %d\n", phy->phy_id);

    for (addr = 0; addr < 32; addr++)
        if (priv->mdio->phy_map[addr])
            printk("Found PHY %d\n", addr);
    return 0;
}

/* probe main function */
static int demo_probe(struct platform_device *pdev)
{
    struct net_device *netdev = NULL;
    struct demo_priv *priv = NULL;
    struct resource *res = NULL;
    struct resource *mmio = NULL;
    int ret;
    int phy;

    printk("\n\n\n\n=======================\n\n\n");

    /* allocate networking device */
    netdev = alloc_etherdev(sizeof(struct demo_priv));    
    if (!netdev) {
        ret = -ENOMEM;
        goto out;
    }

    /* setup driver-private data */
    SET_NETDEV_DEV(netdev, &pdev->dev);
    platform_set_drvdata(pdev, netdev);
    priv = netdev_priv(netdev);
    priv->netdev = netdev;
    strcpy(priv->name, "Usage");

    /***** Request I/O resource *****/
    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "demo_io");
    if (!res) {
        printk(KERN_ERR "cannot obtain I/O memory space.\n");
        ret = -ENODEV;
        goto free;
    }
    mmio = request_mem_region(res->start, resource_size(res), pdev->name);
    if (!mmio) {
        printk(KERN_ERR "cannot request I/O memory space\n");
        ret = -ENODEV;
        goto free;
    }
    priv->base_addr = ioremap(res->start, resource_size(res));
    if (!priv->base_addr) {
        printk("ERROR: memeory mapping failed.\n");
        ret = -ENOMEM;
        goto err_io;
    }

    /* Setup MAC */
    /* Check that the given MAC address is valid. If it isn't, read the 
       current MAC from the controller. */
    if (is_valid_ether_addr(netdev->dev_addr))
        demo_get_mac_address(netdev, netdev->dev_addr);

    /* Soc setup */
    sunxi_geth_register((void *)netdev->base_addr, HW_VERSION, 0x03);

    /***** Setup mdio *****/
    /* allocate mdiobus */
    priv->mdio = mdiobus_alloc();
    if (!priv->mdio) {
        ret = -ENOMEM;
        goto free;
    }
    /* initialize mdiobus */
    priv->mdio->name = "demo-mdio";
    snprintf(priv->mdio->id, MII_BUS_ID_SIZE, "%s-%d",
             priv->mdio->name, pdev->id);
    priv->mdio->read  = demo_mdio_read;
    priv->mdio->write = demo_mdio_write;
    priv->mdio->reset = demo_mdio_reset;
    priv->mdio->priv  = netdev;

    priv->mdio->irq = kmalloc(sizeof(int) * PHY_MAX_ADDR, GFP_KERNEL);
    if (!priv->mdio->irq) {
        ret = -ENOMEM;
        goto free_mdio0;
    }
 
    for (phy = 0; phy < PHY_MAX_ADDR; phy++)
        priv->mdio->irq[phy] = PHY_POLL;

    /* register mdiobus */
    ret = mdiobus_register(priv->mdio);
    if (ret)  {
        printk("failed to register MDIO bus\n");
        goto free_mdio;
    }   

    /* Probe mdiobu */
    ret = demo_mdio_probe(netdev);
    if (ret) {
        printk(KERN_ERR "failed to probe MDIO bus.\n");
        goto free_mdio;
    }

    /* ethernet setup */
    ether_setup(netdev);

    /* setup the net_device  structure */
    netdev->netdev_ops = &demo_netdev_ops;
    netdev->watchdog_timeo = ETHOC_TIMEOUT;
    netdev->features |= 0;

    /* setup NAPI */
    netif_napi_add(netdev, &priv->napi, demo_poll, 64);

    /* initialize lock */
    spin_lock_init(&priv->lock);

    /* register networking device */
    ret = register_netdev(netdev);
    if (ret < 0) {
        printk("failed to register ethernet interface.\n");
        goto free;
    }
    goto out;

free_mdio:
    kfree(priv->mdio->irq);
free_mdio0:
    mdiobus_free(priv->mdio);
err_io:

free:
    free_netdev(netdev);
out:
    return ret;
}

/* 
 * remove main function - shutdown ethernet MC
 */
static int demo_remove(struct platform_device *pdev)
{
    struct net_device *netdev = platform_get_drvdata(pdev);
    struct demo_priv *priv = netdev_priv(netdev);

    platform_set_drvdata(pdev, NULL);

    if (netdev) {
        unregister_netdev(netdev);
        free_netdev(netdev);
    }    
    return 0;
}

/* demo resource */
static struct resource demo_resources[] = {
    {
        .name  = "demo_io",
        .start = GETH_BASE,
        .end   = GETH_BASE + 0x1054,
        .flags = IORESOURCE_MEM,
    }
};

/* platfrom device */
static struct platform_device demo_device = {
    .name = DEV_NAME,
    .id = -1,
    .resource = demo_resources,
    .num_resources = ARRAY_SIZE(demo_resources),
};

/* platfrom driver */
static struct platform_driver demo_driver = {
    .probe = demo_probe,
    .remove = demo_remove,
    .driver = {
        .name = DEV_NAME,
        .owner = THIS_MODULE,
    },
};

/* initialize entry */
static __init int demo_eth_init(void)
{
    int ret;

    /* register platform device */
    ret = platform_device_register(&demo_device);
    if (ret)
        return ret;

    /* register platform driver */
    return platform_driver_register(&demo_driver);
}
device_initcall(demo_eth_init);

/* exit entry */
static __exit void demo_eth_exit(void)
{
    platform_driver_unregister(&demo_driver);
    platform_device_unregister(&demo_device);
}
