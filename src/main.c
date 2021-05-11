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

static int outp[] = { 8, 26, 28, 30, 34 };
static int inp[] = { 2, 4, 6, 10, 12, 14, 16, 18, 20, 22, 24, 32, 33 };

static uint32_t usbh_buf[512/8];

static uint8_t info[] = { CMD_GET_INFO, 3, 0 };

static uint8_t testmode[] = { CMD_TEST_MODE, 10, 0x4e, 0x4b, 0x50, 0x6e,
                            0xd3, 0x10, 0x29, 0x38 };
static uint8_t testmodersp[] = { CMD_TEST_MODE, 0 };

static uint8_t rspbuf[64];

static struct cmd tcmd;
static struct rsp trsp;

struct gw_info gw_info;

static int state = 0;

#define CMD_option_bytes 0
#define CMD_pins         1
#define CMD_led          2

struct cmd {
    uint32_t cmd;
    union {
        uint8_t pins[64/8];
        uint32_t x[28/4];
    } u;
};

struct rsp {
    union {
        uint8_t opt[32];
        uint8_t pins[64/8];
        uint32_t x[32/4];
    } u;
};

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

static void _error(char *s) __attribute__((noreturn));
static void _error(char *s)
{
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

static void error(unsigned int nr) __attribute__((noreturn));
static void error(unsigned int nr)
{
    char s[4];
    snprintf(s, sizeof(s), "E%02u", nr);
    _error(s);
}

static void pin_error(unsigned int nr) __attribute__((noreturn));
static void pin_error(unsigned int nr)
{
    char s[4];
    snprintf(s, sizeof(s), "P%02u", nr);
    _error(s);
}

void USBH_CDC_TransmitCallback(void)
{
    if (cmdrsp.state != CMDRSP_TX_BUSY)
        error(ERR_TX_BAD_CALLBACK);
    cmdrsp.state = CMDRSP_TX_DONE;
}

void USBH_CDC_ReceiveCallback(void)
{
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
        USBH_CDC_Receive(rspbuf, cmdrsp.rsp_len);
        cmdrsp.state = CMDRSP_RX_BUSY;
        cmdrsp.time = time_now();
        break;
    case CMDRSP_RX_BUSY:
        if (time_since(cmdrsp.time) > time_ms(5000))
            error(ERR_RX_TIMEOUT);
        break;
    case CMDRSP_RX_DONE:
        if (cmdrsp.rsp && memcmp(rspbuf, cmdrsp.rsp, cmdrsp.rsp_len)) {
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

static void command_response(void *cmd, unsigned int cmd_len,
                             void *rsp, unsigned int rsp_len)
{
    USBH_CDC_Transmit(cmd, cmd_len);
    cmdrsp.state = CMDRSP_TX_BUSY;
    cmdrsp.time = time_now();

    cmdrsp.rsp = rsp;
    cmdrsp.rsp_len = rsp_len;
}

static void cmd_led(int state)
{
    memset(&tcmd, 0, sizeof(tcmd));
    tcmd.cmd = CMD_led;
    tcmd.u.pins[0] = state;
    command_response(&tcmd, sizeof(tcmd),
                     NULL, sizeof(trsp));
}

static void cmd_set_pin(int pin)
{
    memset(&tcmd, 0, sizeof(tcmd));
    tcmd.cmd = CMD_pins;
    memset(tcmd.u.pins, 0xff, sizeof(tcmd.u.pins));
    if (pin >= 0)
        tcmd.u.pins[pin/8] &= ~(1<<(pin&7));
    command_response(&tcmd, sizeof(tcmd),
                     NULL, sizeof(trsp));
}

static void check_pins(int asserted_pin)
{
    int i;
    pinmask_t mask = read_pinmask();

    memcpy(&trsp, rspbuf, sizeof(trsp));
    for (i = 0; i < ARRAY_SIZE(outp); i++) {
        int pin = outp[i];
        int level = !!(trsp.u.pins[pin/8] & (1<<(pin&7)));
        if (((pin == asserted_pin) && level)
            || ((pin != asserted_pin) && !level))
            pin_error(pin);
    }

    for (i = 0; i < ARRAY_SIZE(inp); i++) {
        int pin = inp[i];
        int level = (mask >> pin) & 1;
        if (((pin == asserted_pin) && level)
            || ((pin != asserted_pin) && !level))
            pin_error(pin);
    }
}

int main(void)
{
    int pin_iter = 0;
    int outer_iter = 0;
    int success = FALSE;

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
        if (!success)
            led_7seg_write_decimal(state);
        switch (state) {
        case 1:
            command_response(info, sizeof(info),
                             NULL, 34);
            break;
        case 2: {
            memcpy(&gw_info, rspbuf+2, sizeof(gw_info));
            printk("v%d.%d %d model:%d.%d\n",
                   gw_info.fw_major, gw_info.fw_minor,
                   gw_info.max_cmd, gw_info.hw_model, gw_info.hw_submodel);
            if ((gw_info.max_cmd < CMD_MAX)
                || !gw_info.is_main_firmware)
                error(ERR_BAD_RESPONSE);
            break;
        }
        case 3:
            command_response(testmode, sizeof(testmode),
                             testmodersp, sizeof(testmodersp));
            break;

            /* Drive the GW outputs (testboard inputs) one by one. */
        case 4:
            if (pin_iter >= ARRAY_SIZE(inp)) {
                pin_iter = 0;
                if (outer_iter++ > 10) {
                    outer_iter = 0;
                    state = 6-1; /* break */
                    break;
                }
            }
            cmd_set_pin(inp[pin_iter]);
            break;
        case 5:
            check_pins(inp[pin_iter]);
            pin_iter++;
            state = 4-1; /* loop */
            break;

            /* Check all pins HIGH */
        case 6:
            cmd_set_pin(-1);
            break;
        case 7:
            check_pins(-1);
            break;

            /* Drive the GW inputs (testboard outputs) one by one. */
        case 8:
            if (pin_iter >= ARRAY_SIZE(outp)) {
                pin_iter = 0;
                if (outer_iter++ > 10) {
                    outer_iter = 0;
                    state = 11-1; /* break */
                    break;
                }
            }
            set_pinmask(-1LL & ~(1ull << outp[pin_iter]));
            break;
        case 9:
            cmd_set_pin(-1);
            break;
        case 10:
            check_pins(outp[pin_iter]);
            pin_iter++;
            state = 8-1; /* loop */
            break;

            /* Check all pins LOW */
        case 11:
            set_pinmask(0);
            memset(&tcmd, 0, sizeof(tcmd));
            tcmd.cmd = CMD_pins;
            command_response(&tcmd, sizeof(tcmd),
                             NULL, sizeof(trsp));
            break;
        case 12: {
            int i;
            pinmask_t mask;
            delay_ms(100); /* linger with drivers working */
            mask = read_pinmask();
            memcpy(&trsp, rspbuf, sizeof(trsp));
            for (i = 0; i < ARRAY_SIZE(outp); i++) {
                int pin = outp[i];
                int level = !!(trsp.u.pins[pin/8] & (1<<(pin&7)));
                if (level)
                    pin_error(pin);
            }
            for (i = 0; i < ARRAY_SIZE(inp); i++) {
                int pin = inp[i];
                int level = (mask >> pin) & 1;
                if (level)
                    pin_error(pin);
            }
            break;
        }

            /* (AT32F4) Check option bytes 
             * From factory:
             *  a5 5a ff ff ff ff ff ff ff ff ff ff ff ff ff ff 
             * OpenOCD stm32f1x unlock 0: 
             *  a5 5a ff 00 ff 00 ff 00 ff 00 ff 00 ff 00 ff 00 */
        case 13:
            if (gw_info.hw_model != 4) {
                /* Skip for other models */
                state = 15-1;
                break;
            }
            memset(&tcmd, 0, sizeof(tcmd));
            tcmd.cmd = CMD_option_bytes;
            command_response(&tcmd, sizeof(tcmd),
                             NULL, sizeof(trsp));
            break;
        case 14: {
            int i;
            memcpy(&trsp, rspbuf, sizeof(trsp));
            if ((trsp.u.opt[0] != 0xa5)
                || (trsp.u.opt[1] != 0x5a))
                _error("OPT");
            for (i = 2; i < 16; i += 2)
                if (trsp.u.opt[i] != 0xff)
                    _error("OPT");
            break;
        }

            /* Finish and flash the LED */
        case 15:
            set_pinmask(-1LL);
            cmd_set_pin(-1);
            led_7seg_write_string("---");
            success = TRUE;
            break;
        case 16:
            delay_ms(100);
            cmd_led(1);
            break;
        case 17:
            delay_ms(100);
            cmd_led(0);
            state = 16-1;
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
