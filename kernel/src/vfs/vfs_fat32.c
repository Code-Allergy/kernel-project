// this can be moved into fat32.c file later after we port.
#include "kernel/time.h"
#include <stdbool.h>

#include <kernel/sched.h>
#include <kernel/fat32.h>
#include <kernel/vfs.h>
#include <kernel/panic.h>
#include <kernel/heap.h>
#include <kernel/mmc.h>
#include <kernel/file.h>
#include <kernel/string.h>

#define FAT32_ATTR_READ_ONLY 0x01
#define FAT32_ATTR_HIDDEN 0x02
#define FAT32_ATTR_SYSTEM 0x04
#define FAT32_ATTR_VOLUME_ID 0x08
#define FAT32_ATTR_DIRECTORY 0x10
#define FAT32_ATTR_ARCHIVE 0x20
#define FAT32_ATTR_LONG_NAME 0x0F


struct fat32_inode_private {
    fat32_fs_t* fs;         // Pointer to the mounted FAT32 filesystem
    fat32_file_t* file;     // Pointer to the file structure
    uint32_t cluster;       // Cluster number for this inode
    uint32_t attributes;    // File attributes (e.g., directory, file)
    // Add other necessary fields (size, creation time, etc.)
};

static vfs_inode_t* vfs_fat32_mount(vfs_mount_t* fat32_mnt, const char* device) {
    // ignore device for now, we only have one device
    (void)device;
    // if (!mount || !device) {
    //     return NULL;
    // }

    if (!fat32_mnt) {
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

    fat32_mnt->fs_data = fs;
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
    fs_root_private->attributes = FAT32_ATTR_DIRECTORY; // directory
    fs_root->mode |= VFS_DIR;
    // TODO the rest

    vfs_dentry_t* root_dentry = kmalloc(sizeof(vfs_dentry_t));
    if (!root_dentry) {
        kfree(fs);
        kfree(fs_root);
        kfree(fs_root_private);
        return NULL;
    }

    root_dentry->inode = fs_root;
    root_dentry->mount = NULL;


    // get the mount point
    vfs_dentry_t* mnt_dir = vfs_finddir("/mnt");
    if (!mnt_dir) panic("No /mnt default path created!\n");

    // // setup the mount point in the vfs
    mnt_dir->mount = fat32_mnt;
    fat32_mnt->mountpoint = mnt_dir;
    fat32_mnt->root = root_dentry;
    fat32_mnt->device = NULL;

    // panic("unimplemented!\n");
    return fs_root; // TODO
}

static int vfs_fat32_unmount(vfs_mount_t* mount) {
    (void) mount;
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
    (void)fd;
    panic("unimplemented!\n");

    return -1;
}


static ssize_t fat32_vfs_read(vfs_inode_t* inode, void* buff, size_t len, off_t offset) {
    int ret = 0;
    struct fat32_inode_private* inode_private = inode->private_data;
    fat32_file_t *file = inode_private->file;

    if ((ret = fat32_read(file, buff, len)) < (int)len) {
        return -1; // TODO error codes
    }

    return len;
}

static ssize_t fat32_vfs_write(vfs_inode_t* inode, const void* buff, size_t len, off_t offset) {
    (void)inode, (void)buff, (void)len, (void)offset;
    panic("unimplemented!\n");
    return -1;
}

static void format_83_filename(const uint8_t fat_name[11], char* output) {
    char name[9], ext[4];

    // Extract name (8 chars) and extension (3 chars)
    memcpy(name, fat_name, 8);
    memcpy(ext, fat_name + 8, 3);

    // Trim trailing spaces
    name[8] = '\0';
    ext[3] = '\0';

    // Find last non-space character in name
    int name_len = 7;
    while (name_len >= 0 && name[name_len] == ' ') name_len--;
    name[name_len + 1] = '\0';

    // Find last non-space character in extension
    int ext_len = 2;
    while (ext_len >= 0 && ext[ext_len] == ' ') ext_len--;
    ext[ext_len + 1] = '\0';

    // Special cases
    if (name[0] == 0x05) {  // Japanese Kanji special marker
        name[0] = 0xE5;
    }

    if (name[0] == 0xE5) {  // Deleted entry
        output[0] = '?';
        output[1] = '\0';
        return;
    }

    // Build final name
    if (ext_len >= 0) {  // Has extension
        snprintf(output, VFS_MAX_FILELEN, "%s.%s", name, ext);
    } else {
        strncpy(output, name, VFS_MAX_FILELEN);
    }

    // Handle volume labels
    if (strcmp(name, "NO NAME") == 0 && ext_len < 0) {
        strcpy(output, ".");
    }
}

static uint8_t compute_checksum(const uint8_t* short_name) {
    uint8_t checksum = 0;
    for (int i = 0; i < 11; i++) {
        checksum = ((checksum >> 1) | (checksum << 7)) + short_name[i];
    }
    return checksum;
}

// ascii only
void append_utf16_to_lfn(const uint16_t* name_part, int count, char* lfn_buffer) {
    for (int i = 0; i < count; i++) {
        uint16_t ch = name_part[i];
        if (ch == 0x0000) return;  // End of filename

        // Simple conversion: use lower byte (works for ASCII)
        // For full UTF-8, implement proper conversion
        char c = (ch & 0xFF);
        if (c != 0xFF && c != 0x00) {  // Skip padding
            strncat(lfn_buffer, &c, 1);
        }
    }
}

static int fat32_vfs_readdir(vfs_dentry_t* dir, dirent_t* buffer, size_t buffer_sz) {
    struct fat32_inode_private* inode_private = dir->inode->private_data;
    fat32_fs_t* fs = inode_private->fs;

    uint32_t current_cluster = inode_private->cluster;
    size_t max_entries = buffer_sz / sizeof(dirent_t);
    uint32_t buffer_idx = 0;

    char lfn_buffer[256] = {0};  // Stores the accumulated LFN
    uint8_t lfn_checksum = 0;
    int lfn_sequence = -1;

    do {
        // Calculate first sector of cluster
        uint32_t sector_start = fs->first_data_sector +
                              (current_cluster - 2) * fs->sectors_per_cluster;

        // Read all sectors in cluster
        for (uint32_t sector = 0; sector < fs->sectors_per_cluster; sector++) {
            uint8_t sector_buffer[FAT32_SECTOR_SIZE];

            // Read sector through disk interface
            int ret = fs->disk.read_sector(sector_start + sector, sector_buffer);
            if (ret != 0) return -1;  // Error

            // Process directory entries
            for (uint32_t i = 0; i < 16; i++) {
                Fat32DirectoryEntry* entry = (Fat32DirectoryEntry*)sector_buffer + i;

                if (entry->attr == FAT32_ATTR_LONG_NAME) {
                    Fat32LFNDirectoryEntry* lfn_entry = (Fat32LFNDirectoryEntry*)entry;

                    // Get sequence number (strip highest bit flag)
                    int seq = lfn_entry->sequence_number & 0x1F;
                    if (seq == 0) continue;  // Invalid entry

                    // First LFN entry in the sequence has 0x40 flag
                    if (lfn_entry->sequence_number & 0x40) {
                        lfn_sequence = seq;
                        memset(lfn_buffer, 0, sizeof(lfn_buffer));
                        lfn_checksum = lfn_entry->checksum;
                    }

                    // Extract UTF-16LE characters from name1/name2/name3 fields
                    append_utf16_to_lfn(lfn_entry->name1, 5, lfn_buffer);
                    append_utf16_to_lfn(lfn_entry->name2, 6, lfn_buffer);
                    append_utf16_to_lfn(lfn_entry->name3, 2, lfn_buffer);

                    continue;  // Skip to next entry
                }

                if (entry->filename[0] == 0x00) return buffer_idx;  // End of entries
                if (entry->filename[0] == 0xE5) continue;  // Skip deleted

                if (lfn_sequence > 0) {
                    // Verify checksum matches the 8.3 entry
                    if (compute_checksum((const uint8_t*)entry->filename) == lfn_checksum) {
                        strncpy(buffer[buffer_idx].d_name, lfn_buffer, VFS_MAX_FILELEN);
                    }
                    lfn_sequence = -1;  // Reset LFN state
                } else {
                    // Fallback to 8.3 name
                    format_83_filename((const uint8_t*)entry->filename, buffer[buffer_idx].d_name);
                }

                buffer_idx++;
                if (buffer_idx >= max_entries) return buffer_idx;
            }
        }

        // Move to next cluster via FAT
        current_cluster = fat32_get_next_cluster(fs, current_cluster);
    } while (current_cluster != FAT32_EOC_MARKER);

    return buffer_idx;
}

static vfs_dentry_t* fat32_create_dentry(vfs_inode_t* inode, fat32_file_t* file, const char* name) {
    vfs_dentry_t* dentry = kmalloc(sizeof(vfs_dentry_t));
    if (!dentry) {
        panic("OUT of memory");
    }

    vfs_inode_t* new_inode = kmalloc(sizeof(vfs_inode_t));
    if (!new_inode) {
        panic("OUT of memory");
    }

    struct fat32_inode_private* inode_private = kmalloc(sizeof(struct fat32_inode_private));
    if (!inode_private) {
        panic("OUT of memory");
    }

    strcpy(dentry->name, name);
    dentry->inode = new_inode;
    dentry->parent = NULL;
    dentry->first_child = NULL;
    dentry->next_sibling = NULL;
    dentry->mount = NULL;

    new_inode->mode = VFS_DIR;// inode_private->attributes; TODO
    new_inode->flags = 0;
    new_inode->size = file->file_size;
    new_inode->uid = 0;
    new_inode->gid = 0;
    time_t now = epoch_now();

    new_inode->atime = now; // TODO read from fat32
    new_inode->mtime = now; // TODO read from fat32
    new_inode->ctime = now; // TODO read from fat32

    new_inode->ops = &fat32_filesystem_ops;
    new_inode->private_data = inode_private;
    new_inode->ref_count = 1;

    inode_private->fs = file->fs;
    inode_private->file = file;
    inode_private->cluster = file->start_cluster;
    inode_private->attributes = 0; // TODO read from fat32

    return dentry;
}

static vfs_dentry_t* fat32_vfs_finddir(vfs_dentry_t* dir, const char* name) {
    (void)dir, (void)name;
    struct fat32_inode_private* inode_private = dir->inode->private_data;

    if (!(inode_private->attributes & FAT32_ATTR_DIRECTORY)) {
        return NULL; // Not a directory, so return NULL
    }

    // if (strcmp(name, ".") == 0) {
    //     return dir;
    // }

    // if (strcmp(name, "..") == 0) {
    //     return dir->parent;
    // }

    // check if the path is more than one level deep
    // for now assume it isn't

    fat32_file_t* file = kmalloc(sizeof(fat32_file_t));
    if (!file) {
        panic("OUT of memory");
    }

    if (fat32_open(inode_private->fs, name, file) != 0) {
        return NULL;
    }

    vfs_dentry_t *dentry = fat32_create_dentry(dir->inode, file, name);

    return dentry;
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

    vfs_inode_t* node = vfs_fat32_mount(mount, NULL);

    LOG(INFO, "Mounting FAT32 filesystem at /mnt (WIP)\n");
}
