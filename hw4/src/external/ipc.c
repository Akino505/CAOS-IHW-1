#include "external/ipc.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

System *g_sys = NULL;

static int g_named = 1;
static sem_t *named_sems[SEM_COUNT];

static void sem_name(int i, char *buf, size_t n) {
    snprintf(buf, n, SEM_PREFIX "%d", i);
}

static int sem_initial(int idx) {
    return (idx == SEM_MTX_MSG || idx == SEM_MTX_RES) ? 1 : 0;
}

sem_t *S(int idx) { return g_named ? named_sems[idx] : &g_sys->unnamed[idx]; }

void msleep(int ms) {
    struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

System *ipc_create(int named) {
    g_named = named;
    shm_unlink(SHM_NAME); /* убрать "хвосты" аварийно завершившегося запуска */
    int fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) {
        perror("ERROR: shm_open");
        return NULL;
    }
    if (ftruncate(fd, sizeof(System)) < 0) {
        perror("ERROR: ftruncate");
        close(fd);
        shm_unlink(SHM_NAME);
        return NULL;
    }
    void *p = mmap(NULL, sizeof(System), PROT_READ | PROT_WRITE, MAP_SHARED,
                   fd, 0);
    close(fd);
    if (p == MAP_FAILED) {
        perror("ERROR: mmap");
        shm_unlink(SHM_NAME);
        return NULL;
    }
    g_sys = p; /* новая shm заполнена нулями */

    for (int i = 0; i < SEM_COUNT; i++) {
        if (named) {
            char name[64];
            sem_name(i, name, sizeof(name));
            sem_unlink(name);
            named_sems[i] =
                sem_open(name, O_CREAT | O_EXCL, 0600, sem_initial(i));
            if (named_sems[i] == SEM_FAILED) {
                perror("ERROR: sem_open");
                return NULL;
            }
        } else if (sem_init(&g_sys->unnamed[i], 1, sem_initial(i)) < 0) {
            perror("ERROR: sem_init");
            return NULL;
        }
    }
    g_sys->clock_pid = getpid();
    return g_sys;
}

System *ipc_attach(int timeout_ms) {
    g_named = 1;
    int waited = 0;
    while (!g_stop) {
        int fd = shm_open(SHM_NAME, O_RDWR, 0600);
        if (fd >= 0) {
            struct stat st;
            if (fstat(fd, &st) == 0 && (size_t)st.st_size >= sizeof(System)) {
                void *p = mmap(NULL, sizeof(System), PROT_READ | PROT_WRITE,
                               MAP_SHARED, fd, 0);
                if (p != MAP_FAILED) {
                    System *c = p;
                    /* init_done и живой clock - иначе это "хвост" старого запуска */
                    if (c->init_done && kill(c->clock_pid, 0) == 0) {
                        close(fd);
                        g_sys = c;
                        for (int i = 0; i < SEM_COUNT; i++) {
                            char name[64];
                            sem_name(i, name, sizeof(name));
                            named_sems[i] = sem_open(name, 0);
                            if (named_sems[i] == SEM_FAILED) {
                                perror("ERROR: sem_open");
                                return NULL;
                            }
                        }
                        return g_sys;
                    }
                    munmap(p, sizeof(System));
                }
            }
            close(fd);
        }
        if (waited >= timeout_ms) {
            fprintf(stderr, "ERROR: clock process was not found "
                            "(start water_clock first)\n");
            return NULL;
        }
        msleep(200);
        waited += 200;
    }
    return NULL;
}

void ipc_detach(void) {
    if (!g_sys)
        return;
    if (g_named)
        for (int i = 0; i < SEM_COUNT; i++)
            if (named_sems[i] && named_sems[i] != SEM_FAILED)
                sem_close(named_sems[i]);
    munmap(g_sys, sizeof(System));
    g_sys = NULL;
}

void ipc_destroy(void) {
    if (!g_sys)
        return;
    for (int i = 0; i < SEM_COUNT; i++) {
        if (g_named) {
            char name[64];
            sem_name(i, name, sizeof(name));
            if (named_sems[i] && named_sems[i] != SEM_FAILED)
                sem_close(named_sems[i]);
            sem_unlink(name);
        } else {
            sem_destroy(&g_sys->unnamed[i]);
        }
    }
    munmap(g_sys, sizeof(System));
    g_sys = NULL;
    shm_unlink(SHM_NAME);
}

/* Ожидание семафора с таймаутом (шаг опроса 100 мс).
 * 0 - получен, 1 - таймаут, -1 - запрошена остановка / ошибка.
 * Опрос нужен, чтобы заметить stop и гибель clock-процесса. */
static int acquire_ex(int idx, int timeout_ms, int honor_stop) {
    int waited = 0;
    for (;;) {
        if (honor_stop && (g_stop || g_sys->stop))
            return -1;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 100000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000L;
        }
        if (sem_timedwait(S(idx), &ts) == 0)
            return 0;
        if (errno == EINTR)
            continue;
        if (errno != ETIMEDOUT)
            return -1;
        waited += 100;
        if (timeout_ms > 0 && waited >= timeout_ms)
            return 1;
        if (g_sys->clock_pid != getpid() && kill(g_sys->clock_pid, 0) == -1 &&
            errno == ESRCH) {
            g_sys->stop = 1; /* clock погиб - завершаемся сами */
            return -1;
        }
    }
}

int sem_acquire(int idx, int timeout_ms) {
    return acquire_ex(idx, timeout_ms, 1);
}
int sem_wait_plain(int idx, int timeout_ms) {
    return acquire_ex(idx, timeout_ms, 0);
}
void sem_release(int idx) { sem_post(S(idx)); }
