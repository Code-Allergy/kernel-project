// kernel/lib/printk.c
#include <kernel/stdarg.h>
#include <kernel/uart.h>

#define MAX_NUMBER_LEN 16

static void print_char(char c) {
    uart_driver.putc(c);
}

static void print_string(const char *s) {
    while (*s) print_char(*s++);
}

static void print_hex(unsigned int num, int pad_zero) {
    const char *digits = "0123456789ABCDEF";
    char buf[8];
    int i = 0;

    if (num == 0 && pad_zero) {
        for (int j = 0; j < 8; j++) print_char('0');
        return;
    }

    do {
        buf[i++] = digits[num & 0xF];
        num >>= 4;
    } while (num > 0 || (pad_zero && i < 8));

    if (pad_zero) while (i < 8) buf[i++] = '0';

    // Print in reverse order
    while (--i >= 0) print_char(buf[i]);
}

static void print_octal(unsigned int num) {
    const char *digits = "01234567";
    char buf[12];
    int i = 0;

    do {
        buf[i++] = digits[num & 0x7];
        num >>= 3;
    } while (num > 0);

    while (--i >= 0) print_char(buf[i]);
}


static void print_binary(unsigned int num) {
    if (num == 0) {
        print_char('0');
        return;
    }

    int started = 0;
    for (int i = 31; i >= 0; i--) {
        unsigned int mask = 1 << i;
        if (num & mask || started) {
            print_char((num & mask) ? '1' : '0');
            started = 1;
            if (i % 4 == 0 && i != 0) print_char('_');
        }
    }
}

static void print_decimal(int num) {
    char buf[MAX_NUMBER_LEN];
    int i = 0;
    int is_negative = 0;

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    do {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    } while (num > 0);

    if (is_negative) buf[i++] = '-';

    while (--i >= 0) print_char(buf[i]);
}


static void print_unsigned_decimal(unsigned int num) {
    char buf[MAX_NUMBER_LEN];
    int i = 0;

    do {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    } while (num > 0);

    while (--i >= 0) print_char(buf[i]);
}


void vprintk(const char *fmt, va_list args) {
    int pad_zero = 0;

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            print_char(*fmt);
            continue;
        }

        fmt++;
        pad_zero = 0;

        if (*fmt == '0') {
            pad_zero = 1;
            fmt++;
        }

        switch (*fmt) {
            case 'd': {
                print_decimal(va_arg(args, int));
                break;
            }
            case 'u': {
                print_unsigned_decimal(va_arg(args, unsigned int));
                break;
            }
            case 'x': {
                print_hex(va_arg(args, unsigned int), pad_zero);
                break;
            }
            case 'p': {
                print_string("0x");
                print_hex(va_arg(args, unsigned int), 1);
                break;
            }
            case 'o':
                print_octal(va_arg(args, unsigned int));
                break;
            case 'b':
                print_binary(va_arg(args, unsigned int));
                break;
            case 's': {
                print_string(va_arg(args, char*));
                break;
            }
            case 'c': {
                print_char(va_arg(args, int));
                break;
            }
            case '%':
                print_char('%');
                break;
            default:
                print_char('%');
                print_char(*fmt);
                break;
        }
    }
}

void printk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
}
