/*
 * SPI demo driver
 *
 * (C) 2017.09.06 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/device.h>

/* SPI interface instruction set */
#define INSTRUCTION_WRITE	0x02
#define INSTRUCTION_READ	0x03

#define DEV_NAME "spi_demo"
#define BUFFER_SIZE  1024

/*
 * Spi device private data, contain tx/rx buffer.
 */
struct spi_demo_priv {
    struct spi_device *spi;
    uint8_t *spi_tx_buf;
    uint8_t *spi_rx_buf;
};

/*
 * Transform data with SPI.
 *
 * Note about handling of error return of spi_demo_trans: accessing
 * registers via SPI is not really different conceptually than using
 * normal I/O assembler instructions, although it's much more
 * complicated from a practical POV. So it's not advisable to always
 * check the return value of this function. Imagine that every
 * read{b,l}, write{b,l} and friends would be bracketed in "if ( < 0)
 * error();", it would be a great mess (well there are some situation
 * when exception handling C++ like could be useful after all). So we
 * just check that transfers are OK at the beginning of our
 * conversation with the chip and to avoid doing really nasty things
 * (like injecting bogus packets in the network stack).
 */
static int spi_demo_trans(struct spi_device *spi, int len)
{
    struct spi_demo_priv *priv = dev_get_drvdata(&spi->dev);
    struct spi_transfer t = {
        .tx_buf = priv->spi_tx_buf,
        .rx_buf = priv->spi_rx_buf,
        .len = len,
        .cs_change = 0,
    };
    struct spi_message m;
    int ret;

    spi_message_init(&m);
    spi_message_add_tail(&t, &m);

    ret = spi_sync(spi, &m);
    if (ret)
        printk(KERN_ERR "spi transfer failed: ret = %d\n", ret);
    return ret;
}

/*
 * Read data from SPI bus.
 */
static uint8_t spi_demo_read(struct spi_device *spi, uint8_t reg)
{
    struct spi_demo_priv *priv = dev_get_drvdata(&spi->dev);
    uint8_t val = 0;

    priv->spi_tx_buf[0] = INSTRUCTION_READ;
    priv->spi_tx_buf[1] = reg;

    spi_demo_trans(spi, 3);
    val = priv->spi_rx_buf[2];

    return val;
}

/*
 * Wirte data to SPI bus.
 */
static void spi_demo_write(struct spi_device *spi, uint8_t reg, uint8_t val)
{
    struct spi_demo_priv *priv = dev_get_drvdata(&spi->dev);

    priv->spi_tx_buf[0] = INSTRUCTION_WRITE;
    priv->spi_tx_buf[1] = reg;
    priv->spi_tx_buf[2] = val;

    spi_demo_trans(spi, 3);
}

/*
 * Probe spi device driver.
 */
static int spi_demo_probe(struct spi_device *spi)
{
    struct spi_demo_priv *priv;
    uint8_t value;

    /* allocate private data */
    priv = (struct spi_demo_private *)kmalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    /* allocate memory for buffer */
    priv->spi_tx_buf = (uint8_t *)kmalloc(sizeof(uint8_t) * BUFFER_SIZE, GFP_KERNEL);
    if (!priv->spi_tx_buf)
        goto error_buf0;
    priv->spi_rx_buf = (uint8_t *)kmalloc(sizeof(uint8_t) * BUFFER_SIZE, GFP_KERNEL);
    if (!priv->spi_rx_buf)
        goto error_buf1;
    /* Setup private data */
    priv->spi = spi;
    dev_set_drvdata(&spi->dev, priv);

    /* Test SPI read/write */
    value = spi_demo_read(spi, 0x2A);
    printk(KERN_INFO "SPI device register 0x2a default value %#2x\n", value);
    spi_demo_write(spi, 0x2A, 0x78);
    if (0x78 == spi_demo_read(spi, 0x2A))
        printk(KERN_INFO "SPI bus work well.\n");

    return 0;
error_buf1:
    kfree(priv->spi_tx_buf);
error_buf0:
    kfree(priv);
    return -ENOMEM;
}

/*
 * Remove spi driver.
 */
static int __devexit spi_demo_remove(struct spi_device *spi) 
{
    struct spi_demo_priv *priv = dev_get_drvdata(&spi->dev);

    /* free rx/tx buffer */
    kfree(priv->spi_rx_buf);
    kfree(priv->spi_tx_buf);

    /* free spi private data */
    kfree(priv);
    return 0;
}

static struct spi_driver spi_demo_driver = {
    .driver = {
        .name  = DEV_NAME,
        .bus   = &spi_bus_type,
        .owner = THIS_MODULE,
    },
    .probe  = spi_demo_probe,
    .remove = __devexit_p(spi_demo_remove),
};

static struct spi_board_info spi_demo_board __initdata = {
    .modalias = DEV_NAME,
    .max_speed_hz = 1000000,
    .bus_num  = 0,
    .chip_select = 0,
    .mode  = 0,
};

/*
 * Register board infor of SPI device into system.
 * 
 * Note! This funciton must invoke on arch_initcall.
 */
static __init int spi_board_init(void)
{
    spi_register_board_info(&spi_demo_board, 1);
    printk(KERN_INFO "* SPI board info initialize.\n");
    return 0;
}
arch_initcall(spi_board_init);

static __init int spi_demo_init(void)
{
    printk("spi demo init.\n");
    return spi_register_driver(&spi_demo_driver);
}
device_initcall(spi_demo_init);
