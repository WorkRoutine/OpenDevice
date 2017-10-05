I2C
-------------------------------------------------

I²C (Inter-Integrated Circuit), pronounced I-squared-C, is a multi-master, multi-slave, packet 
switched, single-ended, serial computer bus invented by Philips Semiconductor (now NXP 
Semiconductors). It is typically used for attaching lower-speed peripheral ICs to processors and 
microcontrollers in short-distance, intra-board communication. Alternatively I²C is spelled 
I2C (pronounced I-two-C) or IIC (pronounced I-I-C).

SMBus, defined by Intel in 1995, is a subset of I²C, defining a stricter usage. One purpose of 
SMBus is to promote robustness and interoperability. Accordingly, modern I²C systems incorporate 
some policies and rules from SMBus, sometimes supporting both I²C and SMBus, requiring only minimal
 reconfiguration either by commanding or output pin use.

## File list

  * i2c_device.c

    Basic routine of i2c device for registering. It will create device on I2C bus, and match a 
    i2c driver to probe.

  * i2c_msbus.c

    Basic i2c write and read with smbus interface.

## Core Function

  * i2c_get_adapter

    Get usage count for i2c bus adapter, it will increace counter of i2c bus adapter.

  * i2c_put_adapter

    Release usage count for i2c bus adapter, it will decrease counter of i2c bus adapter.

  * i2c_new_device

    Create a new i2c device on i2c bus, we should offer i2c bus adapter and board information 
    of i2c.

  * i2c_add_driver

    Regiter a i2c driver into i2c sub-system, it will be match by i2c specify device.

  * i2c_del_driver

    Unregister a i2c driver from i2c sub-system. 

  * i2c_smbus_read_byte_data

    SMBUS read a byte in I2C Bus. More information see i2c_msbus.c

  * i2c_smbus_write_byte_data

    SMBUS write a byte into I2C Bus. More information see i2c_msbus.c

## External linker

