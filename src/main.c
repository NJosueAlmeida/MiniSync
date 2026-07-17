#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <signal.h>

// Cabeceras del proyecto
#include "comunes.h"
#include "escaner.h"
#include "ipc.h"
#include "registro.h"
#include "demonio.h"

// Variables globales para el manejo de limpieza en las señales
struct stats *global_stats = NULL;
sem_t *global_semaforo = NULL;
pid_t pid_logger = -1;

// Manejador de señales para finalizar el demonio de manera limpia
void manejador_apagado(int sig) {
    (void)sig; // Evitar advertencia de variable sin usar
    
    // Enviar señal de apagado al Logger por la FIFO
    enviar_mensaje_registro("QUIT");
    
    // Esperar a que el Logger termine
    if (pid_logger > 0) {
        waitpid(pid_logger, NULL, 0);
    }
    
    // Limpiar semáforos y memoria compartida del sistema operativo
    limpiar_ipc(global_stats, global_semaforo);
    
    exit(0);
}

int main() {
    // Definir la ruta absoluta de tu proyecto (WSL monta tu disco C en /mnt/c/)
    const char *RUTA_PROYECTO = "/mnt/c/Users/LENOVO/Desktop/MiniSync";

    // Configurar las señales de apagado
    struct sigaction sa;
    sa.sa_handler = manejador_apagado;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Inicializar la FIFO de registro para comunicación con el Logger
    if (inicializar_registro_fifo() < 0) {
        fprintf(stderr, "Error al inicializar la FIFO de registro.\n");
        return EXIT_FAILURE;
    }

    // Proceso Logger: Se bifurca para crear un proceso hijo que ejecutará el registrador
    pid_logger = fork();
    if (pid_logger < 0) {
        perror("Error al bifurcar para el Logger");
        return EXIT_FAILURE;
    }

    if (pid_logger == 0) {
        // Proceso Hijo: Ejecuta el registrador
        ejecutar_proceso_logger();
        exit(0);
    }

    // El proceso padre continúa como el demonio monitor
    
    // Inicializar los recursos IPC 
    global_stats = inicializar_ipc(&global_semaforo);
    if (global_stats == NULL || global_semaforo == NULL) {
        fprintf(stderr, "Error al inicializar recursos IPC.\n");
        return EXIT_FAILURE;
    }

    printf("[MONITOR] Iniciando miniSync. Convirtiéndose en demonio de segundo plano...\n");
    sleep(1); // Pequeña pausa para asegurar la inicialización visual

    // Activamos la funcion para desasociar el proceso y crear un demonio
    desasociar_y_crear_demonio();

    // --- Segundo plano---

    // Arreglo local para guardar el estado anterior de los archivos (Sincronización Incremental)
    MetadatosArchivo estado_anterior[MAX_ARCHIVOS];
    int cantidad_anterior = 0;
    memset(estado_anterior, 0, sizeof(estado_anterior));

    // Bucle infinito del demonio monitor
    while (1) {
        MetadatosArchivo estado_actual[MAX_ARCHIVOS];
        int cantidad_actual = 0;
        memset(estado_actual, 0, sizeof(estado_actual));

        // Construir ruta de origen absoluta para que el escáner no falle en /
        char ruta_origen_abs[MAX_RUTA];
        snprintf(ruta_origen_abs, sizeof(ruta_origen_abs), "%s/origen", RUTA_PROYECTO);

        // Escanear la carpeta origen recursivamente
        if (escanear_directorio_recursivo(ruta_origen_abs, estado_actual, &cantidad_actual) == 0) {
            
            // Comparar cada archivo actual con el estado guardado para ver si requiere backup
            for (int i = 0; i < cantidad_actual; i++) {
                int requiere_copia = 1; // Por defecto asumimos que es nuevo

                for (int j = 0; j < cantidad_anterior; j++) {
                    if (strcmp(estado_actual[i].ruta_archivo, estado_anterior[j].ruta_archivo) == 0) {
                        // El archivo ya existía. Comparar tamaño y fecha de modificación
                        if (estado_actual[i].tamano == estado_anterior[j].tamano &&
                            estado_actual[i].fecha_modificacion == estado_anterior[j].fecha_modificacion) {
                            requiere_copia = 0; // No cambió, no requiere copia incremental
                        }
                        break;
                    }
                }

                if (requiere_copia) {
                    // Lanzar un proceso Worker para copiar el archivo de manera independiente usando fork()
                    pid_t pid_worker = fork();
                    if (pid_worker == 0) {
                        // Código del proceso Worker
                        char ruta_destino[MAX_RUTA];
                        
                        // Ruta de destino a la carpeta llamada backup
                        char *puntero_origen = strstr(estado_actual[i].ruta_archivo, "origen");
                        if (puntero_origen != NULL) {
                            snprintf(ruta_destino, sizeof(ruta_destino), "%s/backup%s", RUTA_PROYECTO, puntero_origen + 6);
                        } else {
                            snprintf(ruta_destino, sizeof(ruta_destino), "%s/backup/%s", RUTA_PROYECTO, estado_actual[i].ruta_archivo);
                        }

                        // Asegurar que existan subcarpetas de destino si es una ruta recursiva
                        char aux_ruta[MAX_RUTA];
                        strncpy(aux_ruta, ruta_destino, MAX_RUTA);
                        char *ultimo_slash = strrchr(aux_ruta, '/');
                        if (ultimo_slash != NULL) {
                            *ultimo_slash = '\0';
                            // Comando rápido para asegurar directorios anidados en backup
                            char cmd_mkdir[MAX_RUTA + 16];
                            snprintf(cmd_mkdir, sizeof(cmd_mkdir), "mkdir -p \"%s\"", aux_ruta);
                            (void)system(cmd_mkdir);
                        }

                        // Ejecutar la copia física usando llamadas al sistema de bajo nivel
                        int resultado_copia = copiar_archivo(estado_actual[i].ruta_archivo, ruta_destino);

                        // Proteger la sección crítica de las estadísticas compartidas usando el semáforo
                        sem_wait(global_semaforo);
                        if (resultado_copia == 0) {
                            global_stats->archivos_copiados++;
                            global_stats->bytes_copiados += estado_actual[i].tamano;
                            
                            // Formatear y enviar mensaje al Logger
                            char msg[512];
                            snprintf(msg, sizeof(msg), "copiado %s (%ld bytes)", estado_actual[i].ruta_archivo, (long)estado_actual[i].tamano);
                            enviar_mensaje_registro(msg);
                        } else {
                            global_stats->errores++;
                            
                            char msg[512];
                            snprintf(msg, sizeof(msg), "ERROR al copiar %s", estado_actual[i].ruta_archivo);
                            enviar_mensaje_registro(msg);
                        }
                        sem_post(global_semaforo);

                        exit(0); // El Worker termina su tarea
                    } else if (pid_worker > 0) {
                        // El proceso Monitor continúa y limpia el Worker para evitar procesos zombies
                        waitpid(pid_worker, NULL, 0);
                    }
                }
            }

            // Actualizar nuestro registro local para el siguiente ciclo
            cantidad_anterior = cantidad_actual;
            memcpy(estado_anterior, estado_actual, sizeof(estado_actual));
        }

        // Dormir 5 segundos antes de volver a escanear
        sleep(5);
    }

    return 0;
}