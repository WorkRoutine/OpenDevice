/*
 * bus demo code
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/string.h>

#include "demo_device.h"

/* display device name */
static ssize_t modalias_show(struct device *dev, struct device_attribute *a,
                             char *buf)
{
    struct demo_device *ddev = to_demo_device(dev);
    int len = snprintf(buf, PAGE_SIZE, "demo:%s\n", ddev->name);

    return (len >= PAGE_SIZE) ? (PAGE_SIZE - 1): len;
}

/* attribute group */
static struct device_attribute demo_attrs[] = {
    __ATTR_RO(modalias),
    __ATTR_NULL,
};

/* match id table */
static const struct demo_device_id *demo_match_id(const struct demo_device_id *id,
                    struct demo_device *ddev)
{
    while (id->name[0]) {
        if (strcmp(ddev->name, id->name) == 0) {
            ddev->id_entry = id;
            return id;
        }
        id++;
    }
    return NULL;
}

/* device match driver */
static int demo_match(struct device *dev, struct device_driver *drv)
{
    struct demo_device *ddev = to_demo_device(dev);
    struct demo_driver *ddrv = to_demo_driver(drv);

    /* try to match against the id table */
    if (ddrv->id_table)
        return demo_match_id(ddrv->id_table,ddev) != NULL;

    /* fall-back to driver name match */
    return strcmp(ddev->name, drv->name) == 0;
}

/* bus type match */
static struct bus_type demo_bus_type = {
    .name          = "demo_bus",
    .dev_attrs     = demo_attrs,
    .match         = demo_match,
};

/* root parent for demo bus */
static struct device demo_bus = {
    .init_name = "demo_bus",
};

/* Add a device into demo bus */
int demo_device_add(struct demo_device *ddev)
{
    int ret;

    if (!ddev)
        return -EINVAL;

    if (!ddev->dev.parent)
        ddev->dev.parent = &demo_bus;

    ddev->dev.bus = &demo_bus_type;

    switch (ddev->id) {
    default:
        dev_set_name(&ddev->dev, "%s.%d", ddev->name, ddev->id);
        break;
    case DEMO_DEVICE_NONE:
        dev_set_name(&ddev->dev, "%s", ddev->name);
        break;
    }
    
    printk(KERN_INFO "Register demo device '%s'. Parent at %s\n",
               dev_name(&ddev->dev), dev_name(ddev->dev.parent));

    ret = device_add(&ddev->dev);
    if (ret == 0)
        return ret;

    return ret;
}

/*
 * Remove a demo bus device
 */
void demo_device_del(struct demo_device *ddev)
{
    if (ddev)
        device_del(&ddev->dev);
}

/*
 * Destroy a demo device
 */
void demo_device_put(struct demo_device *ddev)
{
    if (ddev)
        put_device(&ddev->dev);
}

/*
 * Register a device into demo bus.
 */
int demo_device_register(struct demo_device *ddev)
{
    device_initialize(&ddev->dev);
    return demo_device_add(ddev);
}

/* 
 * Un-Register a device from demo bus 
 */
void demo_device_unregister(struct demo_device *ddev)
{
    demo_device_del(ddev);
    demo_device_put(ddev);
}

/* initialization entry */
static __init int demo_bus_init(void)
{
    int error;

    /* Register root device for demo bus. */
    error = device_register(&demo_bus);
    if (error) {
        printk(KERN_ERR "Unable to regiser demo bus.\n");
        return error;
    }

    /* Registe bus into /sys/bus */
    error = bus_register(&demo_bus_type);
    if (error) {
        printk(KERN_ERR "Unable to register bus.\n");
        goto out;
    }
    return 0;

/* error area */
out:
    device_unregister(&demo_bus);
    return error;
}
device_initcall(demo_bus_init);

/* exit entry */
static __exit void demo_bus_exit(void)
{
    device_unregister(&demo_bus);
    bus_unregister(&demo_bus_type);
}
