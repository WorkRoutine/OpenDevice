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

#include <linux/device.h>
#include <linux/string.h>

#define DEMO_NAME_SIZE     20

#define to_demo_device(x)  container_of((x),struct demo_device,dev)
#define to_demo_driver(x)  (container_of((x),struct demo_driver,driver))

/* id table */
struct demo_device_id 
{
    char name[DEMO_NAME_SIZE];
    unsigned long driver_data
        __attribute__((aligned(sizeof(unsigned long))));
};

/* private bus device */
struct demo_device 
{
    const char *name;
    struct device dev;
    const struct demo_device_id *id_entry;
};

/* private bus driver */
struct demo_driver
{
    int (*probe)(struct demo_device *);
    int (*remove)(struct demo_device *);
    void (*shutdown)(struct demo_device *);
    int (*suspend)(struct demo_device *);
    int (*resume)(struct demo_device *);
    struct device_driver driver;
    const struct demo_device_id *id_table;
};

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

static struct bus_type demo_bus_type = {
    .name          = "demo_bus",
    .dev_attrs     = demo_attrs,
    .match         = demo_match,
};

/* initialization entry */
static __init int demo_bus_init(void)
{
    int error;

    /* Registe bus into /sys/bus */
    error = bus_register(&demo_bus_type);
    if (error) {
        printk(KERN_ERR "Unable to register bus.\n");
        return error;
    }
    return 0;
}
device_initcall(demo_bus_init);

/* exit entry */
static __exit void demo_bus_exit(void)
{
    bus_unregister(&demo_bus_type);
}
