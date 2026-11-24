#ifndef LOGGER_H
#define LOGGER_H

void logger_init(const char *filename);
void logger_attach(const char *filename);
void logger_close(void);
void logger_set_stream_fd(int fd);
void log_event(const char *format, ...);

#endif
