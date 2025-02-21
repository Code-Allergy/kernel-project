#ifndef KERNEL_VFS_H
#define KERNEL_VFS_H

#include <stddef.h>
#include <stdint.h>

#define VFS_MAX_FILELEN 256

struct vfs_ops;
struct vfs_mount;

typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint32_t time_t;
typedef uint32_t dev_t;

typedef struct vfs_inode {
    uint32_t inode_number;         // Unique identifier
    uint32_t mode;                 // File mode (permissions, type)
    uint32_t flags;                // Status flags
    size_t size;                   // File size
    uid_t uid;                     // Owner user ID
    gid_t gid;                     // Owner group ID
    time_t atime, mtime, ctime;     // Timestamps

    // device-specific data
    dev_t dev;                   // Device ID for device files
    struct device_ops* dev_ops;  // Device-specific operations

    struct vfs_mount* mount;        // Mounted filesystem
    struct vfs_ops* ops;            // Filesystem operations
    void* private_data;             // Filesystem-specific data
    uint32_t ref_count;             // Reference count for open files
} vfs_inode_t;


typedef struct vfs_dentry {
    char name[VFS_MAX_FILELEN];
    struct vfs_inode* inode;
    struct vfs_dentry* parent;
    struct vfs_dentry* first_child;
    struct vfs_dentry* next_sibling;
} vfs_dentry_t;

// userspace dirent structure
typedef struct dirent {
    uint32_t d_ino;    // Inode number
    char d_name[VFS_MAX_FILELEN];  // Filename
} dirent_t;

typedef int32_t ssize_t;
typedef size_t off_t;


typedef int (*open_fn)(vfs_dentry_t*, int flags);
typedef int (*close_fn)(int fd);
typedef ssize_t (*read_fn)(vfs_inode_t*, void*, size_t, off_t);
typedef ssize_t (*write_fn)(vfs_inode_t*, const void*, size_t, off_t);
typedef int (*readdir_fn)(vfs_inode_t*, dirent_t*, size_t); // size_t should be the BUFFER SIZE (bytes) NOT number of entries
typedef vfs_dentry_t* (*lookup_fn)(vfs_inode_t*, const char* name);

// File operations structure
typedef struct vfs_ops {
    open_fn open;
    close_fn close;
    read_fn read;
    write_fn write;
    readdir_fn readdir;
    lookup_fn lookup;

    // todo - block device
    ssize_t (*read_block)(vfs_inode_t* inode, void* buffer, size_t count, uint64_t block);
    ssize_t (*write_block)(vfs_inode_t* inode, const void* buffer, size_t count, uint64_t block);

} vfs_ops_t;

void vfs_init(void);

// File system operations
typedef struct filesystem_ops {
    vfs_inode_t* (*mount)(struct vfs_mount*, const char* device);
    int (*unmount)(struct vfs_mount*);
} filesystem_ops_t;

// Filesystem type (like ext2, fat32, etc.)
typedef struct filesystem_type {
    const char* name;
    filesystem_ops_t ops;
} filesystem_type_t;

// Mount point information
typedef struct vfs_mount {
    vfs_inode_t* mountpoint;    // Where this fs is mounted
    vfs_inode_t* root;          // Root of mounted filesystem
    filesystem_type_t* fs_type;     // Type of filesystem
    const char* device;             // Device name (if any)
    void* fs_data;                  // Filesystem-specific data
} vfs_mount_t;

typedef struct device_ops {
    /* Block device operations */
    ssize_t (*read_block)(dev_t dev, void* buffer, size_t count, uint64_t block_num);
    ssize_t (*write_block)(dev_t dev, const void* buffer, size_t count, uint64_t block_num);
    size_t block_size;   // Size of each block

    /* TODO - device-specific control operations */
    // int (*ioctl)(dev_t dev, unsigned int cmd, unsigned long arg);
} device_ops_t;

// File types
#define VFS_DIR 0x4000
#define VFS_REG 0x8000
#define VFS_CHR 0x2000
#define VFS_BLK 0x6000

#define S_ISBLK(node) (((node)->mode & VFS_BLK) == VFS_BLK)
#define S_ISCHR(node) (((node)->mode & VFS_CHR) == VFS_CHR)
#define S_ISDIR(node) (((node)->mode & VFS_DIR) == VFS_DIR)
#define S_ISREG(node) (((node)->mode & VFS_REG) == VFS_REG)

// use all linux flags lol
/* Owner permissions */
#define S_IRUSR  0400    /* Read permission for owner */
#define S_IWUSR  0200    /* Write permission for owner */
#define S_IXUSR  0100    /* Execute (or search) permission for owner */
#define S_IRWXU  (S_IRUSR | S_IWUSR | S_IXUSR)  /* Read, write, execute for owner */

/* Group permissions */
#define S_IRGRP  0040    /* Read permission for group */
#define S_IWGRP  0020    /* Write permission for group */
#define S_IXGRP  0010    /* Execute (or search) permission for group */
#define S_IRWXG  (S_IRGRP | S_IWGRP | S_IXGRP)  /* Read, write, execute for group */

/* Others permissions */
#define S_IROTH  0004    /* Read permission for others */
#define S_IWOTH  0002    /* Write permission for others */
#define S_IXOTH  0001    /* Execute (or search) permission for others */
#define S_IRWXO  (S_IROTH | S_IWOTH | S_IXOTH)  /* Read, write, execute for others */

/* Special mode bits */
#define S_ISUID  04000   /* Set user ID on execution */
#define S_ISGID  02000   /* Set group ID on execution */
#define S_ISVTX  01000   /* Sticky bit (restricted deletion flag) */

extern vfs_dentry_t* vfs_root_node;

// FAT32 filesystem
extern filesystem_type_t fat32_filesystem_type;
extern vfs_ops_t fat32_filesystem_ops;

int vfs_default_open(vfs_dentry_t* entry, int flags);
int vfs_default_close(int fd);
vfs_dentry_t* vfs_finddir(const char* path);

// temp - these should be all one mega init vfs hardware function
int zero_device_init(void);
int ones_device_init(void);
int uart0_vfs_device_init(void);

#endif // KERNEL_VFS_H
