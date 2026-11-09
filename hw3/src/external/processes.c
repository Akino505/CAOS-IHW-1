#include "external/processes.h"
#include "external/ipc.h"
#include "external/logger.h"
#include "utils/role.h"
#include "utils/phase.h"
#include "utils/strategy_type.h"
#include "simulation.h"
#include "strategies.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "utils/sem_enum.h"

const char *role_name(int role) {
    switch (role) {
    case ROLE_NETWORK: return "network";
    case ROLE_DISPATCHER: return "dispatcher";
    case ROLE_DISTRICT: return "district";
    case ROLE_PIPE: return "pipe";
    default: return "sensor";
    }
}

int worker_total(System *sys) {
    return 2 + sys->districts_number + sys->pipes_number + sys->sensors_number;
}

static void step(System *sys, int role, int idx) {
    switch (role) {
    case ROLE_NETWORK: network_work(sys); break;
    case ROLE_DISPATCHER: dispatcher_work(sys); break;
    case ROLE_DISTRICT: district_work(sys, idx); break;
    case ROLE_PIPE: water_physics(sys, idx); break;
    default: sensors_work(sys, idx); break;
    }
}

int run_worker(System *sys, int role, int idx) {
    int go, phase, limit = 1;
    switch (role) {
    case ROLE_NETWORK:
        go = SEM_GO_NETWORK; phase = PH_NETWORK; break;
    case ROLE_DISPATCHER:
        go = SEM_GO_DISPATCHER; phase = PH_DISPATCHER; break;
    case ROLE_DISTRICT:
        limit = sys->districts_number;
        go = SEM_GO_DISTRICT + idx; phase = PH_DISTRICT; break;
    case ROLE_PIPE:
        limit = sys->pipes_number;
        go = SEM_GO_PIPE + idx; phase = PH_PIPE; break;
    default:
        limit = sys->sensors_number;
        go = SEM_GO_SENSOR + idx; phase = PH_SENSOR; break;
    }
    if (role >= ROLE_DISTRICT && (idx < 0 || idx >= limit)) {
        fprintf(stderr, "ERROR: %s index %d is out of range (0..%d)\n",
                role_name(role), idx, limit - 1);
        return 2;
    }
    if (role == ROLE_NETWORK)
        srand(sys->seed);
    if (role == ROLE_DISPATCHER) {
        sys->dispatcher.strategy =
            sys->dispatcher.strategy_type == STRATEGY_SEQUENTIAL
                ? sequential_strategy
                : greedy_strategy;
    }

    char who[32];
    if (role >= ROLE_DISTRICT)
        snprintf(who, sizeof(who), "%s #%d", role_name(role), idx);
    else
        snprintf(who, sizeof(who), "%s", role_name(role));
    log_event("[SYS] Process %s started (pid=%d)\n", who, (int)getpid());
    sem_release(SEM_READY);

    for (;;) {
        if (sem_acquire(go, 0) != 0)
            break;
        step(sys, role, idx);
        sem_release(SEM_DONE + phase);
    }
    sem_release(SEM_EXIT);
    return 0;
}

static int run_phase(int phase, int go_base, int n) {
    for (int i = 0; i < n; i++)
        sem_release(go_base + i);
    for (int i = 0; i < n; i++) {
        int r = sem_acquire(SEM_DONE + phase, 10000);
        if (r != 0) {
            if (r == 1)
                log_event("[SYS] ERROR: a worker did not answer in phase %d\n",
                          phase);
            return -1;
        }
    }
    return 0;
}

static int run_sensors(System *sys) {
    for (int i = 0; i < sys->sensors_number; i++)
        if (run_phase(PH_SENSOR, SEM_GO_SENSOR + i, 1))
            return -1;
    return 0;
}

int clock_run(System *sys, int delay_ms) {
    int expected = worker_total(sys);
    log_event("[SYS] Clock process started (pid=%d); waiting for %d worker "
              "processes\n",
              (int)getpid(), expected);
    for (int i = 0; i < expected; i++) {
        if (sem_acquire(SEM_READY, 60000) != 0) {
            log_event("[SYS] ERROR: workers did not start (%d of %d ready)\n",
                      i, expected);
            sys->stop = 1;
            return 1;
        }
    }
    log_event("[SYS] All %d workers are ready. START: starting the "
              "simulation\n",
              expected);

    while ((sys->max_time == 0 || sys->current_time < sys->max_time) &&
           !sys->emergency && !g_stop && !sys->stop) {
        if (run_phase(PH_NETWORK, SEM_GO_NETWORK, 1) ||
            run_phase(PH_DISPATCHER, SEM_GO_DISPATCHER, 1) ||
            run_phase(PH_DISTRICT, SEM_GO_DISTRICT, sys->districts_number) ||
            run_phase(PH_PIPE, SEM_GO_PIPE, sys->pipes_number) ||
            run_sensors(sys))
            break;
        show_levels(sys);
        check_emergency(sys);
        sys->current_time++;
        if (delay_ms > 0)
            msleep(delay_ms);
    }

    sys->stop = 1;
    int exited = 0;
    for (int i = 0; i < expected; i++) {
        if (sem_wait_plain(SEM_EXIT, 3000) != 0)
            break;
        exited++;
    }
    log_event("[SYS] %d of %d workers exited\n", exited, expected);
    return 0;
}
