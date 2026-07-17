#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "registro.h"
#include "comunes.h"

int inicializar_registro_fifo() {
    // Eliminar la FIFO si ya existía de ejecuciones previas
    unlink(FIFO_REGISTRO_RUTA);

    // Crear la FIFO con permisos de lectura/escritura (0666)
    if (mkfifo(FIFO_REGISTRO_RUTA, 0666) == -1) {
        perror("Error al crear la FIFO de registro");
        return -1;
    }
    return 0;
}

void ejecutar_proceso_logger() {
    int fd_fifo;
    char buffer[256];
    FILE *archivo_log;

    // Abrimos la FIFO en modo lectura para recibir mensajes de los Workers
    fd_fifo = open(FIFO_REGISTRO_RUTA, O_RDONLY);
    if (fd_fifo == -1) {
        perror("Logger: Error al abrir la FIFO en lectura");
        exit(1);
    }

    // Abrimos el archivo de log en modo append para registrar los mensajes
    archivo_log = fopen("/mnt/c/Users/LENOVO/Desktop/MiniSync/minisync_registro.log", "a");
    if (archivo_log == NULL) {
        perror("Logger: Error al abrir minisync_registro.log");
        close(fd_fifo);
        exit(1);
    }

    // El Logger lee continuamente los mensajes que mandan los Workers
    while (1) {
        ssize_t bytes_leidos = read(fd_fifo, buffer, sizeof(buffer) - 1);
        if (bytes_leidos > 0) {
            buffer[bytes_leidos] = '\0';

            // Si recibe la señal de apagado, rompe el bucle
            if (strcmp(buffer, "QUIT") == 0) {
                break;
            }

            // Obtener fecha y hora actual del sistema
            time_t tiempo_actual = time(NULL);
            struct tm *info_tiempo = localtime(&tiempo_actual);
            char marca_tiempo[64];
            strftime(marca_tiempo, sizeof(marca_tiempo), "%Y-%m-%d %H:%M:%S", info_tiempo);

            // Escribir en el archivo log y en la pantalla de depuración
            fprintf(archivo_log, "[%s] %s\n", marca_tiempo, buffer);
            fflush(archivo_log);
            printf("[LOGGER] [%s] %s\n", marca_tiempo, buffer);
        } else if (bytes_leidos == 0) {
            // No hay escritores activos en este momento, dormimos brevemente para no saturar la CPU
            usleep(100000);
        } else {
            break; // Error en la lectura
        }
    }

    fclose(archivo_log);
    close(fd_fifo);
    unlink(FIFO_REGISTRO_RUTA); // Limpiar la FIFO del sistema
    exit(0);
}

void enviar_mensaje_registro(const char *mensaje) {
    int fd_fifo = open(FIFO_REGISTRO_RUTA, O_WRONLY);
    if (fd_fifo != -1) {
        write(fd_fifo, mensaje, strlen(mensaje));
        close(fd_fifo);
    }
}