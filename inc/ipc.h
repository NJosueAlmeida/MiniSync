#ifndef IPC_H
#define IPC_H

#include <semaphore.h>
#include "comunes.h"

// Inicializar canales de comunicación con Memoria Compartida y Semáforo
struct stats* inicializar_ipc(sem_t **semaforo);

// Cerrar y desvincular recursos IPC creados
void limpiar_ipc(struct stats *shm_stats, sem_t *semaforo);

#endif 