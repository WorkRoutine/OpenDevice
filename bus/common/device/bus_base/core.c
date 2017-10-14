/*
 * Core for device subsyste.
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>

#include "device.h"

/* initialize entry */
static __init int demo_core_init(void)
{
    /* demo device core initialization */
    demo_device_init();

    /* demo bus core initialization */
    demo_buses_init();

    return 0;
}
device_initcall(demo_core_init);

/* exit entry */
static __exit void demo_core_exit(void)
{
}
