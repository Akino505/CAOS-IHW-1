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

void init_system(System *sys) {
    // hardcode for now
    int pumps_number = 2;
    int reservoirs_number = 2;
    int sensors_number = 2;
    int districts_number = 2;
    int pipes_number = 4;
    //---
    sys->current_time = 0;
    sys->emergency = false;

    sys->message_capacity = 100;
    sys->messages = malloc(sys->message_capacity * sizeof(Message));
    sys->message_count = 0;
    sys->global_messages_counter = 0;

    sys->districts_number = districts_number;
    sys->districts = malloc(sys->districts_number * sizeof(District));

    sys->districts[0].schedule = malloc(sizeof(SchedulePoint));
    sys->districts[0].schedule[0].time = 0;
    sys->districts[0].schedule[0].consumption = 8.0;
    sys->districts[0] =
        (District){.id = 0, .current_point = &sys->districts[0].schedule[0]};

    sys->districts[1].schedule = malloc(sizeof(SchedulePoint));
    sys->districts[1].schedule[0].time = 0;
    sys->districts[1].schedule[0].consumption = 12.0;
    sys->districts[1] =
        (District){.id = 1, .current_point = &sys->districts[1].schedule[0]};

    sys->reservoir_number = reservoirs_number;
    sys->reservoirs = malloc(sys->reservoir_number * sizeof(Reservoir));
    sys->reservoirs[0] =
        (Reservoir){.id = 0, .volume = 50, .min_volume = 20, .max_volume = 100};
    sys->reservoirs[1] =
        (Reservoir){.id = 1, .volume = 75, .min_volume = 30, .max_volume = 150};

    sys->pumps_number = pumps_number;
    sys->pumps = malloc(sys->pumps_number * sizeof(Pump));
    sys->pumps[0] = (Pump){.id = 0, .capacity = 10.0, .power = 50};
    sys->pumps[1] = (Pump){.id = 1, .capacity = 15.0, .power = 80};

    sys->pipes_number = pipes_number;
    sys->pipes = malloc(sys->pipes_number * sizeof(Pipe));
    sys->pipes[0] = (Pipe){.id = 0,
                           .capacity = 20.0,
                           .source_id = 0,
                           .source_type = PUMP,
                           .destination_id = 0,
                           .destination_type = RESERVOIR};
    sys->pipes[1] = (Pipe){.id = 1,
                           .capacity = 15.0,
                           .source_id = 0,
                           .source_type = RESERVOIR,
                           .destination_id = 0,
                           .destination_type = DISTRICT};
    sys->pipes[2] = (Pipe){.id = 2,
                           .capacity = 25.0,
                           .source_id = 1,
                           .source_type = PUMP,
                           .destination_id = 1,
                           .destination_type = RESERVOIR};
    sys->pipes[3] = (Pipe){.id = 3,
                           .capacity = 20.0,
                           .source_id = 1,
                           .source_type = RESERVOIR,
                           .destination_id = 1,
                           .destination_type = DISTRICT};

    sys->sensors_number = sensors_number;
    sys->sensors = malloc(sys->sensors_number * sizeof(Sensor));
    sys->sensors[0] = (Sensor){
        .id = 0, .measurement_frequency = 3, .reservoir_id = 0, .next_time = 0};
    sys->sensors[1] = (Sensor){
        .id = 1, .measurement_frequency = 3, .reservoir_id = 1, .next_time = 0};

    sys->dispatcher.on_pumps = malloc(sys->pumps_number * sizeof(bool));
    for (int i = 0; i < pumps_number; ++i) {
        sys->dispatcher.on_pumps[i] = false;
    }
    sys->dispatcher.max_power = 100;
    sys->dispatcher.reservoirs_last_volume =
        malloc(sys->reservoir_number * sizeof(double));
    for (int i = 0; i < sys->reservoir_number; ++i) {
        sys->dispatcher.reservoirs_last_volume[i] = sys->reservoirs[i].volume;
    }
};

double calculate_flow(Pipe *pipe, System *sys) {
    double source_wants = 0.0;
    if (pipe->source_type == PUMP) {
        if (sys->dispatcher.on_pumps[pipe->source_id]) {
            source_wants = sys->pumps[pipe->source_id].capacity;
        } else {
            source_wants = 0.0;
        }
    } else { // source_type == RESERVOIR
        source_wants =
            sys->districts[pipe->destination_id].current_point->consumption;
        if (source_wants > sys->reservoirs[pipe->source_id].volume) {
            source_wants = sys->reservoirs[pipe->source_id].volume;
        }
    }
    double actual_flow = source_wants;
    if (actual_flow > pipe->capacity) {
        actual_flow = pipe->capacity;
    }
    return actual_flow;
}

void water_physics(System *sys) {
    for (int i = 0; i < sys->pipes_number; ++i) {
        Pipe *pipe = &sys->pipes[i];
        double flow = calculate_flow(pipe, sys);
        if (pipe->source_type == RESERVOIR) {
            sys->reservoirs[pipe->source_id].volume -= flow;
        }
        if (pipe->destination_type == RESERVOIR) {
            sys->reservoirs[pipe->destination_id].volume += flow;
        }
    }
}

void check_emergency(System *sys) {
    for (int i = 0; i < sys->reservoir_number; i++) {
        if (sys->reservoirs[i].volume > sys->reservoirs[i].max_volume) {
            printf("[T=%03d] АВАРИЯ: Резервуар %d переполнен (%.1f > %.1f)\n",
                   sys->current_time, i, sys->reservoirs[i].volume,
                   sys->reservoirs[i].max_volume);
            sys->emergency = true;
        }
        if (sys->reservoirs[i].volume < sys->reservoirs[i].min_volume) {
            printf("[T=%03d] АВАРИЯ: Резервуар %d высох (%.1f < %.1f)\n",
                   sys->current_time, i, sys->reservoirs[i].volume,
                   sys->reservoirs[i].min_volume);
            sys->emergency = true;
        }
    }

    double total_power = 0.0;
    for (int i = 0; i < sys->pumps_number; i++) {
        if (sys->dispatcher.on_pumps[i]) {
            total_power += sys->pumps[i].power;
        }
    }
    if (total_power > sys->dispatcher.max_power) {
        printf("[T=%03d] АВАРИЯ: Превышен лимит мощности (%.1f > %d)\n",
               sys->current_time, total_power, sys->dispatcher.max_power);
        sys->emergency = true;
    }
}

void add_message(System *sys, Message msg) {
    if (sys->message_count >= sys->message_capacity) {
        sys->message_capacity *= 2;
        sys->messages =
            realloc(sys->messages, sys->message_capacity * sizeof(Message));
    }
    sys->messages[sys->message_count] = msg;
    sys->message_count++;
}

void remove_message_at(System *sys, int idx) {
    for (int i = idx; i < sys->message_count - 1; ++i) {
        sys->messages[i] = sys->messages[i + 1];
    }
    sys->message_count--;
}

void sensors_work(System *sys) {
    for (int i = 0; i < sys->sensors_number; i++) {
        Sensor *sensor = &sys->sensors[i];
        if (sys->current_time >= sensor->next_time) {
            double measured_volume =
                sys->reservoirs[sensor->reservoir_id].volume;
            Message msg;
            msg.id = sys->global_messages_counter++;
            msg.current_time = sys->current_time;
            msg.reservoir_id = sensor->reservoir_id;
            msg.volume = measured_volume;
            msg.delivery_time = sys->current_time;
            add_message(sys, msg);

            printf("[T=%03d] Датчик %d сделал замер: резервуар %d, уровень "
                   "%.1f л (сообщение ID=%d)\n",
                   sys->current_time, sensor->id, sensor->reservoir_id,
                   measured_volume, msg.id);

            sensor->next_time += sensor->measurement_frequency;
        }
    }
}
