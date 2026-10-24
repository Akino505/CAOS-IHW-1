#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "utils/buffer.h"
#include <stdbool.h>
#include "system.h"

struct System;

typedef struct Dispatcher {
    bool *on_pumps;
    double *reservoirs_last_volume;
    int max_power;
    Buffer last_messages_ids;
    void (*strategy)(struct Dispatcher*, struct System*);
} Dispatcher;

#endif
