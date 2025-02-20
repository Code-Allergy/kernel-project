#ifndef KERNEL_VFS_H
#define KERNEL_VFS_H

#include <stddef.h>
#include <stdint.h>


struct vfs_node;
struct vfs_mount;

// typedef ssize_t (*read_fn)(struct vfs_node*, void*, size_t, off_t);
// typedef ssize_t (*write_fn)(struct vfs_node*, const void*, size_t, off_t);
typedef int (*open_fn)(struct vfs_node*, int flags);
typedef int (*close_fn)(int fd);
typedef struct vfs_node* (*lookup_fn)(struct vfs_node*, const char* name);
typedef int (*readdir_fn)(struct vfs_node*, struct dirent*, size_t);


typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint32_t time_t;

// File operations structure
typedef struct vfs_ops {
    // read_fn read;
    // write_fn write;
    open_fn open;
    close_fn close;
    lookup_fn lookup;
    readdir_fn readdir;
} vfs_ops_t;

// File system operations
typedef struct filesystem_ops {
    struct vfs_node* (*mount)(struct vfs_mount*, const char* device);
    int (*unmount)(struct vfs_mount*);
} filesystem_ops_t;

// Filesystem type (like ext2, fat32, etc.)
typedef struct filesystem_type {
    const char* name;
    filesystem_ops_t ops;
} filesystem_type_t;

// Mount point information
typedef struct vfs_mount {
    struct vfs_node* mountpoint;    // Where this fs is mounted
    struct vfs_node* root;          // Root of mounted filesystem
    filesystem_type_t* fs_type;     // Type of filesystem
    const char* device;             // Device name (if any)
    void* fs_data;                  // Filesystem-specific data
} vfs_mount_t;

// The actual file/directory node
typedef struct vfs_node {
    char name[256];                 // Name of file/directory
    uint32_t mode;                  // Access mode and type
    uint32_t flags;                 // Status flags
    size_t size;                    // Size of file
    uid_t uid;                      // User ID
    gid_t gid;                      // Group ID
    time_t atime;                   // Access time
    time_t mtime;                   // Modification time
    time_t ctime;                   // Creation time

    vfs_ops_t* ops;                // Operations on this node
    void* private_data;            // Filesystem-specific data
    struct vfs_mount* mount;       // Mount information


    struct vfs_node* parent;       // Parent directory
    struct vfs_node* first_child;  // First child in directory
    struct vfs_node* next_sibling; // Next sibling in parent's directory
} vfs_node_t;


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


extern vfs_node_t* vfs_root_node;

void vfs_init(void);

#endif // KERNEL_VFS_H
