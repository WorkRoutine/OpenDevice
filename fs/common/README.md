VFS
-------------------------------------------

A virtual file system (VFS) or virtual filesystem switch is an abstraction 
layer on top of a more concrete file system. The purpose of a VFS is to 
allow client applications to access different types of concrete file systems 
in a uniform way. A VFS can, for example, be used to access local and 
network storage devices transparently without the client application 
noticing the difference. It can be used to bridge the differences in 
Windows, classic Mac OS/macOS and Unix filesystems, so that applications 
can access files on local file systems of those types without having to 
know what type of file system they are accessing.

A VFS specifies an interface (or a "contract") between the kernel and a 
concrete file system. Therefore, it is easy to add support for new file 
system types to the kernel simply by fulfilling the contract. The terms 
of the contract might change incompatibly from release to release, which 
would require that concrete file system support be recompiled, and possibly 
modified before recompilation, to allow it to work with a new release of 
the operating system; or the supplier of the operating system might make 
only backward-compatible changes to the contract, so that concrete file 
system support built for a given release of the operating system would 
work with future versions of the operating system

## File list

  * fs_register_filesystem.c

    Basic way to register a file system into VFS.

## Core function

  * register_filesystem

    Register a new file system into VFS.

  * unregister_filesystem

    Un-register a file system from VFS.

## External link
