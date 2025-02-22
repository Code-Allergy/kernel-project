#include "kernel/sched.h"
#include "kernel/syscall.h"
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

static const char* log_syscall_string = "[ SVC ]";

// default log level for now dynamically is always DEBUG
enum LOG_LEVEL current_log_level = DEBUG;

struct log_msg {
  uint64_t timestamp; // as TICKS
  int level;
  const char* file;
  const char* func;
  int line;
  uint32_t current_pid;
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

static inline void get_ascii_colour(enum LOG_LEVEL level, const char** colour, const char** reset) {
    #ifdef USE_ASCII_COLOR // use ascii escape codes for color
    *colour = log_level_colors[level];
    *reset = log_level_reset_color;
    #else
    *color = "";
    *reset = "";
    #endif
}

static inline void add_color_prefix(char** buf, const char* colour, size_t* remaining) {
    int written = snprintf(*buf, *remaining, "%s", colour);
    *buf += written;
    *remaining -= written;
}

static inline void add_timestamp(struct log_msg* msg, char** buf, size_t* remaining) {
    #ifdef LOG_TIME
    int written = 0;
    uint64_t all_usec = clock_timer.ticks_to_us(msg->timestamp);
    uint32_t sec = all_usec / 1000000;
    uint32_t usec = all_usec % 1000000;

    written = snprintf(*buf, *remaining, "[%u.%07u] ", sec, usec);
    *buf += written;
    *remaining -= written;
    #endif
}

static inline void add_log_level(struct log_msg* msg, char** buf, size_t* remaining) {
    int written = snprintf(*buf, *remaining, "%-7s", log_level_strings[msg->level]);
    *buf += written;
    *remaining -= written;
}

static inline void add_syscall_prefix(char** buf, size_t* remaining) {
    int written = snprintf(*buf, *remaining, "%s", log_syscall_string);
    *buf += written;
    *remaining -= written;
}

static inline void add_fileline(struct log_msg* msg, char** buf, size_t* remaining) {
    // Add file and line information
    #if defined(LOG_FILE_NAME) || defined(LOG_LINE_NUM)
    int written = 0;
    written = snprintf(*buf, *remaining, "[");
    *buf += written;
    *remaining -= written;

    #ifdef LOG_FILE_NAME
    written = snprintf(*buf, *remaining, "%s", msg->file);
    *buf += written;
    *remaining -= written;
    #endif

    #ifdef LOG_LINE_NUM
    #ifdef LOG_FILE_NAME
    written = snprintf(*buf, *remaining, ":");
    *buf += written;
    *remaining -= written;
    #endif
    written = snprintf(*buf, *remaining, "%d", msg->line);
    *buf += written;
    *remaining -= written;
    #endif

    written = snprintf(*buf, *remaining, "] ");
    *buf += written;
    *remaining -= written;
    #endif
}

static inline void add_function(struct log_msg* msg, char** buf, size_t* remaining) {
    #ifdef LOG_FUNCTION_NAME
    int written = snprintf(*buf, *remaining, "[%s] ", msg->func);
    *buf += written;
    *remaining -= written;
    #endif
}

static inline void add_user_message(char** buf, size_t* remaining, char* user_msg) {
    int written = snprintf(*buf, *remaining, "%s", user_msg);
    *buf += written;
    *remaining -= written;
}

static inline void add_color_reset(char** buf, const char* reset, size_t* remaining) {
    int written = snprintf(*buf, *remaining, "%s", reset);
    *buf += written;
    *remaining -= written;
}

static inline void add_newline(char** buf, size_t* remaining) {
    int written = snprintf(*buf, *remaining, "\n");
    *buf += written;
    *remaining -= written;
}

static inline void add_current_pid(char** buf, size_t* remaining) {
    #ifdef LOG_PID
    int written = snprintf(*buf, *remaining, "[PID: %d] ", current_process->pid);
    *buf += written;
    *remaining -= written;
    #endif
}

static inline void add_current_ppid(char** buf, size_t* remaining) {
    #ifdef LOG_PPID
    int written = snprintf(*buf, *remaining, "[PPID: %d] ", current_process->ppid);
    *buf += written;
    *remaining -= written;
    #endif
}

static inline void add_syscall_message(
    char** buf,
    size_t* remaining,
    uint32_t syscall_num
) {
    uint32_t* user_stack = current_process->stack_top;
    int written;
    switch (syscall_table[syscall_num].num_args) {
        case 0: written = snprintf(*buf, *remaining, "%s()", syscall_table[syscall_num].name); break;
        case 1: written = snprintf(*buf, *remaining, "%s(%d)", syscall_table[syscall_num].name, user_stack[0]); break;
        case 2: written = snprintf(*buf, *remaining, "%s(%d, %d)", syscall_table[syscall_num].name, user_stack[0], user_stack[1]); break;
        case 3: written = snprintf(*buf, *remaining, "%s(%d, %d, %d)", syscall_table[syscall_num].name, user_stack[0], user_stack[1], user_stack[2]); break;
        case 4: written = snprintf(*buf, *remaining, "%s(%d, %d, %d, %d)", syscall_table[syscall_num].name, user_stack[0], user_stack[1], user_stack[2], user_stack[3]); break;
    }
    *buf += written;
    *remaining -= written;
}

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

    const char* color, *reset;
    get_ascii_colour(msg->level, &color, &reset);

    // Start building the formatted message
    char *buf = msg->message;
    size_t remaining = LOG_MAX_MESSAGE_SIZE;

    add_color_prefix(&buf, color, &remaining);
    add_log_level(msg, &buf, &remaining);
    add_timestamp(msg, &buf, &remaining);
    add_fileline(msg, &buf, &remaining);
    add_function(msg, &buf, &remaining);
    add_user_message(&buf, &remaining, user_msg);
    add_color_reset(&buf, reset, &remaining);

    // Ensure null-termination
    msg->message[LOG_MAX_MESSAGE_SIZE - 1] = '\0';
}




void log_commit(int level, const char *file, const char *func, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    struct log_msg msg;
    msg.current_pid = current_process ? current_process->pid : -1;
    msg.timestamp = clock_timer.get_ticks();
    msg.level = level;
    msg.file = file;
    msg.line = line;
    msg.func = func;

    format_log_message(&msg, fmt, args);
    va_end(args);

    ring_buffer_write(&log_buffer, &msg);
}


// SYSCALL LOGGING

void format_log_syscall_message(int syscall_num, struct log_msg* msg) {
    char *buf = msg->message;
    size_t remaining = LOG_MAX_MESSAGE_SIZE;
    const char* color, *reset;
    get_ascii_colour(msg->level, &color, &reset);

    // start
    add_color_prefix(&buf, color, &remaining);
    add_syscall_prefix(&buf, &remaining);


    // body
    add_timestamp(msg, &buf, &remaining);
    add_current_pid(&buf, &remaining);
    add_current_ppid(&buf, &remaining);
    add_syscall_message(&buf, &remaining, syscall_num);


    // end
    add_color_reset(&buf, reset, &remaining);
    add_newline(&buf, &remaining);

    // Ensure null-termination
    msg->message[LOG_MAX_MESSAGE_SIZE - 1] = '\0';
}

void log_syscall_commit(uint32_t syscall) {
    struct log_msg msg;
    msg.current_pid = current_process ? current_process->pid : -1;
    msg.timestamp = clock_timer.get_ticks();
    msg.level = INFO; // unused field?
    msg.file = "";
    msg.line = 0;
    msg.func = "";

    format_log_syscall_message(syscall, &msg);

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
