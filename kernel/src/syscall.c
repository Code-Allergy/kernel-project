#include "elf32.h"
#include <kernel/vfs.h>
#include <kernel/syscall.h>
#include <kernel/printk.h>
#include <stdint.h>
#include <kernel/mmu.h>
#include <kernel/boot.h>
#include <kernel/string.h>
#include <kernel/heap.h>
#include <kernel/sched.h>
#include <kernel/paging.h>
#include <kernel/panic.h>
#include <kernel/errno.h>
#include <kernel/file.h>
#include <kernel/time.h>
#include <kernel/timer.h>
#include <kernel/sleep.h>

#define __user

// we will use some macros later to clean up the warnings

// here for now, later we can move this
// copy len bytes to current_process dest from src
int copy_to_user(uint8_t* __user dest, const uint8_t* src, size_t len) {
    if (!mmu_driver.is_user_addr((uint32_t) dest, len)) {
        panic("failed tto check if user addr");
        return -1;
    }

    // TODO: any other checks? we don't need to copy to kvirt memory, because of ttbr0/ttbr1
    memcpy(dest, src, len);
    return 0;
}

int copy_from_user(uint8_t* dest, const uint8_t* __user src, size_t len) {
    if (!mmu_driver.is_user_addr((uint32_t) src, len)) {
        panic("failed to check if user addr");
        return -1;
    }

    memcpy(dest, src, len);
    return 0;
}

DEFINE_SYSCALL0(fork) {
    process_t* child = create_process(NULL, current_process);
    if (!child) {
        return -1;
    }

    child->forked = 1;
    child->stack_top[0] = 0; // return value of fork in child is 0
    mmu_driver.set_l1_with_asid(current_process->ttbr0, current_process->asid);

    return child->pid; // return value of fork in parent is child's pid
}
END_SYSCALL

DEFINE_SYSCALL0(getpid) {
    return current_process->pid;
}
END_SYSCALL

DEFINE_SYSCALL0(yield) {
    scheduler_driver.schedule_next = 1;
    current_process->state = PROCESS_READY;
    return 0;
}
END_SYSCALL

// for now
DEFINE_SYSCALL2(open, const char*, path, int, flags) {
    if (!path) return -EINVAL;

    vfs_dentry_t *dentry = vfs_root_node->inode->ops->lookup(vfs_root_node, path);

    // if (!dentry) {
    //     if (!(flags & O_CREAT)) return -ENOENT; // no file and no create flag
    //     // TODO handle flag
    //     // find the parent directory instead
    // }

    // check if the inode has operations and an open function
    if (!dentry->inode || !dentry->inode->ops || !dentry->inode->ops->open) {
        return -ENOTSUP; // Operation not supported
    }

    return dentry->inode->ops->open(dentry, flags);
}
END_SYSCALL

DEFINE_SYSCALL1(close, int, fd) {
    int res = vfs_root_node->inode->ops->close(fd);

    return res;
}
END_SYSCALL

DEFINE_SYSCALL3(read, int, fd, char*, buff, size_t, count) {
    if (fd < 0 || !buff || count == 0) {
        return -EINVAL; // Invalid arguments
    }

    vfs_file_t* file = current_process->fd_table[fd];
    if (!file) {
        return -EBADF; // Bad file descriptor
    }

    if (!file->dirent->inode || !file->dirent->inode->ops || !file->dirent->inode->ops->read) {
        return -ENOTSUP; // Operation not supported
    }

    ssize_t bytes_read = file->dirent->inode->ops->read(file, buff, count);

    // update the offset
    file->offset += bytes_read;

    return bytes_read;
}
END_SYSCALL

DEFINE_SYSCALL3(write, int, fd, const char*, buff, size_t, count) {
    if (fd < 0 || !buff || count == 0) {
        return -EINVAL; // Invalid arguments
    }

    vfs_file_t* file = current_process->fd_table[fd];
    if (!file) {
        return -EBADF; // Bad file descriptor
    }

    if (!file->dirent->inode || !file->dirent->inode->ops || !file->dirent->inode->ops->write) {
        return -ENOTSUP; // Operation not supported
    }

    return file->dirent->inode->ops->write(file, buff, count);
}
END_SYSCALL


DEFINE_SYSCALL3(readdir, int, fd, struct dirent*, buf, size_t, len) {
    if (fd < 0 || !buf || len == 0) {
        return -EINVAL; // Invalid arguments
    }

    vfs_file_t* file = current_process->fd_table[fd];
    if (!file) {
        return -EBADF; // Bad file descriptor
    }

    vfs_dentry_t* dir = file->dirent;
    if (!(S_ISDIR(dir->inode))) {
        return -ENOTDIR; // Not a directory
    }

    if (!file->dirent->inode || !file->dirent->inode->ops || !file->dirent->inode->ops->readdir) {
        return -ENOTSUP; // Operation not supported
    }

    return dir->inode->ops->readdir(dir, buf, len);
}
END_SYSCALL

DEFINE_SYSCALL1(exec, char*, path) {
    if (!path) return -EINVAL;
    // check that path is valid

    vfs_dentry_t* dentry = vfs_root_node->inode->ops->lookup(vfs_root_node, path);
    if (!dentry) {
        return -ENOENT; // No such file or directory
    }

    uint8_t* buffer = (uint8_t*) kmalloc(dentry->inode->size);
    if (!buffer) {
        return -ENOMEM; // Out of memory
    }

    int fd = dentry->inode->ops->open(dentry, 0);
    if (dentry->inode->ops->read(current_process->fd_table[fd], buffer, dentry->inode->size)
        != (ssize_t)dentry->inode->size) {
            return -EIO; // I/O error
    }

    dentry->inode->ops->close(fd);
    binary_t* bin = load_elf32(buffer, dentry->inode->size);

    if (swap_process(bin, current_process) != 0) { // might need to propagate error
        return -ENOMEM; // Out of memory
    }

    return 0;
}
END_SYSCALL


DEFINE_SYSCALL2(debug, const char*, buf, int, len) {
    // TODO: we should still copy from user space, or maybe we don't need to?
    // after we have proper domain and ttbr1 setup, we can just use the user space pointer, as long as we check if it's valid
    printk("[SYS_DEBUG]: %s", buf);
    return 0;
    // syscall_return(&current_process->context, 0);
}
END_SYSCALL

// should put exit status in process and not fully free the process, just the memory and mark process as dead
DEFINE_SYSCALL1(exit, int, exit_status) {
    // iterate through all pages and free them if they are not shared, otherwise decrement the ref count
    free_process_memory(current_process);

    // // TODO free anything else in the process
    // // TODO reassign PID or parents of children
    // // TODO clean up any open files
    free_aligned_pages(&kpage_allocator, current_process->ttbr0, 4);
    memset(current_process, 0, sizeof(process_t));
    current_process->state = PROCESS_NONE;
    current_process = NULL;
    scheduler_driver.schedule_next = 1;
    return 0;
}
END_SYSCALL

// TODO - verify pointer is valid
DEFINE_SYSCALL1(time, uint64_t*, ptr) {
    if (ptr == NULL) return -1;
    epoch_t* time = (epoch_t*)ptr;
    *time = epoch_now();
    return 0;
}
END_SYSCALL

// TODO - verify pointer is valid
DEFINE_SYSCALL2(gettimeofday, timeval_t*, tv, timezone_t*, tz) {
    if (tv) tv_now(tv);
    if (tz) fill_tz(tz);

    return 0;
}
END_SYSCALL


DEFINE_SYSCALL2(usleep, uint32_t, us_high, uint32_t, us_low) {
    uint64_t us = ((uint64_t) us_high << 32) | (uint64_t) us_low;

    current_process->wake_ticks = clock_timer.get_ticks() + clock_timer.us_to_ticks(us);
    current_process->state = PROCESS_SLEEPING;

    if (sleep_queue.count < MAX_PROCESSES) {
        sleep_queue.procs[sleep_queue.count++] = current_process;
    } else {
        // this should never be able to happen
        panic("Unable to sleep! sleep_queue.count > MAX_PROCESSES!\n");
    }

    scheduler_driver.schedule_next = 1;
    return 0;
}
END_SYSCALL

DEFINE_SYSCALL3(lseek, int, fd, off_t, offset, int, when) {
    if (fd < 0) return -EBADF; // bad file descriptor
    if (when < 0 || when > 2) return -EINVAL; // invalid whence

    vfs_file_t* file = current_process->fd_table[fd];
    if (!file) return -EBADF; // bad file descriptor

    // verify that the offset is valid
    int new_offset = 0;
    switch (when) {
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            new_offset = file->offset + offset;
            break;
        case SEEK_END:
            new_offset = file->dirent->inode->size + offset;
            break;
    }

    if (new_offset < 0 || new_offset > (int32_t) file->dirent->inode->size) return -EINVAL; // invalid offset

    file->offset = new_offset;
    return new_offset;
}
END_SYSCALL

const syscall_entry_t syscall_table[NR_SYSCALLS + 1] = {
    [SYS_DEBUG]        = {{.fn2 = sys_debug},          "debug",        2},
    [SYS_EXIT]         = {{.fn1 = sys_exit},           "exit",         1},
    [SYS_GETPID]       = {{.fn0 = sys_getpid},        "getpid",        0},
    [SYS_YIELD]        = {{.fn0 = sys_yield},          "yield",        0},
    [SYS_OPEN]         = {{.fn2 = sys_open},           "open",         2},
    [SYS_CLOSE]        = {{.fn1 = sys_close},          "close",        1},
    [SYS_FORK]         = {{.fn0 = sys_fork},           "fork",         0},
    [SYS_READDIR]      = {{.fn3 = sys_readdir},        "readdir",      3},
    [SYS_READ]         = {{.fn3 = sys_read},           "read",         3},
    [SYS_WRITE]        = {{.fn3 = sys_write},          "write",        3},
    [SYS_EXEC]         = {{.fn1 = sys_exec},           "exec",         1},
    [SYS_TIME]         = {{.fn1 = sys_time},           "time",         1},
    [SYS_GETTIMEOFDAY] = {{.fn2 = sys_gettimeofday},  "gettimeofday",  2},
    [SYS_USLEEP]       = {{.fn2 = sys_usleep},        "usleep",        2},
    [SYS_LSEEK]        = {{.fn3 = sys_lseek},          "lseek",        3},
};


int handle_syscall(int num, int arg1, int arg2, int arg3, int arg4) {
    int ret = -1;
    if (num >= 0 && num <= NR_SYSCALLS) {
        switch(syscall_table[num].num_args) {
            case 0: ret = syscall_table[num].fn.fn0(); break;
            case 1: ret = syscall_table[num].fn.fn1(arg1); break;
            case 2: ret = syscall_table[num].fn.fn2(arg1, arg2); break;
            case 3: ret = syscall_table[num].fn.fn3(arg1, arg2, arg3); break;
            case 4: ret = syscall_table[num].fn.fn4(arg1, arg2, arg3, arg4); break;
        }
    }

    // if exec, then we don't need to set the return value in the process
    if (num == SYS_EXEC) return ret;

    // process doesn't necessarily exist as runnable anymore, so check for it first
    if (current_process) current_process->stack_top[0] = ret;
    return ret;
}
