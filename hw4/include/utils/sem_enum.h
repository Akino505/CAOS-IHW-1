#ifndef SEM_ENUM_H
#define SEM_ENUM_H

#include "utils/consts.h"
#include "utils/phase.h"
#define SHM_NAME "/water_shm"
#define SEM_PREFIX "/water_sem_"
#define LOG_FILE "log.txt"

enum {
    SEM_GO_NETWORK = 0,
    SEM_GO_DISPATCHER = 1,
    SEM_GO_DISTRICT = 2,
    SEM_GO_PIPE = SEM_GO_DISTRICT + MAX_DISTRICTS,
    SEM_GO_SENSOR = SEM_GO_PIPE + MAX_PIPES,
    SEM_DONE = SEM_GO_SENSOR + MAX_SENSORS,
    SEM_READY = SEM_DONE + PHASES,
    SEM_EXIT,
    SEM_MTX_MSG,
    SEM_MTX_RES,
    SEM_COUNT
};

#endif