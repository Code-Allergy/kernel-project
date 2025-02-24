#include <kernel/vfs.h>
#include <kernel/errno.h>
#include <kernel/heap.h>
#include <kernel/string.h>
#include <kernel/panic.h>
#include <kernel/uart.h>


static int circular_read(char* dest_buffer, size_t count) {
    size_t available;

    // Calculate available data (handle wrap-around)
    if (uart_driver.incoming_buffer_head >= uart_driver.incoming_buffer_tail) {
        available = uart_driver.incoming_buffer_head - uart_driver.incoming_buffer_tail;
    } else {
        available = UART0_INCOMING_BUFFER_SIZE - uart_driver.incoming_buffer_tail + uart_driver.incoming_buffer_head;
    }

    // Nothing to read
    if (available == 0) return 0;

    // Determine actual bytes to read
    size_t bytes_to_read = (count < available) ? count : available;

    // Calculate contiguous space until buffer end
    size_t first_chunk = UART0_INCOMING_BUFFER_SIZE - uart_driver.incoming_buffer_tail;
    if (first_chunk > bytes_to_read) {
        first_chunk = bytes_to_read;
    }

    // Copy first contiguous segment
    memcpy(dest_buffer, uart_driver.incoming_buffer + uart_driver.incoming_buffer_tail, first_chunk);

    // Handle wrap-around if needed
    if (bytes_to_read > first_chunk) {
        size_t second_chunk = bytes_to_read - first_chunk;
        memcpy(dest_buffer + first_chunk, uart_driver.incoming_buffer, second_chunk);
    }

    // Update tail position with wrap-around
    uart_driver.incoming_buffer_tail = (uart_driver.incoming_buffer_tail + bytes_to_read) % UART0_INCOMING_BUFFER_SIZE;

    return bytes_to_read;
}

static ssize_t uart0_read(vfs_file_t* file, void* buffer, size_t count) {
    (void)file, (void)buffer, (void)count;
    if (!(file->flags & OPEN_MODE_READ)) return -EBADF;

    // for now, only non blocking reads are supported until we have wait queues
    if (!(file->flags & OPEN_MODE_NOBLOCK)) return -ENOTSUP;

    // get count bytes from the uart
    ssize_t bytes_read = circular_read((char*)buffer, count);
    if (bytes_read <= 0) return -EAGAIN;

    return count;
}

// TODO verify address is valid
static ssize_t uart0_write(vfs_file_t* file, const void* buffer, size_t count) {
    (void)file;

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
