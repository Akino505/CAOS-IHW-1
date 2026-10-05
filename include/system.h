#ifndef SYSTEM_H
#define SYSTEM_H

#include "dispatcher.h"
#include "district.h"
#include "message.h"
#include "pipe.h"
#include "pump.h"
#include "reservoir.h"
#include "sensor.h"

typedef struct {
    int reservoir_number;
    Reservoir *reservoirs;
    int pumps_number;
    Pump *pumps;
    int districts_number;
    District *districts;
    int pipes_number;
    Pipe *pipes;
    int sensors_number;
    Sensor *sensors;

    Dispatcher dispatcher;

    int current_time;
    bool emergency;
} System;

#endif


// мин|начальный объем|макс
//  reservoir1 - 20|50|100
//  reservoir2 - 30|75|150
// Мощность|производительность
//  pump1 - 50|10
//  pump2 - 80|15
//
//  district1 - 8
//  district2 - 12
//
// tube1 - p1-r1|20
// tube2 - r1-d1|15
// tube3 - p2-r2|25
// tube4 - r2-d2|20

// sensor1 - r1|3
// sensor2 - r2|3

// rand_lost = 20%
// delay = 2
// rand_dubl = 10%
// max_energy = 100
// strategy = greedy
