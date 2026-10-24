#include "external/read_config.h"
#include "strategies.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
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

int read_config(const char *filename, System *sys) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("ERROR: Failed to open the configuration file");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    char keyword[64];
    printf("Reading the configuration file: %s\n", filename);
    while (skip_comments(fd, line) == 0) {
        read_line(fd, line, MAX_LINE_LENGTH);
        if (sscanf(line, "%63s", keyword) != 1)
            continue;

        // RESERVOIRS
        if (strcmp(keyword, "reservoirs") == 0) {
            int count;
            sscanf(line, "%*s %d", &count);
            sys->reservoir_number = count;
            sys->reservoirs = malloc(count * sizeof(Reservoir));

            for (int i = 0; i < count; i++) {
                skip_comments(fd, line);
                read_line(fd, line, MAX_LINE_LENGTH);
                int id;
                double min_v, max_v, volume;
                sscanf(line, "%*s %d %lf %lf %lf", &id, &min_v, &max_v,
                       &volume);
                sys->reservoirs[i].id = id;
                sys->reservoirs[i].min_volume = min_v;
                sys->reservoirs[i].max_volume = max_v;
                sys->reservoirs[i].volume = volume;
            }
            sys->dispatcher.reservoirs_last_volume =
                malloc(sys->reservoir_number * sizeof(double));
            for (int i = 0; i < sys->reservoir_number; ++i) {
                sys->dispatcher.reservoirs_last_volume[i] =
                    sys->reservoirs[i].volume;
            }
        }
        // PUMPS
        else if (strcmp(keyword, "pumps") == 0) {
            int count;
            sscanf(line, "%*s %d", &count);
            sys->pumps_number = count;
            sys->pumps = malloc(count * sizeof(Pump));

            for (int i = 0; i < count; i++) {
                skip_comments(fd, line);
                read_line(fd, line, MAX_LINE_LENGTH);

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
            sys->districts_number = count;
            sys->districts = malloc(count * sizeof(District));
            for (int i = 0; i < count; i++) {
                skip_comments(fd, line);
                read_line(fd, line, MAX_LINE_LENGTH);
                int id, schedule_size;
                sscanf(line, "%*s %d %d", &id, &schedule_size);
                sys->districts[i].id = id;
                sys->districts[i].schedule_size = schedule_size;
                sys->districts[i].schedule =
                    malloc(schedule_size * sizeof(SchedulePoint));
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
                sys->districts[i].current_point =
                    &sys->districts[i].schedule[0];
            }
        }

        // PIPES
        else if (strcmp(keyword, "pipes") == 0) {
            int count;
            sscanf(line, "%*s %d", &count);
            sys->pipes_number = count;
            sys->pipes = malloc(count * sizeof(Pipe));
            for (int i = 0; i < count; i++) {
                skip_comments(fd, line);
                read_line(fd, line, MAX_LINE_LENGTH);
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
            sys->sensors_number = count;
            sys->sensors = malloc(count * sizeof(Sensor));
            for (int i = 0; i < count; i++) {
                skip_comments(fd, line);
                read_line(fd, line, MAX_LINE_LENGTH);
                int id, reservoir_id, freq;
                sscanf(line, "%*s %d %d %d", &id, &reservoir_id, &freq);
                sys->sensors[i].id = id;
                sys->sensors[i].reservoir_id = reservoir_id;
                sys->sensors[i].measurement_frequency = freq;
                sys->sensors[i].next_time = 0;
            }
        }

        // NETWORK
        else if (strcmp(keyword, "network") == 0) {
            double loss_prob, dub_prob;
            int max_delay;
            sscanf(line, "%*s %lf %d %lf", &loss_prob, &max_delay, &dub_prob);
            sys->loss_probability = loss_prob;
            sys->delay_probability = loss_prob;
            sys->max_delay = max_delay;
            sys->dublicate_probability = dub_prob;
        }

        // DISPATCHER
        else if (strcmp(keyword, "dispatcher") == 0) {
            int max_power;
            char strategy[32];
            sscanf(line, "%*s %d %31s", &max_power, strategy);
            sys->dispatcher.max_power = max_power;
            if (strcmp(strategy, "greedy") == 0) {
                sys->dispatcher.strategy = greedy_strategy;
                printf("Strategy: Greedy\n");
            } else if (strcmp(strategy, "sequential") == 0) {
                sys->dispatcher.strategy = sequential_strategy;
                printf("Strategy: Sequential\n");
            } else {
                printf("ATTENTION: Unknown strategy: %s. Using greedy by "
                       "default\n",
                       strategy);
                sys->dispatcher.strategy = greedy_strategy;
            }
        }
    }

    close(fd);

    // MESSAGES
    sys->message_capacity = 100;
    sys->messages = malloc(sys->message_capacity * sizeof(Message));
    sys->message_count = 0;
    sys->global_messages_counter = 0;

    // DISPATCHER INIT
    sys->dispatcher.on_pumps = malloc(sys->pumps_number * sizeof(bool));
    for (int i = 0; i < sys->pumps_number; i++) {
        sys->dispatcher.on_pumps[i] = false;
    }
    for (int i = 0; i < BUFFER_SIZE; i++) {
        sys->dispatcher.last_messages_ids.last_ids[i] = -1;
    }
    sys->dispatcher.last_messages_ids.current_idx = 0;
    sys->current_time = 0;
    sys->emergency = false;
    printf("Configuration successfully loaded\n");
    return 0;
}
