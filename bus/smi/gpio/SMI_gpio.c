/*
 * Copyright (C) 2018 buddy.zhang@aliyun.com
 *
 * SMI Bus in GPIO
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

/* Soc header file */
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/pinctrl/consumer.h>
#include <mach/sys_config.h>
#include <mach/sunxi-chip.h>
#include <mach/gpio.h>
#include <asm/gpio.h>
#include <asm/uaccess.h>

#define DEV_NAME "smi-gpio"

#define START_OF_FRAME_2bit   0x01
#define READ_OP_CODE_2bit     0x02
#define WRITE_OP_CODE_2bit    0x01
#define SMI_OP_CODE_2bit      0x00
#define SMI_TA                0x02

#define MDC_OUT()    gpio_direction_output(MDC_gpio->gpio, 0)
#define MDC_H()      gpio_set_value(MDC_gpio->gpio, 1)
#define MDC_L()      gpio_set_value(MDC_gpio->gpio, 0)
#define MDIO_IN()    gpio_direction_output(MDIO_gpio->gpio, 0)
#define MDIO_OUT()   gpio_direction_output(MDIO_gpio->gpio, 1)
#define MDIO_H()     gpio_set_value(MDIO_gpio->gpio, 1)
#define MDIO_L()     gpio_set_value(MDIO_gpio->gpio, 0)
#define READ_MDIO()  gpio_get_value(MDIO_gpio->gpio)

#define delay_us(x)  udelay(x)

#define SMI_IOCTL_BASE      'W'
#define SMI_SET_PHYADDR     _IOR(SMI_IOCTL_BASE,4,int)
#define SMI_SET_PHYREG      _IOR(SMI_IOCTL_BASE,5,int)

/* SMI Bus: MDC pin */
static struct gpio_config *MDC_gpio;
/* SMI Bus: MDIO pin */
static struct gpio_config *MDIO_gpio;

/* Global PHY-port */
static int phy_port;


/* Configure GPIO on SUNXI */
static int register_gpio(struct gpio_config *gpio)
{
    char pin_name[SUNXI_PIN_NAME_MAX_LEN];
    unsigned config;
    int cret;

    /* get gpio name */
    memset(pin_name, 0, SUNXI_PIN_NAME_MAX_LEN);
    sunxi_gpio_to_name(gpio->gpio, pin_name);
    printk("===* GPIO: %s *===\n", pin_name);
    /* set input or output */
    config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, gpio->mul_sel);
    cret   = pin_config_set(SUNXI_PINCTRL, pin_name, config);
    if (cret < 0)
        printk(KERN_ERR  "GPIO-Direct: ERROR.\n");
    else
        printk(KERN_INFO "GPIO-Direct: %s\n",
                         gpio->mul_sel ? "OUTPUT" : "INPUT");
    /* set pull or down */
    config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, gpio->pull);
    cret   = pin_config_set(SUNXI_PINCTRL, pin_name, config);
    if (cret < 0)
        printk(KERN_ERR  "GPIO-Voltage: ERROR\n");
    else
        printk(KERN_INFO "GPIO-VOltage: %s\n",
                         gpio->pull ? "Pull" : "Down");

    /* Set driver level */
    config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, gpio->drv_level);
    cret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
    if (cret < 0)
        printk(KERN_ERR  "Driver-Level: ERROR\n");
    else
        printk(KERN_INFO "Driver-Level: %d\n", gpio->drv_level);

    /* Set default value */
    config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, gpio->data);
    cret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
    if (cret < 0)
        printk(KERN_ERR  "Default-V: ERROR\n");
    else
        printk(KERN_INFO "Default-V: %d\n", gpio->data);
    printk("=========================\n");
    return 0;
}

/* Parse GPIO from sys_config.fex on SUNXI */
static int MGT2600_GPIO_sys_config(void)
{
    script_item_value_type_e type;
    script_item_u            value;

    memset(&value, 0, sizeof(script_item_u));
    /* MGT2600 Platform Procedure */
    /* Reset AUP2600 */
    type = script_get_item("SMI", "used", &value);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk(KERN_ERR "SMI Bus: GPIO parse failed.\n");
        return -EINVAL;
    }
    if (value.val) {
        memset(&value, 0, sizeof(script_item_u));

        MDC_gpio = (struct gpio_config *)kmalloc(
                           sizeof(*MDC_gpio), GFP_KERNEL);
        MDIO_gpio = (struct gpio_config *)kmalloc(
                           sizeof(*MDIO_gpio), GFP_KERNEL);
        memset(MDC_gpio, 0, sizeof (*MDC_gpio));
        memset(MDIO_gpio, 0, sizeof (*MDIO_gpio));

        /* Parse SMI Bus: MDC gpio */
        type = script_get_item("SMI", "MDC", &value);
        if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
            printk(KERN_ERR "SMI Bus: MDC parse failed\n");
            return -EINVAL;
        }
        MDC_gpio->gpio           = value.gpio.gpio;
        MDC_gpio->mul_sel        = value.gpio.mul_sel;
        MDC_gpio->pull           = value.gpio.pull;
        MDC_gpio->drv_level      = value.gpio.drv_level;
        MDC_gpio->data           = value.gpio.data;

        /* Parse SMI Bus: MDIO gpio */
        type = script_get_item("SMI", "MDIO", &value);
        if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
            printk(KERN_ERR "SMI Bus: MDIO parse failed\n");
            return -EINVAL;
        }
        MDIO_gpio->gpio           = value.gpio.gpio;
        MDIO_gpio->mul_sel        = value.gpio.mul_sel;
        MDIO_gpio->pull           = value.gpio.pull;
        MDIO_gpio->drv_level      = value.gpio.drv_level;
        MDIO_gpio->data           = value.gpio.data;
    }

    return 0;
}

/* SMI Read one Byte */
static unsigned char SMI_read_one_byte(void)
{
    unsigned char i, receive = 0;

    for (i = 0; i < 8; i++) {
        receive <<= 1;
        MDC_L();
        delay_us(1);
        MDC_L();
        delay_us(1);
        if (READ_MDIO())
            receive |= 0x01;
        MDC_L();
    }
    return receive;
}

/* SMI Read 2Bit */
static unsigned char SMI_read_2bit(void)
{
    unsigned char i, receive = 0;

    for (i = 0; i < 2; i++) {
        receive <<= 1;
        MDC_L();
        delay_us(1);
        MDC_H();
        delay_us(1);
        if (READ_MDIO())
            receive |= 0x1;
        MDC_L();
    }
    return receive; 
}

/* SMI Write one byte */
static void SMI_write_one_byte(unsigned char data)
{
    unsigned char i;

    for (i = 0; i < 8; i++) {
        MDC_L();
        MDIO = (data & 0x80) >> 7;
        delay_us(1);
        MDC_H();
        delay_us(1);
        MDC_L();
        data <<= 1;
    }
}

/* 
 * SMI Write Frame
 * 
 *   @value: PHY value 
 *
 */
static void SMI_Write_Frame(unsigned short value)
{
    unsigned char addr;

    addr = (phy_port & 0x7) << 5 | (phy_reg & 0x1F);
    MDIO_OUT();
    SMI_write_one_byte(0xFF);
    SMI_write_one_byte(0xFF);
    SMI_write_one_byte(0xFF);
    SMI_write_one_byte(0xFF);
    SMI_write_one_byte(0xFF);
    SMI_write_one_byte(0xFF);
    SMI_write_one_byte(0xFF);
    SMI_write_one_byte(0xFF);

    SMI_write_2bit(START_OF_FRAME_2bit);
    SMI_write_2bit(WRITE_OP_CODE_2bit);
    SMI_write_2bit(0x00);
    SMI_write_one_byte(addr);
    SMI_write_2bit(SMI_TA);
    SMI_write_one_byte(0xFF);
    SMI_write_one_byte(value);
    MDIO_IN();
}

/*
 * SMI Read Frame
 */
static unsigned short SMI_Read_Frame(void)
{
    unsigned char addr;
    unsigned short data;

    addr = (phy_port & 0x7) << 5 | (phy_reg & 0x1F);
    MDIO_OUT();
    SMI_write_one_byte(0xFF);
    SMI_write_one_byte(0xFF);
    SMI_write_one_byte(0xFF);
    SMI_write_one_byte(0xFF);

    SMI_write_2bit(START_OF_FRAME_2bit);
    SMI_write_2bit(READ_OP_CODE_2bit);
    SMI_write_2bit(0x00);
    SMI_write_one_byte(addr);

    MDIO_IN();
    data = SMI_read_2bit();
    data = SMI_read_one_byte();
    data = data << 8;
    data = data | SMI_read_one_byte();

    return data;
}

/*
 * open operation
 */
static int SMI_VFS_open(struct inode *inode,struct file *filp)
{
    return 0;
}
/*
 * release opertion 
 */
static int SMI_VFS_release(struct inode *inode,struct file *filp)
{
    return 0;
}
/*
 * read operation
 */
static ssize_t SMI_VFS_read(struct file *filp,char __user *buffer,size_t count,
		loff_t *offset)
{
    unsigned short buf[30];

    for (i = 0; i < count; i++) {
        buf[i] = SMI_Read_Frame();
    }
    copy_to_user(buffer, buf, count);
    return 0;
}
/*
 * write operation
 */
static ssize_t SMI_VFS_write(struct file *filp,const char __user *buf,
		size_t count,loff_t *offset)
{
    char buffer[30];
    int i;

    copy_from_user(buffer, buf, count);

    for (i = 0; i < count; i++)
        SMI_Write_Frame(buffer[i])
    return 0;
}

static long SMI_VFS_ioctl(struct file *file, unsigned int cmd, 
                             unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    int __user *p = argp;
    int options, retval = -EINVAL;

    switch (cmd) {
    case SMI_SET_PHYADDR:
        if (get_user(options, p))
            return -EFAULT;
        phy_port = (unsigned short)(unsigned long)p;
        return 0;
    case SMI_SET_PHYREG:
        if (get_user(options, p))
            return -EFAULT;
        phy_reg = (unsigned short)(unsigned long)p;
        return 0;
    }
}

/*
 * file_operations
 */
static struct file_operations SMI_fops = {
    .owner     = THIS_MODULE,
    .open      = SMI_VFS_open,
    .release   = SMI_VFS_release,
    .write     = SMI_VFS_write,
    .read      = SMI_VFS_read,
    .unlocked_ioctl = SMI_VFS_ioctl,
};
/*
 * misc struct 
 */

static struct miscdevice misc_SMI = {
    .minor    = MISC_DYNAMIC_MINOR,
    .name     = DEV_NAME,
    .fops     = &SMI_fops,
};
/*
 * Init module
 */
static __init int SMI_module_init(void)
{
    /* Hardware initialize */
    MGT2600_GPIO_sys_config();
    /* Register MDC gpio */
    register_gpio(MDC_gpio);
    /* Register MDIO gpio */
    register_gpio(MDIO_gpio);

    misc_register(&misc_SMI);
    return 0;
}
/*
 * Exit module
 */
static __exit void SMI_module_exit(void)
{
    misc_deregister(&misc_SMI);
    kfree(MDIO_gpio);
    kfree(MDC_gpio);
}
/*
 * module information
 */
module_init(SMI_module_init);
module_exit(SMI_module_exit);

MODULE_LICENSE("GPL");
