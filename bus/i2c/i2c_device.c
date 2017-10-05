/*
 * I2C device demo code
 *
 * (C) 2017.10 buddy.zhang@aliyun.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/module.h>
#include <linux/i2c.h>

#define DEV_NAME "demo_i2c"
#define I2C_ADDR 0x38

/* Compatible family ic */
static const struct i2c_device_id demo_id[] = {
    { DEV_NAME, 0 },
    { }
};

/* Match from dts */
static const struct of_device_id demo_of_match[] = {
    { .compatible = "demo,demo-i2c" },
    { }
};

/* probe driver */
static int demo_probe(struct i2c_client *client,
                      const struct i2c_device_id *id)
{
    printk(KERN_INFO "i2c demo probe.\n");
    return 0;
}

/* i2c driver */
static struct i2c_driver demo_driver = {
    .driver = {
        .name = DEV_NAME,
        .of_match_table = of_match_ptr(demo_of_match),
    },
    .probe    = demo_probe,
    .id_table = demo_id,
};

/* initialization entry */
static __init int demo_i2c_init(void)
{
    struct i2c_board_info i2c_info;
    struct i2c_adapter *adap;
    int ret;

    memset(&i2c_info, 0, sizeof(struct i2c_board_info));
    strlcpy(i2c_info.type, DEV_NAME, I2C_NAME_SIZE);
    i2c_info.addr = I2C_ADDR;

    /* get I2C bus adapter */
    adap = i2c_get_adapter(0);

    /* Register I2C device inforation */
    if (i2c_new_device(adap, &i2c_info) == NULL) {
        printk(KERN_ERR "Unable to register i2c device.\n");
        i2c_put_adapter(adap);
        return -EFAULT;
    }
    i2c_put_adapter(adap);
    
    /* Add i2c driver */
    ret = i2c_add_driver(&demo_driver);
    if (ret) {
        printk(KERN_ERR "Unbale to add i2c driver.\n");
        return -EFAULT;
    }
    printk("I2C device initialize done.\n");

    return 0;
}
device_initcall(demo_i2c_init);

/* Exit entry */
static __exit void demo_i2c_exit(void)
{
    i2c_del_driver(&demo_driver);
}
