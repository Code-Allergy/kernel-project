#include <kernel/panic.h>
#include <kernel/sched.h>
#include <kernel/vfs.h>
#include <kernel/printk.h>
#include <kernel/heap.h>
#include <kernel/fat32.h>
#include <kernel/string.h>
#include <kernel/file.h>
#include <kernel/errno.h>

// Global root node, this is the root of the virtual filesystem at / (root)
vfs_dentry_t* vfs_root_node = NULL;



int vfs_default_open(vfs_dentry_t* entry, int flags) {
    if (!entry) {
        return -EINVAL;
    }

    // create an open file structure for the node
    file_t* file = (file_t*)kmalloc(sizeof(file_t));
    file->dirent = entry;
    file->offset = 0;
    file->flags = flags; // TODO - flags should be handled

    // TODO - handle freed-up file descriptors
    current_process->fd_table[current_process->num_fds] = file;

    return current_process->num_fds++;
}

int vfs_default_close(int fd) {
    if (fd < 0 || fd >= current_process->num_fds) {
        return -EINVAL;
    }

    kfree(current_process->fd_table[fd]);
    current_process->fd_table[fd] = NULL;

    current_process->num_fds--;
    return 0;
}

static int vfs_default_readdir(vfs_dentry_t* dir, dirent_t* buffer, size_t buffer_sz) {
    if (!dir || !buffer || buffer_sz == 0) {
        return -EINVAL; // Invalid arguments
    }

    // Check if this is actually a directory
    if (!(dir->inode->mode & VFS_DIR)) {
        return -ENOTDIR; // Not a directory
    }

    // Default behavior: traverse in-memory dentries if available
    vfs_dentry_t* entry = dir->first_child; // Start from first child
    size_t num_read = 0;
    size_t bytes_read = 0;

    while (entry && bytes_read < buffer_sz) {
        // Store pointer as inode and copy the name
        buffer[num_read].d_ino = (ino_t)entry;
        strncpy(buffer[num_read].d_name, entry->name, sizeof(buffer[num_read].d_name));

        // Move to the next entry in the directory
        entry = entry->next_sibling;
        num_read++;
        bytes_read += sizeof(dirent_t);

        // Stop if we have filled the buffer or exhausted the entries
        if (bytes_read >= buffer_sz) {
            break;
        }
    }

    // Return the number of entries read
    return num_read;
}

// static int vfs_default_readdir(vfs_dentry_t* dir, dirent_t* buffer, size_t buffer_sz) {
//     if (!dir || !buffer || buffer_sz == 0) {
//         return -EINVAL; // Invalid arguments
//     }

//     // Check if this is actually a directory
//     if (!(dir->inode->mode & VFS_DIR)) {
//         return -ENOTDIR; // Not a directory
//     }

//     // If the filesystem has a `readdir` function, delegate to it
//     if (dir->inode->ops && dir->inode->ops->readdir) {
//         return dir->inode->ops->readdir(dir, buffer, buffer_sz);
//     }

//     // Default behavior: traverse in-memory dentries if available
//     vfs_dentry_t* entry = dir->first_child; // Start from first child
//     size_t num_read = 0;
//     size_t bytes_read = 0;

//     while (entry && bytes_read < buffer_sz) {
//         buffer[num_read].d_ino = (ino_t)entry; // Store pointer as inode
//         strncpy(buffer[num_read].d_name, entry->name, sizeof(buffer[num_read].d_name));

//         // Move to the next entry
//         entry = entry->next_sibling;
//         num_read++;
//         bytes_read += sizeof(dirent_t);
//     }

//     return num_read;
// }



vfs_ops_t vfs_ops = {
    .open = vfs_default_open,
    .close = vfs_default_close,
    .readdir = vfs_default_readdir,
};

// TODO finish filling dirent with rc
vfs_dentry_t* vfs_create_dirent(const char* name, uint32_t mode) {
    vfs_inode_t* node = (vfs_inode_t*)kmalloc(sizeof(vfs_inode_t));
    if (!node) return NULL;

    memset(node, 0, sizeof(vfs_inode_t));
    vfs_dentry_t* dentry = (vfs_dentry_t*)kmalloc(sizeof(vfs_dentry_t));
    if (!dentry) return NULL;

    strncpy(dentry->name, name, sizeof(dentry->name) - 1);
    dentry->inode = node;
    node->mode = mode;

    // time_t time = get_time();
    node->atime = 0;
    node->mtime = 0;
    node->ctime = 0;

    return dentry;
}

vfs_dentry_t* vfs_init_root(void) {
    vfs_dentry_t* root = vfs_create_dirent("/", VFS_DIR);
    if (!root) panic("Failed to get memory for root vfs node!");

    root->parent = root;
    root->next_sibling = NULL;

    return root;
}

// Add a child node to a directory
int vfs_add_child(vfs_dentry_t* parent, vfs_dentry_t* child) {
    if (!parent || !child || !S_ISDIR(parent->inode)) {
        return -EINVAL;
    }

    child->parent = parent;
    if (!parent->first_child) {
        // First child
        parent->first_child = child;
    } else {
        // Add to end of sibling list
        vfs_dentry_t* sibling = parent->first_child;
        while (sibling->next_sibling) {
            sibling = sibling->next_sibling;
        }
        sibling->next_sibling = child;
    }

    return 0;
}

// Remove a child node from a directory
int vfs_remove_child(vfs_dentry_t* parent, vfs_dentry_t* child) {
    if (!parent || !child || !S_ISDIR(parent->inode)) {
        return -EINVAL;
    }

    if (parent->first_child == child) {
        // First child
        parent->first_child = child->next_sibling;
    } else {
        // Find child in sibling list
        vfs_dentry_t* sibling = parent->first_child;
        while (sibling && sibling->next_sibling != child) {
            sibling = sibling->next_sibling;
        }

        if (sibling) {
            sibling->next_sibling = child->next_sibling;
        } else {
            return -ENOENT; // Child not found
        }
    }

    child->parent = NULL;
    child->next_sibling = NULL;
    return 0;
}

// Find a child node by name in a directory
vfs_dentry_t* vfs_find_child(vfs_dentry_t* dir, const char* name) {
    if (!dir || !S_ISDIR(dir->inode)) {
        return NULL;
    }

    vfs_dentry_t* child = dir->first_child;
    while (child != NULL) {
        if (strcmp(child->name, name) == 0) {
            return child;
        }
        child = child->next_sibling;
    }

    return NULL;
}

void list_directories(vfs_dentry_t* dir) {
    if (!dir || !S_ISDIR(dir->inode)) {
        printk("Not a directory\n");
        return;
    }

    // Print current directory name
    printk("Listing directory '%s':\n", dir->name);

    // Start with first child
    vfs_dentry_t* current = dir->first_child;

    // If directory is empty
    if (!current) {
        printk("  (empty directory)\n");
        return;
    }

    // Iterate through all siblings
    while (current) {
        // Check if it's a directory
        if (S_ISDIR(current->inode)) {
            // Print directory information
            printk("  %s  ", current->name);

            // Print permissions
            printk("%c%c%c%c%c%c%c%c%c  ",
                   (current->inode->mode & S_IRUSR) ? 'r' : '-',
                   (current->inode->mode & S_IWUSR) ? 'w' : '-',
                   (current->inode->mode & S_IXUSR) ? 'x' : '-',
                   (current->inode->mode & S_IRGRP) ? 'r' : '-',
                   (current->inode->mode & S_IWGRP) ? 'w' : '-',
                   (current->inode->mode & S_IXGRP) ? 'x' : '-',
                   (current->inode->mode & S_IROTH) ? 'r' : '-',
                   (current->inode->mode & S_IWOTH) ? 'w' : '-',
                   (current->inode->mode & S_IXOTH) ? 'x' : '-');

            // Print additional information
            printk("uid: %d gid: %d\n",
                   current->inode->uid, current->inode->gid);
        }
        current = current->next_sibling;
    }
}

// Helper function to create a test directory structure
void create_test_directory_structure(vfs_dentry_t* root) {
    // Create some standard Unix directories
    const char* standard_dirs[] = {
        "bin", "dev", "etc", "home", "lib",
        "mnt", "proc", "sbin", "tmp", "usr",
        "var", NULL
    };

    for (const char** dir_name = standard_dirs; *dir_name; dir_name++) {
        vfs_dentry_t* new_dir = vfs_create_dirent(*dir_name, VFS_DIR | 0755);
        if (new_dir) {
            new_dir->inode->uid = 0;  // root user
            new_dir->inode->gid = 0;  // root group
            new_dir->inode->ops = &vfs_ops;
            vfs_add_child(root, new_dir);
        }
    }
}

int vfs_readdir(vfs_dentry_t* dir, dirent_t* buffer, size_t max_entries) {
    if (!dir || !(dir->inode->mode & VFS_DIR)) {
        return -ENOTDIR;
    }

    vfs_dentry_t* entry = dir->first_child; // Start at first child
    size_t count = 0;

    while (entry && count < max_entries) {
        // buffer[count].d_ino = entry->inode->inode_number;
        strncpy(buffer[count].d_name, entry->name, sizeof(buffer[count].d_name));
        count++;
        entry = entry->next_sibling;
    }

    return count; // Return number of entries read
}


void vfs_init() {
    // Initialize the root directory
    vfs_root_node = vfs_init_root();
    if (!vfs_root_node) panic("Failed to initialize root directory!");
    vfs_root_node->inode->ops = &vfs_ops;

    // Create a test directory structure
    create_test_directory_structure(vfs_root_node);
    printk("RootOK\n");

    // get directories of our fat32 filesystem, and mount them in.

    // List the directories
    list_directories(vfs_root_node);
}

// TODO block device
ssize_t default_block_read(vfs_inode_t* inode, void* buffer, size_t count, uint64_t block) {
    if (!S_ISBLK(inode) || !inode->dev_ops) {
        return -EINVAL;
    }
    return inode->dev_ops->read_block(inode->dev, buffer, count, block);
}

ssize_t default_block_write(vfs_inode_t* inode, const void* buffer, size_t count, uint64_t block) {
    if (!S_ISBLK(inode) || !inode->dev_ops) {
        return -EINVAL;
    }
    return inode->dev_ops->write_block(inode->dev, buffer, count, block);
}

// this only does half of tthe job, we need to implement the other half
vfs_dentry_t* vfs_finddir(const char* path) {
    if (!path || path[0] != '/') {
        return NULL;
    }

    vfs_dentry_t* current = vfs_root_node;
    char* path_copy = strdup(path);
    char* token = strtok(path_copy, "/");
    while (token) {
        current = vfs_find_child(current, token);
        if (!current) {
            break;
        }
        token = strtok(NULL, "/");
    }
    kfree(path_copy);
    return current;
}

// int register_block_device(const char* name, device_ops_t* ops, dev_t dev_id) {
//     vfs_inode_t* inode = create_inode();
//     if (!inode) return -1;

//     inode->mode = VFS_BLK | S_IRUSR | S_IWUSR;
//     inode->dev = dev_id;
//     inode->dev_ops = ops;

//     // Create device file in /dev
//     return create_device_node("/dev", name, inode);
// }
