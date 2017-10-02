/*
 * procfs interface demo code
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/proc_fs.h>

/* proc root */
static struct proc_dir_entry *root;
/* proc subdir */
static struct proc_dir_entry *subdir_devices;
static struct proc_dir_entry *subdir_drivers;
static struct proc_dir_entry *subdir_power;

static __init int demo_proc_init(void)
{
    int ret;

    /* Create proc dirent on /proc/ */
    root = proc_mkdir("demo", NULL);
    if (IS_ERR(root) || !root) {
        printk(KERN_ERR "ERR: Unable to create root proc.\n");
        return -EINVAL;
    }

    /* Create proc dirent on /proc/demo/ */
    subdir_devices = proc_mkdir("devices", root);
    if (IS_ERR(subdir_devices) || !subdir_devices) {
        printk(KERN_ERR "ERR: Unable to create proc dirent devices.\n");
        subdir_devices = NULL;
        ret = -EINVAL;
        goto out;
    }

    /* Create proc dirent on /proc/demo/ */
    subdir_drivers = proc_mkdir("drivers", root);
    if (IS_ERR(subdir_drivers) || !subdir_drivers) {
        printk(KERN_ERR "ERR: Unable to create proc dirent drivers.\n");
        subdir_drivers = NULL;
        ret = -EINVAL;
        goto out_device;
    }
   
    /* Create proc dirent on /proc/demo/devices/ */
    subdir_power = proc_mkdir("power", subdir_devices);
    if (IS_ERR(subdir_power) || !subdir_power) {
        printk(KERN_ERR "Unable to create proc dirent power.\n");
        subdir_power = NULL;
        ret = -EINVAL;
        goto out_driver;
    }

    printk("proc interface initialization.\n");
    return 0;

/* error are */
out_driver:
    remove_proc_entry("drivers", subdir_drivers);
out_device:
    remove_proc_entry("devices", subdir_devices);
out:
    remove_proc_entry("demo", root);
    return ret;
}
device_initcall(demo_proc_init);

static __exit void demo_proc_exit(void)
{
    /* Release resource */
    remove_proc_entry("power", subdir_power);
    remove_proc_entry("drivers", subdir_drivers);
    remove_proc_entry("devices", subdir_devices);
    remove_proc_entry("demo", root);
}
