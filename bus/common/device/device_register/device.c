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

#include <linux/sysfs.h>
#include <linux/kdev_t.h>

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

/* init demo device structure */
void demo_device_initialize(struct demo_device *dev)
{
    dev->kobj.kset = demo_devices_kset;
    kobject_init(&dev->kobj, &demo_device_ktype);
    INIT_LIST_HEAD(&dev->devres_head);
    mutex_init(&dev->mutex);
    spin_lock_init(&dev->devres_lock);
    INIT_LIST_HEAD(&dev->devres_head);
    demo_set_dev_node(dev, -1);
}

/* 
 * increment reference count for device 
 * @dev: demo devicec
 *
 * This simply forwards the call to kobject_get(), though
 * we do take care to provide for the case that we get a NULL
 * pointer passed in.
 */
struct demo_device *demo_get_device(struct demo_device *dev)
{
    return dev ? demo_kobj_to_dev(kobject_get(&dev->kobj)) : NULL;
}

/* decrement reference count */
void demo_put_device(struct demo_device *dev)
{
    /* might sleep */
    if (dev)
        kobject_put(&dev->kobj);
}

static void demo_klist_children_get(struct klist_node *n)
{
    struct demo_device_private *p = demo_to_device_private_parent(n);
    struct demo_device *dev = p->device;

    demo_get_device(dev);
}

static void demo_klist_children_put(struct klist_node *n)
{
    struct demo_device_private *p = demo_to_device_private_parent(n);
    struct demo_device *dev = p->device;

    demo_put_device(dev);
}

/* initialize demo device private data */
int demo_device_private_init(struct demo_device *dev)
{
    dev->p = kzalloc(sizeof(*dev->p), GFP_KERNEL);
    if (!dev->p)
        return -ENOMEM;
    dev->p->device = dev;
    klist_init(&dev->p->klist_children, demo_klist_children_get,
               demo_klist_children_put);
    INIT_LIST_HEAD(&dev->p->deferred_probe);
    return 0;
}

/* set a device name */
int demo_dev_set_name(struct demo_device *dev, const char *fmt, ...)
{
    va_list vargs;
    int err;

    va_start(vargs, fmt);
    err = kobject_set_name_vargs(&dev->kobj, fmt, vargs);
    va_end(vargs);

    return err;
}

static struct kobject *demo_get_device_parent(struct demo_device *dev,
                       struct demo_device *parent)
{
    if (dev->class) {
        struct kobject *kobj = NULL;
        struct kobject *parent_kobj;
        struct kobject *k;

        /*
         * If we have no parent, we live in "virtual".
         * Class-devices with a non class-device as parent, live
         * in a "glue" direntory to prevent namespace collisions.
         */
        if (parent == NULL)
            parent_kobj = demo_virtual_device_parent(dev);
        else if (parent->class && !dev->class->ns_type)
            return &parent->kobj;
        else
            parent_kobj = &parent->kobj;

        /*
         * find our class-directory at the parent and reference it.
         */
        list_for_each_entry(k, &dev->class->p->glue_dirs.list, entry)
            if (k->parent == parent_kobj) {
                kobj = kobject_get(k);
                break;
            }
        if (kobj) {
            return kobj;
        }

        /* or create a new class-direntory at the parent device */
        k = demo_class_dir_create_and_add(dev->class, parent_kobj);
        return k;
    }
    /* subsystems can specify a default root direntory for their devices */
    if (!parent && dev->bus && dev->bus->dev_root)
        return &dev->bus->dev_root->kobj;

    if (parent)
        return &parent->kobj;
    return NULL;
}

/* create sysfs attribute file for demo device. */
int demo_device_create_file(struct demo_device *dev,
                            const struct demo_device_attribute *attr)
{
    int error = 0;

    if (dev) {
        WARN(((attr->attr.mode & S_IWUGO) && !attr->store),
             "Attribute %s: write permission without 'store'\n",
             attr->attr.name);
        WARN(((attr->attr.mode & S_IRUGO) && !attr->show),
             "Attribute %s: read permission without 'show'\n",
             attr->attr.name);
        error = sysfs_create_file(&dev->kobj, &attr->attr);
    }
    return error;
}

/* remove sysfs attribute file */
void demo_device_remove_file(struct demo_device *dev,
            const struct demo_device_attribute *attr)
{
    if (dev)
        sysfs_remove_file(&dev->kobj, &attr->attr);
}

/* select a /sys/demo_dev/ directory for the device */
static struct kobject *demo_device_to_dev_kobj(struct demo_device *dev)
{
    struct kobject *kobj;

    if (dev->class)
        kobj = dev->class->dev_kobj;
    else
        kobj = demo_sysfs_dev_char_kobj;

    return kobj;
}

static int demo_device_create_sys_dev_entry(struct demo_device *dev)
{
    struct kobject *kobj = demo_device_to_dev_kobj(dev);
    int error = 0;
    char devt_str[15];

    if (kobj) {
        format_dev_t(devt_str, dev->devt);
        error = sysfs_create_link(kobj, &dev->kobj, devt_str);
    }
    return error;
}

static inline int demo_device_is_not_partition(struct demo_device *dev)
{
    return 1;
}

static int demo_device_add_class_symlinks(struct demo_device *dev)
{
    int error;

    if (!dev->class)
        return 0;
    error = sysfs_create_link(&dev->kobj,
                  &dev->class->p->subsys.kobj, "demo_subsystem");
    if (error)
        goto out;

    /* Note! no verify */
    if (dev->parent && demo_device_is_not_partition(dev)) {
        error = sysfs_create_link(&dev->kobj, &dev->parent->kobj,
                                  "demo_device");
        if (error)
            goto out_subsys;
    }
    /* link in the class directory pointing to the device */
    error = sysfs_create_link(&dev->class->p->subsys.kobj,
                              &dev->kobj, demo_dev_name(dev));
    if (error)
        goto out_device;

out_device:
    sysfs_remove_link(&dev->kobj, "demo_device");
out_subsys:
    sysfs_remove_link(&dev->kobj, "demo_subsystem");
out:
    return error;
}

static int demo_device_add_attributes(struct demo_device *dev,
           struct demo_device_attribute *attrs)
{
    int error = 0;
    int i;

    if (attrs) {
        for (i = 0; attr_name(attrs[i]); i++) {
            error = demo_device_create_file(dev, &attrs[i]);
            if (error)
                break;
        }
        if (error)
            while (--i >= 0)
                demo_device_remove_file(dev, &attrs[i]);
    }
    return error;
}

static void demo_device_remove_attributes(struct demo_device *dev,
            struct demo_device_attribute *attrs)
{
    int i;

    if (attrs)
        for (i = 0; attr_name(attrs[i]); i++)
            demo_device_remove_file(dev, &attrs[i]);
}

/* create sysfs binary attribute file for device */
int demo_device_create_bin_file(struct demo_device *dev,
                const struct bin_attribute *attr)
{
    int error = -EINVAL;
    if (dev)
        error = sysfs_create_bin_file(&dev->kobj, attr);
    return error;
}

/* remove sysfs binary attribute file */
void demo_device_remove_bin_file(struct demo_device *dev,
                 const struct bin_attribute *attr)
{
    if (dev)
        sysfs_remove_bin_file(&dev->kobj, attr);
}

static int demo_device_add_bin_attributes(struct demo_device *dev,
           struct bin_attribute *attrs)
{
    int error = 0;
    int i;

    if (attrs) {
        for (i = 0; attr_name(attrs[i]); i++) {
            error = demo_device_create_bin_file(dev, &attrs[i]);
            if (error)
                break;
        }
        if (error)
            while (--i >= 0)
                demo_device_remove_bin_file(dev, &attrs[i]);
    }
    return error;
}

static void demo_device_remove_bin_attributes(struct demo_device *dev,
            struct bin_attribute *attrs)
{
    int i;

    if (attrs)
        for (i = 0; attr_name(attrs[i]); i++)
            demo_device_remove_bin_file(dev, &attrs[i]);
}

static int demo_device_add_groups(struct demo_device *dev,
                const struct attribute_group **groups)
{
    int error = 0;
    int i;

    /* Note! no verify */
    if (groups) {
        for (i = 0; groups[i]; i++) {
            error = sysfs_create_group(&dev->kobj, groups[i]);
            if (error) {
                while (--i >= 0)
                    sysfs_remove_group(&dev->kobj, groups[i]);
                break;
            }
        }
    }
    return error;
}

static void demo_device_remove_groups(struct demo_device *dev,
            const struct attribute_group **groups)
{
    int i;

    if (groups)
        for (i = 0; groups[i]; i++)
            sysfs_remove_group(&dev->kobj, groups[i]);
}

static int demo_device_add_attrs(struct demo_device *dev)
{
    struct demo_class *class = dev->class;
    const struct demo_device_type *type = dev->type;
    int error;

    /* Note! no verify */
    if (class) {
        error = demo_device_add_attributes(dev, class->dev_attrs);
        if (error)
            return error;
        error = demo_device_add_bin_attributes(dev, class->dev_bin_attrs);
        if (error)
            goto err_remove_class_attrs;
    }

    /* Note! no verify */
    if (type) {
        error = demo_device_add_groups(dev, type->groups);
        if (error)
            goto err_remove_class_bin_attrs;
    }

    error = demo_device_add_groups(dev, dev->groups);
    if (error)
        goto err_remove_type_groups;

    return 0;

err_remove_type_groups:
    if (type)
        demo_device_remove_groups(dev, type->groups);
err_remove_class_bin_attrs:
    if (class)
        demo_device_remove_bin_attributes(dev, class->dev_bin_attrs);
err_remove_class_attrs:
    if (class)
        demo_device_remove_attributes(dev, class->dev_attrs);

    return error;
} 

static ssize_t demo_show_dev(struct demo_device *dev, 
               struct demo_device_attribute *attr, char *buf)
{
    return print_dev_t(buf, dev->devt);
}

static struct demo_device_attribute demo_devt_attr =
    __ATTR(demo_dev, S_IRUGO, demo_show_dev, NULL);

static ssize_t demo_show_uevent(struct demo_device *dev,
               struct demo_device_attribute *attr, char *buf)
{
    struct kobject *top_kobj;
    struct kset *kset;
    struct kobj_uevent_env *env = NULL;
    int i;
    size_t count = 0;
    int retval;

    /* search the kset, the device belongs to */
    top_kobj = &dev->kobj;
    while (!top_kobj->kset && top_kobj->parent)
        top_kobj = top_kobj->parent;
    if (!top_kobj->kset)
        goto out;

    kset = top_kobj->kset;
    if (!kset->uevent_ops || !kset->uevent_ops->uevent)
        goto out;

    /* respect filter */
    if (kset->uevent_ops || !kset->uevent_ops->uevent)
        goto out;

    /* respect filter */
    if (kset->uevent_ops && kset->uevent_ops->filter)
        if (!kset->uevent_ops->filter(kset, &dev->kobj))
            goto out;

    env = kzalloc(sizeof(struct kobj_uevent_env), GFP_KERNEL);
    if (!env)
        return -ENOMEM;

    /* let the kset specify function add its keys */
    retval = kset->uevent_ops->uevent(kset, &dev->kobj, env);
    if (retval)
        goto out;

    /* copy keys to file */
    for (i = 0; i < env->envp_idx; i++)
        count += sprintf(&buf[count], "%s\n", env->envp[i]);
out:
    kfree(env);
    return count;
}

static ssize_t demo_store_uevent(struct demo_device *dev, 
       struct demo_device_attribute *attr, const char *buf, size_t count)
{
    enum kobject_action action;

    if (kobject_action_type(buf, count, &action) == 0)
        kobject_uevent(&dev->kobj, action);
    else
        printk(KERN_ERR "%s uevent: unknow action-string.\n",
                      demo_dev_name(dev));
    return count;
}

static struct demo_device_attribute demo_uevent_attr = 
     __ATTR(demo_uevent, S_IRUGO | S_IWUSR, 
            demo_show_uevent, demo_store_uevent);

static void demo_device_remove_attrs(struct demo_device *dev)
{
    struct demo_class *class = dev->class;
    const struct demo_device_type *type = dev->type;

    demo_device_remove_groups(dev, dev->groups);

    /* Note! no verify */
    if (type)
        demo_device_remove_groups(dev, type->groups);

    /* Note! no verify */
    if (class) {
        demo_device_remove_attributes(dev, class->dev_attrs);
        demo_device_remove_bin_attributes(dev, class->dev_bin_attrs);
    }
}

static void demo_cleanup_glue_dir(struct demo_device *dev, 
                                  struct kobject *glue_dir)
{
    /* see if we live in a 'glue' directory */
    if (!glue_dir || !dev->class ||
         glue_dir->kset != &dev->class->p->glue_dirs)
        return;

    kobject_put(glue_dir);
}

static void demo_cleanup_device_parent(struct demo_device *dev)
{
    demo_cleanup_glue_dir(dev, dev->kobj.parent);
}

static void demo_device_remove_sys_dev_entry(struct demo_device *dev)
{
    struct kobject *kobj = demo_device_to_dev_kobj(dev);
    char devt_str[15];

    if (kobj) {
        format_dev_t(devt_str, dev->devt);
        sysfs_remove_link(kobj, devt_str);
    }
}

static void demo_device_remove_class_symlinks(struct demo_device *dev)
{
    if (!dev->class)
        return;

    if (dev->parent && demo_device_is_not_partition(dev))
        sysfs_remove_link(&dev->kobj, "demo_device");
    sysfs_remove_link(&dev->kobj, "demo_subsystem");
    sysfs_delete_link(&dev->class->p->subsys.kobj, &dev->kobj,
                      demo_dev_name(dev));
}

/* add demo device to device hierarchy */
int demo_device_add(struct demo_device *dev)
{
    struct demo_device *parent = NULL;
    struct kobject *kobj;
    struct demo_class_interface *class_intf;
    int error = -EINVAL;

    dev = demo_get_device(dev);
    if (!dev)
        goto done;

    /* Note! demo device must clear when dynamic allocate. */
    if (!dev->p) {
        error = demo_device_private_init(dev);
        if (error)
            goto done;
    }

    /* for statically allocated devices, which should all be converted
     * some day, we need to initiailze the name. We prevent reading back
     * the name, and force the use of dev_name() */
    if (dev->init_name) {
        demo_dev_set_name(dev, "%s", dev->init_name);
        dev->init_name = NULL;
    }

    /* Note! non verify */
    /* subsystem can specify simple device enumeration */
    if (!demo_dev_name(dev) && dev->bus && dev->bus->dev_name)
        demo_dev_set_name(dev, "%s%u", dev->bus->dev_name, dev->id);

    if (!demo_dev_name(dev)) {
        error = -EINVAL;
        goto name_error;
    }
    printk("demo device '%s'\n", demo_dev_name(dev));

    parent = demo_get_device(dev->parent);
    kobj = demo_get_device_parent(dev, parent);
    if (kobj)
        dev->kobj.parent = kobj;

    /* use parent numa_node */
    if (parent)
        demo_set_dev_node(dev, demo_dev_to_node(parent));

    /* first, register with generic layer. */
    /* We require the name to be set before, and pass NULL */
    error = kobject_add(&dev->kobj, dev->kobj.parent, NULL);
    if (error)
        goto Error;

    error = demo_device_create_file(dev, &demo_uevent_attr);
    if (error)
        goto attrError;

    /* Note! no verify */
    if (MAJOR(dev->devt)) {
        error = demo_device_create_file(dev, &demo_devt_attr);
        if (error)
            goto ueventattrError;

        error = demo_device_create_sys_dev_entry(dev);
        if (error)
            goto devtattrError;
    }

    error = demo_device_add_class_symlinks(dev);
    if (error)
        goto SymlinkError;
    printk(KERN_INFO "'%s' create class link.\n", demo_dev_name(dev));

    error = demo_device_add_attrs(dev);
    if (error)
        goto AttrsError;
    error = demo_bus_add_device(dev);
    if (error)
        goto BusError;

    /* Note! no verify */
    /* Notify clients of device addition. This call must come
     * before kobject_uevent */
    if (dev->bus)
        blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
              DEMO_BUS_NOTIFY_ADD_DEVICE, dev);

    kobject_uevent(&dev->kobj, KOBJ_ADD);
    /* Note! non-verify */
    demo_bus_probe_device(dev); 
    /* Note! no verify */
    if (parent)
        klist_add_tail(&dev->p->knode_parent,
              &parent->p->klist_children);

    /* Note! no verify */
    if (dev->class) {
        /* tie the class to the device */
        klist_add_tail(&dev->knode_class,
                       &dev->class->p->klist_devices);
        /* notify any interfaces that the device is here */
        list_for_each_entry(class_intf,
                       &dev->class->p->interfaces, node)
            if (class_intf->add_dev)
                class_intf->add_dev(dev, class_intf);
    }
done:
    demo_put_device(dev);
    return error;
BusError:
    demo_device_remove_attrs(dev);
AttrsError:
    demo_device_remove_class_symlinks(dev);
SymlinkError:
    if (MAJOR(dev->devt))
        demo_device_remove_sys_dev_entry(dev);
devtattrError:
    if (MAJOR(dev->devt))
        demo_device_remove_file(dev, &demo_devt_attr);
ueventattrError:
    demo_device_remove_file(dev, &demo_uevent_attr);
attrError:
    kobject_uevent(&dev->kobj, KOBJ_REMOVE);
    kobject_del(&dev->kobj);
Error:
    demo_cleanup_device_parent(dev);
    if (parent)
        demo_put_device(parent);
name_error:
    kfree(dev->p);
    dev->p = NULL;
    goto done;
}

/* register a demo device with the system */
int demo_device_register(struct demo_device *dev)
{
    demo_device_initialize(dev);
    return demo_device_add(dev);
}

/* delete demo device from system */
void demo_device_del(struct demo_device *dev)
{
    struct demo_device *parent = dev->parent;
    struct demo_class_interface *class_intf;

    /* Note! no verify */
    /* Notify client of device removal. */
    if (dev->bus)
        blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
               DEMO_BUS_NOTIFY_DEL_DEVICE, dev);
    /* Note! no verify */
    if (parent)
        klist_del(&dev->p->knode_parent);
    /* Note! no verify */
    if (MAJOR(dev->devt)) {
        demo_device_remove_sys_dev_entry(dev);
        demo_device_remove_file(dev, &demo_devt_attr);
    }
    /* Note! no verify */
    if (dev->class) {
        demo_device_remove_class_symlinks(dev);

        /* notify any interface that the device is now gone. */
        list_for_each_entry(class_intf,
                 &dev->class->p->interfaces, node)
            if (class_intf->remove_dev)
                class_intf->remove_dev(dev, class_intf);
        /* remove the device from the class list */
        klist_del(&dev->knode_class);
    }
    demo_device_remove_file(dev, &demo_uevent_attr);
    demo_device_remove_attrs(dev);
    demo_bus_remove_device(dev);
    demo_driver_deferred_probe_del(dev);

    kobject_uevent(&dev->kobj, KOBJ_REMOVE);
    demo_cleanup_device_parent(dev);
    kobject_del(&dev->kobj);
    demo_put_device(parent);
}

/* unregister demo device from system */
void demo_device_unregister(struct demo_device *dev)
{
    printk(KERN_INFO "demo device: '%s'\n", demo_dev_name(dev));
    demo_device_del(dev);
    //demo_put_device(dev);
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
