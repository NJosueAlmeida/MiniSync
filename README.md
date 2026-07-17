
# Mini Sistema de Sincronizacion de Archivos

Estudiante: Nelson Josue Almeida Pozo
Resumen del Proyecto

Este desarrollo consiste en la implementacion de un servicio automatizado en segundo plano encargado de supervisar directorios jerarquicos en el entorno operativo Linux. El sistema monitoriza de forma incremental la carpeta origen, detecta modificaciones en los archivos regulares mediante la inspeccion de i-nodos y los replica en la carpeta backup utilizando una arquitectura multiproceso coordinada por tuberias anonimas, Named Pipes (FIFO) y memoria compartida protegida por semaforos POSIX.

Instrucciones de Uso Tecnico

Borron de binarios y objetos: make clean
Construccion del ejecutable: make
Lanzamiento del servicio daemon: ./miniSync /ruta/de/origen /ruta/de/backup
Nota: Reemplazar las rutas de ejemplo por los directorios absolutos o relativos especificos de su computadora.
Finalizacion limpia del servicio: killall miniSync

Organizacion del Repositorio

```text
MiniSync/
├── backup/               <-- Directorio de destino donde el File System replica los datos
├── build/                <-- Archivos objeto temporales de la compilacion
├── inc/                  <-- Archivos de cabecera (.h)
│   ├── comunes.h
│   ├── demonio.h
│   ├── escaner.h
│   ├── ipc.h
│   └── registro.h
├── origen/               <-- Directorio fuente monitorizado con sus subcarpetas y archivos
│   ├── codigo/
│   ├── documentos/
│   ├── imagenes/
│   ├── nota.txt
│   └── prueba.txt
├── src/                  <-- Archivos de codigo fuente (.c)
├── Makefile
├── miniSync              <-- Ejecutable binario principal del sistema
├── minisync_registro.log <-- Archivo fisico de la bitacora en tiempo de ejecucion
├── README.md
└── scan

```

Modelo de Datos y Estados

Definicion de la Estructura de Metadatos

Las propiedades de control de cada archivo inspeccionado se encapsulan en la siguiente estructura dentro de comunes.h:

```c
typedef struct { 
    char ruta[512]; 
    unsigned long inode; 
    long long tamano; 
    long permisos; 
    long long mtime; 
} MetadatosArchivo;

```

Estructura de Estadisticas Globales (Memoria Compartida)

Los datos acumulados de rendimiento residen en el siguiente registro:

```c
struct stats { 
    long archivos_copiados; 
    long bytes_copiados; 
    long errores; 
};

```

Procesos de Ejecucion del Sistema

Modulo Monitor Principal (Proceso Padre)

Actua como el orquestador principal del sistema corriendo en modo Daemon (demonio.c). Se despierta en intervalos fijos de 5 segundos para invocar el escaneo jerarquico y comparar el estado de los i-nodos en la memoria RAM (main.c). Al identificar alteraciones, bifurca procesos efimeros y recolecta de forma inmediata su estado con waitpid() para mitigar la acumulacion de procesos zombies.

Procesos Workers (Procesos Hijos)

Entidades de ejecucion creadas dinamicamente mediante fork() para procesar archivos modificados (escaner.c). Reciben ordenes de transferencia por medio de tuberias anonimas (pipe), ejecutan copias de bajo nivel sin bufer en bloques fijos de 4KB (open, read, write) y notifican eventos operacionales inyectando cadenas de texto hacia el componente de registro.

Proceso Logger (Bitacora Asincrona)

Proceso independiente que centraliza la escritura de eventos en el almacenamiento fisico (registro.c). Realiza lecturas bloqueantes sobre una tuberia con nombre creada con mkfifo(), formatea los mensajes con estampas de tiempo de la biblioteca time.h y los vuelca de forma secuencial en minisync_registro.log para evitar interferir con el flujo principal del Monitor.

Mecanismos de Control de Concurrencia

Memoria Compartida POSIX: Segmento comun creado con shm_open(), ftruncate() y mapeado con mmap() para el acceso directo y simultaneo de los procesos a la estructura stats (ipc.c).
Semaforos POSIX con Nombre: Mecanismo configurado con sem_open() inicializado en uno. Funciona como un candado de exclusion mutua (Mutex) encargado de aislar la region critica durante la actualizacion de los contadores globales.

Logica de Planificacion y Resultados

Parametros de Sincronizacion Incremental

La evaluacion dinamica ocurre cada 5 segundos mediante la funcion sleep(5), la cual remueve al proceso de la cola de ejecucion de la CPU reduciendo el consumo a valores cercanos al 0%. La transferencia fisica se realiza utilizando exclusivamente descriptores nativos de bajo nivel, por lo que la tasa de transferencia en megabytes por segundo (MB/sec) y archivos por segundo (archivos/sec) queda acotada unicamente por la velocidad fisica del hardware subyacente.

Modulo de Validacion (Evidencia de Ejecucion)

El archivo minisync_registro.log ubicado en la raiz del proyecto almacena el historial cronologico de las copias efectuadas por los Workers de forma asincrona. Sirve como evidencia empirica para comprobar la consistencia de las marcas de tiempo en formato [AAAA-MM-DD HH:MM:SS], verificar el tamano exacto transferido en bytes y validar que el escaneo jerarquico preserve la estructura recursiva al replicar directorios internos (como origen/codigo/, origen/documentos/ u origen/imagenes/) de manera correcta.

```