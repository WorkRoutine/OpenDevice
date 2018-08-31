/*
* Dummy serial driver by sdliu <sdliu@hongdian.com>
* The real hardware was installed in the MCU which is attached to our SOC(HI3520D).
* This driver depends on a serial manager which will process in the userspace
*/

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/amba/bus.h>
#include <linux/amba/serial.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/tty_flip.h>
#include <linux/circ_buf.h>

#include <asm/io.h>
#include <linux/platform_device.h>

// #include <asm/sizes.h>

#define prt_dbg
#ifdef prt_dbg
#define drintk printk
#else
#define drintk(...) 
#endif

#define DUMMY_SERIAL_NR         4

static int dummy_serial_major = 0;
static int dummy_serial_minor_start = 0;
static struct dummy_uart_port *dummy_array[DUMMY_SERIAL_NR];

unsigned int dummy_serial_nr = 1;
module_param(dummy_serial_nr, uint, S_IRUGO);


int dummy_fasync(int fd, struct file *filp, int mode);

struct dummy_port_data {
    unsigned long tx_fifo_size;
    unsigned long rx_fifo_size;
};

struct dummy_uart_port {
    struct uart_port port;

    struct dummy_port_data *port_data;

    char type[12];

    unsigned char *tx_fifo;
    unsigned char *rx_fifo;

    unsigned long tx_len;
    unsigned long rx_len;

    unsigned int mctrl;
    unsigned int baud;

    struct ktermios termios;
    struct fasync_struct *async_queue;
    struct semaphore async_sem;

    struct completion manager_activie;
    struct completion write_ok; 
    wait_queue_head_t poll_wq; 

    int manager_reset;

    int is_default_termios : 1;
    unsigned long status;

    struct cdev c_dev;
    int index;
};



static struct class *dummy_class;

static unsigned int dummy_tx_empty(struct uart_port *port)
{

    struct dummy_uart_port *dummy = (struct dummy_uart_port *)port;

    drintk("dummy_tx_empty %d\n", dummy->tx_len);

    return dummy->tx_len > 0 ? 0 : TIOCSER_TEMT;
}

static void dummy_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    struct dummy_uart_port *dummy = (struct dummy_uart_port *)port;
    dummy->mctrl = mctrl;

    drintk("dummy_set_mctrl!\n");   
}

static unsigned int dummy_get_mctrl(struct uart_port *port)
{
    struct dummy_uart_port *dummy = (struct dummy_uart_port *)port;

    drintk("dummy_get_mctrl!\n");
    return dummy->mctrl;
}

static void dummy_stop_tx(struct uart_port *port)
{
    drintk("dummy_stop_tx!\n");
}

static void dummy_start_tx(struct uart_port *port)
{   
    struct dummy_uart_port *dummy = (struct dummy_uart_port*)port;
    struct circ_buf *xmit = &port->state->xmit;
    int i = 0;

    drintk("dummy_start_tx!\n");

    do {        
        dummy->tx_fifo[dummy->tx_len++] = xmit->buf[xmit->tail];

        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;  

        if (uart_circ_empty(xmit)) {
            break;
        }

    } while (dummy->tx_len < dummy->port_data->tx_fifo_size); 

    for (i = 0; i < dummy->tx_len; i++) {
        drintk("%c ", dummy->tx_fifo[i]);
    }
    drintk("\n");

    wake_up(&dummy->poll_wq);

    init_completion(&dummy->write_ok);
    wait_for_completion(&dummy->write_ok);
}

static void dummy_stop_rx(struct uart_port *port)
{
    drintk("dummy_stop_rx!\n");
}

static void dummy_enable_ms(struct uart_port *port)
{
    drintk("dummy_enable_ms!\n");
}

static void dummy_break_ctl(struct uart_port *port, int break_state)
{
    drintk("dummy_break_ctl!\n");
}

static int dummy_startup(struct uart_port *port)
{
    drintk("dummy_startup!\n");

    return 0;
}

static void dummy_shutdown(struct uart_port *port)
{
    drintk("dummy_shutdown!\n");
}

static void dummy_flush_buffer(struct uart_port *port)
{
    drintk("dummy_flush_buffer!\n");
}


static void
dummy_set_termios(struct uart_port *port, struct ktermios *termios, struct ktermios *old)
{
    unsigned int baud = 0;
    struct dummy_uart_port *dummy = (struct dummy_uart_port*)port;

    baud = uart_get_baud_rate(port, termios, old, 0, 460800);
    dummy->baud = baud;

    memcpy(&dummy->termios, termios, sizeof(struct ktermios));

    drintk("set baudrate to %d\n", baud);

#if 1
    down(&dummy->async_sem);
    if (dummy->async_queue == NULL) {   
        up(&dummy->async_sem);

        init_completion(&dummy->manager_activie);
        wait_for_completion(&dummy->manager_activie);

    } else {
        up(&dummy->async_sem);
    }

    kill_fasync(&dummy->async_queue, SIGIO, POLL_IN);
#endif

}

static const char *dummy_type(struct uart_port *port)
{
    struct dummy_uart_port *dummy = (struct dummy_uart_port*)port;
    drintk("dummy_type\n");             
    return dummy->type;
}

static void dummy_release_port(struct uart_port *port)
{
    drintk("dummy_release_port\n");
}

static int dummy_request_port(struct uart_port *port)
{
    drintk("dummy_request_port\n");
    return 0;
}

static void dummy_config_port(struct uart_port *port, int flags)
{
    drintk("dummy_config_port\n");
    dummy_request_port(port);
    drintk("port->type = %d\n", port->type);

    port->type = PORT_AMBA;
}

static int dummy_verify_port(struct uart_port *port, struct serial_struct *ser)
{

    drintk("dummy_verify_port\n");

    return 0;
}

int dummy_open(struct inode *i, struct file *file)
{
    int minor = iminor(i);
    int index = minor - dummy_serial_minor_start;

    file->private_data = dummy_array[index];

    drintk("minor %d index %d private_data %p\n", minor, index, file->private_data);

    return 0;
}

int dummy_release(struct inode *i, struct file *file)
{
    struct dummy_uart_port *dummy = (struct dummy_uart_port *)file->private_data;

    dummy_fasync(-1, file, 0);

    down(&dummy->async_sem);    
    dummy->async_queue = NULL;
    dummy->manager_reset = 1;
    up(&dummy->async_sem);

    file->private_data = NULL;
    return 0;
}

ssize_t dummy_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    struct dummy_uart_port *dummy = (struct dummy_uart_port *)file->private_data;
    unsigned long tx_len = dummy->tx_len;
    int read_len = 0;

    read_len = size > tx_len ? tx_len : size;
    copy_to_user(buf, dummy->tx_fifo, read_len);
    dummy->tx_len -= read_len;

    if (dummy->tx_len == 0)
        complete(&dummy->write_ok);

    return read_len;
}

ssize_t dummy_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    struct dummy_uart_port *dummy = (struct dummy_uart_port *)file->private_data;
    // struct tty_struct *tty = NULL;
    struct uart_port *port = NULL;
    unsigned char ch = 0;
    int i = 0;  

    // struct tty_port *tty = NULL;

    if (dummy == NULL)
    {
        printk("dummy is nullptr\n");
        return -EIO;
    }

    port = &dummy->port;
    if (dummy->port.state == NULL)
    {
        printk("dummy->port.state is nullptr\n");
        return -EIO;    
    }   

    struct tty_port *tty = NULL;
    tty = &dummy->port.state->port;

    if (tty == NULL)
    {
        printk("tty is nullptr\n");
        return -EIO;    
    }   

    copy_from_user(dummy->rx_fifo, buf, size);
    dummy->rx_len = size;

    for (i = 0; i < size ; i++) {
        ch = *(dummy->rx_fifo + i);
        port->icount.rx++;

        tty_insert_flip_char(tty, ch, 0);
    }

    tty_flip_buffer_push(tty);  


    return size;
}

long dummy_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct dummy_uart_port *dummy = (struct dummy_uart_port *)file->private_data;

    switch (cmd){
    case 0xde:
        printk("dummy_ioctl baud %d\n", dummy->baud);
        copy_to_user((void *)arg, &dummy->baud, sizeof(unsigned int));
        break;

    default:
        break;
    }   

    return 0;
}

unsigned int dummy_poll(struct file *file, struct poll_table_struct *pwait)
{
    struct dummy_uart_port *dummy = (struct dummy_uart_port *)file->private_data;
    unsigned int mask = 0;

    printk("%d tx_len %d\n", dummy->index, dummy->tx_len);
    poll_wait(file, &dummy->poll_wq, pwait);

    if (dummy->tx_len)
        mask |= POLLIN | POLLRDNORM;

    return mask;
}

int dummy_fasync(int fd, struct file *filp, int mode) 
{
    struct dummy_uart_port *dummy = (struct dummy_uart_port *)filp->private_data;
    int ret = 0;

    down(&dummy->async_sem);
    ret = fasync_helper(fd, filp, mode, &dummy->async_queue);

    if (dummy->manager_reset) {
        dummy->manager_reset = 0;
        kill_fasync(&dummy->async_queue, SIGIO, POLL_IN);       
    }

    up(&dummy->async_sem);

    complete(&dummy->manager_activie);

    return ret;
}

static struct uart_ops dummy_uart_ops = {
    .tx_empty   = dummy_tx_empty,
    .set_mctrl  = dummy_set_mctrl,
    .get_mctrl  = dummy_get_mctrl,
    .stop_tx    = dummy_stop_tx,
    .start_tx   = dummy_start_tx,
    .stop_rx    = dummy_stop_rx,
    .enable_ms  = dummy_enable_ms,
    .break_ctl  = dummy_break_ctl,
    .startup    = dummy_startup,
    .shutdown   = dummy_shutdown,
    .flush_buffer   = dummy_flush_buffer,
    .set_termios    = dummy_set_termios,
    .type       = dummy_type,
    .release_port   = dummy_release_port,
    .request_port   = dummy_request_port,
    .config_port    = dummy_config_port,
    .verify_port    = dummy_verify_port,
#ifdef CONFIG_CONSOLE_POLL
    .poll_get_char = NULL,
    .poll_put_char = NULL,
#endif
};

static struct uart_driver dummy_driver = {
    .owner          = THIS_MODULE,
    .driver_name    = "ttyDUM",
    .dev_name       = "ttyDUM",
    .major          = 0,                    // AUTO allocate
    .minor          = 0,
    .nr             = DUMMY_SERIAL_NR,
    .cons           = NULL,
};

struct file_operations dummy_fops = {
    .open           = dummy_open,
    .release        = dummy_release,
    .read           = dummy_read,
    .write          = dummy_write,
    .unlocked_ioctl = dummy_ioctl,
    .poll           = dummy_poll,
    .fasync         = dummy_fasync,
};

int create_manager_device(struct platform_device *pdev, int index)
{
    struct dummy_uart_port *dummy = NULL;
    struct dummy_port_data *data = NULL;
    struct device *tmp = NULL;
    int ret = 0;
    dev_t dev = 0;
    char dev_name[64] = {0};

    data = (struct dummy_port_data *)pdev->dev.platform_data;
    if (!data) {
        printk("not platform data\n");
        return -EINVAL;
    }

    dummy = (struct dummy_uart_port *)\
                kmalloc(sizeof(struct dummy_uart_port), GFP_KERNEL);
    if (!dummy) {
        printk("malloc dummy error\n");
        return -ENOMEM;
    }

    memset(dummy, 0, sizeof(struct dummy_uart_port));
    dummy->index = index;
    dummy->is_default_termios = 1;  
    sema_init(&dummy->async_sem, 1);
    init_completion(&dummy->manager_activie);
    init_waitqueue_head(&dummy->poll_wq);

    dummy->tx_fifo = (unsigned char*)kmalloc(data->tx_fifo_size, GFP_KERNEL);
    dummy->rx_fifo = (unsigned char*)kmalloc(data->rx_fifo_size, GFP_KERNEL);
    if (!dummy->tx_fifo || !dummy->rx_fifo) {
        printk("dummy fifo kmalloc err rx=%p tx=%p\n", \
                dummy->rx_fifo, dummy->tx_fifo);
        ret = -ENOMEM;
        goto FIFO_ERR;
    }

    dummy->port_data = data;    

    dummy->port.dev = &(pdev->dev);
    dummy->port.mapbase = 0;
    dummy->port.membase = (unsigned char *)(0xdeadbeef);
    dummy->port.iotype = UPIO_MEM;
    dummy->port.irq = 0;
    dummy->port.fifosize = 16;
    dummy->port.ops = &dummy_uart_ops;
    dummy->port.flags = UPF_BOOT_AUTOCONF;
    dummy->port.line = index;
    dummy->port.type = PORT_AMBA;

    ret = uart_add_one_port(&dummy_driver, &dummy->port);
    if (ret) {
        printk("uart drv add one port err. ret = 0x%08x\n", ret);
        goto PORT_ERR;
    }

    dummy_array[index] = dummy;

    if (!dummy_serial_major) {
        sprintf(dev_name, "serialdum%d", 0);
        ret = alloc_chrdev_region(&dev, 0, DUMMY_SERIAL_NR, dev_name);
        if (!ret) {
            dummy_serial_major = MAJOR(dev);
            dummy_serial_minor_start = MINOR(dev);
        } else {
            printk("register cdev err, ret=%d\n", ret);
            goto DEV_ERR;   
        }   
    } else {
        dev = MKDEV(dummy_serial_major, dummy_serial_minor_start + index);
        sprintf(dev_name, "serialdum%d", index);
    }   

    cdev_init(&dummy->c_dev, &dummy_fops);
    dummy->c_dev.owner = THIS_MODULE;   
    cdev_add(&dummy->c_dev, dev, DUMMY_SERIAL_NR);

    tmp = device_create(dummy_class, NULL, dev, NULL, dev_name);
    if (NULL == tmp) {
        printk("create device err! %d, %s\n", dev, dev_name);
    }

    printk("create dummy serial manager device /dev/%s\n", dev_name);

    return ret;

DEV_ERR:
    uart_remove_one_port(&dummy_driver, &dummy->port);

PORT_ERR:
    if (dummy->rx_fifo)
        kfree(dummy->rx_fifo);

    if (dummy->tx_fifo)
        kfree(dummy->tx_fifo);

FIFO_ERR:
    kfree(dummy);

    return ret;
}


static int serial_dummy_probe(struct platform_device *pdev)
{
    int i = 0;
    int ret = 0;

    for(i = 0; i < dummy_serial_nr; i++) {
        ret = create_manager_device(pdev, i);
        if (ret) {
            printk("create dummy manager device err, index = %d\n", i);
        }
    }   
    return 0;
}

static int serial_dummy_remove(struct platform_device *dev)
{   
    struct dummy_uart_port *dummy =  NULL;
    int i = 0;
    dev_t dev_num = 0;

    for (i = 0; i < dummy_serial_nr; i++) {
        dummy = dummy_array[i];

        dev_num = MKDEV(dummy_serial_major, dummy_serial_minor_start + i);
        device_destroy(dummy_class, dev_num);
        cdev_del(&dummy->c_dev);

        uart_remove_one_port(&dummy_driver, &dummy->port);

        if (dummy->rx_fifo)
            kfree(dummy->rx_fifo);

        if (dummy->tx_fifo)
            kfree(dummy->tx_fifo);

        kfree(dummy);
    }

    dev_num = MKDEV(dummy_serial_major, dummy_serial_minor_start);
    unregister_chrdev_region(dev_num, DUMMY_SERIAL_NR);

    return 0;
}

static struct platform_driver dummy_serial_dirver = {
    .probe      = serial_dummy_probe,
    .remove     = serial_dummy_remove,
    .driver     = {
        .name   = "dummy_serial",
        .owner  = THIS_MODULE,
    }
};

static struct dummy_port_data dummy_serial_dev_data = {
        .tx_fifo_size = 2 * 1024,
        .rx_fifo_size = 2 * 1024,
};

void dummy_serial_release(struct device *dev);
static struct platform_device dummy_serial_dev = {
    .name       = "dummy_serial",
    .dev        = {
        .release = dummy_serial_release,
        .platform_data = (void *)(&dummy_serial_dev_data),
    }
};

void dummy_serial_release(struct device *dev)
{


}

static int __init serial_dummy_init(void)
{   
    int ret = 0;   

    printk("Hi3520d dummy serial drv, by sdliu ver.20151202\n");    

    if (dummy_serial_nr > DUMMY_SERIAL_NR) {
        printk("dummy serial nr(%d) is more than max(%d)\n", \
            dummy_serial_nr, DUMMY_SERIAL_NR);
        return -EINVAL;
    }

    ret = platform_device_register(&dummy_serial_dev);
    dummy_class  = class_create(THIS_MODULE, "dumtty");

    ret = uart_register_driver(&dummy_driver);
    if (ret) {
        printk("dummy drv register err ret = 0x%08x\n", ret);
        return -EINVAL;
    }

    ret = platform_driver_register(&dummy_serial_dirver);
    if (ret) {
        printk("dummy platform drv register err ret = %d\n", ret);
        platform_device_unregister(&dummy_serial_dev);
        uart_unregister_driver(&dummy_driver);
    }

    return ret;
}

static void __exit serial_dummy_exit(void)
{   
    platform_driver_unregister(&dummy_serial_dirver);
    uart_unregister_driver(&dummy_driver);
    class_destroy(dummy_class);

    platform_device_unregister(&dummy_serial_dev);
}

module_init(serial_dummy_init);
module_exit(serial_dummy_exit);

MODULE_AUTHOR("sdliu");
MODULE_DESCRIPTION("DUMMY serial driver");
MODULE_LICENSE("GPL");
