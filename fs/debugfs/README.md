Debugfs
---------------------------------------

debugfs is a special file system available in the Linux kernel since version 2.6.10-rc3. It was written by Greg Kroah-Hartman. debugfs is a simple-to-use RAM-based file system specially designed for debugging purposes. It exists as a simple way for kernel developers to make information available to user space. Unlike /proc, which is only meant for information about a process, or sysfs, which has strict one-value-per-file rules, debugfs has no rules at all. Developers can put any information they want there.

## File list

  * base.c

    Base on debugfs, which utilize `debugfs_create_dir` to create debugfs root
    and `debugfs_create_file` to create file node on debugfs.

  * Debugfs_subdir.c

    Create mulit subdir on root dirent or sub-dirent. utilize 
    `debugfs_create_dir` and pass dirent into this function. It will create
    a subdir on `/sys/kernel/debug/xxx/subdir`.

  * Debugfs_uvalue.c

    Create different size value on debugfs dirent, we can use `echo` or 
    `cat` to read and write this value on device driver.

  * Debugfs_xvalue.c

    Create different size value in hex on debugfs dirent, we can use 
    `echo` or `cat` to read or write this value on device driver.

## Core function

  * debugfs_create_dir

    This function will create a dirent on `/sys/kernel/debug/`. When invoked, it
    will return a `struct dentry`, this struction hold the dirent information,
    program will set it as root dirent for debugfs. On the tree, we can create
    more file under root dirent.

  * debugfs_create_file

    This function will create a file on debugfs dirent, so we shuold pass 
    debugfs dirent,file opermission and file operations into function. On 
    file operation, we can offer more file operations contains `read`, `write` 
    and so on.

  * debugfs_create_ux

    This function will create a file on debugfs dirent, this file will link
    to a value on device driver. we can directly to utilize `echo` or `cat`
    to read and write this vlaue. On this function family, x contains 
    `8,16,32,64`.

  * debugfs_create_xx

    This function will create a file on debugfs dirent, this file will link
    to a value in hex on device driver. we can dirently to utilize `echo` or
    `cat` to read and write this value. On this function family, x contains
    `8,16,32,64`

## More information

  Usefull external linker
