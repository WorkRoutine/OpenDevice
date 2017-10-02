/*
 * debugfs demo code.
 *
 * (C) 2017.09.06 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/fs.h>
#include <linux/debugfs.h>

static struct dentry *debugfs_root;
static struct dentry *subdir_devices;
static struct dentry *subdir_drivers;
static struct dentry *subdir_power;

/* open operation */
static int debugfs_file_open(struct inode *inode, struct file *filp)
{
    /* open debugfs file */
    return 0;
}

/* relase operation */
static int debugfs_file_release(struct inode *inode, struct file *filp)
{
    /* release debug file */
    return 0;
}

/* read operation */
static ssize_t debugfs_file_read(struct file *filp, char __user *buf, 
                                 size_t count, loff_t *loff) 
{
    /* read operation */
    return count;
}

/* write operation */
static ssize_t debugfs_file_write(struct file *filp, const char __user *buf, 
                                  size_t count, loff_t *loff)
{
    /* write to debugfs file */
    return count;
}

/* debugfs file operation */
static const struct file_operations debugfs_file_ops = {
    .open        = debugfs_file_open,
    .read        = debugfs_file_read,
    .write       = debugfs_file_write,
    .release     = debugfs_file_release,
};

/* Initialize entry */
static __init int demo_debugfs_init(void)
{
    int ret;

    /* create debugfs root directory. */
    debugfs_root = debugfs_create_dir("demo", NULL);
    if (IS_ERR(debugfs_root) || !debugfs_root) {
        printk(KERN_ERR "ERR: faild to create debugfs directory.\n");
        debugfs_root = NULL;
        return -EINVAL;
    }

    /* Create debugfs file */
    if (!debugfs_create_file("files", S_IFREG | S_IRUGO,
                        debugfs_root, NULL, &debugfs_file_ops)) {
        printk(KERN_ERR "Unable to create file on debugfs.\n");
        ret = -EINVAL;
        goto out;
    }
  
    /* Create sub-dirent on root debugfs */
    subdir_devices = debugfs_create_dir("devices", debugfs_root);
    if (IS_ERR(subdir_devices) || !subdir_devices) {
        printk(KERN_ERR "ERR: failed to create subdir-devices\n");
        subdir_devices = NULL;
        ret = -EINVAL;
        goto out;
    }

    /* Create same layer sub-dirent on root debugfs */
    subdir_drivers = debugfs_create_dir("drivers", debugfs_root);
    if (IS_ERR(subdir_drivers) || !subdir_drivers) {
        printk(KERN_ERR "ERR: failed to create subdir-drivers\n");
        subdir_drivers = NULL;
        ret = -EINVAL;
        goto out_devices;
    }

    /* Create sub-dirent on sub-dirent */
    subdir_power = debugfs_create_dir("power", subdir_devices);
    if (IS_ERR(subdir_power) || !subdir_power) {
        printk(KERN_ERR "ERR: failed to create subdir-power\n");
        subdir_power = NULL;
        goto out_drivers;
    }

    printk("Debugfs demo code init.\n");
    return 0;
/* error area */
out_drivers:
    debugfs_remove_recursive(subdir_drivers);
out_devices:
    debugfs_remove_recursive(subdir_devices);
out:
    debugfs_remove_recursive(debugfs_root);
    debugfs_root = NULL;
    return ret;
}
device_initcall(demo_debugfs_init);

/* Exit entry */
static __exit void demo_debugfs_exit(void)
{
    /* release debugfs dirent */
    debugfs_remove_recursive(subdir_power);
    debugfs_remove_recursive(subdir_drivers);
    debugfs_remove_recursive(subdir_devices);
    debugfs_remove_recursive(debugfs_root);
}
