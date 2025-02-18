#include "kernel/panic.h"
#include <kernel/vfs.h>
#include <kernel/printk.h>
#include <kernel/heap.h>
#include <kernel/fat32.h>
#include <kernel/string.h>

#define VFS_DIR 0x4000
#define S_ISDIR(node) ((node)->mode & VFS_DIR)

// this shouldn't be here
#define EINVAL 22

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


void vfs_init() {
    // Initialize the root directory
    vfs_node_t* root = vfs_init_root();
    if (!root) panic("Failed to initialize root directory!");

    // Create a test directory structure
    create_test_directory_structure(root);

    // List the directories
    list_directories(root);
}
