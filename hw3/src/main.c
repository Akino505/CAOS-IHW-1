#include "external/clean.h"
#include "external/ipc.h"
#include "external/logger.h"
#include "external/processes.h"
#include "external/read_config.h"
#include "external/signal_handler.h"
#include "external/statistics.h"
#include "utils/role.h"
#include "utils/sem_enum.h"
#include "simulation.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_TIME 100

#ifdef UNNAMED
#define NAMED 0
#define VARIANT "unnamed POSIX semaphores in shared memory"
#else
#define NAMED 1
#define VARIANT "named POSIX semaphores"
#endif

static pid_t pids[64];
static int npids;

static void spawn(System *sys, int role, int idx) {
    pid_t p = fork();
    if (p < 0) {
        perror("fork");
        sys->stop = 1;
        return;
    }
    if (p == 0) {
        int rc = run_worker(sys, role, idx);
        ipc_detach();
        _exit(rc);
    }
    pids[npids++] = p;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <config_file> [delay_ms] [max_time] [seed]\n",
                argv[0]);
        return 1;
    }
    int delay = 500;
    int max_time = MAX_TIME;
    unsigned seed = 1;
    const char *config_file = argv[1];
    if (argc > 2) {
        delay = atoi(argv[2]);
    }
    if (argc > 3) {
        max_time = atoi(argv[3]);
    }
    if (argc > 4) {
        seed = (unsigned)strtoul(argv[4], NULL, 10);
    }

    logger_init(LOG_FILE);
    log_event("START: starting the simulation (%s)\n", VARIANT);
    System *sys = ipc_create(NAMED);
    if (sys == NULL)
        return 1;
    init_signal(sys);
    if (read_config(config_file, sys) != 0) {
        log_event("ERROR: Failed to read configuration\n");
        clean_system(sys);
        return 1;
    }
    sys->max_time = max_time;
    sys->seed = seed;
    sys->init_done = 1;

    spawn(sys, ROLE_NETWORK, 0);
    spawn(sys, ROLE_DISPATCHER, 0);
    for (int i = 0; i < sys->districts_number; i++) spawn(sys, ROLE_DISTRICT, i);
    for (int i = 0; i < sys->pipes_number; i++) spawn(sys, ROLE_PIPE, i);
    for (int i = 0; i < sys->sensors_number; i++) spawn(sys, ROLE_SENSOR, i);

    clock_run(sys, delay);

    for (int i = 0; i < npids; i++)
        waitpid(pids[i], NULL, 0);
    log_event("FINISH: Finish simulation\n");
    if (g_stop)
        log_event("Program terminated by the user\n");
    print_statistics(sys);
    logger_close();
    clean_system(sys);
    return 0;
}
