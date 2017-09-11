/*
 * get random number.
 *
 * (C) 2017.09.06 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>

extern void get_random_bytes(void *buf, int nbytes);

static __init int _hlist_demo_init(void)
{
    int randnum;

    get_random_bytes(&randnum, sizeof(int));

    printk("Random number %d\n", randum);
    return 0;
}
device_initcall(_hlist_demo_init);
