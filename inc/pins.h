/*
 * pins.h
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

enum { _A = 0, _B, _C, _D, _E, _F, _G };

struct pin_mapping {
    uint8_t pin_id;
    uint8_t gpio_bank;
    uint8_t gpio_pin;
};

typedef uint64_t pinmask_t;

GPIO gpio_from_id(uint8_t id);
void pins_init(void);
void set_pinmask(pinmask_t mask);
pinmask_t read_pinmask(void);
void print_pinmask(pinmask_t mask);

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
