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

/* size_t value for debugfs file */
static size_t svalue;
static struct dentry *dirsize_t;

/* bool value for debugfs file */
static u32 bvalue;
static struct dentry *dirbool;

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

    /* Create debugfs value */
    dirsize_t = debugfs_create_size_t("value_t", S_IRUSR | S_IWUSR,
                                      debugfs_root, &svalue);
    if (IS_ERR(dirsize_t) || !dirsize_t) {
        printk(KERN_ERR "ERR: failed to create debugfs direntory.\n");
        dirsize_t = NULL;
        ret = -EINVAL;
        goto out;
    }

    /* Create bool for debugfs */
    dirbool = debugfs_create_bool("bool", S_IRUSR | S_IWUSR,
                                  debugfs_root, &bvalue);
    if (IS_ERR(dirbool) || !dirbool) {
        printk(KERN_ERR "ERR: failed to create debugs directory.\n");
        dirbool = NULL;
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
