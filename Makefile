CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lrt

SRC = src/main.c src/expedition.c src/ipc_queue.c
TARGET = bison_rescue

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
