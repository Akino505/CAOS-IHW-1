#ifndef BUFFER_H
#define BUFFER_H

#define BUFFER_SIZE 1000

typedef struct {
    int last_ids[BUFFER_SIZE];
    int current_idx;
} Buffer;

#endif