/*
 * Platform device request resource from DTS
 *
 * (C) 2017.09 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*
                aup: aup2@01c30000 {
                        compatible = "aup,aup_mdio";
                        reg = <0x0 0x01c30000 0x0 0x4000>,
                              <0x0 0x01c00030 0x0 0x1>;
                        pinctrl-names = "default";
                        pinctrl-0     = <&gmac_pins_a>;
                        interrupts = <GIC_SPI 82 IRQ_TYPE_LEVEL_HIGH>;
                        interrupt-names = "gmacirq";
                        clocks = <&clk_gmac>, <&clk_ephy>;
                        clock-names = "gmac", "ephy";
                        phy-mode = "rgmii";
                        tx-delay = <7>;
                        rx-delay = <31>;
                        gmac_power1 = "axp81x_dldo2:2500000";
                        gmac_power2 = "axp81x_eldo2:1800000";
                        gmac_power3 = "axp81x_fldo1:1200000";
                        phy_power_on = <&pio PD 6 1 0 0 0>;
                        status = "okay";
                };
*/
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/clk-private.h>

#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

/* define name for device and driver */
#define DEV_NAME "aup_mdio"

/* probe platform driver */
static int aup_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct device *dev = &pdev->dev;
    int gpio, flag;
    u32 val;
    const char *string;
    struct clk *clk0, *clk1;
    int count, irq;
    struct resource r;
    void __iomem *reg;

    printk("\n\n\n\nProbe\n\n\n\n\n");

    /* Read u32 value from DTB */
    of_property_read_u32(np, "tx-delay", &val);
    printk("TX-delay: %#x\n", val);

    /* Read string from DTB */
    of_property_read_string(np, "phy-mode", &string);
    printk("phy-mode: %s\n", string);

    /* Obtain gpio */
    gpio = of_get_named_gpio_flags(np, "phy_power_on", 0, &flag);
    printk("phy_power_on gpio: %d\n", gpio);

    /* Obtian clk */
    clk0 = of_clk_get(np, 0);
    clk1 = of_clk_get_by_name(np, "ephy");
    printk("clk0: %s\n", clk0->name);
    printk("clk1: %s\n", clk1->name);

    /* obtain irq */
    irq = of_irq_to_resource(np, 0, &r);
    /* Obtain count of irq */
    count = of_irq_count(np);
    printk("IRQ[%s]: %d count: %d\n", r.name, irq, count);
    
    /* iomap */
    reg = of_iomap(np, 0);
    printk("reg: %#x\n", reg);
    reg = of_iomap(np, 1);
    printk("reg: %#x\n", reg);

    return 0;
}

/* remove platform driver */
static int aup_remove(struct platform_device *pdev)
{
    return 0;
}

static const struct of_device_id aup_of_match[] = {
    { .compatible = "aup,aup_mdio", },
    { },
};
MODULE_DEVICE_TABLE(of, aup_of_match);

/* platform driver information */
static struct platform_driver aup_driver = {
    .probe  = aup_probe,
    .remove = aup_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = DEV_NAME, /* Same as device name */
        .of_match_table = aup_of_match,
    }, 
};

module_platform_driver(aup_driver);
