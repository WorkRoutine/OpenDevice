/*
 * device subsystem demo code
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/list.h>

/* demo device */
struct demo_device {
    struct kobject kobj;
    struct demo_private *p;

    const char *init_name; /* initial name of the device */

    /* device resource list */
    struct list_head devres_head;

    void (*release)(struct demo_device *dev);
};

/* demo attribute */
struct demo_attribute {
    struct attribute attr;
    ssize_t (*show)(struct demo_device *dev, struct demo_attribute *attr,
                    char *buf);
    ssize_t (*store)(struct demo_device *dev, struct demo_attribute *attr,
                     const char *buf, size_t count);
};

/* demo device private */
struct demo_private {
    void *demo_data;
};

/* demo device resource management */
typedef void (*demo_dr_release_t)(struct demo_device *dev, void *res);
typedef int (*demo_dr_match_t)(struct demo_device *dev, void *res, 
                          void *match_data);

/* demo device resource node */
struct demo_devres_node {
    struct list_head entry;
    demo_dr_release_t release;
};

/* demo device resource */
struct demo_devres {
    struct demo_devres_node node;
    /* -- 3 pointers */
    unsigned long long data[]; /* guarantee ull alignment */
};

struct demo_devres_group {
    struct demo_devres_node node[2];
    void *id;
    int color;
    /* -- 8 pointers */
};

static struct kobject root;

/* /sys/demo */
static struct kset *demo_kset;

#define to_dev_attr(_attr) container_of(_attr, struct demo_attribute, attr)

/* cover kobject to demo device */
static inline struct demo_device *kobj_to_dev(struct kobject *kobj)
{
    return container_of(kobj, struct demo_device, kobj);
}

static inline const char *demo_dev_name(const struct demo_device *dev)
{
    /* Use the init name until the kobject becomes available */
    if (dev->init_name)
        return dev->init_name;

    return kobject_name(&dev->kobj);
}

/*
 * Release functions for demo devres group. These callbacks are used only
 * for identification.
 */
static void demo_group_open_release(struct demo_device *dev, void *res)
{
    /* noop */
}

static void demo_group_close_release(struct demo_device *dev, void *res)
{
    /* noop */
}

static struct demo_devres_group *demo_node_to_group(
              struct demo_devres_node *node)
{
    if (node->release == &demo_group_open_release)
        return container_of(node, struct demo_devres_group, node[0]);
    if (node->release == &demo_group_close_release)
        return container_of(node, struct demo_devres_group, node[1]);
    return NULL;
}

static int demo_remove_nodes(struct demo_device *dev,
           struct list_head *first, struct list_head *end,
           struct list_head *todo)
{
    int cnt = 0, nr_groups = 0;
    struct list_head *cur;

    /* First pass - move normal devres entries to @todo and clear
     * devres_group colors. 
     */
    cur = first;
    while (cur != end) {
        struct demo_devres_node *node;
        struct demo_devres_group *grp;

        node = list_entry(cur, struct demo_devres_node, entry);
        cur = cur->next;

        grp = demo_node_to_group(node);
        if (grp) {
            /* clear color of group markers in the first pass */
            grp->color = 0;
            nr_groups++;
        } else {
            /* regular devres entry */
            if (&node->entry == first)
                first = first->next;
            list_move_tail(&node->entry, todo);
            cnt++;
        }
    }

    if (!nr_groups)
        return cnt;

    /* Second pass - Scan groups and color them. A group gets
     * color value of two iff the group is wholly contained in
     * [cur, end]. That is, for a closed group, both opening and
     * closing markers should be in the range, while just the 
     * opening marker is enough for an open group.
     */
    cur = first;
    while (cur != end) {
        struct demo_devres_node *node;
        struct demo_devres_group *grp;

        node = list_entry(cur, struct demo_devres_node, entry);
        cur = cur->next;

        grp = demo_node_to_group(node);
        BUG_ON(!grp || list_empty(&grp->node[0].entry));

        grp->color++;
        if (list_empty(&grp->node[1].entry))
            grp->color++;

        BUG_ON(grp->color <= 0 || grp->color > 2);
        if (grp->color == 2) {
            /* No need to update cur or end. The removed
             * nodes are always before both.
             */
            list_move_tail(&grp->node[0].entry, todo);
            list_del_init(&grp->node[1].entry);
        }
    }
    return cnt;
}

/* release device resource */
static int demo_release_nodes(struct demo_device *dev, 
           struct list_head *first, struct list_head *end, unsigned long flags)
{
    LIST_HEAD(todo);
    int cnt;
    struct demo_devres *dr, *tmp;

    cnt = demo_remove_nodes(dev, first, end, &todo);
    
    /* Release. Note that both demo_devres and demo_devres_group are
     * handled as devres in the following loop. This is safe.
     */
    list_for_each_entry_safe_reverse(dr, tmp, &todo, node.entry) {
        dr->node.release(dev, dr->data);
        kfree(dr);
    }
    return cnt;
}

/* Release all managed resources */
int demo_devres_release_all(struct demo_device *dev)
{
    unsigned long flags;

    /* Looks like an uninitialized device structure */
    if (WARN_ON(dev->devres_head.next == NULL))
        return -ENODEV;

    return demo_release_nodes(dev, dev->devres_head.next, 
                              &dev->devres_head, flags);
}

/* free demo device structure. */
static void demo_release(struct kobject *kobj)
{
    struct demo_device *dev = kobj_to_dev(kobj);
    struct demo_private *p = dev->p;

    demo_devres_release_all(dev);

    if (dev->release)
        dev->release(dev);
    else
        WARN(1, KERN_ERR "Demo device '%s' does not have a release() "
                "function, it is broken and must be fixed.\n",
                demo_dev_name(dev));
    kfree(p);
}

static const void *demo_namespace(struct kobject *kobj)
{
    struct demo_device *dev = kobj_to_dev(kobj);
    const void *ns = NULL;

    return ns;
}

/* demo device show */
static ssize_t demo_attr_show(struct kobject *kobj, struct attribute *attr,
               char *buf)
{
    struct demo_attribute *demo_attr = to_dev_attr(attr);
    struct demo_device *dev = kobj_to_dev(kobj);
    ssize_t ret = -EIO;

    if (demo_attr->show)
        return ret = demo_attr->show(dev, demo_attr, buf);
    return ret;
}

/* demo device store */
static ssize_t demo_attr_store(struct kobject *kobj, struct attribute *attr,
                               const char *buf, size_t count)
{
    struct demo_attribute *demo_attr = to_dev_attr(attr);
    struct demo_device *dev = kobj_to_dev(kobj);
    ssize_t ret = -EIO;

    if (demo_attr->store)
        ret = demo_attr->store(dev, demo_attr, buf, count);
    return ret;
}

/* attribute for sysfs */
static const struct sysfs_ops demo_sysfs_ops = {
    .show  = demo_attr_show,
    .store = demo_attr_store,
};

/* ktype for /sys/devices */
static struct kobj_type demo_ktype = {
    .release     = demo_release,
    .sysfs_ops   = &demo_sysfs_ops,
    .namespace   = demo_namespace,
}; 

/* kset uevent filter */
static int demo_uevent_filter(struct kset *kset, struct kobject *kobj)
{
    struct kobj_type *ktype = get_ktype(kobj);

    if (ktype == &demo_ktype) {
        struct demo_device *dev = kobj_to_dev(kobj);
        /* no bus and class */
        return 0;
    }
    return 0;
}

/* kset uevent name */
static const char *demo_uevent_name(struct kset *kset, struct kobject *kobj)
{
    struct demo_device *dev = kobj_to_dev(kobj);
    /* no bus or class */
    return NULL;
}

static int demo_uevent(struct kset *kset, struct kobject *kobj,
                       struct kobj_uevent_env *env)
{
    struct demo_device *dev = kobj_to_dev(kobj);
    int retval = 0;

    /* add demo device node proiperities present */
    return 0;
}

static const struct kset_uevent_ops demo_uevent_ops = {
    .filter    = demo_uevent_filter,
    .name      = demo_uevent_name,
    .uevent    = demo_uevent,
};

/* initiailization entry */
static __init int demo_kobject_init(void)
{
    /* initialize kobject */
    demo_kset = kset_create_and_add("demo", &demo_uevent_ops, NULL);

    if (!demo_kset) {
        printk(KERN_ERR "Unable to create kset.\n");
        return -ENOMEM;
    }

    root.kset = demo_kset;
    kobject_init(&root, &demo_ktype);
    printk("Demo initialize all\n");
    return 0;
}
device_initcall(demo_kobject_init);

/* exit entry */
static __exit void demo_kobject_exit(void)
{
}
