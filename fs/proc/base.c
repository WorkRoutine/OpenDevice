/*
 * procfs interface demo code
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 */
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/proc_fs.h>

/* proc root */
static struct proc_dir_entry *root;

static __init int demo_proc_init(void)
{
    root = proc_mkdir("demo", NULL);
    if (IS_ERR(root) || !root) {
        printk(KERN_ERR "ERR: Unable to create root proc.\n");
        return -EINVAL;
    }
   
    printk("proc interface initialization.\n");
    return 0;
}
device_initcall(demo_proc_init);

static __exit void demo_proc_exit(void)
{
    /* Release resource */
    remove_proc_entry("demo", root);
}
