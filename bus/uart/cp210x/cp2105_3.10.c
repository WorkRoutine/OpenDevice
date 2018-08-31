/*
 * Silicon Laboratories CP210x USB to RS232 serial adaptor driver
 *
 * Copyright (C) 2005 Craig Shelley (craig@microtron.org.uk)
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License version
 *      2 as published by the Free Software Foundation.
 *
 * Support to set flow control line levels using TIOCMGET and TIOCMSET
 * thanks to Karl Hiramoto karl@hiramoto.org. RTSCTS hardware flow
 * control thanks to Munir Nassar nassarmu@real-time.com
 *
 * CP2015 USB to Uart on MGT2600
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>

#include <linux/tty.h>
#include <linux/usb.h>
#include <linux/usb/serial.h>

#define DRIVER_DESC "Demo USB serial adaptor driver"

/* Config request types */
#define REQTYPE_HOST_TO_INTERFACE       0x41
#define REQTYPE_INTERFACE_TO_HOST       0xc1
#define REQTYPE_HOST_TO_DEVICE          0x40
#define REQTYPE_DEVICE_TO_HOST          0xc0

/* Config request codes */
#define CP210X_IFC_ENABLE       0x00
#define CP210X_SET_BAUDDIV      0x01
#define CP210X_GET_BAUDDIV      0x02
#define CP210X_SET_LINE_CTL     0x03
#define CP210X_GET_LINE_CTL     0x04
#define CP210X_SET_BREAK        0x05
#define CP210X_IMM_CHAR         0x06
#define CP210X_SET_MHS          0x07
#define CP210X_GET_MDMSTS       0x08
#define CP210X_SET_XON          0x09
#define CP210X_SET_XOFF         0x0A
#define CP210X_SET_EVENTMASK    0x0B
#define CP210X_GET_EVENTMASK    0x0C
#define CP210X_SET_CHAR         0x0D
#define CP210X_GET_CHARS        0x0E
#define CP210X_GET_PROPS        0x0F
#define CP210X_GET_COMM_STATUS  0x10
#define CP210X_RESET            0x11
#define CP210X_PURGE            0x12
#define CP210X_SET_FLOW         0x13
#define CP210X_GET_FLOW         0x14
#define CP210X_EMBED_EVENTS     0x15
#define CP210X_GET_EVENTSTATE   0x16
#define CP210X_SET_CHARS        0x19
#define CP210X_GET_BAUDRATE     0x1D
#define CP210X_SET_BAUDRATE     0x1E

/* CP210X_IFC_ENABLE */
#define UART_ENABLE             0x0001
#define UART_DISABLE            0x0000

/* CP210X_(SET|GET)_BAUDDIV */
#define BAUD_RATE_GEN_FREQ      0x384000

/* CP210X_(SET|GET)_LINE_CTL */
#define BITS_DATA_MASK          0X0f00
#define BITS_DATA_5             0X0500
#define BITS_DATA_6             0X0600
#define BITS_DATA_7             0X0700
#define BITS_DATA_8             0X0800
#define BITS_DATA_9             0X0900

#define BITS_PARITY_MASK        0x00f0
#define BITS_PARITY_NONE        0x0000
#define BITS_PARITY_ODD         0x0010
#define BITS_PARITY_EVEN        0x0020
#define BITS_PARITY_MARK        0x0030
#define BITS_PARITY_SPACE       0x0040

#define BITS_STOP_MASK          0x000f
#define BITS_STOP_1             0x0000
#define BITS_STOP_1_5           0x0001
#define BITS_STOP_2             0x0002

/* CP210X_SET_BREAK */
#define BREAK_ON                0x0001
#define BREAK_OFF               0x0000

/* CP210X_(SET_MHS|GET_MDMSTS) */
#define CONTROL_DTR             0x0001
#define CONTROL_RTS             0x0002
#define CONTROL_CTS             0x0010
#define CONTROL_DSR             0x0020
#define CONTROL_RING            0x0040
#define CONTROL_DCD             0x0080
#define CONTROL_WRITE_DTR       0x0100
#define CONTROL_WRITE_RTS       0x0200

static const struct usb_device_id id_table[] = {
    { USB_DEVICE(0x10C4, 0xEA70)}, /* Silicon Labs factory default */
    { } /* Terminating entry */
};

struct cp210x_serial_private {
    __u8  bInterfaceNumber;
};

MODULE_DEVICE_TABLE(usb, id_table);

/*
 * cp210x_get_config
 * Reads from the CP210x configuration registers
 * 'size' is specified in bytes.
 * 'data' is a pointer to a pre-allocated array of integers large
 * enough to hold 'size' bytes (with 4 bytes to each integer)
 */
static int cp210x_get_config(struct usb_serial_port *port, u8 request,
                unsigned int *data, int size)
{
    struct usb_serial *serial = port->serial;
    struct cp210x_serial_private *spriv = usb_get_serial_data(serial);
    __le32 *buf;
    int result, i, length;

    /* Number of integers required to contain the array */
    length = (((size - 1) | 3) + 1) / 4;

    buf = kcalloc(length, sizeof(__le32), GFP_KERNEL);
    if (!buf) {
        dev_err(&port->dev, "%s - out of memory.\n", __func__);
        return -ENOMEM;
    }

    /* Issue the request, attempting to read 'size' bytes */
    result = usb_control_msg(serial->dev, usb_rcvctrlpipe(serial->dev, 0),
                 request, REQTYPE_INTERFACE_TO_HOST, 0x0000,
                 spriv->bInterfaceNumber, buf, size,
                 USB_CTRL_GET_TIMEOUT);

    /* Convert data into an array of integers */
    for (i = 0; i < length; i++)
        data[i] = le32_to_cpu(buf[i]);

    kfree(buf);

    if (result != size) {
        dev_dbg(&port->dev, "%s - Unable to send config request, request=0x%x size=%d result=%d\n",
                        __func__, request, size, result);
        if (result > 0)
            result = -EPROTO;

        return result;
    }

        return 0;
}

/*
 * cp201x_set_config
 * Writes to the demo configuration registers
 * Values less than 16 bits wide are sent directly
 * 'size' is specified in bytes.
 */
static int cp210x_set_config(struct usb_serial_port *port, u8 request,
     unsigned int *data, int size)
{
    struct usb_serial *serial = port->serial;
    struct cp210x_serial_private *spriv = usb_get_serial_data(serial);
    __le32 *buf;
    int result, i, length;

    /* Number of integers required to contain the array */
    length = (((size - 1) | 3) + 1) / 4;

    buf = kmalloc(length * sizeof(__le32), GFP_KERNEL);
    if (!buf) {
        dev_err(&port->dev, "%s - out of memory.\n", __func__);
        return -ENOMEM;
    }

    /* Array of integers into bytes */
    for (i = 0; i < length; i++)
        buf[i] = cpu_to_le32(data[i]);

    if (size > 2) {
        result = usb_control_msg(serial->dev,
                     usb_sndctrlpipe(serial->dev, 0),
                     request, REQTYPE_HOST_TO_INTERFACE, 0x0000,
                     spriv->bInterfaceNumber, buf, size,
                     USB_CTRL_SET_TIMEOUT);
    } else {
        result = usb_control_msg(serial->dev,
                     usb_sndctrlpipe(serial->dev, 0),
                     request, REQTYPE_HOST_TO_INTERFACE, data[0],
                     spriv->bInterfaceNumber, NULL, 0,
                     USB_CTRL_SET_TIMEOUT);
    }

    kfree(buf);

    if ((size > 2 && result != size) || result < 0) {
                dev_dbg(&port->dev, "%s - Unable to send request, request=0x%x size=%d result=%d\n",
                        __func__, request, size, result);
        if (result > 0)
            result = -EPROTO;

            return result;
    }

    return 0;
}

/*
 * demo_set_config_single
 * Convenience function for calling demo_set_config on single data value
 * without requiring an integer pointer
 */
static inline int cp210x_set_config_single(struct usb_serial_port *port,
      u8 request, unsigned int data)
{
    return cp210x_set_config(port, request, &data, 2);
}

/*
 * cp210x_get_termios_port
 * This is the heart of cp210x_get_termios which always uses a &usb_serial_port.
 */
static void cp210x_get_termios_port(struct usb_serial_port *port,
          unsigned int *cflagp, unsigned int *baudp)
{
    struct device *dev = &port->dev;
    unsigned int cflag, modem_ctl[4];
    unsigned int baud;
    unsigned int bits;

    cp210x_get_config(port, CP210X_GET_BAUDRATE, &baud, 4);

    dev_dbg(dev, "%s - baud rate = %d\n", __func__, baud);
    *baudp = baud;

    cflag = *cflagp;
    cp210x_get_config(port, CP210X_GET_LINE_CTL, &bits, 2);
    cflag &= ~CSIZE;
    switch (bits & BITS_DATA_MASK) {
    case BITS_DATA_5:
        dev_dbg(dev, "%s - data bits = 5\n", __func__);
        cflag |= CS5;
        break;
    case BITS_DATA_6:
        dev_dbg(dev, "%s - data bits = 6\n", __func__);
        cflag |= CS6;
        break;
    case BITS_DATA_7:
        dev_dbg(dev, "%s - data bits = 7\n", __func__);
        cflag |= CS7;
        break;
    case BITS_DATA_8:
        dev_dbg(dev, "%s - data bits = 8\n", __func__);
        cflag |= CS8;
        break;
    case BITS_DATA_9:
        dev_dbg(dev, "%s - data bits = 9 (not supported, using 8 data bits)\n", __func__);
        cflag |= CS8;
        bits &= ~BITS_DATA_MASK;
        bits |= BITS_DATA_8;
        cp210x_set_config(port, CP210X_SET_LINE_CTL, &bits, 2);
        break;
    default:
        dev_dbg(dev, "%s - Unknown number of data bits, using 8\n", __func__);
        cflag |= CS8;
        bits &= ~BITS_DATA_MASK;
        bits |= BITS_DATA_8;
        cp210x_set_config(port, CP210X_SET_LINE_CTL, &bits, 2);
        break;
    }

    switch (bits & BITS_PARITY_MASK) {
    case BITS_PARITY_NONE:
        dev_dbg(dev, "%s - parity = NONE\n", __func__);
        cflag &= ~PARENB;
        break;
    case BITS_PARITY_ODD:
        dev_dbg(dev, "%s - parity = ODD\n", __func__);
        cflag |= (PARENB|PARODD);
        break;
    case BITS_PARITY_EVEN:
        dev_dbg(dev, "%s - parity = EVEN\n", __func__);
        cflag &= ~PARODD;
        cflag |= PARENB;
        break;
    case BITS_PARITY_MARK:
        dev_dbg(dev, "%s - parity = MARK\n", __func__);
        cflag |= (PARENB|PARODD|CMSPAR);
        break;
    case BITS_PARITY_SPACE:
        dev_dbg(dev, "%s - parity = SPACE\n", __func__);
        cflag &= ~PARODD;
        cflag |= (PARENB|CMSPAR);
        break;
    default:
        dev_dbg(dev, "%s - Unknown parity mode, disabling parity\n", __func__);
        cflag &= ~PARENB;
        bits &= ~BITS_PARITY_MASK;
        cp210x_set_config(port, CP210X_SET_LINE_CTL, &bits, 2);
        break;
    }

    cflag &= ~CSTOPB;
    switch (bits & BITS_STOP_MASK) {
    case BITS_STOP_1:
        dev_dbg(dev, "%s - stop bits = 1\n", __func__);
        break;
    case BITS_STOP_1_5:
        dev_dbg(dev, "%s - stop bits = 1.5 (not supported, using 1 stop bit)\n", __func__);
        bits &= ~BITS_STOP_MASK;
        cp210x_set_config(port, CP210X_SET_LINE_CTL, &bits, 2);
        break;
    case BITS_STOP_2:
        dev_dbg(dev, "%s - stop bits = 2\n", __func__);
        cflag |= CSTOPB;
        break;
    default:
        dev_dbg(dev, "%s - Unknown number of stop bits, using 1 stop bit\n", __func__);
        bits &= ~BITS_STOP_MASK;
        cp210x_set_config(port, CP210X_SET_LINE_CTL, &bits, 2);
        break;
    }

    cp210x_get_config(port, CP210X_GET_FLOW, modem_ctl, 16);
    if (modem_ctl[0] & 0x0008) {
        dev_dbg(dev, "%s - flow control = CRTSCTS\n", __func__);
        cflag |= CRTSCTS;
    } else {
        dev_dbg(dev, "%s - flow control = NONE\n", __func__);
        cflag &= ~CRTSCTS;
    }

    *cflagp = cflag;
}

/*
 * cp210x_get_termios
 * Reads the baud rate, data bits, parity, stop bits and flow control mode
 * from the device, corrects any unsupported values, and configures the 
 * termios structure to reflect the state of the device
 */
static void cp210x_get_termios(struct tty_struct *tty,
                   struct usb_serial_port *port)
{
    unsigned int baud;

    if (tty) {
        cp210x_get_termios_port(tty->driver_data,
                 &tty->termios->c_cflag, &baud);
        tty_encode_baud_rate(tty, baud, baud);
    } else {
        unsigned int cflag;
        cflag = 0;
        cp210x_get_termios_port(port, &cflag, &baud);
    }
}

/*
 * cp210x_quantise_baudrate
 * Quantises the baud rate as per AN205 Table 1
 */
static unsigned int cp210x_quantise_baudrate(unsigned int baud)
{
    if (baud <= 300)
        baud = 300;
    else if (baud <= 600)      baud = 600;
    else if (baud <= 1200)     baud = 1200;
    else if (baud <= 1800)     baud = 1800;
    else if (baud <= 2400)     baud = 2400;
    else if (baud <= 4000)     baud = 4000;
    else if (baud <= 4803)     baud = 4800;
    else if (baud <= 7207)     baud = 7200;
    else if (baud <= 9612)     baud = 9600;
    else if (baud <= 14428)    baud = 14400;
    else if (baud <= 16062)    baud = 16000;
    else if (baud <= 19250)    baud = 19200;
    else if (baud <= 28912)    baud = 28800;
    else if (baud <= 38601)    baud = 38400;
    else if (baud <= 51558)    baud = 51200;
    else if (baud <= 56280)    baud = 56000;
    else if (baud <= 58053)    baud = 57600;
    else if (baud <= 64111)    baud = 64000;
    else if (baud <= 77608)    baud = 76800;
    else if (baud <= 117028)   baud = 115200;
    else if (baud <= 129347)   baud = 128000;
    else if (baud <= 156868)   baud = 153600;
    else if (baud <= 237832)   baud = 230400;
    else if (baud <= 254234)   baud = 250000;
    else if (baud <= 273066)   baud = 256000;
    else if (baud <= 491520)   baud = 460800;
    else if (baud <= 567138)   baud = 500000;
    else if (baud <= 670254)   baud = 576000;
    else if (baud < 1000000)
        baud = 921600;
    else if (baud > 2000000)
        baud = 2000000;
    return baud;
}

/*
 * CP2101 supports the following baud rates:
 *
 *      300, 600, 1200, 1800, 2400, 4800, 7200, 9600, 14400, 19200, 28800,
 *      38400, 56000, 57600, 115200, 128000, 230400, 460800, 921600
 *
 * CP2102 and CP2103 support the following additional rates:
 *
 *      4000, 16000, 51200, 64000, 76800, 153600, 250000, 256000, 500000,
 *      576000
 *
 * The device will map a requested rate to a supported one, but the result
 * of requests for rates greater than 1053257 is undefined (see AN205).
 *
 * CP2104, CP2105 and CP2110 support most rates up to 2M, 921k and 1M baud,
 * respectively, with an error less than 1%. The actual rates are determined
 * by
 *
 *      div = round(freq / (2 x prescale x request))
 *      actual = freq / (2 x prescale x div)
 *
 * For CP2104 and CP2105 freq is 48Mhz and prescale is 4 for request <= 365bps
 * or 1 otherwise.
 * For CP2110 freq is 24Mhz and prescale is 4 for request <= 300bps or 1
 * otherwise.
 */
static void cp210x_change_speed(struct tty_struct *tty,
                struct usb_serial_port *port, struct ktermios *old_termios)
{
    u32 baud;

    baud = tty->termios->c_ospeed;

    /* This maps the requested rate to a rate valid on cp2102 or cp2103,
     * or to an arbitrary rate in [1M,2M].
     *
     * NOTE: B0 is not implemented.
     */
    baud = cp210x_quantise_baudrate(baud);

    dev_dbg(&port->dev, "%s - setting baud rate to %u\n", __func__, baud);
    if (cp210x_set_config(port, CP210X_SET_BAUDRATE, &baud, sizeof(baud))) {
        dev_warn(&port->dev, "failed to set baud rate to %u\n", baud);
        if (old_termios)
            baud = old_termios->c_ospeed;
        else
            baud = 9600;
    }

    tty_encode_baud_rate(tty, baud, baud);
}

static int cp210x_tiocmset_port(struct usb_serial_port *port,
                unsigned int set, unsigned int clear)
{
    unsigned int control = 0;

    if (set & TIOCM_RTS) {
        control |= CONTROL_RTS;
        control |= CONTROL_WRITE_RTS;
    }
    if (set & TIOCM_DTR) {
        control |= CONTROL_DTR;
        control |= CONTROL_WRITE_DTR;
    }
    if (clear & TIOCM_RTS) {
        control &= ~CONTROL_RTS;
        control |= CONTROL_WRITE_RTS;
    }
    if (clear & TIOCM_DTR) {
        control &= ~CONTROL_DTR;
        control |= CONTROL_WRITE_DTR;
    }

    dev_dbg(&port->dev, "%s - control = 0x%.4x\n", __func__, control);

    return cp210x_set_config(port, CP210X_SET_MHS, &control, 2);
}

static int cp210x_open(struct tty_struct *tty, struct usb_serial_port *port)
{
    int result;

    result = cp210x_set_config_single(port, CP210X_IFC_ENABLE,
                           UART_ENABLE);

    if (result) {
        dev_err(&port->dev, "%s - Unable to enable UART\n", __func__);
        return result;
    }

    /* Congirure the termios structure */
    cp210x_get_termios(tty, port);

    /* The baud rate must be initialised on cp2104 */
    if (tty)
        cp210x_change_speed(tty, port, NULL);

    return usb_serial_generic_open(tty, port);
}

static void cp210x_close(struct usb_serial_port *port)
{
    usb_serial_generic_close(port);
    cp210x_set_config_single(port, CP210X_IFC_ENABLE, UART_DISABLE);
}

static void cp210x_break_ctl(struct tty_struct *tty, int break_state)
{
    struct usb_serial_port *port = tty->driver_data;
    unsigned int state;

    if (break_state == 0)
        state = BREAK_OFF;
    else
        state = BREAK_ON;
    dev_dbg(&port->dev, "%s - turning break %s\n", __func__,
          state == BREAK_OFF ? "off" : "on");
    cp210x_set_config(port, CP210X_SET_BREAK, &state, 2);
}

static void cp210x_set_termios(struct tty_struct *tty,
                struct usb_serial_port *port, struct ktermios *old_termios)
{
    struct device *dev = &port->dev;
    unsigned int cflag, old_cflag;
    unsigned int bits;
    unsigned int modem_ctl[4];

    dev_dbg(dev, "%s - port %d\n", __func__, port->number);

    if (!tty)
        return;

    cflag = tty->termios->c_cflag;
    old_cflag = old_termios->c_cflag;

    if (tty->termios->c_ospeed != old_termios->c_ospeed)
        cp210x_change_speed(tty, port, old_termios);

    /* If the number of data bits is to be updated */
    if ((cflag & CSIZE) != (old_cflag & CSIZE)) {
        cp210x_get_config(port, CP210X_GET_LINE_CTL, &bits, 2);
        bits &= ~BITS_DATA_MASK;
        switch (cflag & CSIZE) {
        case CS5:
            bits |= BITS_DATA_5;
            dev_dbg(dev, "%s - data bits = 5\n", __func__);
            break;
        case CS6:
            bits |= BITS_DATA_6;
            dev_dbg(dev, "%s - data bits = 6\n", __func__);
            break;
        case CS7:
            bits |= BITS_DATA_7;
            dev_dbg(dev, "%s - data bits = 7\n", __func__);
            break;
        case CS8:
            bits |= BITS_DATA_8;
            dev_dbg(dev, "%s - data bits = 8\n", __func__);
        /*case CS9:
            bits |= BITS_DATA_9;
            dev_dbg(dev, "%s - data bits = 9\n", __func__);
            break;*/
        default:
            dev_dbg(dev, "cp210x driver does not support the number of bits requested, using 8 bit mode\n");
            bits |= BITS_DATA_8;
            break;
        }
        if (cp210x_set_config(port, CP210X_SET_LINE_CTL, &bits, 2))
            dev_dbg(dev, "Number of data bits requested not supported by device\n");
    }

    if ((cflag & (PARENB|PARODD|CMSPAR)) != (old_cflag & (PARENB|PARODD|CMSPAR))) {
        cp210x_get_config(port, CP210X_GET_LINE_CTL, &bits, 2);
        bits &= ~BITS_PARITY_MASK;
        if (cflag & PARENB) {
            if (cflag & CMSPAR) {
                if (cflag & PARODD) {
                    bits |= BITS_PARITY_MARK;
                    dev_dbg(dev, "%s - parity = MARK\n", __func__);
                } else {
                    bits |= BITS_PARITY_SPACE;
                    dev_dbg(dev, "%s - parity = SPACE\n", __func__);
                }
        } else {
            if (cflag & PARODD) {
                bits |= BITS_PARITY_ODD;
                dev_dbg(dev, "%s - parity = ODD\n", __func__);
            } else {
                bits |= BITS_PARITY_EVEN;
                dev_dbg(dev, "%s - parity = EVEN\n", __func__);
            }
        }
    }
    if (cp210x_set_config(port, CP210X_SET_LINE_CTL, &bits, 2))
        dev_dbg(dev, "Parity mode not supported by device\n");
    }
    if ((cflag & CSTOPB) != (old_cflag & CSTOPB)) {
        cp210x_get_config(port, CP210X_GET_LINE_CTL, &bits, 2);
        bits &= ~BITS_STOP_MASK;
        if (cflag & CSTOPB) {
            bits |= BITS_STOP_2;
            dev_dbg(dev, "%s - stop bits = 2\n", __func__);
        } else {
            bits |= BITS_STOP_1;
            dev_dbg(dev, "%s - stop bits = 1\n", __func__);
        }
        if (cp210x_set_config(port, CP210X_SET_LINE_CTL, &bits, 2))
            dev_dbg(dev, "Number of stop bits requested not supported by device\n");
   }

   if ((cflag & CRTSCTS) != (old_cflag & CRTSCTS)) {
       cp210x_get_config(port, CP210X_GET_FLOW, modem_ctl, 16);
       dev_dbg(dev, "%s - read modem controls = 0x%.4x 0x%.4x 0x%.4x 0x%.4x\n",
                    __func__, modem_ctl[0], modem_ctl[1],
                    modem_ctl[2], modem_ctl[3]);

       if (cflag & CRTSCTS) {
           modem_ctl[0] &= ~0x7B;
           modem_ctl[0] |= 0x09;
           modem_ctl[1] = 0x80;
           dev_dbg(dev, "%s - flow control = CRTSCTS\n", __func__);
       } else {
           modem_ctl[0] &= ~0x7B;
           modem_ctl[0] |= 0x01;
           modem_ctl[1] |= 0x40;
           dev_dbg(dev, "%s - flow control = NONE\n", __func__);
       }

       dev_dbg(dev, "%s - write modem controls = 0x%.4x 0x%.4x 0x%.4x 0x%.4x\n",
                   __func__, modem_ctl[0], modem_ctl[1],
                   modem_ctl[2], modem_ctl[3]);
       cp210x_set_config(port, CP210X_SET_FLOW, modem_ctl, 16);
    }
}

static int cp210x_tiocmget(struct tty_struct *tty)
{
    struct usb_serial_port *port = tty->driver_data;
    unsigned int control;
    int result;

    cp210x_get_config(port, CP210X_GET_MDMSTS, &control, 1);

    result = ((control & CONTROL_DTR) ? TIOCM_DTR : 0)
            |((control & CONTROL_RTS) ? TIOCM_RTS : 0)
            |((control & CONTROL_CTS) ? TIOCM_CTS : 0)
            |((control & CONTROL_DSR) ? TIOCM_DSR : 0)
            |((control & CONTROL_RING)? TIOCM_RI  : 0)
            |((control & CONTROL_DCD) ? TIOCM_CD  : 0);

    dev_dbg(&port->dev, "%s - control = 0x%.2x\n", __func__, control);

    return result;
}

static int cp210x_tiocmset(struct tty_struct *tty,
                unsigned int set, unsigned int clear)
{
    struct usb_serial_port *port = tty->driver_data;
    return cp210x_tiocmset_port(port, set, clear);
}

static int cp210x_startup(struct usb_serial *serial)
{
    struct usb_host_interface *cur_altsetting;
    struct cp210x_serial_private *spriv;

    /* cp210x buffers behave strangely unless device is reset */
    usb_reset_device(serial->dev);

    spriv = kzalloc(sizeof(*spriv), GFP_KERNEL);
    if (!spriv)
        return -ENOMEM;

    cur_altsetting = serial->interface->cur_altsetting;
    spriv->bInterfaceNumber = cur_altsetting->desc.bInterfaceNumber;

    usb_set_serial_data(serial, spriv);

    return 0;
}

static void cp210x_release(struct usb_serial *serial)
{
    struct cp210x_serial_private *spriv;

    spriv = usb_get_serial_data(serial);
    kfree(spriv);
}

static void cp210x_dtr_rts(struct usb_serial_port *p, int on)
{
    if (on)
        cp210x_tiocmset_port(p, TIOCM_DTR|TIOCM_RTS, 0);
    else
        cp210x_tiocmset_port(p, 0, TIOCM_DTR|TIOCM_RTS);
}

static struct usb_serial_driver demo_device = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = "CP2015 MGT2600",
    },
    .id_table           = id_table,
    .num_ports          = 1,
    .bulk_in_size       = 256,
    .bulk_out_size      = 256,
    .open               = cp210x_open,
    .close              = cp210x_close,
    .break_ctl          = cp210x_break_ctl,
    .set_termios        = cp210x_set_termios,
    .tiocmget           = cp210x_tiocmget,
    .tiocmset           = cp210x_tiocmset,
    .attach             = cp210x_startup,
    .release            = cp210x_release,
    .dtr_rts            = cp210x_dtr_rts
};

static struct usb_serial_driver * const demo_drivers[] = {
    &demo_device, NULL
};

module_usb_serial_driver(demo_drivers, id_table);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
