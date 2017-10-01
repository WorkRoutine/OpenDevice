/*
 * MDIO userspace demo code
 *
 * (C) 2017.09 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MDIO_DEV_PATH    "/dev/mdio"

typedef unsigned char u8;

/* MDIO COMMAND */
enum mido_ioctl {
    MDIO_IOC_NULL,
    MDIO_IOC_SET_PHYID,
    MDIO_IOC_SET_REGID,
    MDIO_IOC_GET_PHYID,
    MDIO_IOC_GET_REGID,
    MDIO_IOC_NUM,
};

/*
 * MDIO read
 * @fd: file handler
 * @reg: the address of register.
 *
 * @return: the value from register.
 */
static u8 MDIO_read(int fd, u8 reg)
{
    u8 value;

    ioctl(fd, MDIO_IOC_SET_REGID, &reg);
    read(fd, &value, sizeof(value));
    return value; 
}

/*
 * MDIO write
 * @fd: file handle
 * @reg: the address of register.
 * @val: the value of written
 *
 * @return: 0 succeed
 */
static int MDIO_write(int fd, u8 reg, u8 value)
{
    ioctl(fd, MDIO_IOC_SET_REGID, &reg);
    write(fd, &value, sizeof(value));
    return 0;
}

/*
 * Setup PHY ID
 * @fd: file handler
 * @id: phy id
 *
 * @return: 0 success.
 */
static int MDIO_set_PHYID(int fd, int id)
{
    ioctl(fd, MDIO_IOC_SET_PHYID, &id);
    return 0;
}

int main(void)
{
    int fd;
    int value;

    /* open file */
    fd = open(MDIO_DEV_PATH, O_RDWR);
    if (fd < 0) {
        printf("Can't open device.\n");
        return -1;
    }
    
    /* Setup MDIO */
    MDIO_set_PHYID(fd, 0x02);
    
    /* Read MDIO Register */
    value = MDIO_read(fd, 0x05);
    printf("Prev value %#x\n", value);

    /* Write MDIO register. */
    MDIO_write(fd, 0x05, 0x02);

    /* Read MDIO Register */
    value = MDIO_read(fd, 0x05);
    printf("After value %#x\n", value);

    /* close file */
    close(fd);
    return 0;
}
