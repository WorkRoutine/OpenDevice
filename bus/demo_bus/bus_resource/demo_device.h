#ifndef _DEMO_DEVICE_H
#define _DEMO_DEVICE_H

#include <linux/device.h>

#define DEMO_NAME_SIZE     20
#define DEMO_DEVICE_NONE   (-1)

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
    int id;

    /* Resource */
    u32 num_resources;
    struct resource *resource;
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

static inline void *demo_get_drvdata(const struct demo_device *ddev)
{
    return dev_get_drvdata(&ddev->dev);
}

static inline void demo_set_drvdata(struct demo_device *ddev, void *data)
{
    dev_set_drvdata(&ddev->dev, data);
}

/* Register a device into demo bus */
extern int demo_device_register(struct demo_device *ddev);
/* Un-Register a device from demo bus */
extern void demo_device_unregister(struct demo_device *ddev);
/* Register a driver into demo bus */
extern int demo_driver_register(struct demo_driver *ddrv);
/* Unregister a driver from demo bus */
extern void demo_driver_unregister(struct demo_driver *ddrv);
/* get resource from bus by name */
extern struct resource *demo_get_resource_byname(struct demo_device *dev,
                 unsigned int type, const char *name);
/* get a resource for a device */
extern struct resource *demo_get_resource(struct demo_device *dev,
                 unsigned int type, unsigned int num);

#endif
