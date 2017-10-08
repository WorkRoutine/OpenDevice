BUS resource
----------------------------------------------

A virtual bus on `/sys/bus`, it contain basic function for device `register` 
and `unregister`. Addition, it offer get resource from bus device.

## File list

  * base.c

    This is core function for creating and registeing a new virutal bus.
    This file offer basic device function for registing and unregisting
    a bus device. Base on basic base.c, this version offer core driver
    function for registering and unregistering a driver. Addition, it 
    offer function for getting resource from bus device.

  * resource.c

    This is basic bus device file. It offset basic driver function,
    on driver, it offer `probe` and `remove` function to do real operation
    for driver. The core function is registering or unregistering a
    driver on bus. Addition, it can get resource from bus device.

  * demo_device.h

    Basic definition and data struction for demo bus.

## Core function

  * demo_get_resource

    Get resource from bus device.

  * demo_get_resource_byname

    Get resource from bus by name.

  * demo_driver_register

    Register a demo bus driver into bus. If succeed, the bus will build
    a driver node on `/sys/bus/drivers/xxx`.

  * demo_driver_unregister

    Un-Register a demo bus driver from bus. If succeed, the bus will 
    remove a driver node from `/sys/bus/drivers/xxx`

  * demo_device_register

    Register a demo bus device into bus. If succeed, the bus will build
    a device node on `/sys/bus/devices/xxx`

  * demo_device_unregister

    Un-Register a demo bus device from bus. If succeed, the bus will 
    remove a device node from `/sys/bus/devices/xxx` 

## External link 
