# Makefile - Dots and Boxes (CMPS 241, Sprints 1, 2 and 3)
#
#   make            build ./dots_and_boxes (with debug symbols for GDB)
#   make run        build and run
#   make test       build and run the automated board and bot tests
#   make valgrind   run the tests and scripted games under Valgrind
#   make netgame    play a full scripted network game between two processes (127.0.0.1)
#   make helgrind   the same network game under Helgrind (checks the thread locking)
#   make clean      remove build files

CC      ?= gcc
CFLAGS  = -std=gnu11 -Wall -Wextra -Wpedantic -g -O0 -Iinclude -pthread
LDFLAGS = -pthread

TARGET  = dots_and_boxes
TESTS   = test_board test_bot
BUILD   = build

SRCS    = src/main.c src/game.c src/board.c src/input.c src/bot.c src/net.c
OBJS    = $(SRCS:src/%.c=$(BUILD)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/test_%.o: tests/test_%.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

test_board: $(BUILD)/test_board.o $(BUILD)/board.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_bot: $(BUILD)/test_bot.o $(BUILD)/board.o $(BUILD)/bot.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

test: $(TESTS)
	./test_board
	./test_bot

VG = valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1

valgrind: $(TARGET) $(TESTS)
	$(VG) ./test_board
	$(VG) ./test_bot --quick
	$(VG) ./$(TARGET) < tests/full_game.txt > /dev/null
	$(VG) ./$(TARGET) < tests/bot_game.txt > /dev/null
	TOOL="$(VG)" PORT=5598 sh tests/net_game.sh

netgame: $(TARGET)
	sh tests/net_game.sh

helgrind: $(TARGET)
	TOOL="valgrind --tool=helgrind --error-exitcode=1" sh tests/net_game.sh

clean:
	rm -rf $(BUILD) $(TARGET) $(TESTS)

-include $(OBJS:.o=.d) $(BUILD)/test_board.d $(BUILD)/test_bot.d

.PHONY: all run test valgrind netgame helgrind clean
