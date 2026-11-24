#ifndef TCP_H
#define TCP_H
#include <sys/types.h>

/* Запускает TCP-ретранслятор событий. Возвращает PID процесса и fd для логов. */
pid_t tcp_start(const char *bind_ip, int port, int *event_fd);
void tcp_stop(pid_t relay_pid, int event_fd);

#endif
