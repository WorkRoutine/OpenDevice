/*
 * bus core demo code
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "device.h"

#define demo_bus_attr(_attr) container_of(_attr, \
        struct demo_bus_attribute, attr)

/* /sys/demo_bus */
static struct kset *demo_bus_kset;

/* /sys/demo_device/system */
static struct kset *demo_system_kset;

/* sysfs bindings for buses */
static ssize_t demo_bus_attr_show(struct kobject *kobj, 
               struct attribute *attr, char *buf)
{
    struct demo_bus_attribute *bus_attr = demo_bus_attr(attr);
    struct demo_subsys_private *subsys_priv = demo_subsys_private(kobj);
    ssize_t ret = 0;

    if (bus_attr->show)
        ret = bus_attr->show(subsys_priv->bus, buf);
    return ret;
}

static ssize_t demo_bus_attr_store(struct kobject *kobj, 
               struct attribute *attr, const char *buf, size_t count)
{
    struct demo_bus_attribute *bus_attr = demo_bus_attr(attr);
    struct demo_subsys_private *subsys_priv = demo_subsys_private(kobj);
    ssize_t ret = 0;

    if (bus_attr->store)
        ret = bus_attr->store(subsys_priv->bus, buf, count);
    return ret;
}

static const struct sysfs_ops demo_bus_sysfs_ops = {
    .show  = demo_bus_attr_show,
    .store = demo_bus_attr_store,
};

static struct kobj_type demo_bus_ktype = {
    .sysfs_ops = &demo_bus_sysfs_ops,
};

static int demo_bus_uevent_filter(struct kset *kset, struct kobject *kobj)
{
    struct kobj_type *ktype = get_ktype(kobj);

    if (ktype == &demo_bus_ktype)
        return 1;
    return 0;
}

static const struct kset_uevent_ops demo_bus_uevent_ops = {
    .filter = demo_bus_uevent_filter,
};

int __init demo_buses_init(void)
{
    demo_bus_kset = kset_create_and_add("demo_bus", 
                    &demo_bus_uevent_ops, NULL);
    if (!demo_bus_kset)
        return -ENOMEM;

    demo_system_kset = kset_create_and_add("demo_system", NULL,
                       &demo_devices_kset->kobj);
    if (!demo_system_kset)
        return -ENOMEM;

    return 0;
}
