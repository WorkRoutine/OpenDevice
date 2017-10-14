/*
 * class core demo code
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "device.h"

/* Hotplug events for demo classes go to the demo class subsys */
static struct kset *demo_class_kset;

int __init demo_classes_init(void)
{
    demo_class_kset = kset_create_and_add("demo_class", NULL, NULL);
    if (!demo_class_kset)
        return -ENOMEM;

    return 0;
}
