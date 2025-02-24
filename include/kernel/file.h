#ifndef KERNEL_FILE_H
#define KERNEL_FILE_H

#define MAX_FDS 32

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// typedef size_t ino_t;

/* Per–process file descriptor table entry */


// struct dirent {
//     ino_t d_ino;          // Inode number
//     off_t d_off;          // Offset to the next dirent
//     uint16_t d_reclen;    // Length of this record
//     uint8_t d_type;       // Type of file (DT_REG, DT_DIR, etc.)
//     char d_name[256];     // Null-terminated filename
// } dir_t;


#endif // KERNEL_FILE_H
