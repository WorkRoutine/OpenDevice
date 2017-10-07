Demo bus
-------------------------------------

The common templet for register a new virtual bus into system.

## File/Dirent list

  * base/

    This is basic routine to register a new virtual bus into system. It offer
    basic device function to register a new device into bus or to unregister
    a device from bus.

  * bus_driver/

    This is basic routine to register a new driver into bus or unregister
    a driver from bus.

  * bus_resource/

    This is basic routine to get resource from bus device.

## Core function

  * bus_register

    Register a specify bus into system. If succeed, the `sysfs` will create
    a new virtual bus on `/sys/bus/`

  * bus_unregister

    Un-Register a specify bus from system. If succeed, the `sysfs` will
    remove a virtual bus from `/sys/bus/`

## External link
