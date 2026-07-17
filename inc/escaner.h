#ifndef ESCANER_H
#define ESCANER_H

#include "comunes.h"

// Recorrido recursivo de carpetas
int escanear_directorio_recursivo(const char *ruta_base, MetadatosArchivo *lista_archivos, int *contador);

// Copiado de archivos de bajo nivel 
int copiar_archivo(const char *origen, const char *destino);

#endif 