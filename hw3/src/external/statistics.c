#include "external/statistics.h"
#include "external/ipc.h"
#include "external/logger.h"

void print_statistics(System *sys) {
    log_event("\n\n========== STATISTICS ==========\n");

    log_event("Total time: %d virtual seconds (Ticks)\n", sys->current_time);
    if (sys->emergency) {
        log_event("Reason for termination: АВАРИЯ\n");
    } else if (g_stop) {
        log_event("Reason for termination: прерывание пользователем\n");
    } else {
        log_event("Reason for termination: истекло время моделирования\n");
    }
    log_event("\n---------- Messages ----------\n");
    log_event("Total amount: %d\n", sys->global_messages_counter);
    log_event("Left in queue: %d\n", sys->message_count);

    log_event("\n---------- Final levels ----------\n");
    for (int i = 0; i < sys->reservoir_number; i++) {
        log_event("Reservoir %d: %.1f l (min: %.1f, max: %.1f)\n", i,
                  sys->reservoirs[i].volume, sys->reservoirs[i].min_volume,
                  sys->reservoirs[i].max_volume);
    }

    log_event("\n---------- Final pumps state ----------\n");
    for (int i = 0; i < sys->pumps_number; i++) {
        log_event("Pump %d: %s\n", i,
                  sys->dispatcher.on_pumps[i] ? "ON" : "OFF");
    }
    log_event("========================================\n");
}
