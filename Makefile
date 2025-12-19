CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -g -Iinclude -MMD -MP

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))
DEP = $(OBJ:.o=.d)
TARGET = $(BIN_DIR)/main

TEST_SRC = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ = $(patsubst $(TEST_DIR)/%.c, $(OBJ_DIR)/%.o, $(TEST_SRC))
TEST_TARGET = $(BIN_DIR)/test_runner

all: $(TARGET)

$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(TEST_TARGET): $(OBJ) $(TEST_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(TEST_OBJ)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

-include $(DEP)

test:
	@if [ -n "$(TEST_SRC)" ]; then \
		$(MAKE) $(TEST_TARGET); \
		./$(TEST_TARGET); \
	else \
		echo "No test files found in $(TEST_DIR)"; \
	fi

clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

.PHONY: all clean test
