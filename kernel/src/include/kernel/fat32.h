#ifndef _KERNEL_FAT32_H
#define _KERNEL_FAT32_H

#include <stdint.h>
#include <stddef.h>

#define FAT32_ERROR_IO                  -1
#define FAT32_ERROR_INVALID_BOOT_SECTOR -2
#define FAT32_ERROR_NO_FILE             -3
#define FAT32_ERROR_BAD_PARAMETER       -4
#define FAT32_ERROR_CORRUPTED_FS        -5
#define FAT32_SUCCESS                    0
#define FAT32_EOC                        1


#define FAT32_SECTOR_SIZE 512
#define FAT32_BOOT_SECTOR 0
#define FAT32_BOOT_SECTOR_SIGNATURE 0xAA55
#define FAT32_MAX_PATH 256
#define FAT32_MAX_COMPONENTS 16
#define FAT32_MAX_COMPONENT_LENGTH 64

#define FAT32_EOC_MARKER 0x0FFFFFFF
#define FAT32_LAST_MARKER 0x0FFFFFF8


// static only in bootloader
typedef struct {
    char components[FAT32_MAX_COMPONENTS][FAT32_MAX_COMPONENT_LENGTH];
    int num_components;
} fat32_path_static_t;

typedef fat32_path_static_t fat32_path_t;

static inline void fat32_path_init(fat32_path_t *path) {
    path->num_components = 0;
}

/* api structs */

/*
 * Disk I/O abstraction.
 */
typedef struct {
    /*
     * read one sector from somewhere.
     *   sector:   The absolute sector number to read.
     *   buffer:   A pointer to a buffer with at least 'bytes_per_sector' bytes.
     * Returns 0 on success.
     */
    int (*read_sector)(uint32_t sector, uint8_t *buffer);
    int (*read_sectors)(uint32_t sector, uint8_t* buffer, uint32_t count);
} fat32_diskio_t;

typedef struct {
    fat32_diskio_t disk;

    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t  num_fats;
    uint32_t sectors_per_fat;      /* For FAT32, BPB_FATSz32 */
    uint32_t root_cluster;         /* For FAT32, BPB_RootClus */

    uint32_t first_data_sector;    /* The first sector of the data region */
    uint32_t fat_start_sector;     /* The first FAT's starting sector */
} fat32_fs_t;

/*
 * File handle structure.
 *
 * Used to track a file opened from the FAT32 volume.
 */
typedef struct {
    fat32_fs_t *fs;            /* Reference to the mounted filesystem */
    uint32_t start_cluster;    /* Starting cluster number from the directory entry */
    uint32_t current_cluster;  /* Current cluster number while reading */
    uint32_t current_cluster_index;  /* Index of last cluster */
    uint32_t file_size;        /* Total file size in bytes */
    uint32_t file_offset;      /* Current offset into the file */
    /* You might also cache a small buffer for partial sector reads */
} fat32_file_t;

/*
 * Directory entry structure.
 *
 * A minimal representation of a directory entry (only the short filename variant).
 */
typedef struct {
    char     name[12];         /* 8.3 name plus a null terminator */
    uint8_t  attributes;       /* File attributes */
    uint32_t start_cluster;    /* Starting cluster number */
    uint32_t file_size;        /* File size in bytes */

    uint8_t is_initialized;    /* Set to 1 if this entry is valid */
} fat32_dir_entry_t;

/*===========================================================================
  API Functions
  These functions comprise the public interface to your FAT32 driver.
  ===========================================================================*/

/**
 * @brief Mounts (initializes) the FAT32 filesystem.
 *
 * This function reads the boot sector and sets up the FAT32 filesystem structure.
 *
 * @param fs       Pointer to a fat32_fs_t structure (allocated by the caller).
 * @param io       Pointer to a fat32_diskio_t structure with I/O function pointers.
 * @return         FAT32_SUCCESS on success, or an error code.
 */
int fat32_mount(fat32_fs_t *fs, const fat32_diskio_t *io);


/**
 * @brief Opens a file given its path.
 *
 * Given a path (e.g., "/kernel.bin" for a file in the root directory),
 * this function locates the file and fills in a fat32_file_t structure.
 *
 * @param fs       Mounted FAT32 filesystem pointer.
 * @param path     Null-terminated path to the file.
 * @param file     Pointer to a fat32_file_t structure (allocated by the caller).
 * @return         FAT32_SUCCESS on success, or an error code.
 */
int fat32_open(fat32_fs_t *fs, const char *path, fat32_file_t *file);


/**
 * @brief Reads data from an open file.
 *
 * Reads up to 'size' bytes from the file, updating the file offset
 * and following cluster chains as necessary.
 *
 * @param file         Pointer to an open fat32_file_t.
 * @param buffer       Pointer to the destination buffer.
 * @param size         Number of bytes to read.
 * @return             Real bytes read on success, or an error code.
 */
int fat32_read(fat32_file_t *file, void *buffer, int size);


/**
 * @brief Closes an open file.
 *
 * Currently, this may simply be a placeholder as no cleanup is needed.
 *
 * @param file     Pointer to the fat32_file_t to close.
 * @return         FAT32_SUCCESS on success, or an error code.
 */
int fat32_close(fat32_file_t *file);


/**
 * @brief Lists the contents of a directory.
 *
 * Given a directory path, this function returns an array of directory entries.
 * The caller is responsible for freeing the array when done.
 *
 * @param fs           Mounted FAT32 filesystem pointer.
 * @param path         Null-terminated path to the directory.
 * @param entries      Output pointer that will point to an allocated array of entries.
 * @param entry_count  Output pointer for the number of entries.
 * @return             FAT32_SUCCESS on success, or an error code.
 */
int fat32_list_dir(fat32_fs_t *fs, const char *path,
                   fat32_dir_entry_t **entries, size_t *entry_count);


/**
 * @brief Helper: Converts a FAT32 cluster number to an absolute sector number.
 *
 * Uses the mounted filesystem's parameters to calculate the first sector of
 * a given cluster.
 *
 * @param fs         Mounted FAT32 filesystem pointer.
 * @param cluster    FAT32 cluster number (>= 2).
 * @return           Absolute sector number on the disk.
 */
uint32_t fat32_cluster_to_sector(fat32_fs_t *fs, uint32_t cluster);


extern const fat32_diskio_t mmc_fat32_diskio;


/* parsing structs */
typedef struct __attribute__((packed)) {
    uint8_t  jmpBoot[3];       // Jump instruction
    char     oemName[8];       // OEM Name
    uint16_t bytesPerSector;   // Bytes per sector (typically 512)
    uint8_t  sectorsPerCluster;// Sectors per cluster
    uint16_t reservedSectors;  // Reserved sectors (Boot Sector + FSInfo)
    uint8_t  numFATs;          // Number of FAT tables (usually 2)
    uint16_t rootEntryCount;   // Always 0 for FAT32
    uint16_t totalSectors16;   // Only used if < 65536 sectors, else 0
    uint8_t  mediaType;        // Media descriptor
    uint16_t sectorsPerFAT16;  // Unused in FAT32
    uint16_t sectorsPerTrack;  // Geometry data
    uint16_t numHeads;         // Geometry data
    uint32_t hiddenSectors;    // Used for partitions (0 in superfloppy)
    uint32_t totalSectors32;   // Total sectors for FAT32
    uint32_t sectorsPerFAT32;  // Sectors per FAT
    uint16_t extFlags;         // Mirroring info
    uint16_t fsVersion;        // FAT32 version
    uint32_t rootCluster;      // First cluster of root directory (typically 2)
    uint16_t fsInfo;           // Sector number of FSInfo structure
    uint16_t backupBootSector; // Sector number of backup boot sector
    uint8_t  reserved[12];     // Unused
    uint8_t  driveNumber;      // BIOS drive number
    uint8_t  reserved1;        // Unused
    uint8_t  bootSignature;    // 0x29 if extended boot signature
    uint32_t volumeID;         // Volume serial number
    char     volumeLabel[11];  // Volume label
    char     fsType[8];        // "FAT32   "
    uint8_t  bootCode[420];    // Bootloader code
    uint16_t bootSectorSig;    // 0xAA55 (Boot sector signature)
} Fat32BootSector;

typedef struct __attribute__((packed)) {
    char     filename[11];     // File name (padded with spaces)
    uint8_t  attr;            // Attributes (readonly, hidden, system, etc.)
    uint8_t  ntRes;           // Reserved for Windows NT
    uint8_t  creationTimeTenth;// Tenth-of-a-second timestamp
    uint16_t creationTime;    // Creation time
    uint16_t creationDate;    // Creation date
    uint16_t lastAccessDate;  // Last access date
    uint16_t firstClusterHigh;// High word of first cluster number
    uint16_t writeTime;       // Last modified time
    uint16_t writeDate;       // Last modified date
    uint16_t firstClusterLow; // Low word of first cluster number
    uint32_t fileSize;        // File size in bytes
} Fat32DirectoryEntry;

#endif
