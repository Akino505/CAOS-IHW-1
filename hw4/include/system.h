#ifndef SYSTEM_H
#define SYSTEM_H

#include "dispatcher.h"
#include "district.h"
#include "message.h"
#include "pipe.h"
#include "pump.h"
#include "reservoir.h"
#include "sensor.h"
#include <semaphore.h>

#include "utils/consts.h"
#include "utils/sem_enum.h"


typedef struct System{
    int current_time;
    volatile int emergency;
    Reservoir reservoirs[MAX_RESERVOIRS];
    int reservoir_number;
    Pump pumps[MAX_PUMPS];
    int pumps_number;
    District districts[MAX_DISTRICTS];
    int districts_number;
    Pipe pipes[MAX_PIPES];
    int pipes_number;
    Sensor sensors[MAX_SENSORS];
    int sensors_number;
    Message messages[MAX_MESSAGES];
    int message_count;
    int message_capacity;
    int global_messages_counter;
    double loss_probability;
    double delay_probability;
    int max_delay;
    double dublicate_probability;
    Dispatcher dispatcher;
    volatile int init_done;
    volatile int stop;
    pid_t clock_pid;
    int max_time;
    unsigned seed;
    sem_t unnamed[SEM_COUNT];
} System;

#endif
