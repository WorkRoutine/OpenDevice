/*
 * Clock
 *
 * (C) 2017.12 <buddy.zhang@aliyun.com>
 */
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/clk.h>

static __init int demo_clk_init(void)
{
    struct clk *clk_pll1;
    struct clk *clk_pll2;
    struct clk *clk_cluster0;
    struct clk *clk_cluster1;

    unsigned long rate;
    unsigned long target_rate = 1008000000;

    /* Request pll1 clock */
    clk_pll1 = clk_get(NULL, "pll_cpu0");
    if (!clk_pll1) {
        printk("can't get correct pll1 clock.\n");
        return -EINVAL;
    }

    /* Request pll2 clock */
    clk_pll2 = clk_get(NULL, "pll_cpu1");
    if (!clk_pll2) {
        printk("can't get correct pll2 clock.\n");
        return -EINVAL;
    }

    /* Request cluster0 clock */
    clk_cluster0 = clk_get(NULL, "cluster0");
    if (!clk_cluster0) {
        printk("can't get correct cluster0 clock.\n");
        return -EINVAL;
    }

    /* Request cluster1 clock */
    clk_cluster1 = clk_get(NULL, "cluster1");
    if (!clk_cluster1) {
        printk("can't get correct cluster1 clock.\n");
        return -EINVAL;
    }

    /* get pll1 clock rate */
    rate = clk_get_rate(clk_pll1);
    printk("Pll1 rate is %ld\n", rate);
    /* get pll2 clock rate */
    rate = clk_get_rate(clk_pll2);
    printk("Pll2 rate is %ld\n", rate);
    /* get cluster0 clock rate */
    rate = clk_get_rate(clk_cluster0);
    printk("cluster0 rate is %ld\n", rate);
    /* get cluster1 clock rate */
    rate = clk_get_rate(clk_cluster1);
    printk("cluster1 rate is %ld\n", rate);

    /* Set pll1 clk rate */
    clk_set_rate(clk_pll1, target_rate);
    /* Set pll2 clk rate */
    clk_set_rate(clk_pll2, target_rate);
    /* Set cluster0 clk rate */
    clk_set_rate(clk_cluster0, target_rate);
    /* Set cluster1 clk rate */
    clk_set_rate(clk_cluster1, target_rate);

    /* enable clock */
    clk_enable(clk_pll1);
    clk_enable(clk_pll2);
    clk_enable(clk_cluster0);
    clk_enable(clk_cluster1);

    /* disable clock */
    clk_disable(clk_pll1);
    clk_disable(clk_pll2);
    clk_disable(clk_cluster0);
    clk_disable(clk_cluster1);

    /* Release clk resource */
    clk_put(clk_pll1);
    clk_put(clk_pll2);
    clk_put(clk_cluster0);
    clk_put(clk_cluster1);

    return 0;
}
late_initcall(demo_clk_init);
