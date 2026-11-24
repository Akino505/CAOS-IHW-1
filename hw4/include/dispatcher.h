#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "system.h"
#include "lastmsg.h"
#include <stdbool.h>
#include "utils/strategy_type.h"
#include "utils/consts.h"

struct System;

typedef struct Dispatcher {
    bool on_pumps[MAX_PUMPS];
    double reservoirs_last_volume[MAX_RESERVOIRS];
    int max_power;
    LastMessagesIds last_messages_ids;
    StrategyType strategy_type;
    void (*strategy)(struct Dispatcher*, struct System*);
} Dispatcher;

#endif
