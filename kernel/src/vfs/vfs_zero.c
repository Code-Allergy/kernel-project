#include <kernel/vfs.h>
#include <kernel/errno.h>
#include <kernel/heap.h>
#include <kernel/string.h>
#include <kernel/panic.h>

static ssize_t zero_read(vfs_file_t* file, void* buffer, size_t count) {
    (void)file; // Unused parameters
    char* buf = (char*)buffer;

    // Fill the buffer with 0s
    for (size_t i = 0; i < count; i++) {
        buf[i] = '\0';
    }

    return count;
}

static ssize_t zero_write(vfs_file_t* file, const void* buffer, size_t count) {
    (void)file, (void)buffer, (void)count;
    return -1; // Return error, device is read-only
}

// Device operations structure
static vfs_ops_t ones_ops = {
    .read = zero_read,
    .write = zero_write,
    .open = vfs_default_open,     // Use default VFS open
    .close = vfs_default_close,   // Use default VFS close
    .readdir = NULL,              // Not applicable for char devices
    .lookup = NULL,               // Not applicable for char devices
};

// Function to create and register the ones device
int zero_device_init(void) {
    vfs_inode_t* inode = (vfs_inode_t*)kmalloc(sizeof(vfs_inode_t));
    if (!inode) {
        return -1;
    }

    // Initialize the inode
    memset(inode, 0, sizeof(vfs_inode_t));
    inode->mode = VFS_CHR | S_IRUSR | S_IWUSR; // Character device with read/write permissions
    inode->ops = &ones_ops;

    // Create dentry for the device
    vfs_dentry_t* dentry = (vfs_dentry_t*)kmalloc(sizeof(vfs_dentry_t));
    if (!dentry) {
        kfree(inode);
        return -1;
    }

    memset(dentry, 0, sizeof(vfs_dentry_t));
    strcpy(dentry->name, "zero");
    dentry->inode = inode;

    // get the dev directory
    vfs_dentry_t* dev_directory = vfs_finddir("/dev");
    if (!dev_directory) panic("Failed to find /dev directory when loading critical device!");

    if (dev_directory) {
        dentry->parent = dev_directory;
        dentry->next_sibling = dev_directory->first_child;
        dev_directory->first_child = dentry;
    }

    LOG(INFO, "Mounted virtual char device 'zero' at /dev/zero\n");
    return 0;
}
