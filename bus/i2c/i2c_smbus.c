/*
 * I2C driver smbus demo code
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
#include <linux/slab.h>

#define DEV_NAME "demo_i2c"
#define I2C_ADDR 0x38
#define REG_BASE 0x00

/* private data */
struct demo_data 
{
    struct i2c_client *client;
};

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
    struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
    struct demo_data *priv;
    u8 flags;
    int ret;

    /* I2C adapter function check */
    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA
            | I2C_FUNC_SMBUS_I2C_BLOCK)) {
        printk(KERN_ERR "I2C[%s] doesn't support required functionality.\n", dev_name(&adapter->dev));
        return -EIO;
    }

    priv = (struct demo_data *)kmalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        printk(KERN_ERR "No free memory to allocate.\n");
        return -ENOMEM;
    }

    /* Get i2c client */
    priv->client = client;
    /* link i2c client */
    i2c_set_clientdata(client, priv);

    /* SMBUS read */
    flags = i2c_smbus_read_byte_data(client, REG_BASE + 0x00);
    /* SMBUS write */
    ret = i2c_smbus_write_byte_data(client, REG_BASE + 0x00, 0xD8);
    if (ret < 0) {
        printk(KERN_ERR "Unable to write data.\n");
        return ret;
    }


    printk(KERN_INFO "i2c demo probe.\n");
    return 0;
}

/* i2c driver remove */
static int demo_remove(struct i2c_client *client)
{
    struct demo_data *priv;

    priv = i2c_get_clientdata(client);
    kfree(priv);

    return 0;
}

/* i2c driver */
static struct i2c_driver demo_driver = {
    .driver = {
        .name = DEV_NAME,
        .of_match_table = of_match_ptr(demo_of_match),
    },
    .probe    = demo_probe,
    .remove   = demo_remove,
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
