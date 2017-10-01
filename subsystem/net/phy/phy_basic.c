/*
 * phy demo code
 *
 * (C) 2017.09 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/list.h>  

#define DEV_NAME "demo_mdio"
#define MII_REGS_NUM 29

/* private data for mdio bus */
struct demo_mdio_priv 
{
    int irqs[PHY_MAX_ADDR];
    struct mii_bus *bus;
    struct list_head phys;
    struct platform_device *pdev;
};

/* phy status */
struct demo_phy_status {
    int link;
    int speed;
    int duplex;
    int pause;
    int asym_pause;
};

/* private data for phy */
struct demo_phy_priv
{
    int id;
    u16 regs[MII_REGS_NUM];
    struct phy_device *phydev;
    struct demo_phy_status status;
    int (*link_update)(struct net_device *, struct demo_phy_status *);
    struct list_head node;
};

/* private data */
static struct demo_mdio_priv demo_priv = {
    .phys = LIST_HEAD_INIT(demo_priv.phys),
}; 

static int demo_phy_update_regs(struct demo_phy_priv *priv)
{
    u16 bmsr = BMSR_ANEGCAPABLE;
    u16 bmcr = 0;
    u16 lpagb = 0;
    u16 lpa = 0;

    if (priv->status.duplex) {
        bmcr |= BMCR_FULLDPLX;

        switch (priv->status.speed) {
        case 1000:
            bmsr   |= BMSR_ESTATEN;
            bmcr   |= BMCR_SPEED1000;
            lpagb  |= LPA_1000FULL;
            break;
        case 100:
            bmsr   |= BMSR_100FULL;
            bmcr   |= BMCR_SPEED100;
            lpa    |= LPA_100FULL;
            break;
        case 10:
            bmsr   |= BMSR_10FULL;
            lpa    |= LPA_10FULL;
            break;
        default:
            printk(KERN_WARNING "Demo phy: unknow speed.\n");
            return -EINVAL;
        }
    } else {
        switch (priv->status.speed) {
        case 1000:
            bmsr   |= BMSR_ESTATEN;
            bmcr   |= BMCR_SPEED1000;
            lpagb  |= LPA_1000HALF;
            break;
        case 100:
            bmsr   |= BMSR_100HALF;
            bmcr   |= BMCR_SPEED100;
            lpa    |= LPA_100HALF;
            break;
        case 10:
            bmsr   |= BMSR_10HALF;
            lpa    |= LPA_10HALF;
            break;
        default:
            printk(KERN_WARNING "fixed phy: unknown speed\n");
            return -EINVAL;
        }
    }
    if (priv->status.link)
        bmsr |= BMSR_LSTATUS | BMSR_ANEGCOMPLETE;
    if (priv->status.pause)
        lpa |= LPA_PAUSE_CAP;
    if (priv->status.asym_pause)
        lpa |= LPA_PAUSE_ASYM;

    priv->regs[MII_PHYSID1] = priv->id >> 16;
    priv->regs[MII_PHYSID2] = priv->id;

    priv->regs[MII_BMSR] = bmsr;
    priv->regs[MII_BMCR] = bmcr;
    priv->regs[MII_LPA]  = lpa;
    priv->regs[MII_STAT1000] = lpagb;

    return 0;
}

/* mdio read */
static int demo_mdio_read(struct mii_bus *bus, int phy_id, int reg_num)
{
    struct demo_mdio_priv *priv_bus = bus->priv;
    struct demo_phy_priv *priv_phy;

    if (reg_num >= MII_REGS_NUM)
        return -1;

    list_for_each_entry(priv_phy, &priv_bus->phys, node) {
        if (priv_phy->id == phy_id) {
            /* Issue callback if user registered it. */
            if (priv_phy->link_update) {
                priv_phy->link_update(priv_phy->phydev->attached_dev, 
                                      &priv_phy->status);
                demo_phy_update_regs(priv_phy);
            }
            return priv_phy->regs[reg_num];
        }
    }
    return 0xFFFF;
}

/* mdio write */
static int demo_mdio_write(struct mii_bus *bus, int phy_id, int reg_num,
                          u16 val)
{
    return 0;
}

/*
 * If somthing weird is required to be done with link/speed,
 * network driver is able to assign a function to implent this.
 * May be useful for PHY's that need to be software-drivern.
 */
int demo_phy_set_link_update(struct phy_device *phydev,
    int (*link_update)(struct net_device *, struct demo_phy_status *))
{
    struct demo_mdio_priv *bus = &demo_priv;
    struct demo_phy_priv *priv;

    if (!link_update || !phydev || !phydev->bus)
        return -EINVAL;

    list_for_each_entry(priv, &bus->phys, node) {
        if (priv->id == phydev->phy_id) {
            priv->link_update = link_update;
            priv->phydev = phydev;
            return 0;
        }
    }
    return -ENOENT;
}
EXPORT_SYMBOL(demo_phy_set_link_update);

int demo_phy_add(unsigned int irq, int phy_id,
                 struct demo_phy_status *status)
{
    int ret;
    struct demo_mdio_priv *bus = &demo_priv;
    struct demo_phy_priv *priv;

    priv = (struct demo_phy_priv *)kzalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;
    memset(priv->regs, 0xFF, sizeof(priv->regs[0]) * MII_REGS_NUM);
    bus->irqs[phy_id] = irq;
    priv->id = phy_id;
    priv->status = *status;

    ret = demo_phy_update_regs(priv);
    if (ret)
        goto err_regs;

    list_add_tail(&priv->node, &bus->phys);
    return 0;

err_regs:
    kfree(priv);
    return ret;
}
EXPORT_SYMBOL(demo_phy_add);

/* exit entry */
static __exit void demo_mido_exit(void)
{
    struct demo_mdio_priv *bus = &demo_priv;
    struct demo_phy_priv *priv, *tmp;

    mdiobus_unregister(bus->bus);
    mdiobus_free(bus->bus);
    platform_device_unregister(bus->pdev);

    list_for_each_entry_safe(priv, tmp, &bus->phys, node) {
        list_del(&priv->node);
        kfree(priv);
    }
}

/* Initialize modibus */
static __init int demo_mdio_init(void)
{
    struct demo_mdio_priv *priv = &demo_priv;
    struct platform_device *pdev;
    int ret;

    /* allocate platform device */
    priv->pdev = platform_device_register_simple(DEV_NAME, 0, NULL, 0);
    if (IS_ERR(priv->pdev)) {
        ret = PTR_ERR(priv->pdev);
        goto err_pdev;
    }

    /* allocate mdiobus */
    priv->bus = mdiobus_alloc();
    if (priv->bus == NULL) {
        ret = -ENOMEM;
        goto err_mdiobus_reg;
    }

    /* Initialize mido bus */
    snprintf(priv->bus->id, MII_BUS_ID_SIZE, "demo-0");
    priv->bus->name = "Demo MDIO Bus";
    priv->bus->priv = priv;
    priv->bus->parent = &priv->pdev->dev;
    priv->bus->read   = demo_mdio_read;
    priv->bus->write  = demo_mdio_write;
    priv->bus->irq    = priv->irqs; 

    /* register midobus */
    ret = mdiobus_register(priv->bus);
    if (ret)
        goto err_mdiobus_alloc;

    printk("MDIO init down.\n");
    return 0;

/* Error route */
err_mdiobus_alloc:
    mdiobus_free(priv->bus);
err_mdiobus_reg:
    platform_device_unregister(priv->pdev);
err_pdev:
    return ret;
}
device_initcall(demo_mdio_init);
