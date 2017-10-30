/*
 * device resource core demo code.
 *
 * (C) 2017.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>

#include "device.h"

struct demo_devres_node {
    struct list_head entry;
    demo_dr_release_t release;
};

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

/* Release function for devres group. These callbacks are used only
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
    int cnt= 0, nr_groups = 0;
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

static int demo_release_nodes(struct demo_device *dev, struct list_head *first,
                         struct list_head *end, unsigned long flags)
{
    LIST_HEAD(todo);
    int cnt;
    struct demo_devres *dr, *tmp;

    cnt = demo_remove_nodes(dev, first, end, &todo);

    /* Release. Note that both devres and devres_group are
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

    /* Looks like an uninitialized demo device structure */
    if (WARN_ON(dev->devres_head.next == NULL))
        return -ENODEV;
    return demo_release_nodes(dev, dev->devres_head.next, 
                             &dev->devres_head, flags);
}
