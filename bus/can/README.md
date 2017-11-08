CAN: Controller Area Network
------------------------------------------------------------

![can timing](https://github.com/EmulateSpace/PictureSet/blob/master/github/cantimg.jpeg)

A Controller Area Network (CAN bus) is a robust vehicle bus standard designed to allow 
microcontrollers and devices to communicate with each other in applications without a host 
computer. It is a message-based protocol, designed originally for multiplex electrical 
wiring within automobiles to save on copper, but is also used in many other contexts.

## File List

  * Simple_can_device.c

    Basic and simple can device driver. the file contains how to register a new can device
    into CAN Bus. And it has offset how to un-register a can device from CAN Bus.

  * can_device_parse_can_frame.c

    Full function to introduce how to delivered can frame on Can device driver. this is a
    useful and individual can device that send and receive can frames from CAN Bus.

## Core Function

  * alloc_candev

    Allocate a new can device struction for setting specify device information. This 
    struction belong to NET system, so net device is part of your struction.

  * free_candev

    This function will free a can device from CAN core. Note, set it NULL after free it.

  * register_candev

    Register a specify CAN device into CAN core, if succeed, user can utilize net-tools
    to operate this CAN device, such as `ifconfig` and `can-tool` and so on.

  * unregister_candev

    Un-register a specify CAN device from CAN core. it's similar to remove operation.

  * can_dropped_invalid_skb

    Send vaild CAN frame to CAN Bus. The core function to send CAN frame.

  * can_put_echo_skb

    Echo CAN frame to NET.

## External Link
