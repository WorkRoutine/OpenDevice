Procfs
---------------------------------------

The proc filesystem (procfs) is a special filesystem in Unix-like operating 
systems that presents information about processes and other system 
information in a hierarchical file-like structure, providing a more 
convenient and standardized method for dynamically accessing process 
data held in the kernel than traditional tracing methods or direct access 
to kernel memory. Typically, it is mapped to a mount point named `/proc` 
at boot time. The proc file system acts as an interface to internal 
data structures in the kernel. It can be used to obtain information 
about the system and to change certain kernel parameters at runtime (sysctl).

Many Unix-like operating systems support the proc filesystem, including 
Solaris, IRIX, Tru64 UNIX, BSD, Linux, IBM AIX, QNX, and Plan 9 from 
Bell Labs. The Linux kernel extends it to non–process-related data. The 
proc filesystem provides a method of communication between kernel space 
and user space. For example, the GNU version of the process reporting 
utility ps uses the proc file system to obtain its data, without using 
any specialized system calls.

## File list

  * base.c

    Base on procfs, which utilize `proc_mkdir` to create procfs root
    and `remove_proc_entry` to remove dirent from `/proc`.

  * Procfs_subdir.c

    Build different sub-dirent, contain same layer and sub-layer.

## Core function

  * proc_mkdir

    This function will create a proc dirent on specify dirent. and then,
    we can find the node on `/proc`. More information see `base.c`

  * remove_proc_entry

    This function will remove a dirent from specify proc dirent. More 
    information see `base.c`

## More information

  Usefull external linker
