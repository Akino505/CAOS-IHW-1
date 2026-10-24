#ifndef SENSOR_H
#define SENSOR_H

typedef struct {
    int id;
    int reservoir_id;
    int measurement_frequency;
    // int current_time;
    int next_time;
} Sensor;

#endif