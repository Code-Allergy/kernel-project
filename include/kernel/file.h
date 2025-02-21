#ifndef KERNEL_FILE_H
#define KERNEL_FILE_H
#include <kernel/vfs.h>

#define MAX_FDS 32

typedef size_t off_t;
typedef size_t ino_t;

/* Per–process file descriptor table entry */
typedef struct file {
    vfs_dentry_t *dirent;    /* Pointer to the underlying directory entry */
    off_t offset;           /* Current file offset */
    int flags;              /* Open flags (e.g., O_RDONLY, etc.) */
    size_t refcount;        /* Reference count */
    void* private_data;  /* Pointer to private data */
} file_t;

// struct dirent {
//     ino_t d_ino;          // Inode number
//     off_t d_off;          // Offset to the next dirent
//     uint16_t d_reclen;    // Length of this record
//     uint8_t d_type;       // Type of file (DT_REG, DT_DIR, etc.)
//     char d_name[256];     // Null-terminated filename
// } dir_t;


#endif // KERNEL_FILE_H
