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

/* u8 value for hexadecimal */
static u8 xvalue8;
static struct dentry *dirxu8;

/* u16 value for hexadecimal */
static u16 xvalue16;
static struct dentry *dirxu16;

/* u32 value for hexadecimal */
static u32 xvalue32;
static struct dentry *dirxu32;

/* u64 value for hexadecimal */
static u64 xvalue64;
static struct dentry *dirxu64;

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

    /* Create u8 for hexadecimal on debugfs */
    dirxu8 = debugfs_create_x8("x8", S_IRUGO | S_IWUGO,
                               debugfs_root, &xvalue8);
    if (IS_ERR(dirxu8) || !dirxu8) {
        printk(KERN_ERR "ERR: failed to create debugfs xu8.\n");
        dirxu8 = NULL;
        ret = -EINVAL;
        goto out;
    }

    /* Create u16 for hexadecimal on debugfs */
    dirxu16 = debugfs_create_x16("x16", S_IRUGO | S_IWUGO,
                                 debugfs_root, &xvalue16);
    if (IS_ERR(dirxu16) || !dirxu16) {
        printk(KERN_ERR "ERR: failed to create debugfs xu16.\n");
        dirxu16 = NULL;
        ret = -EINVAL;
        goto out;
    }

    /* Create u32 for hexadecimal on debugfs. */
    dirxu32 = debugfs_create_x32("x32", S_IRUGO | S_IWUGO,
                                 debugfs_root, &xvalue32);
    if (IS_ERR(dirxu32) || !dirxu32) {
        printk(KERN_ERR "ERR: failed to create debugfs xu32!\n");
        dirxu32 = NULL;
        ret = -EINVAL;
        goto out;
    }

    /* Create u64 for hexadecimal on debugfs */
    dirxu64 = debugfs_create_x64("x64", S_IRUGO | S_IWUGO,
                                 debugfs_root, &xvalue64);
    if (IS_ERR(dirxu64) || !dirxu64) {
        printk(KERN_ERR "ERR: failed to create debugfs xu64!\n");
        dirxu64 = NULL;
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
