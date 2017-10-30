/*
 * class core demo code
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "device.h"

/* Hotplug events for demo classes go to the demo class subsys */
static struct kset *demo_class_kset;

#define demo_to_class_dir(obj)  container_of(obj, \
                          struct demo_class_dir, kobj)

static void demo_class_dir_release(struct kobject *kobj)
{
    struct demo_class_dir *dir = demo_to_class_dir(kobj);
    kfree(dir);
}

static const struct kobj_ns_type_operations *demo_class_dir_child_ns_type(
                    struct kobject *kobj)
{
    struct demo_class_dir *dir = demo_to_class_dir(kobj);
    return dir->class->ns_type;
}

static struct kobj_type demo_class_dir_ktype = {
    .release        = demo_class_dir_release,
    .sysfs_ops      = &kobj_sysfs_ops,
    .child_ns_type  = demo_class_dir_child_ns_type
};

struct kobject *demo_class_dir_create_and_add(struct demo_class *class,
                struct kobject *parent_kobj)
{
    struct demo_class_dir *dir;
    int retval;

    dir = kzalloc(sizeof(*dir), GFP_KERNEL);
    if (!dir)
        return NULL;

    dir->class = class;
    kobject_init(&dir->kobj, &demo_class_dir_ktype);

    dir->kobj.kset = &class->p->glue_dirs;

    retval = kobject_add(&dir->kobj, parent_kobj, "%s", class->name);
    if (retval < 0) {
        kobject_put(&dir->kobj);
        return NULL;
    }
    return &dir->kobj;
}

int __init demo_classes_init(void)
{
    demo_class_kset = kset_create_and_add("demo_class", NULL, NULL);
    if (!demo_class_kset)
        return -ENOMEM;

    return 0;
}
