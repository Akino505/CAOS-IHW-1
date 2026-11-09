#include "external/signal_handler.h"
#include "external/clean.h"
#include "external/statistics.h"
#include <signal.h>
#include <string.h>

volatile sig_atomic_t g_stop = 0;
static System *sys_ptr = NULL;

static void signal_handler(int signum) {
    (void)signum;
    g_stop = 1;
    if (sys_ptr != NULL)
        sys_ptr->stop = 1;
}

void init_signal(System *sys) {
    sys_ptr = sys;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);
}
