/*
 * i2c usage demo code on userspace
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

/* I2C bus */
#define I2C_DEV_CHIP            "/dev/i2c-0"

#define I2C_M_WR                0

/* i2c device handler */
static unsigned int i2c_fd;

/* i2c slave address */
static unsigned int i2c_slave_addr = 0x50;

/*
 * I2C read
 * @fd: file handler
 * @addr: i2c slave 7-bit address
 * @offset: read position.
 * @buf: buffer for reading data.
 * @len: length for reading.
 *
 * @return: the number of i2c_msg has read. 
 *          succeed is 2.
 */
static int I2CBus_packetRead(int fd, unsigned char addr, unsigned char offset, 
                             unsigned char *buf, unsigned char len)
{
    unsigned char tmpaddr[2];
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msgset;
    int rc;

    tmpaddr[0]     = offset;
    msgs[0].addr   = addr & 0xfe;
    msgs[0].flags  = I2C_M_WR;
    msgs[0].len    = 1;
    msgs[0].buf    = tmpaddr;

    msgs[1].addr   = addr & 0xfe;
    msgs[1].flags  = I2C_M_RD;  ;
    msgs[1].len    = len;
    msgs[1].buf    = buf;

    msgset.msgs    = msgs;
    msgset.nmsgs   = 2;

    rc = ioctl(fd, I2C_RDWR, &msgset);
    return rc;
}

/* 
 * I2C write
 * @fd: file handler.
 * @addr: i2c slave 7-bit address
 * @offset: write position
 * @buf: buffer for writuing data.
 * @len: the length for writing
 *
 * @return: the number of i2c_msg has write.
 *          succeed is 1.
 */
static int I2CBus_packetWrite(int fd, unsigned char addr, unsigned char offset,
                              unsigned char *buf, unsigned char len)
{
    unsigned char tmpaddr[2];
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msgset;
    int rc;

    tmpaddr[0]     = offset;
    tmpaddr[1]     = buf[0];
    msgs[0].addr   = addr & 0xfe;
    msgs[0].flags  = I2C_M_WR;
    msgs[0].len    = 2;
    msgs[0].buf    = tmpaddr;

    msgset.msgs    = msgs;
    msgset.nmsgs   = 1;

    rc = ioctl(fd, I2C_RDWR, &msgset); 
    return rc;
}


/* open i2c slave device */
static open_i2c_device(void)
{
    i2c_fd = open(I2C_DEV_CHIP, O_RDWR);
    if (i2c_fd < 0) {
        aup_debug("ERROR: fail to open %s\n", I2C_DEV_CHIP);
        exit(1);
    }
}

/* exit i2c slave device */
static close_i2c_device(void)
{
    close(i2c_fd);
}

/* main function */
int main(int argc, char *argv[])
{
    int err;
    char buf[10];
    int i;

    /* open i2c bus */
    open_i2c_device();

    /* Read data from I2C bus */
    err = I2CBus_packetRead(i2c_fd, i2c_slave_addr, 0x00, buf, 2);    
    if (err != 1) {
        printf("ERROR: can't read data from i2c bus.\n");
        goto err_read;
    }

    /* Write data to I2C bus */
    buf[0] = 0x55;
    buf[1] = 0xAA;

    /* loop delivered message on I2C bus. */
    for (i = 0; i < 2; i++) {
        err = I2CBus_packetWrite(i2c_fd, i2c_slave_addr, i, &buf[i], 1);
        /* Must add delay to wait i2c write done. */
        usleep(4000);
        if (err != 2) {
            printf("ERROR: Can't write data to i2c bus.\n");
            goto err_write;
        }
    }
    err = 0;
err_write:
    ;
err_read:
    ;
    /* close i2c bus */
    close_i2c_device();
    return err;
}

