CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lrt

SRC = src/main.c src/expedition.c src/ipc_queue.c
TARGET = bison_rescue

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)