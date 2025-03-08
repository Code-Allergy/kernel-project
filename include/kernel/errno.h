#ifndef KERNEL_ERRNO_H
#define KERNEL_ERRNO_H

/* Error pointer definitions */
#define MAX_ERRNO 4095  /* Maximum value for errno */

/* Convert an error code to an error pointer */
#define ERR_PTR(error) ((void *)((long)(error)))

/* Check if a pointer is an error pointer */
#define IS_ERR(ptr) ((unsigned long)(ptr) >= (unsigned long)(-MAX_ERRNO))

/* Extract the error code from an error pointer */
#define PTR_ERR(ptr) ((long)(ptr))

/* Check if a pointer is NULL or an error pointer */
#define IS_ERR_OR_NULL(ptr) (!ptr || IS_ERR(ptr))

#define ECHILD 10
#define ENOMEM 12
#define EINVAL 22
#define ENOSYS 38
#define ENOTDIR 20
#define ENOENT 2
#define EEXIST 17
#define EBADF 9
#define ENOTSUP 95
#define EAGAIN 11
#define EIO 5
#define EMFILE 24

#endif // KERNEL_ERRNO_H
