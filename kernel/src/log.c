#include <kernel/stdarg.h>

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
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
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
enum LOG_LEVEL current_log_level = INFO;

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
    struct log_msg* buffer[LOG_MAX_BUFFER_LENGTH];
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

    rb->buffer[rb->head] = data;
    rb->head = next_head;
    spinlock_release(&rb->lock);
    return 0;
}

int ring_buffer_read(struct ring_buffer *rb, struct log_msg** data) {
    spinlock_acquire(&rb->lock);

    if (rb->tail == rb->head) {
        // Buffer is empty
        spinlock_release(&rb->lock);
        return 1;
    }

    *data = rb->buffer[rb->tail];
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
    // vsnprintf(user_msg, sizeof(user_msg), fmt, args_copy);
    va_end(args_copy);

    strncpy(msg->message, user_msg, LOG_MAX_MESSAGE_SIZE-1);

}


void log_commit(int level, const char *file, const char *func, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    struct log_msg* msg = (struct log_msg*) kmalloc(sizeof(struct log_msg));
    if (msg == NULL) {
        va_end(args);
        return; // we can direct serial output here, we're probably out of memory or fucked the heap
    }
    msg->timestamp = clock_timer.get_ticks();
    msg->level = level;
    msg->file = file;
    msg->line = line;
    msg->func = func;

    format_log_message(msg, fmt, args);
    va_end(args);

    ring_buffer_write(&log_buffer, msg);
}


void log_consume(void) {
    while (!ring_buffer_empty(&log_buffer)) {
        struct log_msg* msg;
        ring_buffer_read(&log_buffer, &msg);
        // for now, just print the message out
        printk("%s", msg->message);
        kfree(msg);
    }
}
