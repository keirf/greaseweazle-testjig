/*
 * main.c
 * 
 * System initialisation and navigation main loop.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

#include "usb/stm32_usbh/inc/usbh_cdc.h"

int EXC_reset(void) __attribute__((alias("main")));

static uint32_t usbh_buf[512/8];

static uint8_t bus[] = { CMD_SET_BUS_TYPE, 3, BUS_SHUGART };
static uint8_t busrsp[] = { CMD_SET_BUS_TYPE, 0 };

static uint8_t select[] = { CMD_SELECT, 3, 0 };
static uint8_t selectrsp[] = { CMD_SELECT, 0 };

static uint8_t seek[] = { CMD_SEEK, 3, 80 };
static uint8_t seekrsp[] = { CMD_SEEK, ACK_NO_TRK0 };

static uint8_t rspbuf[64];

static int state = 0;

#define ERR_TX_TIMEOUT      10
#define ERR_TX_BAD_CALLBACK 11
#define ERR_RX_TIMEOUT      20
#define ERR_RX_BAD_CALLBACK 21
#define ERR_BAD_RESPONSE    30

enum {
    CMDRSP_IDLE = 0,
    CMDRSP_TX_BUSY,
    CMDRSP_TX_DONE,
    CMDRSP_RX_BUSY,
    CMDRSP_RX_DONE
};
static struct {
    uint8_t *rsp;
    unsigned int rsp_len;
    unsigned int state;
    time_t time;
} cmdrsp;

static void error(unsigned int nr) __attribute__((noreturn));
static void error(unsigned int nr)
{
    char s[4];
    snprintf(s, sizeof(s), "E%02u", nr);
    for (;;) {
        if (!usbh_cdc_connected())
            system_reset();
        led_7seg_write_string(s);
        delay_ms(500);
        if (!usbh_cdc_connected())
            system_reset();
        led_7seg_write_decimal(state);
        delay_ms(500);
    }
}

static void success(void) __attribute__((noreturn));
static void success(void)
{
    led_7seg_write_string("---");
    while (usbh_cdc_connected())
        continue;
    system_reset();
}

void USBH_CDC_TransmitCallback(void)
{
    printk("TX DONE\n");
    if (cmdrsp.state != CMDRSP_TX_BUSY)
        error(ERR_TX_BAD_CALLBACK);
    cmdrsp.state = CMDRSP_TX_DONE;
}

void USBH_CDC_ReceiveCallback(void)
{
    printk("RX DONE %02x %02x\n", rspbuf[0], rspbuf[1]);
    if (cmdrsp.state != CMDRSP_RX_BUSY)
        error(ERR_RX_BAD_CALLBACK);
    cmdrsp.state = CMDRSP_RX_DONE;
}

static void command_response_handle(void)
{
    int i;

    switch (cmdrsp.state) {
    case CMDRSP_TX_BUSY:
        if (time_since(cmdrsp.time) > time_ms(500))
            error(ERR_TX_TIMEOUT);
        break;
    case CMDRSP_TX_DONE:
        printk("Receiving %d\n", cmdrsp.rsp_len);
        USBH_CDC_Receive(rspbuf, cmdrsp.rsp_len);
        cmdrsp.state = CMDRSP_RX_BUSY;
        cmdrsp.time = time_now();
        break;
    case CMDRSP_RX_BUSY:
        if (time_since(cmdrsp.time) > time_ms(5000))
            error(ERR_RX_TIMEOUT);
        break;
    case CMDRSP_RX_DONE:
        if (memcmp(rspbuf, cmdrsp.rsp, cmdrsp.rsp_len)) {
            printk("RX Mismatch: [ ");
            for (i = 0; i < cmdrsp.rsp_len; i++)
                printk("%02x ", rspbuf[i]);
            printk("] != [ ");
            for (i = 0; i < cmdrsp.rsp_len; i++)
                printk("%02x ", cmdrsp.rsp[i]);
            printk("]\n");
            error(ERR_BAD_RESPONSE);
        }
        cmdrsp.state = CMDRSP_IDLE;
        break;
    }
}

static void command_response(uint8_t *cmd, unsigned int cmd_len,
                             uint8_t *rsp, unsigned int rsp_len)
{
    USBH_CDC_Transmit(cmd, cmd_len);
    cmdrsp.state = CMDRSP_TX_BUSY;
    cmdrsp.time = time_now();

    cmdrsp.rsp = rsp;
    cmdrsp.rsp_len = rsp_len;
}

int main(void)
{
    /* Relocate DATA. Initialise BSS. */
    if (_sdat != _ldat)
        memcpy(_sdat, _ldat, _edat-_sdat);
    memset(_sbss, 0, _ebss-_sbss);

    stm32_init();
    time_init();
    console_init();
    console_crash_on_input();
    pins_init();
    delay_ms(200); /* 5v settle */

    printk("\n** Greaseweazle TestBoard v%s for Gotek\n", fw_ver);
    printk("** Keir Fraser <keir.xen@gmail.com>\n");
    printk("** https://github.com/keirf/FlashFloppy\n\n");

    led_7seg_init();

    usbh_cdc_init();
    usbh_cdc_buffer_set((void *)usbh_buf);
    led_7seg_write_string("USB");

    print_pinmask(read_pinmask());

    for (;;) {
        usbh_cdc_process();
        if (!usbh_cdc_connected()) {
            if (state || cmdrsp.state)
                system_reset();
            continue;
        }
        if (cmdrsp.state) {
            command_response_handle();
            continue;
        }

        state++;
        led_7seg_write_decimal(state);
        switch (state) {
        case 1:
            command_response(bus, sizeof(bus),
                             busrsp, sizeof(busrsp));
            break;
        case 2:
            command_response(select, sizeof(select),
                             selectrsp, sizeof(selectrsp));
            break;
        case 3:
            command_response(seek, sizeof(seek),
                             seekrsp, sizeof(seekrsp));
            break;
        case 4:
            success();
            break;
        }
    }

    return 0;
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
