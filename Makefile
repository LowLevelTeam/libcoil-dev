CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
LDFLAGS = -lm

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build
LIB_DIR = lib

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%.o,$(TEST_SRCS))

LIB_STATIC = $(LIB_DIR)/libcoil.a
LIB_SHARED = $(LIB_DIR)/libcoil.so

TEST_BIN = $(BUILD_DIR)/coil_test

.PHONY: all clean test static shared

all: static shared test

# Create build directories
$(BUILD_DIR) $(LIB_DIR):
	mkdir -p $@

# Compile library source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

# Compile test source files
$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create static library
$(LIB_STATIC): $(OBJS) | $(LIB_DIR)
	ar rcs $@ $^

# Create shared library
$(LIB_SHARED): $(OBJS) | $(LIB_DIR)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

# Build test program
$(TEST_BIN): $(TEST_OBJS) $(LIB_STATIC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build targets
static: $(LIB_STATIC)
shared: $(LIB_SHARED)
test: $(TEST_BIN)

# Run the test program
run-test: test
	./$(TEST_BIN)

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR)