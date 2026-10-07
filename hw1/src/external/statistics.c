#include "external/statistics.h"
#include <stdio.h>

void print_statistics(System *sys) {
    printf("\n\n========== STATISTICS ==========\n");

    printf("Total time: %d virtual seconds (Ticks)\n", sys->current_time);
    if(sys->emergency){
        printf("Reason for termination: АВАРИЯ\n");
    }
    printf("\n---------- Messages ----------\n");
    printf("Total amount: %d\n", sys->global_messages_counter);
    printf("Left in queue: %d\n", sys->message_count);

    printf("\n---------- Final levels ----------\n");
    for (int i = 0; i < sys->reservoir_number; i++) {
        printf("Reservoir %d: %.1f l (min: %.1f, max: %.1f)\n", i,
               sys->reservoirs[i].volume, sys->reservoirs[i].min_volume,
               sys->reservoirs[i].max_volume);
    }

    printf("\n---------- Final pumps state ----------\n");
    for (int i = 0; i < sys->pumps_number; i++) {
        printf("Pump %d: %s\n", i,
               sys->dispatcher.on_pumps[i] ? "ON" : "OFF");
    }
    printf("========================================\n");
}
