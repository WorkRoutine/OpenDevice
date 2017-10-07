DEMO BUS
----------------------------------------------

A virtual bus on `/sys/bus`, it contain basic function for device `register` 
and `unregister`.

## File list

  * base.c

    This is core function for creating and registeing a new virutal bus.
    This file offer basic device function for registing and unregisting
    a bus device.

  * device.c

    This is basic bus device file. It offset basic device function, 
    such as `register` and `unregister`. When a new bus device register
    into bus, the bus will create a new dirent on 
    `/sys/bus/demo_bus/devices`. You can unregister a device from bus
    via `unregister`.

  * demo_device.h

    Basic definition and data struction for demo bus.

## Core function

  * demo_device_register

    Register a demo bus device into bus. If succeed, the bus will build
    a device node on `/sys/bus/devices/xxx`

  * demo_device_unregister

    Un-Register a demo bus device from bus. If succeed, the bus will 
    remove a device node from `/sys/bus/devices/xxx` 

## External link 
