// A simple shell to fall back on
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <syscalls.h>

#define BIN_PATH "/mnt/elf"

#define BUF_SIZE 64
#define PROMPT "> "

typedef int (*builtin_func)(int, char **);
typedef struct {
    char *name;
    builtin_func func;
} builtin_cmd;
// for now, PATH is hardcoded and single val. later we will get it from the environment with getenv syscall.

// struct dirent *entry;

// A simple shell to fall back on
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <syscalls.h>

// for now, path is hardcoded. later we will get it from the environment with getenv syscall.
#define PATH "/mnt/elf"


#define BUF_SIZE 64
#define PROMPT "> "

int cmd_exit(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("Exiting shell!\n");
    exit(0);
}


int cmd_ls(int argc, char **argv) {
    dirent_t entry[4];
    // const char* path = argc > 1 ? argv[1] : ".";
    int dir = open("/", OPEN_MODE_NOBLOCK, 0);

    if (dir < 0) {
        printf("Could not open directory '%s'\n", "/");
        return 1;
    }

    printf("Unimplemented!\n");
    return -1;

    int entries;
    while ((entries = readdir(dir, entry, 4)) > 0) {
        for (int i = 0; i < entries; i++) {
            printf("%s  ", entry[i].d_name);
        }
    }

    printf("\n");
    close(dir);
    return 0;
}

int cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
    return 0;
}


int cmd_cat(int argc, char **argv) {
    int fd;
    char buf[128];
    ssize_t bytes_read;

    if (argc < 2) {
        // Read from stdin if no file provided
        while ((bytes_read = read(0, buf, sizeof(buf))) > 0) {
            write(1, buf, bytes_read);
        }
        return 0;
    }

    // Open file
    fd = open(argv[1], OPEN_MODE_READ, 0);
    if (fd < 0) {
        printf("cat: cannot open %s\n", argv[1]);
        return 1;
    }

    size_t total = 0;
    printf("Contents of %s:\n", argv[1]);
    while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
        printf("bytes_read: %d\n", bytes_read);
        write(1, buf, bytes_read);
        total += bytes_read;
        if (total >= 1024) break; // Stop after 1KB
    }

    printf("read bytes/errL %d\n", bytes_read);

    close(fd);
    return 0;
}

int cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("Available commands:\n");
    printf("  exit\n");
    printf("  echo\n");
    printf("  ls\n");
    printf("  cat\n");
    printf("  help\n");
    printf("  clear\n");
    return 0;
}

int cmd_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("\033[2J\033[H");
    return 0;
}

int cmd_write(int argc, char **argv) {
    int fd;
    char buf[BUF_SIZE];

    if (argc < 2) {
        printf("Usage: write <filename> [text]\n");
        return 1;
    }

    // Open file for appending (create if it doesn't exist)
    fd = open(argv[1], OPEN_MODE_WRITE | OPEN_MODE_CREATE | OPEN_MODE_APPEND, 0);
    if (fd < 0) {
        printf("write: cannot open %s for writing\n", argv[1]);
        return 1;
    }

    // if (argc >= 3) {
    //     // Write command-line arguments directly
    //     for (int i = 2; i < argc; i++) {
    //         write(fd, argv[i], strlen(argv[i]));
    //         if (i < argc - 1) {
    //             write(fd, " ", 1); // Add space between arguments
    //         }
    //     }
    //     write(fd, "\n", 1); // Add newline at the end
    // } else {
    //     // No text arguments provided, read from stdin
    //     printf("Enter text to write (press Ctrl+D to finish):\n");
    //     ssize_t bytes_read;
    //     while ((bytes_read = read(0, buf, sizeof(buf) - 1)) > 0) {
    //         buf[bytes_read] = '\0';
    //         write(fd, buf, bytes_read);
    //     }
    // }

    close(fd);
    return 0;
}

builtin_cmd builtins[] = {
    {"exit", cmd_exit},
    {"echo", cmd_echo},
    {"ls", cmd_ls},
    {"cat", cmd_cat},
    // {"cd", cmd_cd},
    // {"pwd", cmd_pwd},
    {"help", cmd_help},
    {"clear", cmd_clear},
    // {"write", cmd_write},
    {NULL, NULL}
};

builtin_func find_builtin(char *cmd) {
    for (int i = 0; builtins[i].name; i++) {
        if (!strcmp(cmd, builtins[i].name)) {
            return builtins[i].func;
        }
    }
    return NULL;
}




int main(void) {
    char buf[BUF_SIZE];
    size_t pos = 0;

    printf("Welcome to the shell!\n");

    // Write initial prompt
    write(stdout, PROMPT, strlen(PROMPT));

    while (1) {
        // Try to read one character
        char c;
        ssize_t rc = read(stdin, &c, 1);

        if (rc == -EAGAIN) {
            usleep(1000);
            continue;
        }

        if (rc <= 0) {
            // Error or EOF, restart
            pos = 0;
            write(stdout, "\n" PROMPT, strlen("\n" PROMPT));
            continue;
        }

        if (c == '\b' || c == 0x7F) { // backspace
            if (pos > 0) {
                pos--;
                write(stdout, "\b \b", 3);
            }
            continue;
        }

        // Handle enter
        if (c == '\n' || c == '\r') {
            if (pos == 0) {
                write(stdout, "\n"PROMPT, strlen("\n"PROMPT));
                continue;
            }
            buf[pos] = '\0';
            write(stdout, "\n", 1);

            char *argv[4] = {0};
            char *token = buf;
            int argc = 0;

            while (*token && argc < 3) {
                argv[argc++] = token;
                while (*token && *token != ' ') token++;
                if (*token) *token++ = '\0';
            }

            // Fork and exec if file exists, else print error
            builtin_func builtin = find_builtin(argv[0]);
            if (builtin) {
                builtin(argc, argv);
            } else {
                // Execute external program
                int pid = fork();
                if (pid < 0) return -1;
                if (pid == 0) {
                    char exec_path[128];
                    snprintf(exec_path, sizeof(exec_path), "%s/%s", PATH, argv[0]);
                    // for now, just exec and fail. later we can use access syscall to check if file exists.
                    if (exec(exec_path) != 0) {
                        printf("File %s not found (or exec failed!)!\n", argv[0]);
                    }
                    exit(1);
                } else {
                    waitpid(pid);
                }
            }


            // Reset for next command
            pos = 0;
            write(stdout, PROMPT, strlen(PROMPT));
            continue;
        }

        if (pos < BUF_SIZE-1 && c >= 32 && c <= 126) {
            buf[pos++] = c;
            write(stdout, &c, 1);
        }
    }

    return 0;
}
