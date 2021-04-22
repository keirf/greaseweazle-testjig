/*
 * util.h
 * 
 * Utility definitions.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

/* Fast memset/memcpy: Pointers must be word-aligned, count must be a non-zero 
 * multiple of 32 bytes. */
void memset_fast(void *s, int c, size_t n);
void memcpy_fast(void *dest, const void *src, size_t n);

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);
int strcmp_ci(const char *s1, const char *s2); /* case insensitive */
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *dest, const char *src);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
int tolower(int c);
int toupper(int c);
int isspace(int c);

long int strtol(const char *nptr, char **endptr, int base);

void qsort_p(void *base, unsigned int nr,
             int (*compar)(const void *, const void *));

uint32_t rand(void);

unsigned int popcount(uint32_t x);

int vsnprintf(char *str, size_t size, const char *format, va_list ap)
    __attribute__ ((format (printf, 3, 0)));

int snprintf(char *str, size_t size, const char *format, ...)
    __attribute__ ((format (printf, 3, 4)));

#define le16toh(x) (x)
#define le32toh(x) (x)
#define htole16(x) (x)
#define htole32(x) (x)
#define be16toh(x) _rev16(x)
#define be32toh(x) _rev32(x)
#define htobe16(x) _rev16(x)
#define htobe32(x) _rev32(x)

#if !defined(NDEBUG)
/* Log output, to serial console or logfile. */
int vprintk(const char *format, va_list ap)
    __attribute__ ((format (printf, 1, 0)));
int printk(const char *format, ...)
    __attribute__ ((format (printf, 1, 2)));
#else /* NDEBUG && !LOGFILE */
static inline int vprintk(const char *format, va_list ap) { return 0; }
static inline int printk(const char *format, ...) { return 0; }
#endif

#if !defined(NDEBUG)
/* Serial console control */
void console_init(void);
void console_sync(void);
void console_crash_on_input(void);
#else /* NDEBUG */
#define console_init() ((void)0)
#define console_sync() IRQ_global_disable()
#define console_crash_on_input() ((void)0)
#endif

/* CRC-CCITT */
uint16_t crc16_ccitt(const void *buf, size_t len, uint16_t crc);

/* Display: 3-digit 7-segment display */
void led_7seg_init(void);
void led_7seg_write_raw(const uint8_t *d);
void led_7seg_write_string(const char *p);
void led_7seg_write_decimal(unsigned int val);
void led_7seg_display_setting(bool_t enable);
int led_7seg_nr_digits(void);

/* USB stack processing */
void usbh_cdc_init(void);
void usbh_cdc_buffer_set(uint8_t *buf);
void usbh_cdc_process(void);
bool_t usbh_cdc_connected(void);

/* Build info. */
extern const char fw_ver[];
extern const char build_date[];
extern const char build_time[];

/* Text/data/BSS address ranges. */
extern char _stext[], _etext[];
extern char _smaintext[], _emaintext[];
extern char _sdat[], _edat[], _ldat[];
extern char _sbss[], _ebss[];

/* Stacks. */
extern uint32_t _thread_stacktop[], _thread_stackbottom[];
extern uint32_t _irq_stacktop[], _irq_stackbottom[];

/* Default exception handler. */
void EXC_unused(void);

/* IRQ priorities, 0 (highest) to 15 (lowest). */
#define RESET_IRQ_PRI         0
#define TIMER_IRQ_PRI         4
#define USB_IRQ_PRI          14
#define CONSOLE_IRQ_PRI      15

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
