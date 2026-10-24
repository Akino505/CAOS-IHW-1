#ifndef SYSTEM_H
#define SYSTEM_H

#include "dispatcher.h"
#include "district.h"
#include "message.h"
#include "pipe.h"
#include "pump.h"
#include "reservoir.h"
#include "sensor.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef struct System {
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
    int message_count;
    int message_capacity;
    Message *messages;
    int global_messages_counter;

    Dispatcher dispatcher;

    int current_time;
    bool emergency;

    int loss_probability;
    int delay_probability;
    int dublicate_probability;
    int max_delay;

    atomic_int stop_flag;
    pthread_mutex_t data_mutex;
    pthread_mutex_t log_mutex;
    pthread_mutex_t tick_mutex;    
    pthread_cond_t tick_start_cond;
    pthread_cond_t tick_done_cond;
    int workers_done;
    bool tick_ready;  
} System;

#endif
