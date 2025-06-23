CC = gcc
CFLAGS = -Wall -g -std=c11 -Iinc -D_GNU_SOURCE
TARGET = peripheral

OBJ_DIR = obj
SOURCES = src/main.c src/SLOW_peripheral.c src/helper_functions.c
OBJECTS = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "LINK -> Criando o executavel '$@'"
	$(CC) -o $@ $^

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(OBJ_DIR)
	@echo "CC   -> Compilando '$<'"
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	@echo "RUN  -> Executando './$(TARGET)'"
	./$(TARGET)

clean:
	@echo "Limpando arquivos gerados..."
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean run
