CC      = gcc
# Dodano -O2 dla lepszej wydajności i -D_DEFAULT_SOURCE dla kompatybilności i2c
CFLAGS  = -Wall -Wextra -std=c11 -O2 -Iinclude -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
# Usunięto -lwiringPi, zostawiono -lm (dla math.h w control.c)
LDFLAGS = -lm

TARGET  = app
SRC_DIR = src
OBJ_DIR = build

# Automatycznie wykrywa wszystkie pliki .c w src/ (np. main.c, control.c, i2c.c, read_yolo.c)
SRC     = $(wildcard $(SRC_DIR)/*.c)
OBJ     = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run