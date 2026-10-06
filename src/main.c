#include "simulation.h"
#include <stdio.h>

#define MAX_TIME 100

int main() {
    System sys;
    init_system(&sys);
    while (sys.current_time < MAX_TIME && !sys.emergency) {
        printf("=== НАЧАЛО ТИКА T=%d ===\n", sys.current_time);
        network_work(&sys);
        dispatcher_work(&sys);
        water_physics(&sys);
        sensors_work(&sys);
        check_emergency(&sys);
        // log(&system);
        sys.current_time++;
    }
}
