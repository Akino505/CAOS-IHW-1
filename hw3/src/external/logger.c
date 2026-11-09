#include "external/logger.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int log_fd = -1;

static void open_log(const char *filename, int flags) {
    log_fd = open(filename, flags, 0644);
    if (log_fd < 0) {
        perror("ERROR: Failed to open log file");
        exit(1);
    }
}

void logger_init(const char *filename) {
    open_log(filename, O_CREAT | O_WRONLY | O_TRUNC | O_APPEND);
}

void logger_attach(const char *filename) {
    open_log(filename, O_CREAT | O_WRONLY | O_APPEND);
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
    if (len < 0)
        return;
    if (len >= (int)sizeof(buffer))
        len = sizeof(buffer) - 1;
    ssize_t r = write(STDOUT_FILENO, buffer, len);
    (void)r;
    if (log_fd >= 0) {
        r = write(log_fd, buffer, len);
        (void)r;
    }
}
