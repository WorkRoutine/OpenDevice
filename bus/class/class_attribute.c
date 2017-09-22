/*
 * class attribute demo
 *
 * (C) 2017.09 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/module.h>
#include <linux/device.h>

#define CLASS_NAME  "demo_class"
int demo_value = 500;

/* class attribute: show on userspace */
static ssize_t demo_attr_show(struct class *class,
               struct class_attribute *attr, char *buf)
{
    /* copy data to buffer */
    return sprintf(buf, "%d", demo_value);
}

/* class attribute: store to device */
static ssize_t demo_attr_store(struct class *class,
               struct class_attribute *attr, char *buf, size_t size)
{
    /* get data from userspace */
    sscanf(buf, "%d", &demo_value);
    return size;
}

/* attribute struct interface*/
static struct class_attribute demo_class_attr[] = {
    __ATTR(demo_attr, S_IWUSR | S_IRUGO, demo_attr_show, demo_attr_store),
    __ATTR_NULL,
};

/* struct class */
static struct class demo_class = {
    .name = CLASS_NAME,
    .class_attrs = demo_class_attr,
};

/* Initialize entry */
static __init int demo_class_init(void)
{
    /* Register class */
    class_register(&demo_class);
    return 0;
}
device_initcall(demo_class_init);

/* exit entry */
static __exit void demo_class_exit(void)
{
    /* un-register class */
    class_unregister(&demo_class);
}
