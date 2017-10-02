/*
 * debugfs demo code.
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/fs.h>
#include <linux/debugfs.h>

/* root dirent on debugfs */
static struct dentry *debugfs_root;
/* unsigned char */
static u8 valueu8;
static struct dentry *diru8;

/* unsigned short */
static u16 valueu16;
static struct dentry *diru16;

/* unsigned int */
static u32 valueu32;
static struct dentry *diru32;

/* unsigned long */
static u64 valueu64;
static struct dentry *diru64;

/* Initialize entry */
static __init int demo_debugfs_init(void)
{
    int ret;

    /* Create debugfs root directory. */
    debugfs_root = debugfs_create_dir("demo", NULL);
    if (IS_ERR(debugfs_root) || !debugfs_root) {
        printk(KERN_ERR "ERR: faild to create debugfs directory.\n");
        debugfs_root = NULL;
        return -EINVAL;
    }

    /* Create unsigned char on debugfs */
    diru8 = debugfs_create_u8("valueu8", S_IRUGO | S_IWUGO, 
                              debugfs_root, &valueu8);
    if (IS_ERR(diru8) || !diru8) {
        printk(KERN_ERR "ERR: failed to create debugfs u8 value.\n");
        diru8 = NULL;
        ret = -EINVAL;
        goto out;
    }

    /* Create unsigned short on debugfs */
    diru16 = debugfs_create_u16("valueu16", S_IRUGO | S_IWUGO,
                                debugfs_root, &valueu16);
    if (IS_ERR(diru16) || !diru16) {
        printk(KERN_ERR "ERR: failed to create debugfs u16 value.\n");
        diru16 = NULL;
        ret = -EINVAL;
        goto out;
    }

    /* Create unsigned int on debugfs */
    diru32 = debugfs_create_u32("valueu32", S_IRUGO | S_IWUGO,
                                 debugfs_root, &valueu32);
    if (IS_ERR(diru32) || !diru32) {
        printk(KERN_ERR "ERR: failed to create debugfs u32 value.\n");
        diru32 = NULL;
        ret = -EINVAL;
        goto out;
    }

    /* Create unsigned long on debugfs */
    diru64 = debugfs_create_u64("valueu64", S_IRUGO | S_IWUGO,
                                debugfs_root, &valueu64);
    if (IS_ERR(diru64) || !diru64) {
        printk(KERN_ERR "ERR: failed to create debugfs u64 value.\n");
        diru64 = NULL;
        ret = -EINVAL;
        goto out;
    }

    printk("Debugfs demo code init.\n");
    return 0;

/* error area */
out:
    debugfs_remove_recursive(debugfs_root);
    return ret;
}
device_initcall(demo_debugfs_init);

/* Exit entry */
static __exit void demo_debugfs_exit(void)
{
    /* release debugfs dirent */
    debugfs_remove_recursive(debugfs_root);
}
