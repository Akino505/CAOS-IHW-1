#include "simulation.h"

#define MAX_TIME 100

int main() {
    System system;
    while (system.current_time < MAX_TIME && !system.emergency) {
        init_system(&system);
        water_physics(&system);
        check_emergency(&system);
        // log(&system);
        system.current_time++;
    }
}
