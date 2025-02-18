#include <kernel/fat32.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <stdint.h>

int fat32_get_next_cluster(fat32_fs_t* fs, uint32_t curr, uint32_t* next) {
    if (!fs || !next) {
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
    *next = entry & 0x0FFFFFFF;

    // Check for special cluster values
    if (*next >= 0x0FFFFFF8) {
        *next = FAT32_EOC_MARKER;
    } else if (*next == 0x0FFFFFF7) {
        return FAT32_ERROR_IO;
    } else if (*next < 2) { // Check for reserved clusters
        return FAT32_ERROR_CORRUPTED_FS;
    }

    return 0;
}

static int get_cluster_at_index(fat32_fs_t *fs, uint32_t start_cluster, uint32_t index, uint32_t *target_cluster) {
    if (start_cluster <= 2) {
        return FAT32_ERROR_BAD_PARAMETER;
    }

    uint32_t current_cluster = start_cluster;
    for (uint32_t i = 0; i < index; ++i) {
        uint32_t next_cluster;
        int result = fat32_get_next_cluster(fs, current_cluster, &next_cluster);
        if (result != FAT32_SUCCESS) {
            return result;
        }

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


/**
 * @brief Read a FAT entry to find the next cluster in the chain
 * @param fs Pointer to mounted FAT32 filesystem
 * @param cluster Current cluster number
 * @param next_cluster Pointer to store the next cluster number
 * @return 0 on success, negative error code on failure
 */
int fat32_read_fat_entry(fat32_fs_t *fs, uint32_t cluster, uint32_t *next_cluster) {
    if (!fs || !next_cluster) {
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
    memcpy(next_cluster, &buffer[entry_offset], sizeof(uint32_t));
    *next_cluster &= 0x0FFFFFFF;

    // Check for special values
    if (*next_cluster >= 0x0FFFFFF8) {
        // End of chain marker
        *next_cluster = FAT32_EOC_MARKER;
    } else if (*next_cluster == 0x0FFFFFF7) {
        // Bad cluster
        return FAT32_ERROR_IO;
    }

    return 0;
}

/**
 * @brief Helper function to format a filename into FAT32 8.3 format
 * @param input Input filename
 * @param output Output buffer (must be at least 11 bytes)
 */
__attribute__((noinline)) static void fat32_format_name(const char *input, char *output) {
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

// Function to parse a FAT32 path into components
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

void print_parsed_path(const fat32_path_t *parser) {
    for (int i = 0; i < parser->num_components; i++) {
        printk("Component %d: %s\n", i + 1, parser->components[i]);
    }
}


int fat32_mount(fat32_fs_t *fs, const fat32_diskio_t *io) {
    if (!fs || !io || !io->read_sector) {
        return FAT32_ERROR_BAD_PARAMETER;
    }

    uint8_t buffer[FAT32_SECTOR_SIZE] __attribute__((aligned(8)));
    Fat32BootSector *boot_sector = (Fat32BootSector *)buffer;

    // Read boot sector
    int ret = io->read_sector(FAT32_BOOT_SECTOR, buffer);
    if (ret != 0) {
        return FAT32_ERROR_IO;
    }

    // Verify that the boot sector is valid
    if (boot_sector->bootSectorSig != FAT32_BOOT_SECTOR_SIGNATURE ||
        boot_sector->sectorsPerCluster == 0 ||
        boot_sector->reservedSectors == 0 ||
        boot_sector->numFATs == 0 ||
        boot_sector->sectorsPerFAT32 == 0) {
            printk("Invalid FAT32 boot sector\n");
            printk("Boot sector signature: %x\n", boot_sector->bootSectorSig);
            printk("Sectors per cluster: %d\n", boot_sector->sectorsPerCluster);
            printk("Reserved sectors: %d\n", boot_sector->reservedSectors);
            printk("Number of FATs: %d\n", boot_sector->numFATs);
            printk("Sectors per FAT32: %d\n", boot_sector->sectorsPerFAT32);
            return FAT32_ERROR_INVALID_BOOT_SECTOR;
    }

    fs->bytes_per_sector = FAT32_SECTOR_SIZE;
    fs->sectors_per_cluster = boot_sector->sectorsPerCluster;
    fs->reserved_sector_count = boot_sector->reservedSectors;
    fs->num_fats = boot_sector->numFATs;
    fs->sectors_per_fat = boot_sector->sectorsPerFAT32;
    fs->first_data_sector = boot_sector->reservedSectors + boot_sector->numFATs * boot_sector->sectorsPerFAT32;
    fs->fat_start_sector = boot_sector->reservedSectors;
    fs->root_cluster = boot_sector->rootCluster;
    fs->disk.read_sector = io->read_sector;
    fs->disk.read_sectors = io->read_sectors;

    return 0;
}

// TODO: look into why this gets optimized out
int __attribute__((optimize("O0"))) fat32_read_dir_entry(fat32_fs_t* fs, fat32_dir_entry_t* current_dir, const char* name) {
    if (!fs || !name) return FAT32_ERROR_BAD_PARAMETER;

    int ret;
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
                    printk("End\n");
                    return -1; // TODO: Return proper error code
                }
                if (current_entry->filename[0] == 0xE5) {
                    printk("Deleted\n");
                    // Deleted entry, skip
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
        uint32_t next_cluster;
        ret = fat32_read_fat_entry(fs, current_cluster, &next_cluster);
        if (ret != 0) {
            return ret;
        }

        current_cluster = next_cluster;
        sector_target = fs->first_data_sector +
                        (current_cluster - 2) * fs->sectors_per_cluster;
    }

    return FAT32_ERROR_NO_FILE;  // Entry not found
}

int fat32_open(fat32_fs_t* fs, const char* path, fat32_file_t* file) {
    int ret = 0;
    fat32_dir_entry_t current_dir = { .is_initialized = 0 };
    int current_component = 0;
    fat32_path_t path_struct;

    fat32_path_init(&path_struct);

    parse_fat32_path(path, &path_struct);
    while (path_struct.num_components > 0) {
        // Read the directory entry for the current path component
        if ((ret = fat32_read_dir_entry(fs, &current_dir, path_struct.components[current_component])) != 0) {
            printk("Failed to read dir entry %s (got err %d)\n", path_struct.components[current_component], ret);
            return -1;
        }
        path_struct.num_components--;
        current_component++;
    }

    file->fs = fs;
    file->start_cluster = current_dir.start_cluster;
    file->current_cluster = current_dir.start_cluster;
    file->file_size = current_dir.file_size;
    file->file_offset = 0;

    return FAT32_SUCCESS;
}

int fat32_read(fat32_file_t *file, void *buffer, int size) {
    if (!file || !buffer) {
        return FAT32_ERROR_BAD_PARAMETER;
    }
    if (file->file_offset >= file->file_size || size == 0) {
        return 0;
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
            printk("ERROR: Got %d from get_cluster_at_index\n", result);
            return bytes_read > 0 ? (int)bytes_read : result;
        }

        // Calculate how much we can read from this cluster
        uint32_t bytes_in_cluster = cluster_size - cluster_offset;
        uint32_t bytes_to_read = (remaining < bytes_in_cluster) ? remaining : bytes_in_cluster;

        // try and get this working
        if (0 && cluster_offset == 0 && remaining >= cluster_size) {
            uint32_t cluster_sector = fat32_cluster_to_sector(file->fs, target_cluster);
            uint32_t sectors_to_read = file->fs->sectors_per_cluster;
            // Use bulk read for full clusters
            int result = file->fs->disk.read_sectors(cluster_sector, buf_ptr, sectors_to_read);
            if (result != 0) return bytes_read;

            buf_ptr += cluster_size;
            bytes_read += cluster_size;
            remaining -= cluster_size;
            file->file_offset += cluster_size;
        } else {


            // Calculate starting sector and position within the cluster
            uint32_t cluster_start_sector = fat32_cluster_to_sector(fs, target_cluster);
            uint32_t sector_offset = cluster_offset % fs->bytes_per_sector;
            uint32_t sector_in_cluster = cluster_offset % fs->bytes_per_sector;
            uint32_t current_sector = cluster_start_sector + sector_in_cluster;

            // printk("Writing %d bytes, remaining: %d\n", bytes_to_read, remaining - bytes_to_read);
            // Read sectors within the cluster until we fulfill the request or exhaust the cluster
            while (bytes_to_read > 0) {
                uint8_t sector_buffer[FAT32_SECTOR_SIZE] __attribute__((aligned(8)));
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
        }

        // Update current_cluster to the last accessed cluster
        file->current_cluster = target_cluster;
        // If more data is needed, move to the next cluster
        if (bytes_to_read > 0) {
            uint32_t next_cluster;
            result = fat32_get_next_cluster(fs, target_cluster, &next_cluster);
            if (result != FAT32_SUCCESS) {
                printk("Non success from fat32_get_next_cluster!\n");
                return bytes_read > 0 ? (int)bytes_read : result;
            }
            if (next_cluster >= FAT32_LAST_MARKER || next_cluster == FAT32_EOC_MARKER) {
                printk("End of cluster chain!\n");
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
    // start area relative to the start of the partition
    return ((cluster - 2) * fs->sectors_per_cluster) + fs->first_data_sector;
}
