#include "external/signal_handler.h"
#include "external/clean.h"
#include "external/logger.h"
#include "external/statistics.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static System *sys_ptr = NULL;

static void signal_handler(int signum) {
    if (sys_ptr != NULL) {
        print_statistics(sys_ptr);
        clean_system(sys_ptr);
        logger_close();
    }
    printf("Program terminated by the user\n");
    exit(0);
}

void init_signal(System *sys) {
    sys_ptr = sys;
    signal(SIGINT, signal_handler);
}
