#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#include <kernel/string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static int uint64_to_str(char *buf, size_t len, uint64_t num, int base, int uppercase) {
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    char temp[32];
    int pos = 0;
    int written = 0;

    if (num == 0) {
        temp[pos++] = '0';
    } else {
        while (num > 0 && pos < sizeof(temp)-1) {
            temp[pos++] = digits[num % base];
            num /= base;
        }
    }

    // Reverse digits
    for (int i = pos-1; i >= 0; i--) {
        if (written < (int)len-1) {
            buf[written++] = temp[i];
        }
    }

    return written;
}

static int int64_to_str(char *buf, size_t len, int64_t num, int base) {
    int written = 0;

    if (num < 0) {
        if (written < (int)len-1) {
            buf[written++] = '-';
        }
        num = -num;
    }

    return written + uint64_to_str(buf + written, len - written, num, base, 0);
}


static int uint_to_str(char *buf, size_t len, unsigned int num, int base) {
    const char *digits = "0123456789abcdef";
    char temp[32];
    int pos = 0;
    int written = 0;

    if (num == 0) {
        temp[pos++] = '0';
    } else {
        while (num > 0 && pos < sizeof(temp)-1) {
            temp[pos++] = digits[num % base];
            num /= base;
        }
    }

    // Reverse digits
    for (int i = pos-1; i >= 0; i--) {
        if (written < (int)len-1) {
            buf[written++] = temp[i];
        }
    }

    return written;
}

static int int_to_str(char *buf, size_t len, int num, int base) {
    int written = 0;

    if (num < 0) {
        if (written < (int)len-1) {
            buf[written++] = '-';
        }
        num = -num;
    }

    return written + uint_to_str(buf + written, len - written, num, base);
}

// TODO - improve feature set
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args) {
    int written = 0;
    const char *p = fmt;
    char num_buf[32];
    int pad_width, pad_zero, length;

    while (*p && written < (int)size) {
        if (*p != '%') {
            buf[written++] = *p++;
            continue;
        }

        const char *start = p++;
        int left_justify = 0;
        pad_zero = 0;
        pad_width = 0;
        length = 0;

        // Parse flags
        while (1) {
            switch (*p) {
                case '-': left_justify = 1; p++; break;
                case '0': pad_zero = 1; p++; break;
                default: goto parse_width;
            }
        }

parse_width:
        // Parse width
        if (*p == '*') {
            pad_width = va_arg(args, int);
            p++;
        } else {
            while (*p >= '0' && *p <= '9') {
                pad_width = pad_width * 10 + (*p - '0');
                p++;
            }
        }

        // Parse length
        if (*p == 'l') {
            length++;
            p++;
            if (*p == 'l') {
                length++;
                p++;
            }
        }

        char spec = *p;
        int base = 10;
        int is_signed = 0;
        int uppercase = 0;
        uint64_t num_u = 0;
        int64_t num_s = 0;
        char *str = NULL;
        int prefix_len = 0;
        char prefix[4] = {0};

        // Handle specifier
        switch (spec) {
            case 'd':
            case 'i':
                is_signed = 1;
                if (length == 0) num_s = va_arg(args, int);
                else if (length == 1) num_s = va_arg(args, long);
                else num_s = va_arg(args, long long);
                break;

            case 'u':
            case 'x':
            case 'X':
                if (length == 0) num_u = va_arg(args, unsigned int);
                else if (length == 1) num_u = va_arg(args, unsigned long);
                else num_u = va_arg(args, unsigned long long);

                if (spec == 'x' || spec == 'X') {
                    base = 16;
                    uppercase = (spec == 'X');
                }
                break;

            case 'p':
                num_u = (uintptr_t)va_arg(args, void*);
                base = 16;
                prefix[0] = '0';
                prefix[1] = 'x';
                prefix_len = 2;
                break;

            case 's':
                str = va_arg(args, char*);
                break;

            case 'c': {
                char c = (char)va_arg(args, int);
                str = &c;
                break;
            }

            case '%':
                str = "%";
                break;

            default:
                // Unknown specifier
                p = start;
                buf[written++] = *p++;
                continue;
        }

        // Format numbers
        int num_len = 0;
        if (str) {
            num_len = str ? strlen(str) : 0;
        } else if (is_signed) {
            num_len = int64_to_str(num_buf, sizeof(num_buf), num_s, base);
        } else {
            num_len = uint64_to_str(num_buf, sizeof(num_buf), num_u, base, uppercase);
        }

        // Calculate padding
        int total_len = prefix_len + num_len;
        int pad_space = MAX(pad_width - total_len, 0);
        int pad_left = left_justify ? 0 : pad_space;
        int pad_right = left_justify ? pad_space : 0;

        // Write padding left
        if (!left_justify && pad_left > 0) {
            char pad_char = pad_zero ? '0' : ' ';
            for (int i = 0; i < pad_left && written < (int)size; i++) {
                buf[written++] = pad_char;
            }
        }

        // Write prefix
        for (int i = 0; i < prefix_len && written < (int)size; i++) {
            buf[written++] = prefix[i];
        }

        // Write content
        if (str) {
            strncpy(buf + written, str, size - written);
            written += MIN(num_len, size - written);
        } else {
            strncpy(buf + written, num_buf, size - written);
            written += MIN(num_len, size - written);
        }

        // Write padding right
        for (int i = 0; i < pad_right && written < (int)size; i++) {
            buf[written++] = ' ';
        }

        p++;
    }

    // Null-terminate
    if (size > 0) {
        buf[written < (int)size ? written : size-1] = '\0';
    }

    return written;
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return len;
}
