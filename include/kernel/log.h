#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#include <stdint.h>
#include <kernel/sched.h>

#define CONFIG_LOG_LEVEL DEBUG

// try and consume the log buffer after this many ticks
#define LOG_CONSUME_TICKS 64

#define LOG_MAX_MESSAGE_SIZE 256
#define LOG_MAX_BUFFER_LENGTH 1024

#define LOG_TIME
#define LOG_LINE_NUM
#define LOG_FUNCTION_NAME
#define LOG_FILE_NAME
#define LOG_PID
#define LOG_PPID

#define TRACE_SYSCALLS         // turn this into tracing of syscalls later
#define LOG_SYSCALL_LEVEL DEBUG // the level that the syscall log messages will be logged at
#define USE_ASCII_COLOR


enum LOG_LEVEL {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

extern enum LOG_LEVEL current_log_level;

#define LOG(level, fmt, ...) \
  if (level >= CONFIG_LOG_LEVEL && level >= current_log_level) \
    log_commit(level, __FILE__, __FUNCTION__,  __LINE__, fmt, ##__VA_ARGS__)


#ifdef TRACE_SYSCALLS
#define LOG_SYSCALL(sysc)\
    log_syscall_commit(sysc)
#else
#define LOG_SYSCALL(sysc)
#endif

void log_commit(int level, const char* file, const char* func, int line, const char* fmt, ...);
void log_syscall_commit(uint32_t syscall);
void log_consume(void);

#endif
