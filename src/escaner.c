#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "escaner.h"

// Recorrido recursivo del directorio
int escanear_directorio_recursivo(const char *ruta_base, MetadatosArchivo *lista_archivos, int *contador) {
    DIR *directorio;
    struct dirent *entrada;
    struct stat info_archivo;
    char nueva_ruta[MAX_RUTA];

    directorio = opendir(ruta_base);
    if (directorio == NULL) {
        return -1; 
    }

    while ((entrada = readdir(directorio)) != NULL) {
        // Ignorar los directorios de navegacion del padre y el actual
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue;
        }

        snprintf(nueva_ruta, sizeof(nueva_ruta), "%s/%s", ruta_base, entrada->d_name);

        // lstat para leer metadatos evitando seguir enlaces simbólicos infinitamente  
        if (lstat(nueva_ruta, &info_archivo) < 0) {
            continue; 
        }

        if (S_ISDIR(info_archivo.st_mode)) {
            // Recursividad en subcarpetas
            escanear_directorio_recursivo(nueva_ruta, lista_archivos, contador);
        } else if (S_ISREG(info_archivo.st_mode)) {
            // Guardar metadatos en el arreglo si hay espacio disponible
            if (*contador < MAX_ARCHIVOS) {
                MetadatosArchivo *meta = &lista_archivos[*contador];
                
                strncpy(meta->ruta_archivo, nueva_ruta, MAX_RUTA);
                meta->numero_inodo = info_archivo.st_ino;
                meta->tamano = info_archivo.st_size;
                meta->permisos = info_archivo.st_mode;
                meta->fecha_modificacion = info_archivo.st_mtime;

                (*contador)++;
            }
        }
    }

    closedir(directorio);
    return 0;
}

// Copiado manual de bajo nivel usando write y read para evitar problemas con archivos grandes
int copiar_archivo(const char *origen, const char *destino) {
    int descriptor_origen, descriptor_destino;
    ssize_t bytes_leidos, bytes_escritos;
    char buffer[4096]; // Búfer de transferencia de 4KB

    descriptor_origen = open(origen, O_RDONLY);
    if (descriptor_origen < 0) {
        return -1; 
    }

    // Usamos O_WRONLY | O_CREAT | O_TRUNC para crear o truncar el archivo de destino
    descriptor_destino = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (descriptor_destino < 0) {
        close(descriptor_origen);
        return -1; 
    }

    // Bucle de lectura y escritura
    while ((bytes_leidos = read(descriptor_origen, buffer, sizeof(buffer))) > 0) {
        bytes_escritos = write(descriptor_destino, buffer, bytes_leidos);
        if (bytes_escritos != bytes_leidos) {
            close(descriptor_origen);
            close(descriptor_destino);
            return -1; 
        }
    }

    close(descriptor_origen);
    close(descriptor_destino);
    return 0; 
}