/*
 * pins.c
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

const static struct pin_mapping out_pins[] = {
    {  8, _B,  6 },
    { 26, _B, 14 },
    { 28, _B, 13 },
    { 30, _B, 12 },
    { 34, _B,  0 },
    {  0,  0,  0 }
};

const static struct pin_mapping in_pins[] = {
    {  2, _B,  9 },
    {  4, _B,  8 },
    {  6, _B,  7 },
    { 10, _B,  5 },
    { 12, _B,  4 },
    { 14, _B,  3 },
    { 16, _A, 15 },
    { 18, _F,  7 },
    { 20, _F,  6 },
    { 22, _A,  8 },
    { 24, _B, 15 },
    { 32, _B,  2 },
    { 33, _B,  1 },
    {  0,  0,  0 }
};

GPIO gpio_from_id(uint8_t id)
{
    switch (id) {
    case _A: return gpioa;
    case _B: return gpiob;
    case _C: return gpioc;
    case _F: return gpiof;
    }
    ASSERT(0);
    return NULL;
}

void pins_init(void)
{
    const struct pin_mapping *ipin, *opin;

    for (ipin = in_pins; ipin->pin_id != 0; ipin++) {
        gpio_configure_pin(gpio_from_id(ipin->gpio_bank), ipin->gpio_pin,
                           GPI_floating);
    }

    for (opin = out_pins; opin->pin_id != 0; opin++) {
        gpio_configure_pin(gpio_from_id(opin->gpio_bank), opin->gpio_pin,
                           GPO_opendrain(_2MHz, HIGH));
    }
}

pinmask_t read_pinmask(void)
{
    const struct pin_mapping *ipin;
    pinmask_t mask = 0;

    for (ipin = in_pins; ipin->pin_id != 0; ipin++) {
        if (gpio_read_pin(gpio_from_id(ipin->gpio_bank), ipin->gpio_pin))
            mask |= (uint64_t)1u << ipin->pin_id;
    }

    return mask;
}

void print_pinmask(pinmask_t mask)
{
    bool_t first = TRUE;
    int i;
    printk("[");
    for (i = 0; i < 64; i++) {
        if (mask&1) {
            if (!first) printk(",");
            printk("%d", i);
            first = FALSE;
        }
        mask >>= 1;
    }
    printk("]");
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
