/*
 * hlist demo code.
 *
 * (C) 2017.09.06 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>

/* header of hlist */
#include <linux/list.h>
#include <linux/hash.h>


#define HASHTABLE_SIZE  16
#define TEST_NR         32

extern void get_random_bytes(void *buf, int nbytes);

/* define private data */
struct demo_node {
    struct hlist_node node;
    int nr;
};

/* define a hash table */
static struct hlist_head hashtable[HASHTABLE_SIZE];

/* 
 * Calculate hash value.
 */
static int hash(struct demo_node *node)
{
    return hash_long(node->nr, 10) % HASHTABLE_SIZE;
}

/*
 * Test data initizlie.
 */
static int running_demo(void)
{
    int i;

    for (i = 0; i < TEST_NR; i++) {
        /* in order to test non-free node. */
        struct demo_node *node = (struct demo_node *)kmalloc(
                    sizeof(*node), GFP_KERNEL);
        if (!node)
            return -ENOMEM;
        /* get random from system */
        get_random_bytes(&node->nr, sizeof(int));
        /* calculate hash value and add into hash table */
        printk("node[%d]-> hash [%d]\n", node->nr, hash(node));
        hlist_add_head(&node->node, &hashtable[hash(node)]);
    }
    return 0;
}


static __init int _hlist_demo_init(void)
{
    int i;
    struct demo_node *node;

    printk("Hlist demo initialize.\n");

    /* running test demo */
    running_demo();

    /* loop all node */
    for (i = 0; i < HASHTABLE_SIZE; i++) {
        printk("Hash Table[%d]: ", i);
        hlist_for_each_entry(node, &hashtable[i], node)    
            printk("-> %6d", node->nr);
        printk("\n");
    }

    return 0;
}
device_initcall(_hlist_demo_init);
