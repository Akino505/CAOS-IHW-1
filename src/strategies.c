#include "strategies.h"
#include <stdio.h>

void sequential_strategy(Dispatcher *disp, System *sys) {
    double current_power = 0.0;
    for (int i = 0; i < sys->pumps_number; i++) {
        if (disp->on_pumps[i]) {
            current_power += sys->pumps[i].power;
        }
    }

    for (int i = 0; i < sys->pumps_number; i++) {
        int target_reservoir = -1;
        for (int j = 0; j < sys->pipes_number; j++) {
            if (sys->pipes[j].source_type == PUMP &&
                sys->pipes[j].source_id == i) {
                target_reservoir = sys->pipes[j].destination_id;
                break;
            }
        }
        if (target_reservoir == -1)
            continue;
        double level = disp->reservoirs_last_volume[target_reservoir];
        double min_lvl = sys->reservoirs[target_reservoir].min_volume;
        double max_lvl = sys->reservoirs[target_reservoir].max_volume;

        if (level < min_lvl && !disp->on_pumps[i]) {
            if (current_power + sys->pumps[i].power <= disp->max_power) {
                disp->on_pumps[i] = true;
                current_power += sys->pumps[i].power;
                printf("[T=%03d] Стратегия: ВКЛЮЧЕН насос %d (резервуар %d: "
                       "%.1f < %.1f)\n",
                       sys->current_time, i, target_reservoir, level, min_lvl);
            }
        } else if (level > max_lvl && disp->on_pumps[i]) {
            disp->on_pumps[i] = false;
            current_power -= sys->pumps[i].power;
            printf("[T=%03d] Стратегия: ВЫКЛЮЧЕН насос %d (резервуар %d: %.1f "
                   "> %.1f)\n",
                   sys->current_time, i, target_reservoir, level, max_lvl);
        }
    }
}
void greedy_strategy(Dispatcher *disp, System *sys) {
    double current_power = 0.0;
    for (int i = 0; i < sys->pumps_number; i++) {
        if (disp->on_pumps[i]) {
            current_power += sys->pumps[i].power;
        }
    }

    typedef struct {
        int pump_id;
        int reservoir_id;
        double criticality;
    } PumpPriority;

    PumpPriority priorities[sys->pumps_number];
    int num_priorities = 0;

    for (int i = 0; i < sys->pumps_number; i++) {
        int target_reservoir = -1;
        for (int j = 0; j < sys->pipes_number; j++) {
            if (sys->pipes[j].source_type == PUMP &&
                sys->pipes[j].source_id == i) {
                target_reservoir = sys->pipes[j].destination_id;
                break;
            }
        }

        if (target_reservoir == -1)
            continue;

        double level = disp->reservoirs_last_volume[target_reservoir];
        double min_lvl = sys->reservoirs[target_reservoir].min_volume;
        double max_lvl = sys->reservoirs[target_reservoir].max_volume;

        double distance_to_min = level - min_lvl;
        double distance_to_max = max_lvl - level;
        double criticality = (distance_to_min < distance_to_max)
                                 ? distance_to_min
                                 : distance_to_max;

        priorities[num_priorities].pump_id = i;
        priorities[num_priorities].reservoir_id = target_reservoir;
        priorities[num_priorities].criticality = criticality;
        num_priorities++;
    }

    for (int i = 0; i < num_priorities - 1; i++) {
        for (int j = 0; j < num_priorities - i - 1; j++) {
            if (priorities[j].criticality > priorities[j + 1].criticality) {
                PumpPriority temp = priorities[j];
                priorities[j] = priorities[j + 1];
                priorities[j + 1] = temp;
            }
        }
    }

    for (int i = 0; i < num_priorities; i++) {
        int pump_id = priorities[i].pump_id;
        int res_id = priorities[i].reservoir_id;
        double crit = priorities[i].criticality;

        if (crit < 0) {
            if (!disp->on_pumps[pump_id]) {
                if (current_power + sys->pumps[pump_id].power <=
                    disp->max_power) {
                    disp->on_pumps[pump_id] = true;
                    current_power += sys->pumps[pump_id].power;
                    printf("[T=%03d] Стратегия: ВКЛЮЧЕН насос %d "
                           "(резервуар %d)\n",
                           sys->current_time, pump_id, res_id);
                } else {
                    printf("[T=%03d] Стратегия: Насос %d НЕ включен (не "
                           "хватает мощности, нужно %d Вт)\n",
                           sys->current_time, pump_id,
                           sys->pumps[pump_id].power);
                }
            }
        } else {
            if (disp->on_pumps[pump_id]) {
                disp->on_pumps[pump_id] = false;
                current_power -= sys->pumps[pump_id].power;
                printf("[T=%03d] Стратегия: ВЫКЛЮЧЕН насос %d "
                       "(резервуар %d)\n",
                       sys->current_time, pump_id, res_id);
            }
        }
    }
}
