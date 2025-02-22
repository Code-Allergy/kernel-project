#include <kernel/vfs.h>
#include <kernel/errno.h>
#include <kernel/heap.h>
#include <kernel/string.h>
#include <kernel/panic.h>
#include <kernel/uart.h>

static ssize_t uart0_read(vfs_inode_t* inode, void* buffer, size_t count, off_t offset) {
    panic("unimplemented!\n");
    // need some kind of process blocking here first

    return count;
}

static ssize_t uart0_write(vfs_inode_t* inode, const void* buffer, size_t count, off_t offset) {
    (void)inode, (void)offset;

    for (size_t i = 0; i < count; i++) uart_driver.putc(((char*)buffer)[i]);

    return count;
}

// Device operations structure
static vfs_ops_t uart0_vfs_ops = {
    .read = uart0_read,
    .write = uart0_write,
    .open = vfs_default_open,     // TODO: check later on that uart is actually open first
    .close = vfs_default_close,   // TODO: we could close the uart here
    .readdir = NULL,              // Not applicable for char devices
    .lookup = NULL,               // Not applicable for char devices
};

// Function to create and register the ones device
int uart0_vfs_device_init(void) {
    vfs_inode_t* inode = (vfs_inode_t*)kmalloc(sizeof(vfs_inode_t));
    if (!inode) {
        return -1;
    }

    // Initialize the inode
    memset(inode, 0, sizeof(vfs_inode_t));
    inode->mode = VFS_CHR | S_IRUSR | S_IWUSR; // Character device with read/write permissions
    inode->ops = &uart0_vfs_ops;

    // Create dentry for the device
    vfs_dentry_t* dentry = (vfs_dentry_t*)kmalloc(sizeof(vfs_dentry_t));
    if (!dentry) {
        kfree(inode);
        return -1;
    }

    memset(dentry, 0, sizeof(vfs_dentry_t));
    strcpy(dentry->name, "uart0");
    dentry->inode = inode;

    // get the dev directory
    vfs_dentry_t* dev_directory = vfs_finddir("/dev");
    if (!dev_directory) panic("Failed to find /dev directory when loading critical device!");

    if (dev_directory) {
        dentry->parent = dev_directory;
        dentry->next_sibling = dev_directory->first_child;
        dev_directory->first_child = dentry;
    }

    LOG(INFO, "Mounted virtual char device 'uart0' at /dev/uart0\n");
    return 0;
}
