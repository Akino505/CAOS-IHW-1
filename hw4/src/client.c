#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }
    char *end = NULL;
    long port = strtol(argv[2], &end, 10);
    if (!end || *end || port < 1 || port > 65535) {
        fprintf(stderr, "Invalid TCP port: %s\n", argv[2]);
        return 1;
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4 address: %s\n", argv[1]);
        close(fd); return 1;
    }
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); close(fd); return 1;
    }
    printf("Connected to simulation server %s:%ld. Waiting for events...\n",
           argv[1], port);
    char buffer[4096];
    ssize_t n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        size_t off = 0;
        while (off < (size_t)n) {
            ssize_t w = write(STDOUT_FILENO, buffer + off, (size_t)n - off);
            if (w < 0) { if (errno == EINTR) continue; close(fd); return 1; }
            off += (size_t)w;
        }
    }
    if (n < 0) perror("read");
    else fprintf(stderr, "Server closed the connection.\n");
    close(fd);
    return n < 0 ? 1 : 0;
}
