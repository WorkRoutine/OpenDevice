Device
-------------------------------------

Kobject for devices on `/sys/devices`


## File list

  * base.c

    Basic function to register device into sysfs, and create a dirent under 
    `/sys/devices`, it is subdir for `devices`

## Core function

  * device_register

    Register a device into device-subsystem. it will build a dirent on 
    `/sys/devices`

  * device_unregister

    Remove a device from device-subsystem. it will remove a dirent from 
    `/sys/devices`
