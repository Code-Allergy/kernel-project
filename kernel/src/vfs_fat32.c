// this can be moved into fat32.c file later after we port.
#include <kernel/fat32.h>
#include <kernel/vfs.h>
#include <kernel/panic.h>
#include <kernel/heap.h>

static vfs_inode_t* vfs_fat32_mount(vfs_mount_t* mount, const char* device) {
    if (!mount || !device) {
        return NULL;
    }

    fat32_fs_t* fs = kmalloc(sizeof(fat32_fs_t));
    if (!fs) {
        return NULL;
    }


    panic("unimplemented!\n");
    return NULL;
}

static int vfs_fat32_unmount(vfs_mount_t* mount) {
    panic("unimplemented!\n");

    return -1;
}

static int fat32_vfs_open(vfs_dentry_t* dirent, int flags) {
    panic("unimplemented!\n");

    return -1;
}

static int fat32_vfs_close(int fd) {
    panic("unimplemented!\n");

    return -1;
}


static ssize_t fat32_vfs_read(vfs_inode_t* inode, void* buff, size_t len, off_t offset) {
    panic("unimplemented!\n");

}

static ssize_t fat32_vfs_write(vfs_inode_t* inode, const void* buff, size_t len, off_t offset) {
    panic("unimplemented!\n");

}

static int fat32_vfs_readdir(vfs_inode_t* inode, dirent_t* buffer, size_t buffer_sz) {
    panic("unimplemented!\n");

    return 0;
}

static vfs_dentry_t* fat32_vfs_finddir(vfs_inode_t* inode, const char* name) {
    panic("unimplemented!\n");

    return NULL;
}


vfs_ops_t fat32_filesystem_ops = {
    .open = fat32_vfs_open,
    .close = fat32_vfs_close,
    .read = fat32_vfs_read,
    .write = fat32_vfs_write,
    .readdir = fat32_vfs_readdir,
    .lookup = fat32_vfs_finddir,
};

filesystem_type_t fat32_filesystem_type = {
    .name = "fat32",
    .ops = {
        .mount = vfs_fat32_mount,
        .unmount = vfs_fat32_unmount
    }
};
