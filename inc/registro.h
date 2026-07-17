#ifndef REGISTRO_H
#define REGISTRO_H

// Función para inicializar la FIFO de registro
int inicializar_registro_fifo();

// Función que ejecuta el proceso independiente Logger 
void ejecutar_proceso_logger();

// Función auxiliar que los Workers usarán para enviar mensajes al Logger
void enviar_mensaje_registro(const char *mensaje);

#endif 