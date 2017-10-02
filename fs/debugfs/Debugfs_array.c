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

struct demo_array 
{
    int a;
    char name[20];
};

/* array on debugfs */
static struct demo_array array[3] = 
{
    {
        10,
        "Hello",
    },
    {
        20,
        "World",
    },
};
static struct dentry *dirarray;
static struct debugfs_blob_wrapper blob = {
    .data = array,
    .size = sizeof(array),
};


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

    dirarray = debugfs_create_blob("arrray", S_IRUSR | S_IWUSR,
                                   debugfs_root, &blob);
    if (IS_ERR(dirarray) || !dirarray) {
        printk(KERN_ERR "ERR: failed to create debugfs blob.\n");
        dirarray = NULL;
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
