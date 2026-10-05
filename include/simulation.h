#ifndef SIMULATION_H
#define SIMULATION_H

#include "system.h"

void init_system(System *sys);
void water_physics(System *sys);
void check_emergency(System *sys);
void log_print(System *sys);
void sensors_work(System *sys);

#endif
