#ifndef SIMULATION_H
#define SIMULATION_H

#include "system.h"
#include "strategies.h"

void init_system(System *sys);
void water_physics(System *sys);
void check_emergency(System *sys);
void log_print(System *sys);
void sensors_work(System *sys);
void network_work(System *sys);
void dispatcher_work(System *sys);

#endif
