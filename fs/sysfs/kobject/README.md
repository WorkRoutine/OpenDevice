Kobject
-------------------------------------------



## File list

  * base.c

    Basic function to create a dentry on `/sys`.

  * kobject_multi_subdir.c

    Build multi dentry on root or sub dirent.

## Core function

  * kobject_create_and_add

    Create a dentry on `/sys`. we can set parent dentry and dentry name.

  * kobject_put

    Rlease a kobject and remove specify dentry from `/sys`
