CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g # -nostdlib (needs lseek without stdlib for this)
LDFLAGS = -lcoilt

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build
LIB_DIR = lib
INCLUDE_DIR = include

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%.o,$(TEST_SRCS))
HEADERS = $(wildcard $(INCLUDE_DIR)/coil/*.h)

LIB_STATIC = $(LIB_DIR)/libcoil.a
LIB_SHARED = $(LIB_DIR)/libcoil.so

TEST_BIN = $(BUILD_DIR)/coil_test

# Default installation directories
prefix = /usr/local
libdir = $(prefix)/lib
includedir = $(prefix)/include/coil

.PHONY: all clean test static shared install

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

# Install libraries and headers
install: static shared
	mkdir -p /usr/local/include/coil
	install -d $(libdir)
	install -d $(includedir)
	install -m 644 $(LIB_STATIC) $(libdir)
	install -m 755 $(LIB_SHARED) $(libdir)
	install -m 644 $(HEADERS) $(includedir)
	ldconfig

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR)