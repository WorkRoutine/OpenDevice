kset
---------------------------------------


## File list

  * base.c

    Basic kset operation, the function invoke kset_create_and_add() to create a node 
    on `/sys/...`.

## Core function

  * kset_create_and_add

    Create a kset and add into `/sys`, it contain uevent operations.

  * kset_unregister

    Unregister a kset from sysfs. It will remove node from `/sys` 
