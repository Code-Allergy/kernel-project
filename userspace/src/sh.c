// A simple shell to fall back on
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <syscalls.h>



#define BUF_SIZE 64
#define PROMPT "> "

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

            if (strlen(argv[0]) == 4 && !memcmp(argv[0], "exit", 4)) {
                exit(0);
            }

            // Fork and exec if file exists, else print error

            // for now, don't give a shit if it exists, just fork and report the fail
            int pid = fork();
            if (pid == 0) {
                if (exec(argv[0]) != 0) {
                    printf("File %s not found (or exec failed!)!\n", argv[0]);
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
