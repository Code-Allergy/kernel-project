#include <kernel/fat32.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/panic.h>
#include <stdint.h>
#include <stdbool.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

uint32_t fat32_get_next_cluster(fat32_fs_t* fs, uint32_t curr) {
    uint32_t value;
    if (!fs) {
        return FAT32_ERROR_BAD_PARAMETER;
    }

    // Each FAT entry is 4 bytes
    uint32_t fat_offset = curr * 4;

    // Calculate which FAT sector contains this cluster
    uint32_t entry_offset = fat_offset % FAT32_SECTOR_SIZE;
    uint32_t fat_sector = fat_offset / FAT32_SECTOR_SIZE;
    fat_sector += fs->fat_start_sector;

    // Read the FAT sector
    uint8_t buffer[FAT32_SECTOR_SIZE];
    int ret = fs->disk.read_sector(fat_sector, buffer);
    if (ret != 0) {
        return FAT32_ERROR_IO;
    }

    // Get the FAT entry value
    uint32_t entry = 0;
    entry |= buffer[entry_offset];
    entry |= buffer[entry_offset + 1] << 8;
    entry |= buffer[entry_offset + 2] << 16;
    entry |= buffer[entry_offset + 3] << 24;
    value = entry & 0x0FFFFFFF;

    // Check for special cluster values
    if (value >= 0x0FFFFFF8) {
        value = FAT32_EOC_MARKER;
    } else if (value == 0x0FFFFFF7) {
        return FAT32_ERROR_IO;
    } else if (value < 2) { // Check for reserved clusters
        return FAT32_ERROR_CORRUPTED_FS;
    }

    return value;
}

static int get_cluster_at_index(fat32_fs_t *fs, uint32_t start_cluster, uint32_t index, uint32_t *target_cluster) {
    if (start_cluster <= 2) {
        return FAT32_ERROR_BAD_PARAMETER;
    }

    uint32_t current_cluster = start_cluster;
    for (uint32_t i = 0; i < index; ++i) {
        uint32_t next_cluster = fat32_get_next_cluster(fs, current_cluster);

        // Check if next_cluster is EOC prematurely
        if (next_cluster == FAT32_EOC_MARKER) { // Assuming is_eoc() checks for EOC range
            if (i < index - 1) { // Only error if not the last iteration
                printk("Cluster chain ends prematurely at %u\n", next_cluster);
                return FAT32_ERROR_CORRUPTED_FS;
            }
        }

        current_cluster = next_cluster;
    }

    *target_cluster = current_cluster;
    return FAT32_SUCCESS;
}


int fat32_read_fat_entry(fat32_fs_t *fs, uint32_t cluster) {
    uint32_t next_cluster;
    if (!fs) {
        return FAT32_ERROR_BAD_PARAMETER;
    }

    // Calculate which sector of the FAT contains this cluster's entry
    // Each FAT entry is 4 bytes (32 bits) in FAT32
    uint32_t fat_offset = cluster * 4;
    uint32_t entry_offset = fat_offset % FAT32_SECTOR_SIZE;
    uint32_t fat_sector = fat_offset / FAT32_SECTOR_SIZE;
    fat_sector += fs->fat_start_sector;

    // Read the FAT sector
    uint8_t buffer[FAT32_SECTOR_SIZE];
    int ret = fs->disk.read_sector(fat_sector, buffer);
    if (ret != 0) {
        return FAT32_ERROR_IO;
    }

    // Extract the 32-bit FAT entry
    next_cluster = buffer[entry_offset];
    next_cluster &= 0x0FFFFFFF;

    // Check for special values
    if (next_cluster >= 0x0FFFFFF8) {
        // End of chain marker
        next_cluster = FAT32_EOC_MARKER;
    } else if (next_cluster == 0x0FFFFFF7) {
        // Bad cluster
        return FAT32_ERROR_IO;
    }

    return next_cluster;
}

/**
 * @brief Helper function to format a filename into FAT32 8.3 format
 * @param input Input filename
 * @param output Output buffer (must be at least 11 bytes)
 */
void fat32_format_name(const char *input, char *output) {
    size_t name_len = 0;
    size_t ext_len = 0;
    const char *ext_pos = strchr(input, '.');

    if (ext_pos) {
        name_len = ext_pos - input;
        ext_len = strlen(ext_pos + 1);
    } else {
        name_len = strlen(input);
        ext_len = 0;
    }

    // Copy name (up to 8 characters)
    for (size_t i = 0; i < 8; i++) {
        if (i < name_len) {
            output[i] = toupper((unsigned char)input[i]);
        } else {
            output[i] = ' ';
        }
    }

    // Copy extension (up to 3 characters)
    for (size_t i = 0; i < 3; i++) {
        if (ext_pos && i < ext_len) {
            output[8 + i] = toupper(ext_pos[1 + i]);
        } else {
            output[8 + i] = ' ';
        }
    }
}



// Function to parse a FAT32 path into components - STATIC
void parse_fat32_path(const char *path, fat32_path_t *parser) {
    int path_len = strlen(path);
    int start = 0;
    int component_idx = 0;

    // skip leading dot if present
    if (path[0] == '.') {
        path++;  // Skip the first character (.)
        path_len--;
    }

    for (int i = 0; i <= path_len; ++i) {
        // If we reach a separator or the end of the string, we capture the component
        if (path[i] == '/' || path[i] == '\0') {
            if (i > start) {
                int len = i - start;
                if (len < FAT32_MAX_COMPONENT_LENGTH && component_idx < FAT32_MAX_COMPONENTS) {
                    strncpy(parser->components[component_idx], &path[start], len);
                    parser->components[component_idx][len] = '\0';  // Null-terminate
                    component_idx++;
                }
            }
            start = i + 1;  // Set the start of the next component
        }
    }
    parser->num_components = component_idx;
}


int fat32_mount(fat32_fs_t *fs, const fat32_diskio_t *io) {
    if (!fs || !io || !io->read_sector) {
        return FAT32_ERROR_BAD_PARAMETER;
    }

    uint8_t sector0[FAT32_SECTOR_SIZE];
    uint32_t fat32_start_sector = 0;

    // 1. Read first sector to check for MBR
    if (io->read_sector(0, sector0) != 0) {
        return FAT32_ERROR_IO;
    }

    bool found_via_mbr = false;
    if (sector0[510] == 0x55 && sector0[511] == 0xAA) {
        MBR* mbr = (MBR*)sector0;
        for (int i = 0; i < 4; i++) {
            if (mbr->partitions[i].partition_type == 0x0B ||
                mbr->partitions[i].partition_type == 0x0C) {
                fat32_start_sector = mbr->partitions[i].start_sector;
                found_via_mbr = true;
                break;
            }
        }
    }

    // 2. If no MBR partition, check sector 0 directly
    bool found_via_sector0 = false;
    if (!found_via_mbr) {
        Fat32BootSector* sector0_bs = (Fat32BootSector*)sector0;
        if (sector0_bs->bootSectorSig == FAT32_BOOT_SECTOR_SIGNATURE &&
            sector0_bs->sectorsPerFAT32 != 0) {
            fat32_start_sector = 0;
            found_via_sector0 = true;
        }
    }

    // 3. Final validation
    if (!found_via_mbr && !found_via_sector0) {
        printk("No FAT32 found (MBR:%d, Sector0:%d)\n", found_via_mbr, found_via_sector0);
        return FAT32_ERROR_INVALID_BOOT_SECTOR;
    }

    // 6. Read actual FAT32 boot sector
    uint8_t boot_buffer[FAT32_SECTOR_SIZE];
    if (io->read_sector(fat32_start_sector, boot_buffer) != 0) {
        return FAT32_ERROR_IO;
    }

    Fat32BootSector *boot_sector = (Fat32BootSector *)boot_buffer;

    // 7. Validate FAT32 boot sector
    if (boot_sector->bootSectorSig != FAT32_BOOT_SECTOR_SIGNATURE ||
        boot_sector->sectorsPerCluster == 0 ||
        boot_sector->reservedSectors == 0 ||
        boot_sector->numFATs == 0 ||
        boot_sector->sectorsPerFAT32 == 0) {
            printk("Invalid FAT32 boot sector\n");
            return FAT32_ERROR_INVALID_BOOT_SECTOR;
    }

    fs->disk = *io;


    fs->bytes_per_sector = FAT32_SECTOR_SIZE;
    fs->sectors_per_cluster = boot_sector->sectorsPerCluster;
    fs->reserved_sector_count = boot_sector->reservedSectors;
    fs->num_fats = boot_sector->numFATs;
    fs->sectors_per_fat = boot_sector->sectorsPerFAT32;
    fs->fat_start_sector = fat32_start_sector + boot_sector->reservedSectors;
    fs->first_data_sector = fat32_start_sector +
                           boot_sector->reservedSectors +
                           (boot_sector->numFATs * boot_sector->sectorsPerFAT32);
    fs->root_cluster = boot_sector->rootCluster;
    fs->disk.read_sector = io->read_sector;
    fs->disk.read_sectors = io->read_sectors;
    fs->cluster_size = fs->sectors_per_cluster * fs->bytes_per_sector;
    fs->total_sectors = (boot_sector->totalSectors16 != 0) ? boot_sector->totalSectors16 : boot_sector->totalSectors32;
    uint32_t data_sectors = fs->total_sectors - (fs->first_data_sector - fat32_start_sector);
    fs->total_clusters = data_sectors / fs->sectors_per_cluster;

    return 0;
}

int fat32_read_dir_entry(fat32_fs_t* fs, fat32_dir_entry_t* current_dir, const char* name) {
    if (!fs || !name) return FAT32_ERROR_BAD_PARAMETER;

    uint32_t current_cluster;
    uint32_t sector_target;

    // If current_dir is NULL or not initialized, start from root directory
    if (!current_dir || !current_dir->is_initialized) {
        current_cluster = fs->root_cluster;
        sector_target = fs->first_data_sector +
                        (current_cluster - 2) * fs->sectors_per_cluster;
    } else {
        current_cluster = current_dir->start_cluster;
        sector_target = fs->first_data_sector +
                        (current_cluster - 2) * fs->sectors_per_cluster;
    }

    uint8_t sector_buffer[FAT32_SECTOR_SIZE];
    Fat32DirectoryEntry *dir_entry = (Fat32DirectoryEntry *)sector_buffer;

    char formatted_name[11];
    memset(formatted_name, ' ', sizeof(formatted_name));
    fat32_format_name(name, formatted_name);
    while (current_cluster != FAT32_EOC_MARKER) {
        for (uint32_t sector = 0; sector < fs->sectors_per_cluster; sector++) {
            int ret = fs->disk.read_sector(sector_target + sector, sector_buffer);
            if (ret != 0) {
                return FAT32_ERROR_IO;
            }

            // Process all directory entries in this sector
            for (uint32_t entry = 0; entry < FAT32_SECTOR_SIZE / sizeof(Fat32DirectoryEntry); entry++) {
                Fat32DirectoryEntry* current_entry = &dir_entry[entry];

                // Check if entry is empty or deleted
                if (current_entry->filename[0] == 0x00) {
                    // End of directory
                    return -1; // TODO: Return proper error code
                }
                if (current_entry->filename[0] == 0xE5) {
                    continue;
                }

                // Compare name (ignoring case)
                if (memcmp(formatted_name, current_entry->filename, 11) == 0) {
                    // Found the entry, update current_dir if provided
                    if (current_dir) {
                        current_dir->is_initialized = 1;
                        current_dir->start_cluster =
                            (current_entry->firstClusterHigh << 16) | current_entry->firstClusterLow;
                        current_dir->file_size = current_entry->fileSize;
                        current_dir->attributes = current_entry->attr;
                        // if (current_dir->start_cluster < 2) {
                        //     printk("Start clister: %d\n", current_dir->start_cluster);
                        //     return FAT32_ERROR_CORRUPTED_FS;
                        // }

                        // TODO other crap
                    }
                    return 0;
                }
            }
        }

        // Read next cluster from FAT
        int next_cluster = fat32_read_fat_entry(fs, current_cluster);
        if (next_cluster < 0) {
            return next_cluster;
        }

        current_cluster = next_cluster;
        sector_target = fs->first_data_sector +
                        (current_cluster - 2) * fs->sectors_per_cluster;
    }

    return FAT32_ERROR_NO_FILE;  // Entry not found
}

int fat32_open(fat32_fs_t* fs, const char* path, fat32_file_t* file) {
    fat32_dir_entry_t current_dir = { .is_initialized = 0 };
    fat32_path_t path_struct;

    fat32_path_init(&path_struct);
    parse_fat32_path(path, &path_struct);

    uint32_t parent_cluster = fs->root_cluster; // Start from root as parent

    for (int i = 0; i < path_struct.num_components - 1; i++) {
        // Traverse directories to find the parent
        if (fat32_read_dir_entry(fs, &current_dir, path_struct.components[i]) != 0) {
            return FAT32_ERROR_NO_FILE;
        }
        parent_cluster = current_dir.start_cluster;
    }

    // Open the target file/dir
    const char* target_name = path_struct.components[path_struct.num_components - 1];
    if (fat32_read_dir_entry(fs, &current_dir, target_name) != 0) {
        return FAT32_ERROR_NO_FILE;
    }

    file->fs = fs;
    file->start_cluster = current_dir.start_cluster;
    file->current_cluster = current_dir.start_cluster;
    file->file_size = current_dir.file_size;
    file->parent_dir_cluster = parent_cluster;
    file->file_offset = 0;
    fat32_format_name(target_name, file->formatted_name);

    return FAT32_SUCCESS;
}

int fat32_read(fat32_file_t *file, void *buffer, int size, int offset) {
    if (!file || !buffer) {
        return FAT32_ERROR_BAD_PARAMETER;
    }

    if (file->file_offset >= file->file_size || size == 0) {
        return 0;
    }

    // TODO handle negative offset
    if (offset < 0) {
        return FAT32_ERROR_BAD_PARAMETER;
    }

    if (offset >= (int)file->file_size ||size == 0) {
        return 0;
    }

    if (offset != -1) {
        file->file_offset = offset;
    }

    fat32_fs_t *fs = file->fs;
    const uint8_t sectors_per_cluster = fs->sectors_per_cluster;
    const uint32_t cluster_size = fs->bytes_per_sector * sectors_per_cluster;

    uint32_t bytes_read = 0;
    uint8_t *buf_ptr = (uint8_t *)buffer;
    uint32_t remaining = size;

    // Calculate the maximum readable bytes
    uint32_t max_readable = file->file_size - file->file_offset;
    if (remaining > max_readable) {
        remaining = max_readable;
    }

    while (remaining > 0) {
        uint32_t cluster_offset = file->file_offset % cluster_size;
        uint32_t cluster_index = file->file_offset / cluster_size;
        uint32_t target_cluster;

        int result = get_cluster_at_index(fs, file->start_cluster, cluster_index, &target_cluster);
        if (result != FAT32_SUCCESS) {
            return bytes_read > 0 ? (int)bytes_read : result;
        }

        // Calculate how much we can read from this cluster
        uint32_t bytes_in_cluster = cluster_size - cluster_offset;
        uint32_t bytes_to_read = (remaining < bytes_in_cluster) ? remaining : bytes_in_cluster;

        // Calculate starting sector and position within the cluster
        uint32_t cluster_start_sector = fat32_cluster_to_sector(fs, target_cluster);
        uint32_t sector_offset = cluster_offset % fs->bytes_per_sector;
        uint32_t sector_in_cluster = cluster_offset / fs->bytes_per_sector;
        uint32_t current_sector = cluster_start_sector + sector_in_cluster;

        // Read sectors within the cluster until we fulfill the request or exhaust the cluster
        while (bytes_to_read > 0) {
            uint8_t sector_buffer[FAT32_SECTOR_SIZE] __attribute__((aligned(8))); // 8byte alignment for arm
            result = fs->disk.read_sector(current_sector, sector_buffer);
            if (result != 0) {
                return bytes_read > 0 ? (int)bytes_read : FAT32_ERROR_IO;
            }

            uint32_t bytes_in_sector = fs->bytes_per_sector - sector_offset;
            uint32_t bytes_from_sector = (bytes_to_read < bytes_in_sector) ? bytes_to_read : bytes_in_sector;

            volatile uint8_t *src = sector_buffer + sector_offset;
            for (size_t i = 0; i < bytes_from_sector; i++) {
                buf_ptr[i] = src[i];
            }
            buf_ptr += bytes_from_sector;
            bytes_read += bytes_from_sector;
            remaining -= bytes_from_sector;
            file->file_offset += bytes_from_sector;
            bytes_to_read -= bytes_from_sector;

            // Move to next sector and reset offset
            current_sector++;
            sector_offset = 0;

            // Check if we've exceeded the current cluster
            if ((current_sector - cluster_start_sector) >= sectors_per_cluster) {
                break;
            }
        }

        // Update current_cluster to the last accessed cluster
        file->current_cluster = target_cluster;
        // If more data is needed, move to the next cluster
        if (bytes_to_read > 0) {
            uint32_t next_cluster = fat32_get_next_cluster(fs, target_cluster);
            if (next_cluster >= FAT32_LAST_MARKER || next_cluster == FAT32_EOC_MARKER) {
                break; // End of cluster chain
            }
        }
    }

    return bytes_read;
}

int fat32_close(fat32_file_t *file) {
    if (!file) {
        return FAT32_ERROR_BAD_PARAMETER;
    }

    return FAT32_SUCCESS;
}

uint32_t fat32_cluster_to_sector(fat32_fs_t *fs, uint32_t cluster) {
    if (cluster < 2 || cluster >= (2 + fs->total_clusters)) {
        panic("Invalid cluster %u (max %u)", cluster, 2 + fs->total_clusters);
    }
    return ((cluster - 2) * fs->sectors_per_cluster) + fs->first_data_sector;
}


// WRITE FUNCTIONS

uint32_t get_parent_dir_cluster(const fat32_file_t* file) {
    if (!file) return FAT32_ERROR_BAD_PARAMETER;
    return file->parent_dir_cluster;
}

// Write a value to FAT
int fat32_set_next_cluster(fat32_fs_t* fs, uint32_t cluster, uint32_t value) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->fat_start_sector + (fat_offset / FAT32_SECTOR_SIZE);
    uint32_t entry_offset = fat_offset % FAT32_SECTOR_SIZE;

    // Read FAT sector
    uint8_t sector[FAT32_SECTOR_SIZE];
    if (fs->disk.read_sector(fat_sector, sector) != 0)
        return FAT32_ERROR_IO;

    // Update entry (preserve upper 4 bits)
    uint32_t existing = *((uint32_t*)(sector + entry_offset)) & 0xF0000000;
    *((uint32_t*)(sector + entry_offset)) = existing | (value & 0x0FFFFFFF);

    // Write back to all FAT copies
    for (int i = 0; i < fs->num_fats; i++) {
        if (fs->disk.write_sector(fat_sector + (i * fs->sectors_per_fat), sector) != 0)
            return FAT32_ERROR_IO;
    }

    return FAT32_SUCCESS;
}

// Find first free cluster
int find_free_cluster(fat32_fs_t* fs) {
    uint8_t sector[FAT32_SECTOR_SIZE];

    // Search within valid cluster range
    for (uint32_t cluster = 2; cluster < (2 + fs->total_clusters); cluster++) {
        uint32_t fat_offset = cluster * 4;
        uint32_t fat_sector = fs->fat_start_sector + (fat_offset / FAT32_SECTOR_SIZE);
        uint32_t entry_offset = fat_offset % FAT32_SECTOR_SIZE;

        if (fs->disk.read_sector(fat_sector, sector) != 0)
            return FAT32_ERROR_IO;

        uint32_t entry = *((uint32_t*)(sector + entry_offset)) & 0x0FFFFFFF;
        if (entry == 0x00000000) // Free cluster
            return cluster;
    }
    return FAT32_ERROR_NO_SPACE;
}

int update_directory_entry(fat32_fs_t* fs, fat32_file_t* file) {
    uint32_t dir_cluster = get_parent_dir_cluster(file);
    uint32_t dir_sector = fat32_cluster_to_sector(fs, dir_cluster);

    for (int sector = 0; sector < fs->sectors_per_cluster; sector++) {
        uint8_t buffer[FAT32_SECTOR_SIZE];
        if (fs->disk.read_sector(dir_sector + sector, buffer)) return FAT32_ERROR_IO;

        Fat32DirectoryEntry* entries = (Fat32DirectoryEntry*)buffer;
        for (uint32_t i = 0; i < FAT32_SECTOR_SIZE/sizeof(Fat32DirectoryEntry); i++) {
            // Match by filename, not cluster!
            if (memcmp(entries[i].filename, file->formatted_name, 11) == 0) {
                entries[i].firstClusterHigh = (file->start_cluster >> 16) & 0xFFFF;
                entries[i].firstClusterLow = file->start_cluster & 0xFFFF;
                entries[i].fileSize = file->file_size;

                // Write back to all FAT copies
                for (int fat = 0; fat < fs->num_fats; fat++) {
                    if (fs->disk.write_sector(dir_sector + sector + (fat * fs->sectors_per_fat), buffer)) {
                        return FAT32_ERROR_IO;
                    }
                }
                return FAT32_SUCCESS;
            }
        }
    }
    return FAT32_ERROR_NO_FILE;
}


int fat32_write(fat32_file_t* file, const void* buffer, int size, int offset) {
    if (!file || !buffer) return FAT32_ERROR_BAD_PARAMETER;

    fat32_fs_t* fs = file->fs;
    const uint32_t cluster_size = fs->cluster_size;
    uint32_t bytes_written = 0;
    const uint8_t* buf_ptr = (const uint8_t*)buffer;

    // Handle file expansion
    uint32_t required_clusters = (offset + size + cluster_size - 1) / cluster_size;
    uint32_t current_clusters = (file->file_size + cluster_size - 1) / cluster_size;

    // Allocate additional clusters if needed
    uint32_t clusters_to_add = required_clusters - current_clusters;
    if (clusters_to_add > 0 || file->start_cluster < 2) {
        uint32_t last_cluster = file->start_cluster;

        // If start cluster is invalid (e.g., new file), allocate first cluster
        if (last_cluster < 2) {
            uint32_t new_cluster = find_free_cluster(fs);
            if (new_cluster < 2) return FAT32_ERROR_NO_SPACE;
            file->start_cluster = new_cluster;
            last_cluster = new_cluster;
            fat32_set_next_cluster(fs, new_cluster, FAT32_EOC_MARKER);
            current_clusters = 1;
            clusters_to_add = required_clusters - current_clusters;
        } else {
            // Traverse to last cluster
            while (fat32_get_next_cluster(fs, last_cluster) != FAT32_EOC_MARKER) {
                last_cluster = fat32_get_next_cluster(fs, last_cluster);
            }
        }

        // Allocate new clusters
        for (uint32_t i = 0; i < clusters_to_add; i++) {
            uint32_t new_cluster = find_free_cluster(fs);
            if (new_cluster < 2) return FAT32_ERROR_NO_SPACE;
            fat32_set_next_cluster(fs, last_cluster, new_cluster);
            fat32_set_next_cluster(fs, new_cluster, FAT32_EOC_MARKER);
            last_cluster = new_cluster;
        }

        // Mark final new cluster as EOC
        fat32_set_next_cluster(fs, last_cluster, FAT32_EOC_MARKER);
    }

    // Perform actual write
    uint32_t remaining = size;
    while (remaining > 0) {
        uint32_t cluster_offset = offset % cluster_size;
        uint32_t cluster_index = offset / cluster_size;
        uint32_t target_cluster;

        // Get target cluster, handle EOC during traversal
        int res = get_cluster_at_index(fs, file->start_cluster, cluster_index, &target_cluster);
        if (res != FAT32_SUCCESS) return res;

        // Calculate write size for this cluster
        uint32_t write_size = MIN(remaining, cluster_size - cluster_offset);
        if (write_size == 0) break;

        // Calculate sector parameters
        uint32_t sector_start = fat32_cluster_to_sector(fs, target_cluster);
        uint32_t sector_offset = cluster_offset % fs->bytes_per_sector;
        uint32_t sectors_needed = (write_size + sector_offset + fs->bytes_per_sector - 1) / fs->bytes_per_sector;

        // Write each sector
        for (uint32_t i = 0; i < sectors_needed; i++) {
            uint8_t sector_buffer[FAT32_SECTOR_SIZE] = {0};
            uint32_t current_sector = sector_start + i;

            // Read existing sector if not aligned
            if (i == 0 || sector_offset != 0) {
                if (fs->disk.read_sector(current_sector, sector_buffer) != 0)
                    return FAT32_ERROR_IO;
            }

            // Copy data into sector buffer
            uint32_t copy_size = MIN(write_size, fs->bytes_per_sector - sector_offset);
            memcpy(sector_buffer + sector_offset, buf_ptr, copy_size);

            // Write back the sector
            if (fs->disk.write_sector(current_sector, sector_buffer) != 0)
                return FAT32_ERROR_IO;

            buf_ptr += copy_size;
            offset += copy_size;
            bytes_written += copy_size;
            remaining -= copy_size;
            write_size -= copy_size;
            sector_offset = 0; // Reset after first sector
        }
    }

    // Update file size if expanded beyond current size
    if (offset > (int)file->file_size) {
        file->file_size = offset;
        update_directory_entry(fs, file);
    }

    return bytes_written;
}


int fat32_create(fat32_fs_t* fs, const char* path) {
    fat32_path_t path_struct;
    parse_fat32_path(path, &path_struct);

    // Get parent directory cluster
    uint32_t parent_cluster = fs->root_cluster;
    fat32_dir_entry_t parent_dir = {0};

    // Traverse to parent directory (all components except last)
    for (int i = 0; i < path_struct.num_components - 1; i++) {
        if (fat32_read_dir_entry(fs, &parent_dir, path_struct.components[i]) != 0) {
            return FAT32_ERROR_NO_DIR;
        }
        parent_cluster = parent_dir.start_cluster;
    }

    // Find free directory entry in parent cluster and populate it
    uint32_t dir_cluster = parent_cluster;
    int entry_found = 0;
    int new_cluster = 0;
    while (!entry_found) {
        uint32_t sector_start = fat32_cluster_to_sector(fs, dir_cluster);

        for (uint32_t sector = 0; sector < fs->sectors_per_cluster; sector++) {
            uint8_t buffer[FAT32_SECTOR_SIZE];
            if (fs->disk.read_sector(sector_start + sector, buffer) != 0) {
                return FAT32_ERROR_IO;
            }

            Fat32DirectoryEntry* entries = (Fat32DirectoryEntry*)buffer;

            // Search entries in sector
            for (int i = 0; i < (int)(FAT32_SECTOR_SIZE/sizeof(Fat32DirectoryEntry)); i++) {
                if (entries[i].filename[0] == 0x00 || entries[i].filename[0] == 0xE5) {
                    // Format filename
                    fat32_format_name(
                        path_struct.components[path_struct.num_components-1],
                        entries[i].filename
                    );

                    // Set cluster and size
                    entries[i].firstClusterHigh = 0;
                    entries[i].firstClusterLow = 0;
                    entries[i].fileSize = 0;
                    entries[i].attr = 0; // Regular file

                    // Write back sector
                    if (fs->disk.write_sector(sector_start + sector, buffer) != 0) {
                        return FAT32_ERROR_IO;
                    }

                    entry_found = 1;
                    break;
                }
            }
            if (entry_found) break;
        }


        // If no space, extend directory
        if (!entry_found) {
            int new_cluster = find_free_cluster(fs);
            if (new_cluster == FAT32_ERROR_NO_SPACE) {
                return FAT32_ERROR_NO_SPACE;
            }

            // Link new cluster to directory
            fat32_set_next_cluster(fs, dir_cluster, new_cluster);
            fat32_set_next_cluster(fs, new_cluster, FAT32_EOC_MARKER);
            dir_cluster = new_cluster;

            // Initialize new cluster with empty entries
            uint32_t sector_start = fat32_cluster_to_sector(fs, dir_cluster);
            uint8_t empty_sector[FAT32_SECTOR_SIZE] = {0};
            for (uint32_t s = 0; s < fs->sectors_per_cluster; s++) {
                if (fs->disk.write_sector(sector_start + s, empty_sector) != 0) {
                    return FAT32_ERROR_IO;
                }
            }
        }
    }

    // Allocate first cluster for new file
    new_cluster = find_free_cluster(fs);
    if (new_cluster == FAT32_ERROR_NO_SPACE) {
        return FAT32_ERROR_NO_SPACE;
    }
    fat32_set_next_cluster(fs, new_cluster, FAT32_EOC_MARKER);

    // Update directory entry with new cluster
    fat32_file_t new_file = {
        .fs = fs,
        .start_cluster = new_cluster,
        .file_size = 0,
        .parent_dir_cluster = parent_cluster
    };
    update_directory_entry(fs, &new_file);

    return FAT32_SUCCESS;
}
