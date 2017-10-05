/*
 * Filesystem demo code
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 */
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/shmem_fs.h>

static struct dentry *demo_mount(struct file_system_type *fs_type, 
              int flags, const char *dev_name, void *data)
{
    return mount_single(fs_type, flags, data, shmem_fill_super);
}

/* file system type define */
static struct file_system_type demo_filesystem_type = {
    .name     = "demo-filesystem",
    .mount    = demo_mount,
    .kill_sb  = kill_litter_super,
};

/* initialization entry */
static __init int demo_filesystem_init(void)
{
    int ret;

    ret = register_filesystem(&demo_filesystem_type);
    if (ret) {
        printk(KERN_ERR "Unable to register demo filesystem.\n");
        return ret;
    }

    return 0;
}
device_initcall(demo_filesystem_init);

/* exit entry */
static __exit void demo_filesystem_exit(void)
{
    unregister_filesystem(&demo_filesystem_type);
}
