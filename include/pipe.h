#ifndef PIPE_H
#define PIPE_H

#include "utils/structure_type.h"

typedef struct {
    int id;
    double capacity;
    double flow;
    StructureType source_type;
    int source_id;
    StructureType destination_type;
    int destination_id;
} Pipe;

#endif