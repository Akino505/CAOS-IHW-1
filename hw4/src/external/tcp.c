#define _POSIX_C_SOURCE 200809L
#include "external/tcp.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CLIENTS 64
#define EVENT_BUFFER 4096

static int make_listener(const char *ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        close(fd); errno = EINVAL; return -1;
    }
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
        listen(fd, 16) < 0) {
        close(fd); return -1;
    }
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return fd;
}

static void relay_loop(int read_fd, int server) {
    signal(SIGPIPE, SIG_IGN);
    int clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; ++i) clients[i] = -1;
    char buffer[EVENT_BUFFER];
    for (;;) {
        struct pollfd pfd = {read_fd, POLLIN | POLLHUP, 0};
        int pr = poll(&pfd, 1, 100);
        if (pr > 0 && (pfd.revents & (POLLIN | POLLHUP))) {
            ssize_t n = read(read_fd, buffer, sizeof(buffer));
            if (n <= 0) break;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (clients[i] < 0) continue;
                ssize_t sent = send(clients[i], buffer, (size_t)n,
                                    MSG_DONTWAIT | MSG_NOSIGNAL);
                if (sent != n) { close(clients[i]); clients[i] = -1; }
            }
        }
        for (;;) {
            int c = accept(server, NULL, NULL);
            if (c < 0) break;
            int flags = fcntl(c, F_GETFL, 0);
            fcntl(c, F_SETFL, flags | O_NONBLOCK);
            int placed = 0;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (clients[i] < 0) { clients[i] = c; placed = 1; break; }
            }
            if (!placed) close(c);
        }
    }
    for (int i = 0; i < MAX_CLIENTS; ++i)
        if (clients[i] >= 0) close(clients[i]);
    close(server);
    close(read_fd);
    _exit(0);
}

pid_t tcp_start(const char *bind_ip, int port, int *event_fd) {
    int server = make_listener(bind_ip, port);
    if (server < 0) return -1;
    int fds[2];
    if (pipe(fds) < 0) { close(server); return -1; }
    pid_t pid = fork();
    if (pid < 0) { close(server); close(fds[0]); close(fds[1]); return -1; }
    if (pid == 0) {
        close(fds[1]);
        relay_loop(fds[0], server);
    }
    close(server);
    close(fds[0]);
    *event_fd = fds[1];
    return pid;
}

void tcp_stop(pid_t relay_pid, int event_fd) {
    if (event_fd >= 0) close(event_fd);
    if (relay_pid > 0) waitpid(relay_pid, NULL, 0);
}
