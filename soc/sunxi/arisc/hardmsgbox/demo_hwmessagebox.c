/*
 * linux/drivers/arisc/demo_hwmessagebox.c
 * arisc hw message 
 *
 * (C) 2017.12 <buddy.zhang@aliyun.com>
 *
 * HMB: Hardware message box
 */
#include <linux/init.h>
#include <linux/kernel.h>

#include "arisc_i.h"

/* message attributes(only use 8bit) */
#define	ARISC_MESSAGE_ATTR_SOFTSYN (1<<0) //need soft syn with another cpu
#define	ARISC_MESSAGE_ATTR_HARDSYN (1<<1) //need hard syn with another cpu
/* the modes of arisc dvfs */
#define ARISC_DVFS_SYN             (1<<0)

/*
 * Send message to set DVFS
 */
static int demo_arisc_HMB_DVFS(void)
{
    struct arisc_message *pmessage;

    printk("ARISC: send hardware message to DVFS\n");
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("allocate message failed\n");
        return -ENOMEM;
    }

    /* Initialize message */
    pmessage->type = ARISC_CPUX_DVFS_CFG_VF_REQ;
    pmessage->paras[0] = 0;
    pmessage->paras[1] = 1800000000;
    pmessage->paras[2] = 1080;
    pmessage->paras[3] = 3;
    pmessage->paras[4] = 0;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;

    /* Send request message */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("Message send failed.\n");
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("Hardware message box DVFS done.\n");
    return 0;
}

/*
 * Send message to audio.
 *  Start audio play or capture.
 */
static int demo_arisc_HMB_audio_start(void)
{
    struct arisc_message *pmessage;
    /* mode: 0: play 1: capture */
    int mode = 0;

    printk("ARISC: send hardware message to audio start.\n");
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("allocate message to audio failed");
        return -ENOMEM;
    }
    /*
     * | para[0] |
     * | mode    |
     */
    /* initialize message */
    pmessage->type  = ARISC_AUDIO_START;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->paras[0] = mode; 
    
    /* send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
    
    /* check config fail or not */
    if (pmessage->result) {
        printk("Send message to audio failed\n");
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("Hardware message box Audio done.\n");
    return 0;
}

/*
 * Send message to audio.
 *  Stop audio play or capture
 */
static int demo_arisc_HWB_audio_stop(void)
{
    struct arisc_message *pmessage;
    /* mode: 0 - stop play ... 1 - stop capture*/
    int mode = 0;

    printk("ARISC: send hardware message to audio stop.\n");
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("allocate message to audio stop failed.\n");
        return -ENOMEM;
    }

    /*
     * | para[0] |
     * | mode    |
     */
    /* initialize message */
    pmessage->type = ARISC_AUDIO_STOP;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->paras[0] = mode;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("Send message to audio failed.\n");
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("Hardware message box Auido done.\n");
    return 0;
}

/*
 * Send message to audio
 *   Set audio buffer and period parameters.
 *   mode:              which mode be set- 0: play, 1: capture 
 *   sram_base_addr:    sram base addr of buffer
 *   buffer_size:       the size of buffer
 *   period_size:       the size of period
 *
 * |period|period|period|period|...|period|period|period|period|...|
 * | paly                   buffer | capture                buffer |
 * |                               |
 * 1                               2
 * 1:paly sram_base_addr,          2:capture sram_base_addr;
 * buffer size = capture sram_base_addr - paly sram_base_addr.
 */
static int demo_arisc_HWB_audio_buffer_period(void)
{
    struct arisc_message *pmessage;
    /* mode: 0 - stop play ... 1 - stop capture*/
    int mode = 0;
    unsigned int sram_base_addr = 0x800000;
    unsigned int buffer_size    = 0x100;
    unsigned int period_size    = 0x20;

    printk("ARISC: send hardware message to audio buffer and period.\n");
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("allocate message to audio buffer and period failed.\n");
        return -ENOMEM;
    }

    /*
     * |para[0]   |para[1]       |para[2]    |para[3]    |
     * |mode      |sram_base_addr|buffer_size|period_size|
     */
    /* initialize message */
    pmessage->type = ARISC_AUDIO_SET_BUF_PER_PARAS;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->paras[0] = mode;
    pmessage->paras[1] = sram_base_addr;
    pmessage->paras[2] = buffer_size;
    pmessage->paras[3] = period_size;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("Send message to audio buffer and period failed.\n");
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("Hardware message box Auido buffer and period done.\n");
    return 0;
}

/*
 * Send hardware message to audio
 *   get audio play or capture real-time address.
 *   @mode:    in which mode; 0:play, 1;capture;
 *   @addr:    real-time address in which mode.
 */
static int demo_arisc_HWB_audio_read_time(void)
{
    struct arisc_message *pmessage;
    /* mode: 0 - stop play ... 1 - stop capture*/
    int mode = 0;
    unsigned int address;

    printk("ARISC: send hardware message to audio read-time.\n");
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("allocate message to audio real-time failed.\n");
        return -ENOMEM;
    }

    /*
     * |para[0]   |para[1]       |
     * |mode      |psrc/pdst     |
     */
    /* initialize message */
    pmessage->type = ARISC_AUDIO_GET_POSITION;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->paras[0] = mode;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("Send message to audio real-time failed.\n");
        return -EINVAL;
    }

    address = pmessage->paras[1];
    /* free allocated message */
    arisc_message_free(pmessage);
    printk("Hardware message box Auido real-time-address: %#x done.\n",
            address);
    return 0;
}

/*
 * Send message to audio
 * set audio tdm parameters.
 * @tdm_cfg: audio tdm struct
 *           mode      :in which mode; 0:play, 1;capture;
 *           samplerate:tdm samplerate depend on audio data;
 *           channel   :audio channel number, 1 or 2.
 */
static int demo_arisc_HWB_audio_tdm(void)
{
    struct arisc_message *pmessage;
    /* mode: 0 - stop play ... 1 - stop capture*/
    int mode = 0;
    unsigned int samplerate = 14400;
    unsigned int channum = 0;

    printk("ARISC: send hardware message to audio tdm.\n");
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("allocate message to audio tdm failed.\n");
        return -ENOMEM;
    }

    /*
     * package address and data to message->paras,
     * message->paras data layout:
     * |para[0]   |para[1]       |para[2]    |
     * |mode      |samplerate    |channel    |
     */
    /* initialize message */
    pmessage->type = ARISC_AUDIO_SET_TDM_PARAS;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->paras[0] = mode;
    pmessage->paras[1] = samplerate;
    pmessage->paras[2] = channum;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("Send message to audio tdm failed.\n");
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("Hardware message box Auido TDM done.\n");
    return 0;
}

/*
 * Send message to audio
 * add audio period.
 * @mode:    start audio in which mode ; 0:play, 1;capture.
 * @addr:    period address which will be add in buffer
 */
static int demo_arisc_HWB_audio_add_period(void)
{
    struct arisc_message *pmessage;
    /* mode: 0 - stop play ... 1 - stop capture*/
    int mode = 0;
    unsigned int address = 0x90000000;

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /*
     * |para[0]|para[1]|
     * |mode   |address|
     */
    /* initialize message */
    pmessage->type = ARISC_AUDIO_SET_TDM_PARAS;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->paras[0] = mode;
    pmessage->paras[1] = address;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send message to NMI
 *   disable nmi irq
 */
static int demo_arisc_HWB_disable_NMI(void)
{
    struct arisc_message *pmessage;

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message */
    pmessage->type = ARISC_AXP_DISABLE_IRQ;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send message to NMI
 *  Enable NMI irq
 */
static int demo_arisc_HWB_enable_NMI(void)
{
    struct arisc_message *pmessage;

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message */
    pmessage->type = ARISC_AXP_ENABLE_IRQ;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send message to AXP
 *  get axp chip id. 
 */
static int demo_arisc_HWB_axp_chip_id(void)
{
    struct arisc_message *pmessage;
    unsigned char chip_id[80];
    int i;

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message */
    pmessage->type = ARISC_AXP_GET_CHIP_ID;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;

    memset((void *)pmessage->paras, 0, 16);

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* |paras[0]    |paras[1]    |paras[2]     |paras[3]      |
     * |chip_id[0~3]|chip_id[4~7]|chip_id[8~11]|chip_id[12~15]|
     */
    /* copy message readout data to user data buffer */
    for (i = 0; i < 4; i++) {
        chip_id[0 + i]  = (pmessage->paras[0] >> (i * 8)) & 0xff;
        chip_id[4 + i]  = (pmessage->paras[1] >> (i * 8)) & 0xff;
        chip_id[8 + i]  = (pmessage->paras[2] >> (i * 8)) & 0xff;
        chip_id[12 + i] = (pmessage->paras[3] >> (i * 8)) & 0xff;
    }
    printk("AXP chip id:");
    for (i = 0; i < 16; i++)
        printk(" %#2x", chip_id[i]);
    printk("\n");
    
    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send message to LED bln
 */
static int demo_arisc_HWB_led_bln(void)
{
    struct arisc_message *pmessage;
    unsigned char chip_id[80];
    unsigned long led_rgb = 0, led_onms = 0, led_offms = 0, led_darkms = 0;

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message */
    pmessage->type = ARISC_SET_LED_BLN;
    pmessage->private = (void *)0;  /* set charge magic flag */
    pmessage->paras[0] = led_rgb;
    pmessage->paras[1] = led_onms;
    pmessage->paras[2] = led_offms;
    pmessage->paras[3] = led_darkms;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send message to pmu
 *   set pmu ltf and htf
 */
static int demo_arisc_HWB_axp_pmu_ltf_htf(void)
{
    struct arisc_message *pmessage;
    unsigned int lft = 3200, htf = 237;

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message */
    pmessage->type = ARISC_AXP_SET_PARAS;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->private = (void *)0x00;  /* init pmu paras flag */
    pmessage->paras[0] = lft;
    pmessage->paras[1] = htf;
    pmessage->paras[2] = 0;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send message to PMU
 *   set pmu voltage.
 */
static int demo_arisc_HWB_pmu_set_voltage(void)
{
    struct arisc_message *pmessage;
    unsigned int type = 0, voltage = 1; 

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message */
    pmessage->type = ARISC_SET_PMU_VOLT;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->paras[0] = type;
    pmessage->paras[1] = voltage;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send message to PMU
 *   get pmu voltage.
 */
static int demo_arisc_HWB_pmu_get_voltage(void)
{
    struct arisc_message *pmessage;
    unsigned int type = 0, voltage = 0;

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message */
    pmessage->type = ARISC_GET_PMU_VOLT;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->paras[0] = type;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    voltage = pmessage->paras[1];
    printk("PMU voltage %#x\n", voltage);

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send message to ARISC
 *   set arisc debug level.
 */
static int demo_arisc_HWB_set_debug_level(void)
{
    struct arisc_message *pmessage;
    unsigned int level = 2;

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message */
    pmessage->type = ARISC_SET_DEBUG_LEVEL;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->paras[0] = level;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send message to ARISC
 *   set uart baudrate
 */
static int demo_arisc_HWB_set_uart_baudrate(void)
{
    struct arisc_message *pmessage;
    unsigned int baudrate = 9600;

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message */
    pmessage->type = ARISC_SET_UART_BAUDRATE;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->paras[0] = baudrate;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send message to CPU
 *   set specific pll target frequency.
 *   @freq:    target frequency to be set, based on KHZ;
 *   @pll:     which pll will be set
 *   @mode:    the attribute of message, whether syn or asyn;
 *   @cb:      callback handler;
 *   @cb_arg:  callback handler arguments;
 *   arisc_dvfs_set_cpufreq()
 */
static int demo_arisc_HWB_set_cpufreq(void)
{
    struct arisc_message *pmessage;
    unsigned int msg_attr = 0;
    unsigned int mode = 0, freq, pll; 

    printk("[%s]ARISC: send hardware message.\n", __func__);

    if (mode & ARISC_DVFS_SYN)
        msg_attr |= ARISC_MESSAGE_ATTR_HARDSYN;

    /* allocate a message frame */
    pmessage = arisc_message_allocate(msg_attr);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message
     *
     * |paras[0]|paras[1]|
     * |freq    |pll     |
     */
    /* initialize message */
    pmessage->type = ARISC_CPUX_DVFS_REQ;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->paras[0] = freq;
    pmessage->paras[1] = pll;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send message loopback
 */
static int demo_arisc_HWB_message_loopback(void)
{
    struct arisc_message *pmessage;

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message */
    pmessage->type = ARISC_MESSAGE_LOOPBACK;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->paras[0] = 11;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send message CPU
 *   enter cpu idle.
 *   @para:  parameter for enter cpu idle.
 *      para->flag: 0x01-clear pending, 0x10-enter cpuidle
 *      para->resume_addr: the address cpu0 will run when exit idle
 */
static int demo_arisc_HWB_enter_cpuidle(void)
{
    struct arisc_message *pmessage;
    unsigned int flags = 0x01;
    unsigned int mpidr = (unsigned int)(unsigned long)demo_arisc_HWB_message_loopback;

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message */
    pmessage->type = ARISC_CPUIDLE_ENTER_REQ;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->paras[0] = flags;
    pmessage->paras[0] = mpidr;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send meesage to CPUIdle
 */
static int demo_arisc_HWB_config_cpuidle(void)
{
    struct arisc_message *pmessage;
    unsigned int flags = 0x01;
    unsigned int mpidr = (unsigned int)(unsigned long)demo_arisc_HWB_message_loopback;

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(0);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message */
    pmessage->type = ARISC_CPUIDLE_CFG_REQ;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;
    pmessage->paras[0] = flags;
    pmessage->paras[0] = mpidr;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}

/*
 * Send message to CPU
 *   enter super standby.
 *   @para:  parameter for enter normal standby.
 */
static int demo_arisc_HWB_standby_super(void)
{
    struct arisc_message *pmessage;

    printk("[%s]ARISC: send hardware message.\n", __func__);
    /* allocate a message frame */
    pmessage = arisc_message_allocate(0);
    if (pmessage == NULL) {
        printk("[%s]allocate message failed.\n", __func__);
        return -ENOMEM;
    }

    /* initialize message */
    pmessage->type = ARISC_SSTANDBY_ENTER_REQ;
  //pmessage->type = ARISC_ESSTANDBY_ENTER_REQ;
    pmessage->state = ARISC_MESSAGE_INITIALIZED;
    pmessage->cb.handler = NULL;
    pmessage->cb.arg = NULL;

    /* Send message use hwmsgbox */
    arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

    /* check config fail or not */
    if (pmessage->result) {
        printk("[%s]Send message failed.\n", __func__);
        return -EINVAL;
    }

    /* free allocated message */
    arisc_message_free(pmessage);
    printk("[%s]Hardware message done.\n", __func__);
    return 0;
}


static __init int demo_arisc_hwmessagebox_init(void)
{
    /* send hardware message to DVFS */
    demo_arisc_HMB_DVFS();
    /* send hardware message to audio to play or capture */
    demo_arisc_HMB_audio_start();
    /* send hardware message to audio to stop play or capture. */
    demo_arisc_HWB_audio_stop();
    /* send hardware message to audio to set buffer and period */
    demo_arisc_HWB_audio_buffer_period();
    /* send hardware message to audio get real-time address */
    demo_arisc_HWB_audio_read_time();
    /* send hardware message to audio get TDM */
    demo_arisc_HWB_audio_tdm();
    /* send hardware message to audio to add period */
    demo_arisc_HWB_audio_add_period();
    /* send hardware message to NMI: disable NMI irq */
    demo_arisc_HWB_disable_NMI();
    /* send hardware message to NMI: enable NMI irq */
    demo_arisc_HWB_enable_NMI();
    /* send hardware message to AXP: get chip id */
    demo_arisc_HWB_axp_chip_id();
    /* send hardware message to LED */
    demo_arisc_HWB_led_bln();
    /* send hardware message to axp: set ltf and htf */
    demo_arisc_HWB_axp_pmu_ltf_htf();
    /* send hardware message to pmu: set voltage */
    demo_arisc_HWB_pmu_set_voltage();
    /* send hardware message to pmu: get voltage */
    demo_arisc_HWB_pmu_get_voltage();
    /* send hardware message to set debug level */
    demo_arisc_HWB_set_debug_level();
    /* send hardware message to set uart baudrate */
    demo_arisc_HWB_set_uart_baudrate();
    /* send hardware message to DVFS */
    demo_arisc_HWB_set_cpufreq();
    /* send hardware meesage to loopback */
    demo_arisc_HWB_message_loopback();
    /* send hardware message to CPU enter IDLE */
    demo_arisc_HWB_enter_cpuidle();
    /* send hardware message to config CPU IDLE */
    demo_arisc_HWB_config_cpuidle();
    /* send hardware message to standby */
    demo_arisc_HWB_standby_super();

    return 0;
}

late_initcall(demo_arisc_hwmessagebox_init);
