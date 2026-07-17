#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "ipc.h"


// Función para inicializar la memoria compartida y el semáforo
struct stats* inicializar_ipc(sem_t **semaforo) {
    int fd_shm;
    struct stats *shm_stats;

    // Crear o abrir el objeto de memoria compartida
    fd_shm = shm_open(MEMORIA_COMPARTIDA_RUTA, O_CREAT | O_RDWR, 0666);
    if (fd_shm == -1) {
        perror("Error en shm_open");
        return NULL;
    }

    // Define el tamaño de la memoria compartida
    if (ftruncate(fd_shm, sizeof(struct stats)) == -1) {
        perror("Error en ftruncate");
        close(fd_shm);
        return NULL;
    }

    // Mapear la memoria compartida en el espacio de direcciones del proceso
    shm_stats = (struct stats*) mmap(NULL, sizeof(struct stats), 
                                     PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if (shm_stats == MAP_FAILED) {
        perror("Error en mmap");
        close(fd_shm);
        return NULL;
    }

    // Inicializar los valores a cero si es la primera creación
    shm_stats->archivos_copiados = 0;
    shm_stats->bytes_copiados = 0;
    shm_stats->errores = 0;

    // Abrir el semaforo para sincronización
    *semaforo = sem_open(SEMAFORO_RUTA, O_CREAT, 0666, 1);
    if (*semaforo == SEM_FAILED) {
        perror("Error en sem_open");
        munmap(shm_stats, sizeof(struct stats));
        close(fd_shm);
        return NULL;
    }

    close(fd_shm); // El descriptor se puede cerrar después de mmap sin perder el mapeo
    return shm_stats;
}

void limpiar_ipc(struct stats *shm_stats, sem_t *semaforo) {
    // Desmapeo de la memoria compartida
    if (shm_stats != NULL) {
        munmap(shm_stats, sizeof(struct stats));
    }

    // Cerrar el semáforo
    if (semaforo != NULL) {
        sem_close(semaforo);
    }

    // Eliminar los objetos del sistema operativo
    shm_unlink(MEMORIA_COMPARTIDA_RUTA);
    sem_unlink(SEMAFORO_RUTA);
}