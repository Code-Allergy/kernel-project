#ifndef KERNEL_FILE_H
#define KERNEL_FILE_H
#include <kernel/vfs.h>

#define MAX_FDS 32

typedef size_t off_t;

/* Per–process file descriptor table entry */
typedef struct file {
    vfs_node_t *vnode;  /* Pointer to the underlying VFS node */
    off_t offset;       /* Current file offset */
    int flags;          /* Open flags (e.g., O_RDONLY, etc.) */
} file_t;


#endif // KERNEL_FILE_H
