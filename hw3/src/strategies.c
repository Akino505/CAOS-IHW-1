#include "strategies.h"
#include "external/logger.h"

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
        double min_level = sys->reservoirs[target_reservoir].min_volume;
        double max_level = sys->reservoirs[target_reservoir].max_volume;
        double target_level = (min_level + max_level) / 1.5;
        if (level < min_level && !disp->on_pumps[i]) {
            if (current_power + sys->pumps[i].power <= disp->max_power) {
                disp->on_pumps[i] = true;
                current_power += sys->pumps[i].power;
                log_event("[T=%03d] Стратегия: ВКЛЮЧЕН насос %d (резервуар %d: "
                          "%.1f < %.1f)\n",
                          sys->current_time, i, target_reservoir, level,
                          min_level);
            }
        } else if (level >= target_level && disp->on_pumps[i]) {
            disp->on_pumps[i] = false;
            current_power -= sys->pumps[i].power;
            log_event("[T=%03d] Стратегия: ВЫКЛЮЧЕН насос %d (резервуар %d: "
                      "%.1f >= %.1f)\n",
                      sys->current_time, i, target_reservoir, level,
                      target_level);
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

    PumpPriority priorities[sys->pumps_number > 0 ? sys->pumps_number : 1];
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
        double min_level = sys->reservoirs[target_reservoir].min_volume;
        double max_level = sys->reservoirs[target_reservoir].max_volume;

        double distance_to_min = level - min_level;
        double distance_to_max = max_level - level;
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

        double level = disp->reservoirs_last_volume[res_id];
        double min_level = sys->reservoirs[res_id].min_volume;
        double max_level = sys->reservoirs[res_id].max_volume;

        if (crit < 0) {
            if (level < min_level) {
                if (!disp->on_pumps[pump_id]) {
                    if (current_power + sys->pumps[pump_id].power <=
                        disp->max_power) {
                        disp->on_pumps[pump_id] = true;
                        current_power += sys->pumps[pump_id].power;
                        log_event("[T=%03d] Стратегия: ВКЛЮЧЕН насос %d "
                                  "(резервуар %d)\n",
                                  sys->current_time, pump_id, res_id);
                    } else {
                        log_event("[T=%03d] Стратегия: Насос %d НЕ включен (не "
                                  "хватает мощности, нужно %.1f Вт)\n",
                                  sys->current_time, pump_id,
                                  sys->pumps[pump_id].power);
                    }
                }
            } else if (disp->on_pumps[pump_id]) {
                disp->on_pumps[pump_id] = false;
                current_power -= sys->pumps[pump_id].power;
                log_event("[T=%03d] Стратегия: ВЫКЛЮЧЕН насос %d "
                          "(резервуар %d: %.1f > %.1f)\n",
                          sys->current_time, pump_id, res_id, level,
                          max_level);
            }
        } else {
            double target_level = (min_level + max_level) / 1.5;
            if (disp->on_pumps[pump_id] && level >= target_level) {
                disp->on_pumps[pump_id] = false;
                current_power -= sys->pumps[pump_id].power;
                log_event("[T=%03d] Стратегия: ВЫКЛЮЧЕН насос %d "
                          "(резервуар %d: %.1f >= %.1f)\n",
                          sys->current_time, pump_id, res_id, level,
                          target_level);
            }
        }
    }
}
