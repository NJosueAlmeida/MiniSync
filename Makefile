CC = gcc
CFLAGS = -Wall -Wextra -Iinc -D_GNU_SOURCE
LIBS = -lrt -lpthread

DIR_FUENTE = src
DIR_OBJETO = build

OBJETOS_BASE = $(DIR_OBJETO)/comunes.o $(DIR_OBJETO)/demonio.o $(DIR_OBJETO)/escaner.o $(DIR_OBJETO)/ipc.o $(DIR_OBJETO)/registro.o

all: miniSync scan

miniSync: $(OBJETOS_BASE) $(DIR_OBJETO)/main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

scan: $(DIR_OBJETO)/escaner.o $(DIR_OBJETO)/comunes.o $(DIR_OBJETO)/scan_main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(DIR_OBJETO)/%.o: $(DIR_FUENTE)/%.c
	@mkdir -p $(DIR_OBJETO)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(DIR_OBJETO) miniSync scan

.PHONY: all clean