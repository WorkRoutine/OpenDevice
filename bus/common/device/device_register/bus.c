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

static struct demo_bus_type *demo_bus_get(struct demo_bus_type *bus)
{
    if (bus) {
        kset_get(&bus->p->subsys);
        return bus;
    }
    return NULL;
}

static void demo_bus_put(struct demo_bus_type *bus)
{
    if (bus)
        kset_put(&bus->p->subsys);
}

static void demo_device_remove_attrs(struct demo_bus_type *bus, 
                                     struct demo_device *dev)
{
    int i;

    if (bus->dev_attrs) {
        for (i = 0; attr_name(bus->dev_attrs[i]); i++)
            demo_device_remove_file(dev, &bus->dev_attrs[i]);
    }
}

static int demo_device_add_attrs(struct demo_bus_type *bus,
                                 struct demo_device *dev)
{
    int error = 0;
    int i;

    if (!bus->dev_attrs)
        return 0;

    for (i = 0; attr_name(bus->dev_attrs[i]); i++) {
        error = demo_device_create_file(dev, &bus->dev_attrs[i]);
        if (error) {
            while (--i >= 0)
                demo_device_remove_file(dev, &bus->dev_attrs[i]);
            break;
        }
    }
    return error;
}

/* add device to bus */
int demo_bus_add_device(struct demo_device *dev)
{
    struct demo_bus_type *bus = demo_bus_get(dev->bus);
    int error = 0;

    /* Note! non-verify */
    if (bus) {
        printk(KERN_INFO "bus: '%s': add demo device %s\n", 
                         bus->name, demo_dev_name(dev));
        error = demo_device_add_attrs(bus, dev);
        if (error)
            goto out_put;
        error = sysfs_create_link(&bus->p->devices_kset->kobj,
                &dev->kobj, demo_dev_name(dev));
        if (error)
            goto out_id;
        klist_add_tail(&dev->p->knode_bus, &bus->p->klist_devices);
    }
    return 0;

out_id:
    demo_device_remove_attrs(bus, dev);
out_put:
    demo_bus_put(dev->bus);
    return error;
}

/* probe drivers for a new device */
void demo_bus_probe_device(struct demo_device *dev)
{
    struct demo_bus_type *bus = dev->bus;
    struct demo_subsys_interface *sif;
    int ret;

    if (!bus)
        return;

    if (bus->p->drivers_autoprobe) {
        ret = demo_device_attach(dev);
        WARN_ON(ret < 0);
    }
    list_for_each_entry(sif, &bus->p->interfaces, node)
        if (sif->add_dev)
            sif->add_dev(dev, sif);
}

static struct demo_driver *demo_next_driver(struct klist_iter *i)
{
    struct klist_node *n = klist_next(i);
    struct demo_driver_private *drv_priv;

    if (n) {
        drv_priv = container_of(n, struct demo_driver_private, knode_bus);
        return drv_priv->driver;
    }
    return NULL;
}

/* demo driver iterator */
int demo_bus_for_each_drv(struct demo_bus_type *bus, 
                          struct demo_driver *start, void *data,
            int (*fn)(struct demo_driver *, void *))
{
    struct klist_iter i;
    struct demo_driver *drv;
    int error = 0;

    if (!bus)
        return -EINVAL;

    klist_iter_init_node(&bus->p->klist_drivers, &i,
            start ? &start->p->knode_bus : NULL);
    while ((drv = demo_next_driver(&i)) && !error)
        error = fn(drv, data);
    klist_iter_exit(&i);
    return error;
}

/* remove demo device from demo bus */
void demo_bus_remove_device(struct demo_device *dev)
{
    struct demo_bus_type *bus = dev->bus;
    struct demo_subsys_interface *sif;

    if (!bus)
        return;

    list_for_each_entry(sif, &bus->p->interfaces, node)
        if (sif->remove_dev)
            sif->remove_dev(dev, sif);

    sysfs_remove_link(&dev->kobj, "demo_subsystem");
    sysfs_remove_link(&dev->bus->p->devices_kset->kobj,
               demo_dev_name(dev));
    demo_device_remove_attrs(dev->bus, dev);
    if (klist_node_attached(&dev->p->knode_bus))
        klist_del(&dev->p->knode_bus);

    printk(KERN_INFO "bus: '%s': remove device %s\n",
                dev->bus->name, demo_dev_name(dev));
    demo_device_release_driver(dev);
    demo_bus_put(dev->bus);
}

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
