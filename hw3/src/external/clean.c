#include "external/clean.h"
#include "external/ipc.h"
#include "external/logger.h"

void clean_system(System *sys) {
    (void)sys;
    log_event("Start project cleanup...\n");
    ipc_destroy();
    log_event("Finish project cleanup\n");
}