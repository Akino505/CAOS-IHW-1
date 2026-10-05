#ifndef DISPATCHER_H
#define DISPATHER_H

#include "utils/buffer.h"

typedef struct {
    bool on_pumps[];
    double reservoirs_last_volume[];
    int last_messages;
    int max_power;
    Buffer last_messages_ids;
    int strategy_number;
} Dispatcher;

#endif