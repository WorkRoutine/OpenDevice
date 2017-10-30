/*
 * driver core demo code
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>

#include "device.h"

static LIST_HEAD(demo_deferred_probe_pending_list);
static LIST_HEAD(demo_deferred_probe_active_list);
static struct workqueue_struct *demo_deferred_wq;
static atomic_t demo_deferred_trigger_count = ATOMIC_INIT(0);
static bool demo_driver_deferred_probe_enable = false;

static atomic_t demo_probe_count = ATOMIC_INIT(0);
static DECLARE_WAIT_QUEUE_HEAD(demo_probe_waitqueue);

static int demo_driver_sysfs_add(struct demo_device *dev)
{
    int ret;

    if (dev->bus)
        blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
                DEMO_BUS_NOTIFY_BIND_DRIVER, dev);
    ret = sysfs_create_link(&dev->driver->p->kobj, &dev->kobj,
                kobject_name(&dev->kobj));
    if (ret == 0) {
        ret = sysfs_create_link(&dev->kobj, &dev->driver->p->kobj,
              "demo_driver");
        if (ret)
            sysfs_remove_link(&dev->driver->p->kobj,
               kobject_name(&dev->kobj));
    }
    return 0;
}

static void demo_driver_sysfs_remove(struct demo_device *dev)
{
    struct demo_driver *drv = dev->driver;

    if (drv) {
        sysfs_remove_link(&drv->p->kobj, kobject_name(&dev->kobj));
        sysfs_remove_link(&dev->kobj, "demo_driver");
    }
}

void demo_driver_deferred_probe_del(struct demo_device *dev)
{
    if (!list_empty(&dev->p->deferred_probe)) {
        printk(KERN_DEBUG "Removed from deferred list\n");
        list_del_init(&dev->p->deferred_probe);
    }
}

/* Retry probing devices in the active list */
static void demo_deferred_probe_work_func(struct work_struct *work)
{
    struct demo_device *dev;
    struct demo_device_private *private;

    /*
     * This block processes every device in the deferred 'active' list.
     * Each device is removed from the active list and passed to
     * demo_bus_probe_device() to re-attempt the probe. The loop continues
     * until every device in the active list is removed and retried.
     *
     * Note: Once the device is removed from the list and the mutex is 
     * released, it is possible for the device get freed by another thread
     * and cause a illegal pointer dereference. This code uses
     * get/put_device() to ensurce the device structure cnanot disappera
     * from under our feet.
     */
     while (!list_empty(&demo_deferred_probe_active_list)) {
         private = list_first_entry(&demo_deferred_probe_active_list,
                   typeof(*dev->p), deferred_probe);
         dev = private->device;
         list_del_init(&private->deferred_probe);

         demo_get_device(dev);

         demo_bus_probe_device(dev);
         demo_put_device(dev);
     }
}

static DECLARE_WORK(demo_deferred_probe_work,
       demo_deferred_probe_work_func);

/* kick off re-probing deferred devices */
static void demo_driver_deferred_probe_trigger(void)
{
    if (!demo_driver_deferred_probe_enable)
        return;

    /*
     * A successful probe means that all the devices in the pending list
     * should be triggered to be reprobed. Move all the deferred devices
     * into the active list so they can be retried by the workqueue.
     */
    atomic_inc(&demo_deferred_trigger_count);
    list_splice_tail_init(&demo_deferred_probe_pending_list,
                          &demo_deferred_probe_active_list);
    queue_work(demo_deferred_wq, &demo_deferred_probe_work);
}

static void demo_driver_bound(struct demo_device *dev)
{
    if (klist_node_attached(&dev->p->knode_driver)) {
        printk(KERN_WARNING "%s: device %s already bound\n",
               __func__, kobject_name(&dev->kobj));
        return;
    }
    printk(KERN_INFO "driver: '%s': %s: bound to device '%s'\n", 
           demo_dev_name(dev), __func__, dev->driver->name);

    klist_add_tail(&dev->p->knode_driver, &dev->driver->p->klist_devices);

    /*
     * Make sure the demo device is no longer in one of the deferred lists
     * and kick off retrying all pending devices.
     */
    demo_driver_deferred_probe_del(dev);
    demo_driver_deferred_probe_trigger();

    if (dev->bus)
        blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
             DEMO_BUS_NOTIFY_BOUND_DRIVER, dev);
}

/* bind a demo driver to one demo device */
int demo_device_bind_driver(struct demo_device *dev)
{
    int ret;

    ret = demo_driver_sysfs_add(dev);
    if (!ret)
        demo_driver_bound(dev);
    return ret;
}

/* Enable probing of deferred devices */
static int demo_deferred_probe_initcall(void)
{
    demo_deferred_wq = create_singlethread_workqueue("demo_deferwq");
    if (WARN_ON(!demo_deferred_wq))
        return -ENOMEM;

    demo_driver_deferred_probe_enable = true;
    demo_driver_deferred_probe_trigger();
    /* Sort as many dependencies as possible before exiting initcalls */
    flush_workqueue(demo_deferred_wq);
    return 0;
}
late_initcall(demo_deferred_probe_initcall);

int demo_dev_set_drvdata(struct demo_device *dev, void *data)
{
    int error;

    if (!dev->p) {
        error = demo_device_private_init(dev);
        if (error)
            return error;
    }
    dev->p->driver_data = data;
    return 0;
}

static void demo_driver_deferred_probe_add(struct demo_device *dev)
{
    if (list_empty(&dev->p->deferred_probe)) {
        printk(KERN_DEBUG "Added %s to deferred list.\n", demo_dev_name(dev));
        list_add_tail(&dev->p->deferred_probe, 
                      &demo_deferred_probe_pending_list);
    }
}

static int demo_really_probe(struct demo_device *dev, struct demo_driver *drv)
{
    int ret = 0;
    int local_trigger_count = atomic_read(&demo_deferred_trigger_count);

    atomic_inc(&demo_probe_count);
    printk("bus'%s':%s:probing driver %s with device %s\n",
            drv->bus->name, __func__, drv->name, demo_dev_name(dev));
    WARN_ON(!list_empty(&dev->devres_head));

    dev->driver = drv;

    /* If using pinctrl, bind pins now before probing */
    if (demo_driver_sysfs_add(dev)) {
        printk(KERN_ERR "%s: driver_sysfs_add(%s) failed\n",
               __func__, demo_dev_name(dev));
        goto probe_failed;
    }

    if (dev->bus->probe) {
        ret = dev->bus->probe(dev);
        if (ret)
            goto probe_failed;
    } else if (drv->probe) {
        ret = drv->probe(dev);
        if (ret)
            goto probe_failed;
    }
    demo_driver_bound(dev);
    ret = 1;
    printk(KERN_INFO "bus:'%s': %s:bound device %s to driver %s\n",
           drv->bus->name, __func__, demo_dev_name(dev), drv->name);
    goto done;

probe_failed:
    demo_devres_release_all(dev);
    demo_driver_sysfs_remove(dev);
    dev->driver = NULL;
    demo_dev_set_drvdata(dev, NULL);

    if (ret == -EPROBE_DEFER) {
        /* Driver requested deferred probing */
        printk(KERN_INFO "Driver %s requests probe deferral.\n", drv->name);
        demo_driver_deferred_probe_add(dev);
        /* Did a trigger occur while probing? Need to re-trigger if yes */
        if (local_trigger_count != atomic_read(&demo_deferred_trigger_count))
            demo_driver_deferred_probe_trigger();
    } else if (ret != -ENODEV && ret != -ENXIO) {
        /* driver matched but the probe failed */
        printk(KERN_WARNING "%s: probe of %s failed with error %d\n",
               drv->name, demo_dev_name(dev), ret);
    } else {
        printk(KERN_INFO "%s: probe of %s rejects match %d\n",
               drv->name, demo_dev_name(dev), ret);
    }
    /*
     * Ignore errors returned by->probe so that the next driver can try
     * its luck.
     */
    ret = 0;
done:
    atomic_dec(&demo_probe_count);
    wake_up(&demo_probe_waitqueue);
    return ret;
}

/* attempt to bind demo device & driver together */
int demo_driver_probe_device(struct demo_driver *drv, struct demo_device *dev)
{
    int ret = 0;

    if (!demo_device_is_registered(dev))
        return -ENODEV;

    printk(KERN_INFO "demo bus:'%s':%s: matched device %s with driver %s\n",
           drv->bus->name, __func__, demo_dev_name(dev), drv->name);
    ret = demo_really_probe(dev, drv);
    return ret;
}

static int __demo_device_attach(struct demo_driver *drv, void *data)
{
    struct demo_device *dev = data;

    if (!demo_driver_match_device(drv, dev))
        return 0;

    return demo_driver_probe_device(drv, dev);
}

/* try to attach demo device to a demo driver */
int demo_device_attach(struct demo_device *dev)
{
    int ret = 0;

    if (dev->driver) {
        if (klist_node_attached(&dev->p->knode_driver)) {
            ret = 1;
        }
        ret = demo_device_bind_driver(dev);
        if (ret == 0)
            ret = 1;
        else {
            dev->driver = NULL;
            ret = 0;
        }
    } else {
        ret = demo_bus_for_each_drv(dev->bus, NULL, dev, __demo_device_attach);
    }
    return ret;
}

static void __demo_device_release_driver(struct demo_device *dev)
{
    struct demo_driver *drv;

    drv = dev->driver;
    if (drv) {
        demo_driver_sysfs_remove(dev);
        if (dev->bus)
            blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
               DEMO_BUS_NOTIFY_UNBIND_DRIVER, dev);

        if (dev->bus && dev->bus->remove)
            dev->bus->remove(dev);
        else if (drv->remove)
            drv->remove(dev);
        demo_devres_release_all(dev);
        dev->driver = NULL;
        demo_dev_set_drvdata(dev, NULL);
        klist_remove(&dev->p->knode_driver);
        if (dev->bus)
            blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
                DEMO_BUS_NOTIFY_UNBOUND_DRIVER, dev); 
    }
}

/* manually detach device from driver */
void demo_device_release_driver(struct demo_device *dev)
{
    __demo_device_release_driver(dev);
}
