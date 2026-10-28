CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lrt

SRC = src/main.c src/expedition.c src/ipc_queue.c
TARGET = bison_rescue

TEST_SRC = tests/test_ipc.c src/ipc_queue.c
TEST_TARGET = test_ipc

MOCK_SRC = tests/mock_sender.c src/ipc_queue.c
MOCK_TARGET = mock_sender

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

$(TEST_TARGET): $(TEST_SRC)
	$(CC) $(CFLAGS) $(TEST_SRC) -o $(TEST_TARGET) $(LDFLAGS)

$(MOCK_TARGET): $(MOCK_SRC)
	$(CC) $(CFLAGS) $(MOCK_SRC) -o $(MOCK_TARGET) $(LDFLAGS)

test: $(TARGET) $(TEST_TARGET) $(MOCK_TARGET)
	@chmod +x tests/run_test.sh
	@./tests/run_test.sh

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(MOCK_TARGET)
