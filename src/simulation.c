#include "simulation.h"
#include <stdio.h>
#include <stdlib.h>

// мин|начальный объем|макс
//  reservoir1 - 20|50|100
//  reservoir2 - 30|75|150
// Мощность|производительность
//  pump1 - 50|10
//  pump2 - 80|15
//
//  district1 - 8
//  district2 - 12
//
// tube1 - p1-r1|20
// tube2 - r1-d1|15
// tube3 - p2-r2|25
// tube4 - r2-d2|20

// sensor1 - r1|3
// sensor2 - r2|3

// rand_lost = 20%
// delay = 2
// rand_dubl = 10%
// max_energy = 100
// strategy = greedy

void init_system(System *system) {
    // hardcode for now
    int pumps_number = 2;
    int reservoirs_number = 2;
    int sensors_number = 2;
    int districts_number = 2;
    int pipes_number = 4;
    //---
    system->current_time = 0;
    system->emergency = false;

    system->districts_number = districts_number;
    system->districts = malloc(system->districts_number * sizeof(District));

    system->districts[0] = (District){
        .id = 0,
        .schedule = (SchedulePoint[]){{.time = 0, .consumption = 0.0}},
        .current_point = &system->districts[0].schedule[0]};
    system->districts[1] = (District){
        .id = 1,
        .schedule = (SchedulePoint[]){{.time = 0, .consumption = 12.0}},
        .current_point = &system->districts[1].schedule[0]};

    system->reservoir_number = reservoirs_number;
    system->reservoirs = malloc(system->reservoir_number * sizeof(Reservoir));
    system->reservoirs[0] =
        (Reservoir){.id = 0, .volume = 50, .min_volume = 20, .max_volume = 100};
    system->reservoirs[1] =
        (Reservoir){.id = 1, .volume = 75, .min_volume = 30, .max_volume = 150};

    system->pumps_number = pumps_number;
    system->pumps = malloc(system->pumps_number * sizeof(Pump));
    system->pumps[0] = (Pump){.id = 0, .capacity = 10.0, .power = 50};
    system->pumps[1] = (Pump){.id = 1, .capacity = 15.0, .power = 80};

    system->pipes_number = pipes_number;
    system->pipes = malloc(system->pipes_number * sizeof(Pipe));
    system->pipes[0] = (Pipe){.id = 0,
                              .capacity = 20.0,
                              .source_id = 0,
                              .source_type = PUMP,
                              .destination_id = 0,
                              .destination_type = RESERVOIR};
    system->pipes[1] = (Pipe){.id = 1,
                              .capacity = 15.0,
                              .source_id = 0,
                              .source_type = RESERVOIR,
                              .destination_id = 0,
                              .destination_type = DISTRICT};
    system->pipes[2] = (Pipe){.id = 2,
                              .capacity = 25.0,
                              .source_id = 1,
                              .source_type = PUMP,
                              .destination_id = 1,
                              .destination_type = RESERVOIR};
    system->pipes[3] = (Pipe){.id = 3,
                              .capacity = 20.0,
                              .source_id = 1,
                              .source_type = RESERVOIR,
                              .destination_id = 1,
                              .destination_type = DISTRICT};

    system->sensors_number = sensors_number;
    system->sensors = malloc(system->sensors_number * sizeof(Sensor));
    system->sensors[0] = (Sensor){.id = 0,
                                  .measurement_frequency = 3,
                                  .reservoir_id = 0,
                                  .next_time = system->current_time};
    system->sensors[0] = (Sensor){.id = 1,
                                  .measurement_frequency = 3,
                                  .reservoir_id = 1,
                                  .next_time = system->current_time};

    system->dispatcher.on_pumps = malloc(system->pumps_number * sizeof(bool));
    for (int i = 0; i < pumps_number; ++i) {
        system->dispatcher.on_pumps[i] = false;
    }
    system->dispatcher.max_power = 100;
    system->dispatcher.reservoirs_last_volume =
        malloc(system->reservoir_number * sizeof(double));
    for (int i = 0; i < system->reservoir_number; ++i) {
        system->dispatcher.reservoirs_last_volume[i] =
            system->reservoirs[i].volume;
    }
};

double calculate_flow(Pipe *pipe, System *system) {
    double source_wants = 0.0;
    if (pipe->source_type == PUMP) {
        if (system->dispatcher.on_pumps[pipe->source_id]) {
            source_wants = system->pumps[pipe->source_id].capacity;
        } else {
            source_wants = 0.0;
        }
    } else { // source_type == RESERVOIR
        source_wants =
            system->districts[pipe->destination_id].current_point->consumption;
        if (source_wants > system->reservoirs[pipe->source_id].volume) {
            source_wants = system->reservoirs[pipe->source_id].volume;
        }
    }
    double actual_flow = source_wants;
    if (actual_flow > pipe->capacity) {
        actual_flow = pipe->capacity;
    }
    return actual_flow;
}

void water_physics(System *system) {
    for (int i = 0; i < system->pipes_number; ++i) {
        Pipe *pipe = &system->pipes[i];
        double flow = calculate_flow(pipe, system);
        if (pipe->source_type == RESERVOIR) {
            system->reservoirs[pipe->source_id].volume -= flow;
        }
        if (pipe->destination_type == RESERVOIR) {
            system->reservoirs[pipe->destination_id].volume += flow;
        }
    }
}

void check_emergency(System *system) {
    for (int i = 0; i < system->reservoir_number; i++) {
        if (system->reservoirs[i].volume > system->reservoirs[i].max_volume) {
            printf("[T=%03d] АВАРИЯ: Резервуар %d переполнен (%.1f > %.1f)\n",
                   system->current_time, i, system->reservoirs[i].volume,
                   system->reservoirs[i].max_volume);
            system->emergency = true;
        }
        if (system->reservoirs[i].volume < system->reservoirs[i].min_volume) {
            printf("[T=%03d] АВАРИЯ: Резервуар %d высох (%.1f < %.1f)\n",
                   system->current_time, i, system->reservoirs[i].volume,
                   system->reservoirs[i].min_volume);
            system->emergency = true;
        }
    }

    double total_power = 0.0;
    for (int i = 0; i < system->pumps_number; i++) {
        if (system->dispatcher.on_pumps[i]) {
            total_power += system->pumps[i].power;
        }
    }
    if (total_power > system->dispatcher.max_power) {
        printf("[T=%03d] АВАРИЯ: Превышен лимит мощности (%.1f > %d)\n",
               system->current_time, total_power, system->dispatcher.max_power);
        system->emergency = true;
    }
}
