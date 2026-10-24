#include "external/clean.h"
#include "external/logger.h"
#include "external/read_config.h"
#include "external/signal_handler.h"
#include "external/statistics.h"
#include "simulation.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TOTAL_WORKERS 4
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

    sys.stop_flag = 0;
    sys.workers_done = 0;
    sys.tick_ready = false;
    pthread_mutex_init(&sys.data_mutex, NULL);
    pthread_mutex_init(&sys.log_mutex, NULL);
    pthread_mutex_init(&sys.tick_mutex, NULL);
    pthread_cond_init(&sys.tick_start_cond, NULL);
    pthread_cond_init(&sys.tick_done_cond, NULL);

    init_signal(&sys);
    logger_init("log.txt");

    pthread_t threads[TOTAL_WORKERS];
    pthread_create(&threads[0], NULL, physics_thread_func, &sys);
    pthread_create(&threads[1], NULL, dispatcher_thread_func, &sys);
    pthread_create(&threads[2], NULL, network_thread_func, &sys);
    pthread_create(&threads[3], NULL, sensors_thread_func, &sys);
    while (sys.current_time < MAX_TIME && !sys.emergency && !sys.stop_flag) {
        pthread_mutex_lock(&sys.tick_mutex);
        sys.current_time++;
        sys.workers_done = 0;
        sys.tick_ready = true;
        pthread_cond_broadcast(&sys.tick_start_cond);
        while (sys.workers_done < TOTAL_WORKERS) {
            pthread_cond_wait(&sys.tick_done_cond, &sys.tick_mutex);
        }
        sys.tick_ready = false;
        pthread_mutex_unlock(&sys.tick_mutex);
        usleep(delay * 1000);
    }
    sys.stop_flag = 1;
    pthread_mutex_lock(&sys.tick_mutex);
    sys.tick_ready = true;
    pthread_cond_broadcast(&sys.tick_start_cond);
    pthread_mutex_unlock(&sys.tick_mutex);
    for (int i = 0; i < TOTAL_WORKERS; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("FINISH: Finish the simulation.\n");
    print_statistics(&sys);
    pthread_mutex_destroy(&sys.data_mutex);
    pthread_mutex_destroy(&sys.log_mutex);
    pthread_mutex_destroy(&sys.tick_mutex);
    pthread_cond_destroy(&sys.tick_start_cond);
    pthread_cond_destroy(&sys.tick_done_cond);

    clean_system(&sys);
    return 0;
}
