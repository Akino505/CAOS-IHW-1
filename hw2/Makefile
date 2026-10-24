CC = gcc

CFLAGS = -Wall -Werror -Iinclude

SRC_DIR = src
INC_DIR = include
OBJ_DIR = build

TARGET = water_supply_system

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/simulation.c \
       $(SRC_DIR)/strategies.c \
       $(SRC_DIR)/external/statistics.c \
       $(SRC_DIR)/external/clean.c \
       $(SRC_DIR)/external/signal_handler.c \
	   $(SRC_DIR)/external/logger.c \
	   $(SRC_DIR)/external/read_config.c

OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf $(OBJ_DIR) $(TARGET)

rebuild: clean all

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean rebuild
