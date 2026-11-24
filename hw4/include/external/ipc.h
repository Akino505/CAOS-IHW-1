#ifndef IPC_H
#define IPC_H
#include "system.h"
#include "semaphore.h"
#include <signal.h>

extern System *g_sys;
extern volatile sig_atomic_t g_stop;

System *ipc_create(int named);
System *ipc_attach(int timeout_ms);
void ipc_detach(void); 
void ipc_destroy(void);
sem_t *S(int idx);
int sem_acquire(int idx, int timeout_ms);
int sem_wait_plain(int idx, int timeout_ms);
void sem_release(int idx);
void msleep(int ms);
#endif
