SPI: Serial Peripheral Interface bus
--------------------------------------------------------

![spi timing](https://github.com/EmulateSpace/PictureSet/blob/master/spi/spi_timing.png)

The Serial Peripheral Interface bus (SPI) is a synchronous serial communication interface 
specification used for short distance communication, primarily in embedded systems. The 
interface was developed by Motorola in the late 1980s and has become a de facto standard. 
Typical applications include Secure Digital cards and liquid crystal displays.

SPI devices communicate in full duplex mode using a master-slave architecture with a single 
master. The master device originates the frame for reading and writing. Multiple slave 
devices are supported through selection with individual slave select (SS) lines.

Sometimes SPI is called a four-wire serial bus, contrasting with three-, two-, and one-wire 
serial buses. The SPI may be accurately described as a synchronous serial interface, but it 
is different from the Synchronous Serial Interface (SSI) protocol, which is also a four-wire 
synchronous serial communication protocol. SSI Protocol employs differential signaling and 
provides only a single simplex communication channel.

## File list

  * demo_spi.c

    Basic spi device driver on kernel, contain read and write from SPI bus.

## Core function

  * spi_register_board_info

    regiter basic spi device information into spi core. It contain the max speed, chip_select,
    node and so on.

  * spi_register_driver

    This function will register a spi driver into spi bus. If register succeed, the spi core
    will dispatch the probe procedure. On probe procedure, we should offer buffer or DMA to
    delivered data from/to SPI bus.

  * spi_message_init

    Initialize and package a spi message that contains READ or WRITE flag and data.

  * spi_message_add_tail

    Append the spi message into tail of spi receive- or send-quene. If spi has prepared
    to receive or send message, oldest meesage will be send or received from buffer.

  * spi_sync

    Send or Receive meesage from SPI bus.

## External link
