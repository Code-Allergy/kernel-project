// this can be moved into fat32.c file later after we port.
#include <stdbool.h>

#include <kernel/sched.h>
#include <kernel/fat32.h>
#include <kernel/vfs.h>
#include <kernel/panic.h>
#include <kernel/heap.h>
#include <kernel/mmc.h>
#include <kernel/file.h>
#include <kernel/string.h>

struct fat32_inode_private {
    fat32_fs_t* fs;         // Pointer to the mounted FAT32 filesystem
    uint32_t cluster;       // Cluster number for this inode
    uint32_t attributes;    // File attributes (e.g., directory, file)
    // Add other necessary fields (size, creation time, etc.)
};

static vfs_inode_t* vfs_fat32_mount(vfs_mount_t* mount, const char* device) {
    // ignore device for now, we only have one device

    // if (!mount || !device) {
    //     return NULL;
    // }

    if (!mount) {
        return NULL;
    }

    fat32_fs_t* fs = kmalloc(sizeof(fat32_fs_t));
    if (!fs) {
        return NULL;
    }

    // this should use a block device, for now use our old fat32 code
    if (fat32_mount(fs, &mmc_fat32_diskio) != 0) {
        kfree(fs);
        return NULL;
    }

    mount->fs_data = fs;
    vfs_inode_t* fs_root = kmalloc(sizeof(vfs_inode_t));
    if (!fs_root) {
        kfree(fs);
        return NULL;
    }

    struct fat32_inode_private* fs_root_private = kmalloc(sizeof(struct fat32_inode_private));
    if (!fs_root_private) {
        kfree(fs);
        kfree(fs_root);
        return NULL;
    }

    // setup root fat32 fs inode
    fs_root->ops = &fat32_filesystem_ops;
    fs_root->private_data = fs_root_private;
    fs_root_private->fs = fs;
    fs_root_private->cluster = fs->root_cluster;
    fs_root_private->attributes = 0x10; // directory
    fs_root->mode |= VFS_DIR;
    // TODO the rest

    // get the mount point
    vfs_dentry_t* mnt_dir = vfs_finddir("/mnt");
    if (!mnt_dir) panic("No /mnt default path created!\n");

    // // setup the mount point in the vfs
    mnt_dir->mount = mount;
    mnt_dir->inode = fs_root;
    // mount->root = fs_root;


    // panic("unimplemented!\n");
    return fs_root; // TODO
}

static int vfs_fat32_unmount(vfs_mount_t* mount) {
    panic("unimplemented!\n");

    return -1;
}

static int fat32_vfs_open(vfs_dentry_t* dirent, int flags) {
    struct fat32_inode_private* inode_private = dirent->inode->private_data;
    fat32_file_t* file = kmalloc(sizeof(fat32_file_t));
    if (!file) {
        panic("OUT of memory");
    }

    // FOR NOW, we are reading /mnt (/) so lets just do that.
    if (fat32_open(inode_private->fs, "/elf", file) != 0) {
        return -1;
    }

    file_t* vfs_file = kmalloc(sizeof(file_t));
    if (!vfs_file) {
        panic("OUT of memory");
    }

    vfs_file->private_data = file;
    vfs_file->dirent = dirent;
    vfs_file->offset = 0;
    vfs_file->flags = flags;

    current_process->fd_table[current_process->num_fds] = vfs_file;

    return current_process->num_fds++;
}

static int fat32_vfs_close(int fd) {
    panic("unimplemented!\n");

    return -1;
}


static ssize_t fat32_vfs_read(vfs_inode_t* inode, void* buff, size_t len, off_t offset) {
    panic("unimplemented!\n");
    return -1;

}

static ssize_t fat32_vfs_write(vfs_inode_t* inode, const void* buff, size_t len, off_t offset) {
    panic("unimplemented!\n");
    return -1;
}


static int fat32_vfs_readdir(vfs_dentry_t* dir, dirent_t* buffer, size_t buffer_sz) {
    struct fat32_inode_private* inode_private = dir->inode->private_data;
    int buffer_idx = 0;
    uint32_t current_cluster;
    uint32_t sector_target;


    current_cluster = inode_private->cluster;
    sector_target = inode_private->fs->first_data_sector +
                    (current_cluster - 2) * inode_private->fs->sectors_per_cluster;

    uint8_t sector_buffer[FAT32_SECTOR_SIZE];
    Fat32DirectoryEntry *dir_entry = (Fat32DirectoryEntry *)sector_buffer;

    while (current_cluster != FAT32_EOC_MARKER) {
        for (uint32_t sector = 0; sector < inode_private->fs->sectors_per_cluster; sector++) {
            int ret = inode_private->fs->disk.read_sector(sector_target + sector, sector_buffer);
            if (ret != 0) {
                return FAT32_ERROR_IO;
            }

            // Process all directory entries in this sector
            for (uint32_t entry = 0; entry < FAT32_SECTOR_SIZE / sizeof(Fat32DirectoryEntry); entry++) {
                Fat32DirectoryEntry* current_entry = &dir_entry[entry];

                // Check if entry is empty or deleted
                if (current_entry->filename[0] == 0x00) {
                    return buffer_idx ? buffer_idx : -1;
                }
                if (current_entry->filename[0] == 0xE5) {
                    continue; // Skip deleted entry
                }

                memcpy(&buffer[buffer_idx].d_name, current_entry->filename, 11);
                buffer[buffer_idx].d_name[11] = '\0';
                buffer_idx++;
            }
        }
    }

    return buffer_idx;
}

static vfs_dentry_t* fat32_vfs_finddir(vfs_dentry_t* dir, const char* name) {
    // struct fat32_inode_private* inode_private = dir->inode->private_data;

    // if (!(inode_private->attributes & FAT32_ATTR_DIRECTORY)) {
    //     return NULL; // Not a directory, so return NULL
    // }

    panic("unimplemented!\n");
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

void init_mount_fat32(void) {
    // get mountpoint
    vfs_mount_t *mount = kmalloc(sizeof(vfs_mount_t));
    if (!mount) {
        panic("Failed to allocate memory for mount point!\n");
    }

    // vfs_inode_t* node = vfs_fat32_mount(mount, NULL);

    LOG(INFO, "Mounting FAT32 filesystem at /mnt (WIP)\n");
}
