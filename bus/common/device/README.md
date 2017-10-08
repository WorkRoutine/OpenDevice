Device
-------------------------------------

The device sub-system, it base on `struct device` and `struct device_driver`,
This is core data structure for `bus`, `class` and `pm` and so on.


## File list

  * base.c

    Basic function to register device into sysfs, and create a dirent under 
    `/sys/devices`, it is subdir for `devices`

  * device.c

    The device sub-system base on `kset`, `kobject` and `ktype`. The core
    routine will build a dirent on `/sys/`, it will be root `kobject`
    and `kset` for different demo device.

## Core function

  * device_register

    Register a device into device-subsystem. it will build a dirent on 
    `/sys/devices`

  * device_unregister

    Remove a device from device-subsystem. it will remove a dirent from 
    `/sys/devices`
