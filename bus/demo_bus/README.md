Demo bus
-------------------------------------

The common templet for register a new virtual bus into system.

## Filr/Dirent list

  * base/

    This basic routine to register a new virtual bus into system. It offer
    basic device function to register a new device into bus or to unregister
    a device from bus.

## Core function

  * bus_register

    Register a specify bus into system. If succeed, the `sysfs` will create
    a new virtual bus on `/sys/bus/`

  * bus_unregister

    Un-Register a specify bus from system. If succeed, the `sysfs` will
    remove a virtual bus from `/sys/bus/`

## External link
