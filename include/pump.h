#ifndef PUMP_H
#define PUMP_H

#include "utils/state.h"

typedef struct {
    int id;
    State state;
    int reservoir_id;
    double capacity;
    int power;
} Pump;

#endif