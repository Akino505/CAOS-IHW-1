#ifndef SIMULATION_H
#define SIMULATION_H

#include "system.h"

void init_system(System *system);
void water_physics(System *system);
void check_emergency(System *system);
void log_print(System *system);

#endif
