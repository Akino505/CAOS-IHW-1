#include "external/read_config.h"
#include "external/logger.h"
#include "strategies.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 256

static int read_line(int fd, char *buffer, int max_len) {
    int i = 0;
    char c;
    while (i < max_len - 1) {
        if (read(fd, &c, 1) <= 0) {
            if (i == 0)
                return -1; // EOF
            break;
        }
        if (c == '\n')
            break;
        buffer[i++] = c;
    }
    buffer[i] = '\0';
    return i;
}

static int skip_comments(int fd, char *line) {
    while (1) {
        long pos = lseek(fd, 0, SEEK_CUR);
        int len = read_line(fd, line, MAX_LINE_LENGTH);
        if (len < 0)
            return -1; // EOF
        if (len == 0 || line[0] == '#')
            continue;
        lseek(fd, pos, SEEK_SET);
        return 0;
    }
}

static int next_entry(int fd, char *line) {
    if (skip_comments(fd, line) != 0 ||
        read_line(fd, line, MAX_LINE_LENGTH) < 0) {
        fprintf(stderr, "ERROR: config: unexpected end of file\n");
        return -1;
    }
    return 0;
}

static int count_ok(int count, int max, const char *what) {
    if (count < 0 || count > max) {
        fprintf(stderr, "ERROR: config: %s count must be 0..%d\n", what, max);
        return 0;
    }
    return 1;
}

int read_config(const char *filename, System *sys) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("ERROR: Failed to open the configuration file");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    char keyword[64];
    sys->delay_probability = 30.0;
    sys->max_delay = 1;
    sys->dispatcher.strategy_type = STRATEGY_GREEDY;
    log_event("Reading the configuration file: %s\n", filename);
    while (skip_comments(fd, line) == 0) {
        read_line(fd, line, MAX_LINE_LENGTH);
        if (sscanf(line, "%63s", keyword) != 1)
            continue;

        // RESERVOIRS
        if (strcmp(keyword, "reservoirs") == 0) {
            int count;
            sscanf(line, "%*s %d", &count);
            if (!count_ok(count, MAX_RESERVOIRS, "reservoirs"))
                return -1;
            sys->reservoir_number = count;

            for (int i = 0; i < count; i++) {
                if (next_entry(fd, line) != 0)
                    return -1;
                int id;
                double min_v, max_v, volume;
                sscanf(line, "%*s %d %lf %lf %lf", &id, &min_v, &max_v,
                       &volume);
                sys->reservoirs[i].id = id;
                sys->reservoirs[i].min_volume = min_v;
                sys->reservoirs[i].max_volume = max_v;
                sys->reservoirs[i].volume = volume;
            }
            for (int i = 0; i < sys->reservoir_number; ++i) {
                sys->dispatcher.reservoirs_last_volume[i] =
                    sys->reservoirs[i].volume;
            }
        }
        // PUMPS
        else if (strcmp(keyword, "pumps") == 0) {
            int count;
            sscanf(line, "%*s %d", &count);
            if (!count_ok(count, MAX_PUMPS, "pumps"))
                return -1;
            sys->pumps_number = count;

            for (int i = 0; i < count; i++) {
                if (next_entry(fd, line) != 0)
                    return -1;

                int id;
                double capacity, power;
                sscanf(line, "%*s %d %lf %lf", &id, &capacity, &power);

                sys->pumps[i].id = id;
                sys->pumps[i].capacity = capacity;
                sys->pumps[i].power = power;
            }
        }

        // DISTRICTS
        else if (strcmp(keyword, "districts") == 0) {
            int count;
            sscanf(line, "%*s %d", &count);
            if (!count_ok(count, MAX_DISTRICTS, "districts"))
                return -1;
            sys->districts_number = count;
            for (int i = 0; i < count; i++) {
                if (next_entry(fd, line) != 0)
                    return -1;
                int id, schedule_size;
                sscanf(line, "%*s %d %d", &id, &schedule_size);
                if (schedule_size < 1 || schedule_size > MAX_SCHEDULE) {
                    fprintf(stderr,
                            "ERROR: config: district %d: schedule size must "
                            "be 1..%d\n",
                            id, MAX_SCHEDULE);
                    return -1;
                }
                sys->districts[i].id = id;
                sys->districts[i].schedule_size = schedule_size;
                char *ptr = line;
                ptr = strchr(ptr, ' ');
                ptr++;
                ptr = strchr(ptr, ' ');
                ptr++;
                ptr = strchr(ptr, ' ');
                ptr++;
                for (int j = 0; j < schedule_size; j++) {
                    int time;
                    double consumption;
                    sscanf(ptr, "%d %lf", &time, &consumption);
                    sys->districts[i].schedule[j].time = time;
                    sys->districts[i].schedule[j].consumption = consumption;
                    ptr = strchr(ptr, ' ');
                    ptr++;
                    ptr = strchr(ptr, ' ');
                    ptr++;
                }
                sys->districts[i].current_point = 0;
            }
        }

        // PIPES
        else if (strcmp(keyword, "pipes") == 0) {
            int count;
            sscanf(line, "%*s %d", &count);
            if (!count_ok(count, MAX_PIPES, "pipes"))
                return -1;
            sys->pipes_number = count;
            for (int i = 0; i < count; i++) {
                if (next_entry(fd, line) != 0)
                    return -1;
                int id, source_id, dest_id;
                char source_type_str[32], dest_type_str[32];
                double capacity;
                sscanf(line, "%*s %d %31s %d %31s %d %lf", &id, source_type_str,
                       &source_id, dest_type_str, &dest_id, &capacity);
                sys->pipes[i].id = id;
                sys->pipes[i].capacity = capacity;
                if (strcmp(source_type_str, "pump") == 0) {
                    sys->pipes[i].source_type = PUMP;
                } else if (strcmp(source_type_str, "reservoir") == 0) {
                    sys->pipes[i].source_type = RESERVOIR;
                }
                if (strcmp(dest_type_str, "reservoir") == 0) {
                    sys->pipes[i].destination_type = RESERVOIR;
                } else if (strcmp(dest_type_str, "district") == 0) {
                    sys->pipes[i].destination_type = DISTRICT;
                }
                sys->pipes[i].source_id = source_id;
                sys->pipes[i].destination_id = dest_id;
            }
        }

        // SENSORS
        else if (strcmp(keyword, "sensors") == 0) {
            int count;
            sscanf(line, "%*s %d", &count);
            if (!count_ok(count, MAX_SENSORS, "sensors"))
                return -1;
            sys->sensors_number = count;
            for (int i = 0; i < count; i++) {
                if (next_entry(fd, line) != 0)
                    return -1;
                int id, reservoir_id, freq;
                sscanf(line, "%*s %d %d %d", &id, &reservoir_id, &freq);
                sys->sensors[i].id = id;
                sys->sensors[i].reservoir_id = reservoir_id;
                sys->sensors[i].measurement_frequency = freq;
                sys->sensors[i].next_time = 0;
            }
        }

        // NETWORK: network <loss%> <max_delay> <dup%> [delay%]
        else if (strcmp(keyword, "network") == 0) {
            double loss_prob, dub_prob, delay_prob = -1;
            int max_delay;
            sscanf(line, "%*s %lf %d %lf %lf", &loss_prob, &max_delay,
                   &dub_prob, &delay_prob);
            sys->loss_probability = loss_prob;
            if (delay_prob >= 0)
                sys->delay_probability = delay_prob;
            sys->max_delay = max_delay < 1 ? 1 : max_delay;
            sys->dublicate_probability = dub_prob;
        }

        // DISPATCHER
        else if (strcmp(keyword, "dispatcher") == 0) {
            int max_power;
            char strategy[32];
            sscanf(line, "%*s %d %31s", &max_power, strategy);
            sys->dispatcher.max_power = max_power;
            if (strcmp(strategy, "greedy") == 0) {
                sys->dispatcher.strategy_type = STRATEGY_GREEDY;
                log_event("Strategy: Greedy\n");
            } else if (strcmp(strategy, "sequential") == 0) {
                sys->dispatcher.strategy_type = STRATEGY_SEQUENTIAL;
                log_event("Strategy: Sequential\n");
            } else {
                log_event("ATTENTION: Unknown strategy: %s. Using greedy by "
                       "default\n",
                       strategy);
                sys->dispatcher.strategy_type = STRATEGY_GREEDY;
            }
        }
    }

    close(fd);

    if (sys->reservoir_number == 0) {
        fprintf(stderr, "ERROR: config: no reservoirs defined\n");
        return -1;
    }
    for (int i = 0; i < sys->pipes_number; i++) {
        Pipe *p = &sys->pipes[i];
        int src_n = p->source_type == PUMP ? sys->pumps_number
                                           : sys->reservoir_number;
        int dst_n = p->destination_type == RESERVOIR ? sys->reservoir_number
                                                     : sys->districts_number;
        if (p->source_id < 0 || p->source_id >= src_n ||
            p->destination_id < 0 || p->destination_id >= dst_n) {
            fprintf(stderr, "ERROR: config: pipe %d refers to a non-existent "
                            "object\n", p->id);
            return -1;
        }
    }
    for (int i = 0; i < sys->sensors_number; i++) {
        if (sys->sensors[i].reservoir_id < 0 ||
            sys->sensors[i].reservoir_id >= sys->reservoir_number ||
            sys->sensors[i].measurement_frequency < 1) {
            fprintf(stderr, "ERROR: config: bad sensor %d\n",
                    sys->sensors[i].id);
            return -1;
        }
    }

    // MESSAGES
    sys->message_capacity = MAX_MESSAGES;
    sys->message_count = 0;
    sys->global_messages_counter = 0;

    // DISPATCHER INIT
    for (int i = 0; i < sys->pumps_number; i++) {
        sys->dispatcher.on_pumps[i] = false;
    }
    for (int i = 0; i < BUFFER_SIZE; i++) {
        sys->dispatcher.last_messages_ids.last_ids[i] = -1;
    }
    sys->dispatcher.last_messages_ids.current_idx = 0;
    sys->current_time = 0;
    sys->emergency = false;
    log_event("Configuration successfully loaded\n");
    return 0;
}
