#include "external/clean.h"
#include "external/logger.h"
#include "external/read_config.h"
#include "external/signal_handler.h"
#include "external/statistics.h"
#include "simulation.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_TIME 100

int main(int argc, char *argv[]) {
    printf("START: starting the simulation\n");
    int delay = 500;
    System sys = {0};
    const char *config_file = argv[1];
    if (argc > 2) {
        delay = atoi(argv[2]);
    }
    if (read_config(config_file, &sys) != 0) {
        printf("ERROR: Failed to read configuration\n");
        return 1;
    }
    init_signal(&sys);
    logger_init("log.txt");
    while (sys.current_time < MAX_TIME && !sys.emergency) {
        network_work(&sys);
        dispatcher_work(&sys);
        water_physics(&sys);
        sensors_work(&sys);
        check_emergency(&sys);
        sys.current_time++;
        usleep(delay * 1000);
    }
    printf("FINISH: Finish simulation\n");
    print_statistics(&sys);
    clean_system(&sys);
    return 0;
}
