#include "external/clean.h"
#include <stdio.h>
#include <stdlib.h>

void clean_system(System *sys) {
    printf("Start project cleanup...\n");
    free(sys->reservoirs);
    free(sys->pumps);
    free(sys->pipes);
    free(sys->sensors);
    free(sys->messages);
    for (int i = 0; i < sys->districts_number; ++i) {

        free(sys->districts[i].schedule);
    }
    free(sys->districts);
    free(sys->dispatcher.on_pumps);
    free(sys->dispatcher.reservoirs_last_volume);
    printf("Finish project cleanup\n");
}
