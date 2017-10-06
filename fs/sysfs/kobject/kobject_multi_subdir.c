/*
 * kobject demo code
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/kobject.h>

/* root dentry for kobject */
static struct kobject *root;
/* sub-dirent for kobejct */
static struct kobject *subdir_devices;
static struct kobject *subdir_drivers;
static struct kobject *subdir_power;

/* initialization entry */
static __init int demo_kobject_init(void)
{
    /* create a node on /sys/ */
    root = kobject_create_and_add("demo", NULL);
    if (!root) {
        printk(KERN_ERR "Unable to create kobject.\n");
        return -ENOMEM;
    }
    /* create a dirent on /sys/demo/ */
    subdir_devices = kobject_create_and_add("devices", root);
    if (!subdir_devices) {
        printk(KERN_ERR "Unable to create dirent on /sys/demo/devices.\n");
        goto err_sub1;
    }
    
    /* create a dirent on /sys/demo */
    subdir_drivers = kobject_create_and_add("drivers", root);
    if (!subdir_drivers) {
        printk(KERN_ERR "Unable to create dirent on /sys/demo/drivers\n");
        goto err_sub2;
    }

    /* create a dirent on /sys/demo/devices/power */
    subdir_power = kobject_create_and_add("power", subdir_devices);
    if (!subdir_power) {
        printk(KERN_ERR "Unable to create dirent on /sys/demo/device/power.\n");
        goto err_sub3;
    }
    return 0;

err_sub3:
    kobject_put(subdir_drivers);
err_sub2:
    kobject_put(subdir_devices);
err_sub1:
    kobject_put(root);
    return -ENOMEM;
}
device_initcall(demo_kobject_init);

/* exit entry */
static __exit void demo_kobject_exit(void)
{
    kobject_put(subdir_power);
    kobject_put(subdir_drivers);
    kobject_put(subdir_devices);
    kobject_put(root);
}
