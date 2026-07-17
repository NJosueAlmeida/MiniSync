#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "demonio.h"

void desasociar_y_crear_demonio() {
    pid_t pid;

    /* Primer fork el cual contiene el proceso padre que se ejecuta en la consola y
     el proceso hijo que se convertirá en demonio */ 
    pid = fork();
    if (pid < 0) {
        perror("Error en el primer fork del demonio");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        // El proceso padre termina inmediatamente, permitiendo que el hijo continúe en segundo plano
        exit(EXIT_SUCCESS);
    }

    // Hacemos una nueva sesion para que el proceso demonio no tenga un terminal controladora
    if (setsid() < 0) {
        perror("Error al crear una nueva sesion (setsid)");
        exit(EXIT_FAILURE);
    }

    // Segundo fork que nos asegura que el proceso demonio no pueda adquirir un terminal controladora en el futuro
    pid = fork();
    if (pid < 0) {
        perror("Error en el segundo fork del demonio");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // cambiar la máscara de permisos del proceso demonio para que pueda crear archivos con cualquier permiso
    umask(0);

    // Cambiar el directorio de trabajo a la raíz para evitar bloquear un sistema de archivos
    if (chdir("/") < 0) {
        perror("Error al cambiar al directorio raiz");
        exit(EXIT_FAILURE);
    }

    // 6. Cerrar todos los descriptores de archivos heredados y redirigir stdin, stdout y stderr a /dev/null
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int dev_null = open("/dev/null", O_RDWR);
    if (dev_null != -1) {
        dup2(dev_null, STDIN_FILENO);
        dup2(dev_null, STDOUT_FILENO);
        dup2(dev_null, STDERR_FILENO);
    }
}