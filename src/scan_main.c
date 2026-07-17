#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "comunes.h"
#include "escaner.h"

int main(int argc, char *argv[]) {
    // Validar argumentos
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <directorio>\n", argv[0]);
        return EXIT_FAILURE;
    }

    MetadatosArchivo lista[MAX_ARCHIVOS];
    int contador = 0;

    printf("Escaneando el directorio: %s\n", argv[1]);
    printf("--------------------------------------------------\n");

    // Ejecutar el escáner recursivo
    if (escanear_directorio_recursivo(argv[1], lista, &contador) < 0) {
        perror("Error al escanear el directorio");
        return EXIT_FAILURE;
    }

    // Imprimir los metadatos obtenidos de cada archivo
    for (int i = 0; i < contador; i++) {
        printf("Archivo: %s\n", lista[i].ruta_archivo);
        printf("  - Inodo: %ld\n", (long)lista[i].numero_inodo);
        printf("  - Tamaño: %ld bytes\n", (long)lista[i].tamano);
        printf("  - Permisos: %o\n", lista[i].permisos & 0777);
        printf("  - Modificado: %s", ctime(&lista[i].fecha_modificacion));
        printf("--------------------------------------------------\n");
    }

    printf("Escaneo finalizado. Total de archivos encontrados: %d\n", contador);
    return EXIT_SUCCESS;
}