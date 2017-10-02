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

/* proc file */
static struct proc_dir_entry *node;

/* open */
static int node_open(struct inode *inode, struct file *filp)
{
    return 0;
}

/* release */
static int node_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/* read */
static ssize_t node_read(struct file *filp, char __user *buf, 
                         size_t count, loff_t *loff)
{
    return count;
}

/* write */
static ssize_t node_write(struct file *filp, const char __user *buf, 
                          size_t count, loff_t *loff)
{
    return count;
}
/* struct file operation for node */
static struct file_operations node_fops = {
    .open         = node_open,
    .release      = node_release,
    .read         = node_read,
    .write        = node_write,
};

/* initialize entry */
static __init int demo_proc_init(void)
{
    int ret;

    /* Create proc dirent on /proc/ */
    root = proc_mkdir("demo", NULL);
    if (IS_ERR(root) || !root) {
        printk(KERN_ERR "ERR: Unable to create root proc.\n");
        return -EINVAL;
    }

    node = proc_create("node", S_IRUGO | S_IWUGO,
                       root, &node_fops);
    if (IS_ERR(node) || !node) {
        printk(KERN_ERR "ERR: Unable to create proc file.\n");
        node = NULL;
        ret = -EINVAL;
        goto out;
    }

    printk("proc interface initialization.\n");
    return 0;

/* error area */
out:
    remove_proc_entry("demo", root);
    return ret;
}
device_initcall(demo_proc_init);

/* exit entry */
static __exit void demo_proc_exit(void)
{
    /* Release resource */
    remove_proc_entry("node", node);
    remove_proc_entry("demo", root);
}
