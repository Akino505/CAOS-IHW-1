#ifndef MESSAGE_H
#define MESSAGE_H

typedef struct {
    int id;
    int current_time;
    int delivery_time;
    int reservoir_id;
    double volume;
} Message;

#endif
