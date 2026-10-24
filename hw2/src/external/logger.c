#include "external/logger.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int log_fd = -1;

void logger_init(const char *filename) {
    log_fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (log_fd < 0) {
        perror("ERROR: Failed to open log file");
        exit(1);
    }
}

void logger_close(void) {
    if (log_fd >= 0) {
        close(log_fd);
        log_fd = -1;
    }
}

void log_event(const char *format, ...) {
    char buffer[1024];
    va_list args;

    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    printf("%s", buffer);

    if (log_fd >= 0) {
        write(log_fd, buffer, len);
    }
}
