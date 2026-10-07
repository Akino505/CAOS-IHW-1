#ifndef STARTEGIES_H
#define STRATEGIES_H

#include "system.h"
#include "dispatcher.h"

void sequential_strategy(Dispatcher *disp, System *sys);
void greedy_strategy(Dispatcher *disp, System *sys);

#endif
