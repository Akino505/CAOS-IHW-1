#include "simulation.h"
#include "external/ipc.h"
#include "external/logger.h"
#include "utils/sem_enum.h"
#include <stdio.h>
#include <stdlib.h>

static int lock(int idx) { return sem_wait_plain(idx, 5000) == 0 ? 0 : -1; }
static void unlock(int idx) { sem_release(idx); }

static double calculate_flow(Pipe *pipe, System *sys) {
    double source_wants = 0.0;
    if (pipe->source_type == PUMP) {
        if (sys->dispatcher.on_pumps[pipe->source_id]) {
            source_wants = sys->pumps[pipe->source_id].capacity;
        } else {
            source_wants = 0.0;
        }
    } else { // source_type == RESERVOIR
        District *dist = &sys->districts[pipe->destination_id];
        source_wants = dist->schedule[dist->current_point].consumption;
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

void water_physics(System *sys, int i) {
    Pipe *pipe = &sys->pipes[i];
    if (lock(SEM_MTX_RES))
        return;
    double flow = calculate_flow(pipe, sys);
    if (pipe->source_type == RESERVOIR) {
        sys->reservoirs[pipe->source_id].volume -= flow;
    }
    if (pipe->destination_type == RESERVOIR) {
        sys->reservoirs[pipe->destination_id].volume += flow;
    }
    unlock(SEM_MTX_RES);

    if (flow > 0.0) {
        log_event("[T=%03d] Pipe %d: %.1f l/sec from %s %d to %s %d\n",
                  sys->current_time, pipe->id, flow,
                  pipe->source_type == PUMP ? "Pump" : "Reservoir",
                  pipe->source_id,
                  pipe->destination_type == RESERVOIR ? "Reservoir"
                                                      : "District",
                  pipe->destination_id);
    }
}

void district_work(System *sys, int i) {
    District *dist = &sys->districts[i];
    for (int j = 0; j < dist->schedule_size; j++) {
        if (dist->schedule[j].time <= sys->current_time) {
            dist->current_point = j;
        }
    }
    log_event("[T=%03d] District %d consume %.1f l/sec\n", sys->current_time,
              dist->id, dist->schedule[dist->current_point].consumption);
}

void show_levels(System *sys) {
    for (int i = 0; i < sys->reservoir_number; i++) {
        log_event("[T=%03d] Reservoir %d: actual level %.1f l (min %.1f, max "
                  "%.1f)\n",
                  sys->current_time, i, sys->reservoirs[i].volume,
                  sys->reservoirs[i].min_volume,
                  sys->reservoirs[i].max_volume);
    }
}

void check_emergency(System *sys) {
    for (int i = 0; i < sys->reservoir_number; i++) {
        if (sys->reservoirs[i].volume > sys->reservoirs[i].max_volume) {
            log_event("[T=%03d] CRASH: Reservoir %d overflowed (%.1f > %.1f)\n",
                      sys->current_time, i, sys->reservoirs[i].volume,
                      sys->reservoirs[i].max_volume);
            sys->emergency = true;
        } else if (sys->reservoirs[i].volume <= 0.0) {
            log_event("[T=%03d] CRASH: Reservoir %d dried up (%.1f < %.1f)\n",
                      sys->current_time, i, sys->reservoirs[i].volume,
                      sys->reservoirs[i].min_volume);
            sys->emergency = true;
        } else if (sys->reservoirs[i].volume < sys->reservoirs[i].min_volume) {
            log_event("[T=%03d] ATTENTION: Level in reservoir %d is under "
                      "minimum value (%.1f < %.1f)\n",
                      sys->current_time, i, sys->reservoirs[i].volume,
                      sys->reservoirs[i].min_volume);
        }
    }

    double total_power = 0.0;
    for (int i = 0; i < sys->pumps_number; i++) {
        if (sys->dispatcher.on_pumps[i]) {
            total_power += sys->pumps[i].power;
        }
    }
    if (total_power > sys->dispatcher.max_power) {
        log_event("[T=%03d] CRASH: Power limit exceeded (%.1f > %d)\n",
                  sys->current_time, total_power, sys->dispatcher.max_power);
        sys->emergency = true;
    }
}

static int add_message(System *sys, Message msg) {
    if (sys->message_count >= sys->message_capacity) {
        return -1;
    }
    sys->messages[sys->message_count] = msg;
    sys->message_count++;
    return 0;
}

static void remove_message_at(System *sys, int idx) {
    for (int i = idx; i < sys->message_count - 1; ++i) {
        sys->messages[i] = sys->messages[i + 1];
    }
    sys->message_count--;
}

void sensors_work(System *sys, int i) {
    Sensor *sensor = &sys->sensors[i];
    if (sys->current_time >= sensor->next_time) {
        if (lock(SEM_MTX_RES))
            return;
        double measured_volume = sys->reservoirs[sensor->reservoir_id].volume;
        unlock(SEM_MTX_RES);

        Message msg;
        msg.current_time = sys->current_time;
        msg.reservoir_id = sensor->reservoir_id;
        msg.volume = measured_volume;
        msg.delivery_time = sys->current_time;
        if (lock(SEM_MTX_MSG))
            return;
        msg.id = sys->global_messages_counter++;
        int rc = add_message(sys, msg);
        unlock(SEM_MTX_MSG);

        if (rc == 0) {
            log_event("[T=%03d] Sensor %d: reservoir %d, level "
                      "%.1f l (message ID=%d)\n",
                      sys->current_time, sensor->id, sensor->reservoir_id,
                      measured_volume, msg.id);
        } else {
            log_event("[T=%03d] Sensor %d: message queue is full, reading "
                      "dropped\n",
                      sys->current_time, sensor->id);
        }

        sensor->next_time += sensor->measurement_frequency;
    }
}

void network_work(System *sys) {
    if (lock(SEM_MTX_MSG))
        return;
    int initial_count = sys->message_count;

    for (int i = 0; i < initial_count; i++) {
        Message *msg = &sys->messages[i];

        if (rand() % 100 < sys->loss_probability) {
            log_event("[T=%03d] Message ID=%d is LOST\n", sys->current_time,
                      msg->id);
            remove_message_at(sys, i);
            i--;
            initial_count--;
            continue;
        }

        if (msg->delivery_time == msg->current_time) {
            if (rand() % 100 < sys->delay_probability) {
                int delay = 1 + rand() % (sys->max_delay);
                msg->delivery_time = sys->current_time + delay;
                log_event(
                    "[T=%03d] Message ID=%d delayed for %d sec (will arrive in "
                    "T=%03d)\n",
                    sys->current_time, msg->id, delay, msg->delivery_time);
            }
        }

        if (rand() % 100 < sys->dublicate_probability) {
            Message dublicate;
            dublicate.id = msg->id;
            dublicate.current_time = msg->current_time;
            dublicate.reservoir_id = msg->reservoir_id;
            dublicate.volume = msg->volume;
            dublicate.delivery_time =
                sys->current_time + (rand() % (sys->max_delay + 1));

            if (add_message(sys, dublicate) == 0) {
                log_event("[T=%03d] Duplicate of message ID=%d was created "
                          "(will arrive in T=%03d)\n",
                          sys->current_time, msg->id, dublicate.delivery_time);
            }
        }
    }
    unlock(SEM_MTX_MSG);
}

static bool is_duplicate(Dispatcher *dispatcher, int msg_id) {
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (dispatcher->last_messages_ids.last_ids[i] == msg_id) {
            return true;
        }
    }
    return false;
}

static void add_to_recent_ids(Dispatcher *dispatcher, int msg_id) {
    dispatcher->last_messages_ids
        .last_ids[dispatcher->last_messages_ids.current_idx] = msg_id;
    dispatcher->last_messages_ids.current_idx =
        (dispatcher->last_messages_ids.current_idx + 1) % BUFFER_SIZE;
}

void dispatcher_work(System *sys) {
    if (lock(SEM_MTX_MSG))
        return;
    for (int i = 0; i < sys->message_count; i++) {
        Message *msg = &sys->messages[i];
        if (msg->delivery_time <= sys->current_time) {
            if (is_duplicate(&sys->dispatcher, msg->id)) {
                log_event("[T=%03d] Duplicate of message ID=%d ignored\n",
                          sys->current_time, msg->id);
            } else {
                sys->dispatcher.reservoirs_last_volume[msg->reservoir_id] =
                    msg->volume;
                add_to_recent_ids(&sys->dispatcher, msg->id);
                log_event(
                    "[T=%03d] Dispatcher received the message ID=%d (reservoir "
                    "%d, level %.1f l)\n",
                    sys->current_time, msg->id, msg->reservoir_id, msg->volume);
            }
            remove_message_at(sys, i);
            i--;
        }
    }
    unlock(SEM_MTX_MSG);
    if (sys->dispatcher.strategy != NULL) {
        sys->dispatcher.strategy(&sys->dispatcher, sys);
    }
}
