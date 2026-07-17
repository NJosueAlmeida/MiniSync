#ifndef COMUNES_H
#define COMUNES_H

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

// Límites del sistema
#define MAX_RUTA 1024
#define MAX_ARCHIVOS 500

// Rutas para comunicación entre procesos (IPC)
#define MEMORIA_COMPARTIDA_RUTA "/minisync_estadisticas"
#define SEMAFORO_RUTA "/minisync_semaforo"
#define FIFO_REGISTRO_RUTA "/tmp/minisync_registro.fifo"

// Estructura de metadatos de archivos (Requerimiento de FS)
typedef struct {
    char ruta_archivo[MAX_RUTA];
    ino_t numero_inodo;
    off_t tamano;
    mode_t permisos;
    time_t fecha_modificacion;
} MetadatosArchivo;

// Estructura de estadísticas compartida requerida por el laboratorio
struct stats {
    long archivos_copiados;
    long bytes_copiados;
    long errores;
};

#endif // COMUNES_H