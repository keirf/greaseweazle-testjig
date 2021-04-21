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
static uint8_t select[] = { CMD_SELECT, 3, 0 };
static uint8_t seek[] = { CMD_SEEK, 3, 80 };
static uint8_t ack[2];

static volatile int state = 0;

void USBH_CDC_TransmitCallback(void)
{
    printk("TX DONE\n");
    USBH_CDC_Receive(ack, 2);
}

void USBH_CDC_ReceiveCallback(void)
{
    printk("RX DONE %02x %02x\n", ack[0], ack[1]);
    state++;
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
    delay_ms(200); /* 5v settle */

    printk("\n** Greaseweazle TestBoard v%s for Gotek\n", fw_ver);
    printk("** Keir Fraser <keir.xen@gmail.com>\n");
    printk("** https://github.com/keirf/FlashFloppy\n\n");

    display_init();

    usbh_cdc_init();

    for (;;) {

        usbh_cdc_buffer_set((void *)usbh_buf);
        for (;;) {
            led_7seg_write_decimal(state);
            usbh_cdc_process();
            if (!usbh_cdc_connected()) {
                state = 0;
                continue;
            }
            switch (state) {
            case 0:
                USBH_CDC_Transmit(bus, sizeof(bus));
                state++;
                break;
            case 1:
                break;
            case 2:
                USBH_CDC_Transmit(select, sizeof(select));
                state++;
                break;
            case 3:
                break;
            case 4:
                USBH_CDC_Transmit(seek, sizeof(seek));
                state++;
                break;
            case 5:
                break;
            }
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
