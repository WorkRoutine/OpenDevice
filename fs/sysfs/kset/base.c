/*
 * Kset demo code
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/kobject.h>

/* /sys/demo */
struct kset *demo_kset;

/* uevent filter */
static int demo_uevent_filter(struct kset *kset, struct kobject *kobj)
{
    return 0;
}

/* uevent name */
static const char *demo_uevent_name(struct kset *kset, struct kobject *kobj)
{
    return NULL;
}

/* uevent */
static int demo_uevent(struct kset *kset, struct kobject *kobj,
                       struct kobj_uevent_env *env)
{
    return 0;
}

/* kset uevent operations */
static const struct kset_uevent_ops demo_uevent_ops = {
    .filter    = demo_uevent_filter,
    .name      = demo_uevent_name,
    .uevent    = demo_uevent,
};

/* initialization entry */
static __init int demo_kset_init(void)
{
    demo_kset = kset_create_and_add("demo", &demo_uevent_ops, NULL);
    if (!demo_kset) {
        printk(KERN_INFO "Unable to create new kset.\n");
        return -ENOMEM;
    }
    return 0;
}
device_initcall(demo_kset_init);

/* exit entry */
static __exit void demo_kset_exit(void)
{
    kset_unregister(demo_kset);
}
