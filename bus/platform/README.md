Platform device driver
------------------------------------------

Platform devices are devices that typically appear as autonomous entities in the system. 
This includes legacy port-based devices and host bridges to peripheral buses, and most 
controllers integrated into system-on-chip platforms.  What they usually have in common 
is direct addressing from a CPU bus.  Rarely, a platform_device will be connected through 
a segment of some other kind of bus; but its registers will still be directly addressable.

## File list

  * base.c

    Basic create a device on platform bus, it can match a driver with same device name. 
    When it match specify name, the probe of driver will be invoked.

  * platform_resource.c

    Base on base.c, it add platform resource that contain io memory, irq, clk and so on.
    It can get resource information from platform bus and request from system as specify
    device private data.

## Core function

  * platform_device_register

    Register a device onto platform bus.

  * platform_device_unregister

    Un-Register a device from platform bus.

  * platform_driver_register

    Register a driver into platform bus, it will be probe when platform bus match the same 
    name whit specify device name.

  * platform_driver_unregister

    Un-Register a driver from platform bus.

  * platform_get_resource_byname

    Get resource from platform bus with specify name.

  * request_mem_region

    Request specify memory region as private data on device.

  * platform_get_irq_byname

    Get irq information from platform bus.

  * release_mem_region

    Release a memory region to system.

  * ioremap

    Mapping the physical address as virtual address to by accessed from VM.

  * iounmap

    Cansole the mapping to virual memory.

## External link
