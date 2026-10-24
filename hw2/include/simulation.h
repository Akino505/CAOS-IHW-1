#ifndef SIMULATION_H
#define SIMULATION_H

#include "system.h"
#include "strategies.h"

void init_system(System *sys);
void water_physics(System *sys);
void check_emergency(System *sys);
void sensors_work(System *sys);
void network_work(System *sys);
void dispatcher_work(System *sys);

void *physics_thread_func(void *arg);
void *dispatcher_thread_func(void *arg);
void *network_thread_func(void *arg);
void *sensors_thread_func(void *arg);

#endif
