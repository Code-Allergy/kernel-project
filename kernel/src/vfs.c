#include "kernel/panic.h"
#include "kernel/sched.h"
#include <kernel/vfs.h>
#include <kernel/printk.h>
#include <kernel/heap.h>
#include <kernel/fat32.h>
#include <kernel/string.h>
#include <kernel/file.h>

#define VFS_DIR 0x4000
#define S_ISDIR(node) ((node)->mode & VFS_DIR)
#define S_ISREG(node) (!((node)->mode & VFS_DIR))

// this shouldn't be here
#define EINVAL 22
#define ENOENT 2


int vfs_default_open(vfs_node_t* node, int flags) {
    if (!node) {
        return -EINVAL;
    }

    // create an open file structure for the node
    file_t* file = (file_t*)kmalloc(sizeof(file_t));
    file->vnode = node;
    file->offset = 0;
    file->flags = flags; // TODO - flags should be handled

    // TODO - handle freed-up file descriptors
    current_process->fd_table[current_process->num_fds] = file;

    return current_process->num_fds++;
}

vfs_node_t* vfs_create_node(const char* name, uint32_t mode) {
    vfs_node_t* node = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    if (!node) return NULL;

    memset(node, 0, sizeof(vfs_node_t));
    strncpy(node->name, name, sizeof(node->name) - 1);
    node->mode = mode;

    // time_t time = get_time();
    node->atime = 0;
    node->mtime = 0;
    node->ctime = 0;

    return node;
}

vfs_node_t* vfs_init_root() {
    vfs_node_t* root = vfs_create_node("/", VFS_DIR);
    if (!root) panic("Failed to get memory for root vfs node!");

    root->parent = root;
    root->next_sibling = NULL;

    return root;
}

// Add a child node to a directory
int vfs_add_child(vfs_node_t* parent, vfs_node_t* child) {
    if (!parent || !child || !S_ISDIR(parent)) {
        return -EINVAL;
    }

    child->parent = parent;

    if (!parent->first_child) {
        // First child
        parent->first_child = child;
    } else {
        // Add to end of sibling list
        vfs_node_t* sibling = parent->first_child;
        while (sibling->next_sibling) {
            sibling = sibling->next_sibling;
        }
        sibling->next_sibling = child;
    }

    return 0;
}

// Remove a child node from a directory
int vfs_remove_child(vfs_node_t* parent, vfs_node_t* child) {
    if (!parent || !child || !S_ISDIR(parent)) {
        return -EINVAL;
    }

    if (parent->first_child == child) {
        // First child
        parent->first_child = child->next_sibling;
    } else {
        // Find child in sibling list
        vfs_node_t* sibling = parent->first_child;
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
vfs_node_t* vfs_find_child(vfs_node_t* dir, const char* name) {
    if (!dir || !S_ISDIR(dir)) {
        return NULL;
    }

    vfs_node_t* child = dir->first_child;
    while (child != NULL) {
        if (strcmp(child->name, name) == 0) {
            return child;
        }
        child = child->next_sibling;
    }

    return NULL;
}

void list_directories(vfs_node_t* dir) {
    if (!dir || !S_ISDIR(dir)) {
        printk("Not a directory\n");
        return;
    }

    // Print current directory name
    printk("Listing directory '%s':\n", dir->name);

    // Start with first child
    vfs_node_t* current = dir->first_child;

    // If directory is empty
    if (!current) {
        printk("  (empty directory)\n");
        return;
    }

    // Iterate through all siblings
    while (current) {
        // Check if it's a directory
        if (S_ISDIR(current)) {
            // Print directory information
            printk("  %s  ", current->name);

            // Print permissions
            printk("%c%c%c%c%c%c%c%c%c  ",
                   (current->mode & S_IRUSR) ? 'r' : '-',
                   (current->mode & S_IWUSR) ? 'w' : '-',
                   (current->mode & S_IXUSR) ? 'x' : '-',
                   (current->mode & S_IRGRP) ? 'r' : '-',
                   (current->mode & S_IWGRP) ? 'w' : '-',
                   (current->mode & S_IXGRP) ? 'x' : '-',
                   (current->mode & S_IROTH) ? 'r' : '-',
                   (current->mode & S_IWOTH) ? 'w' : '-',
                   (current->mode & S_IXOTH) ? 'x' : '-');

            // Print additional information
            printk("uid: %d gid: %d\n",
                   current->uid, current->gid);
        }
        current = current->next_sibling;
    }
}

// Helper function to create a test directory structure
void create_test_directory_structure(vfs_node_t* root) {
    // Create some standard Unix directories
    const char* standard_dirs[] = {
        "bin", "dev", "etc", "home", "lib",
        "mnt", "proc", "sbin", "tmp", "usr",
        "var", NULL
    };

    for (const char** dir_name = standard_dirs; *dir_name; dir_name++) {
        vfs_node_t* new_dir = vfs_create_node(*dir_name, VFS_DIR | 0755);
        if (new_dir) {
            new_dir->uid = 0;  // root user
            new_dir->gid = 0;  // root group
            vfs_add_child(root, new_dir);
        }
    }
}

vfs_ops_t vfs_ops = {
    .open = vfs_default_open,
};


void vfs_init() {
    // Initialize the root directory
    vfs_root_node = vfs_init_root();
    if (!vfs_root_node) panic("Failed to initialize root directory!");
    vfs_root_node->ops = &vfs_ops;

    // Create a test directory structure
    create_test_directory_structure(vfs_root_node);

    // get directories of our fat32 filesystem, and mount them in.

    // List the directories
    list_directories(vfs_root_node);
}



vfs_node_t* vfs_root_node = NULL;
