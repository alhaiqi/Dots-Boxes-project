# Makefile - Dots and Boxes (CMPS 270, Sprint 1)
#
#   make            build ./dots_and_boxes (with debug symbols for GDB)
#   make run        build and run
#   make test       build and run the automated board tests
#   make valgrind   run the tests and a scripted game under Valgrind
#   make clean      remove build files

CC      ?= gcc
CFLAGS  = -std=gnu11 -Wall -Wextra -Wpedantic -g -O0 -Iinclude
LDFLAGS =

TARGET  = dots_and_boxes
TESTBIN = test_board
BUILD   = build

SRCS    = src/main.c src/game.c src/board.c src/input.c
OBJS    = $(SRCS:src/%.c=$(BUILD)/%.o)
LIBOBJS = $(BUILD)/board.o $(BUILD)/input.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/test_board.o: tests/test_board.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

$(TESTBIN): $(BUILD)/test_board.o $(LIBOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

test: $(TESTBIN)
	./$(TESTBIN)

valgrind: $(TARGET) $(TESTBIN)
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$(TESTBIN)
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 \
		./$(TARGET) < tests/full_game.txt > /dev/null

clean:
	rm -rf $(BUILD) $(TARGET) $(TESTBIN)

-include $(OBJS:.o=.d) $(BUILD)/test_board.d

.PHONY: all run test valgrind clean
