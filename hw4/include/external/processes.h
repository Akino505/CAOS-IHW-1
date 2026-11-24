#ifndef PROCESSES_H
#define PROCESSES_H
#include "system.h"

const char *role_name(int role);
int run_worker(System *sys, int role, int idx);
int worker_total(System *sys);
int clock_run(System *sys, int delay_ms);
#endif
