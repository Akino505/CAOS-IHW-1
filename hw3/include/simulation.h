#ifndef SIMULATION_H
#define SIMULATION_H

#include "system.h"
#include "strategies.h"

void init_system(System *sys);
void water_physics(System *sys, int i);
void check_emergency(System *sys);
void district_work(System *sys, int i);
void sensors_work(System *sys, int i);
void network_work(System *sys);
void dispatcher_work(System *sys);
void show_levels(System *sys);

#endif
