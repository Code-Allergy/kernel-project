#include <stdarg.h>

#include <stdint.h>
#include <stddef.h>



#include <kernel/log.h>
#include <kernel/time.h>
#include <kernel/spinlock.h>
#include <kernel/heap.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/timer.h>

static const char* log_level_strings[] = {
    "[DEBUG]",
    "[INFO]",
    "[WARN]",
    "[ERROR]",
    "[FATAL]"
};

static const char* log_level_colors[] = {
    "\033[0;36m", // DEBUG
    "\033[0;32m", // INFO
    "\033[0;33m", // WARN
    "\033[0;31m", // ERROR
    "\033[0;35m"  // FATAL
};

static const char* log_level_reset_color = "\033[0m";

// default log level for now dynamically is always DEBUG
enum LOG_LEVEL current_log_level = DEBUG;

struct log_msg {
  uint64_t timestamp; // as TICKS
  int level;
  const char* file;
  const char* func;
  int line;
  char message[LOG_MAX_MESSAGE_SIZE];
};

// we can make this ring buffer generic later on, for now we only use it for logging
struct ring_buffer {
    struct log_msg buffer[LOG_MAX_BUFFER_LENGTH];
    size_t head;               // Write position
    size_t tail;               // Read position
    spinlock_t lock;
};

static struct ring_buffer log_buffer;

void ring_buffer_init(struct ring_buffer *rb) {
    rb->head = 0;
    rb->tail = 0;
    spinlock_init(&rb->lock);
}

int ring_buffer_write(struct ring_buffer *rb, struct log_msg* data) {
    spinlock_acquire(&rb->lock);

    size_t next_head = (rb->head + 1) % LOG_MAX_BUFFER_LENGTH;
    if (next_head == rb->tail) {
        // Buffer is full (handle overflow or return false)
        spinlock_release(&rb->lock);
        return 1;
    }

    memcpy(&rb->buffer[rb->head], data, sizeof(struct log_msg));
    rb->head = next_head;
    spinlock_release(&rb->lock);
    return 0;
}

int ring_buffer_read(struct ring_buffer *rb, struct log_msg* data) {
    spinlock_acquire(&rb->lock);

    if (rb->tail == rb->head) {
        // Buffer is empty
        spinlock_release(&rb->lock);
        return 1;
    }

    memcpy(data, &rb->buffer[rb->tail], sizeof(struct log_msg));

    rb->tail = (rb->tail + 1) % LOG_MAX_BUFFER_LENGTH;
    spinlock_release(&rb->lock);
    return 0;
}

int ring_buffer_empty(struct ring_buffer *rb) {
    return rb->tail == rb->head;
}

// for now just copy the whole message into the log message
void format_log_message(
    struct log_msg* msg,
    const char* fmt,
    va_list args
) {
    // format the raw message
    char user_msg[LOG_MAX_MESSAGE_SIZE];
    va_list args_copy;
    va_copy(args_copy, args);
    vsnprintf(user_msg, sizeof(user_msg), fmt, args_copy);
    va_end(args_copy);

    #ifdef USE_ASCII_COLOR // use ascii escape codes for color
    const char* color = log_level_colors[msg->level];
    const char* reset = log_level_reset_color;
    #else
    const char* color = "";
    const char* reset = "";
    #endif

    // Start building the formatted message
    char *buf = msg->message;
    size_t remaining = LOG_MAX_MESSAGE_SIZE;
    int written = 0;

    // Add color prefix
    written = snprintf(buf, remaining, "%s", color);
    buf += written;
    remaining -= written;

    // Add log level
    written = snprintf(buf, remaining, "%-7s ", log_level_strings[msg->level]);
    buf += written;
    remaining -= written;

    // Add timestamp (if enabled)
    #ifdef LOG_TIME
    uint64_t all_usec = clock_timer.ticks_to_us(msg->timestamp);
    uint32_t sec = all_usec / 1000000;
    uint32_t usec = all_usec % 1000000;

    written = snprintf(buf, remaining, "[%u.%07u] ", sec, usec);
    buf += written;
    remaining -= written;
    #endif

    // Add file and line information
    #if defined(LOG_FILE_NAME) || defined(LOG_LINE_NUM)
    written = snprintf(buf, remaining, "[");
    buf += written;
    remaining -= written;

    #ifdef LOG_FILE_NAME
    written = snprintf(buf, remaining, "%s", msg->file);
    buf += written;
    remaining -= written;
    #endif

    #ifdef LOG_LINE_NUM
    #ifdef LOG_FILE_NAME
    written = snprintf(buf, remaining, ":");
    buf += written;
    remaining -= written;
    #endif
    written = snprintf(buf, remaining, "%d", msg->line);
    buf += written;
    remaining -= written;
    #endif

    written = snprintf(buf, remaining, "] ");
    buf += written;
    remaining -= written;
    #endif

    // Add function name if enabled
    #ifdef LOG_FUNC_NAME
    written = snprintf(buf, remaining, "[%s] ", msg->func);
    buf += written;
    remaining -= written;
    #endif

    // Add user message
    written = snprintf(buf, remaining, "%s%s", user_msg, reset);
    buf += written;
    remaining -= written;

    // Ensure null-termination
    msg->message[LOG_MAX_MESSAGE_SIZE - 1] = '\0';
}


void log_commit(int level, const char *file, const char *func, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    struct log_msg msg;
    msg.timestamp = clock_timer.get_ticks();
    msg.level = level;
    msg.file = file;
    msg.line = line;
    msg.func = func;

    format_log_message(&msg, fmt, args);
    va_end(args);

    ring_buffer_write(&log_buffer, &msg);
}


void log_consume(void) {
    while (!ring_buffer_empty(&log_buffer)) {
        struct log_msg msg;
        ring_buffer_read(&log_buffer, &msg);
        // for now, just print the message over uart
        printk("%s", msg.message);
    }
}
