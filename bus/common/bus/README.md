BUS
----------------------------------------------


## File list

  * base.c

    Create a new virtual bus on `sysfs`, it contains simple device match drvier. Define 
    bus device and bus driver, it base on `struct device` and `struct device_driver`. 
    On driver, it contains `probe`, `remove`, `suspend`, `shutdown` and `id_table`.

## Core function

  * bus_register

    Register a new virtual bus into sysfs.

  * bus_unregister

    Un-Register a virtual bus and remove from `/sys`.

## External link
