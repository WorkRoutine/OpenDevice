/*
 * device core demo code.
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/slab.h>
#include <linux/kernel.h>

#include "device.h"

/* /sys/demo_devices */
struct kset *demo_devices_kset;

/* root demo device kset */
static struct kobj_type demo_device_ktype;

/* path of device node file */
const char *demo_device_get_devnode(struct demo_device *dev,
            umode_t *mode, const char **tmp)
{
    char *s;

    *tmp = NULL;

    /* the device type may provide a specify name */
    if (dev->type && dev->type->devnode)
        *tmp = dev->type->devnode(dev, mode);
    if (*tmp)
        return *tmp;

    /* the class may provide a specific name */
    if (dev->class && dev->class->devnode)
        *tmp = dev->class->devnode(dev, mode);
    if (*tmp)
        return *tmp;

    /* return name without allocation, tmp == NULL */
    if (strchr(demo_dev_name(dev), '!') == NULL)
        return demo_dev_name(dev);

    /* replace '!' in the name with '/' */
    *tmp = kstrdup(demo_dev_name(dev), GFP_KERNEL);
    if (!*tmp)
        return NULL;
    while ((s = strchr(*tmp, '!')))
        s[0] = '/';
    return *tmp;
}

/* demo device uevent filter */
static int demo_uevent_filter(struct kset *kset, struct kobject *kobj)
{
    struct kobj_type *ktype = get_ktype(kobj);

    if (ktype == &demo_device_ktype) {
        struct demo_device *dev = demo_to_dev(kobj);

        if (dev->bus)
            return 1;
        if (dev->class)
            return 1;
    }
    return 0;
}

/* device name on uevent */
static const char *demo_uevent_name(struct kset *kset, struct kobject *kobj)
{
    struct demo_device *dev = demo_to_dev(kobj);

    if (dev->bus)
        return dev->bus->name;
    if (dev->class)
        return dev->class->name;
    return NULL;
}

static int demo_uevent(struct kset *kset, struct kobject *kobj,
           struct kobj_uevent_env *env)
{
    struct demo_device *dev = demo_to_dev(kobj);
    int retval = 0;

    /* add demo device node properties if present */
    if (MAJOR(dev->devt)) {
        const char *tmp;
        const char *name;
        umode_t mode = 0;

        add_uevent_var(env, "MAJOR=%u", MAJOR(dev->devt));
        add_uevent_var(env, "MINOR=%u", MINOR(dev->devt));
        name = demo_device_get_devnode(dev, &mode, &tmp);
        if (name) {
            add_uevent_var(env, "DEVNAME=%s", name);
            kfree(tmp);
            if (mode)
                add_uevent_var(env, "DEVMODE=%#o", mode & 0777);
        }
    }
    if (dev->type && dev->type->name)
        add_uevent_var(env, "DEVTYPE=%s", dev->type->name);

    if (dev->driver)
        add_uevent_var(env, "DRIVER=%s", dev->driver->name);

    /* Have the bus specific function add its stuff */
    if (dev->bus && dev->bus->uevent) {
        retval = dev->bus->uevent(dev, env);
        if (retval)
            printk(KERN_INFO "demo device: '%s': class uevent() "
                   "returned %d\n", demo_dev_name(dev), retval);
    }

    /* have the device type specific fcuntion add its stuff */
    if (dev->type && dev->type->uevent) {
        retval = dev->type->uevent(dev, env);
        if (retval)
            printk(KERN_INFO "demo device:'%s': dev_type uevent() "
                   "returned %d\n", demo_dev_name(dev), retval);
    }
    return retval;
}

/* free demo device struct */
static void demo_device_release(struct kobject *kobj)
{
    struct demo_device *dev = demo_to_dev(kobj);
    struct demo_device_private *p = dev->p;

    if (dev->release)
        dev->release(dev);
    else if (dev->type && dev->type->release)
        dev->type->release(dev);
    else if (dev->class && dev->class->dev_release)
        dev->class->dev_release(dev);
    else
        WARN(1, KERN_ERR "Demo Device '%s' doesn't have a release() "
             "function, it it broken and must be fixed.\n",
             demo_dev_name(dev));
    kfree(p);
}

/* demo device namespace */
static const void *demo_device_namespace(struct kobject *kobj)
{
    struct demo_device *dev = demo_to_dev(kobj);
    const void *ns = NULL;

    if (dev->class && dev->class->ns_type)
        ns = dev->class->namespace(dev);

    return ns;
}

/* common sysfs attribute show */
static ssize_t demo_dev_attr_show(struct kobject *kobj, struct attribute *attr,
                                  char *buf)
{
    struct demo_device_attribute *dev_attr = demo_to_attr(attr);
    struct demo_device *dev = demo_to_dev(kobj);
    ssize_t ret = -EIO;

    if (dev_attr->show)
        return dev_attr->show(dev, dev_attr, buf);
    if (ret >= (ssize_t)PAGE_SIZE) {
        printk(KERN_INFO "demo_dev_attr_show: return bad count.\n");
    }
    return ret;
}

/* common sysfs attribute store */
static ssize_t demo_dev_attr_store(struct kobject *kobj, struct attribute *attr,
                        const char *buf, size_t count)
{
    struct demo_device_attribute *dev_attr = demo_to_attr(attr);
    struct demo_device *dev = demo_to_dev(kobj);
    ssize_t ret = -EIO;

    if (dev_attr->store)
        ret = dev_attr->store(dev, dev_attr, buf, count);
     return ret;
}

static const struct sysfs_ops demo_dev_sysfs_ops = {
    .show      = demo_dev_attr_show,
    .store     = demo_dev_attr_store,
};

static struct kobj_type demo_device_ktype = {
    .release   = demo_device_release,
    .sysfs_ops = &demo_dev_sysfs_ops,
    .namespace = demo_device_namespace,
};

static const struct kset_uevent_ops demo_device_uevent_ops = {
    .filter    = demo_uevent_filter,
    .name      = demo_uevent_name,
    .uevent    = demo_uevent,
};

int __init demo_device_init(void)
{
    demo_devices_kset = kset_create_and_add("demo_devices", 
                        &demo_device_uevent_ops, NULL);
    if (!demo_devices_kset)
        return -ENOMEM;

    return 0;
}
