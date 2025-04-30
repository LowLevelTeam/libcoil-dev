# Makefile for libcoil-dev
# Author: Low Level Team
# Description: Build system for COIL Development Library

# Compiler settings
CC := gcc
AR := ar
CFLAGS := -Wall -Wextra -pedantic -fPIC -O2
LDFLAGS := -shared

# Debug build settings
ifdef DEBUG
  CFLAGS += -g -DDEBUG
  OPTFLAGS := -O0
else
  OPTFLAGS := -O2
endif

# Directories
SRCDIR := src
INCDIR := include
OBJDIR := obj
LIBDIR := lib
TESTDIR := tests
BINDIR := bin

# Library name and version
LIBNAME := coil
LIBVER_MAJOR := 0
LIBVER_MINOR := 1
LIBVER_PATCH := 0
LIBVER := $(LIBVER_MAJOR).$(LIBVER_MINOR).$(LIBVER_PATCH)

# Files and targets
SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
STATIC_LIB := $(LIBDIR)/lib$(LIBNAME).a
SHARED_LIB := $(LIBDIR)/lib$(LIBNAME).so.$(LIBVER)
SHARED_LINK := $(LIBDIR)/lib$(LIBNAME).so
TEST_SRCS := $(wildcard $(TESTDIR)/*.c)
TEST_OBJS := $(patsubst $(TESTDIR)/%.c,$(OBJDIR)/test_%.o,$(TEST_SRCS))
TEST_BIN := $(BINDIR)/test_coil

# Installation directories
PREFIX ?= /usr/local
INSTALL_INC_DIR := $(PREFIX)/include/$(LIBNAME)
INSTALL_LIB_DIR := $(PREFIX)/lib

# Default target
all: dirs $(STATIC_LIB) $(SHARED_LIB)

# Create necessary directories
dirs:
	@mkdir -p $(OBJDIR) $(LIBDIR) $(BINDIR)

# Compile source files to object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(OPTFLAGS) -I$(INCDIR) -c $< -o $@

# Create static library
$(STATIC_LIB): $(OBJS)
	@echo "Creating static library $@..."
	@$(AR) rcs $@ $^

# Create shared library
$(SHARED_LIB): $(OBJS)
	@echo "Creating shared library $@..."
	@$(CC) $(LDFLAGS) -Wl,-soname,lib$(LIBNAME).so.$(LIBVER_MAJOR) -o $@ $^ -lc
	@ln -sf lib$(LIBNAME).so.$(LIBVER) $(SHARED_LINK).$(LIBVER_MAJOR)
	@ln -sf lib$(LIBNAME).so.$(LIBVER_MAJOR) $(SHARED_LINK)

# Compile test object files
$(OBJDIR)/test_%.o: $(TESTDIR)/%.c
	@echo "Compiling test object $<..."
	@mkdir -p $(OBJDIR)
	@$(CC) $(CFLAGS) $(OPTFLAGS) -I$(INCDIR) -c $< -o $@

# Link test program
$(TEST_BIN): $(TEST_OBJS) $(STATIC_LIB)
	@echo "Linking test program $@..."
	@mkdir -p $(BINDIR)
	@$(CC) $(CFLAGS) $(OPTFLAGS) -o $@ $(TEST_OBJS) $(STATIC_LIB)

# Compile tests
tests: $(TEST_BIN)

# Run tests
check: tests
	@echo "Running tests..."
	@echo "Running $(TEST_BIN)..."
	@$(TEST_BIN) || exit 1
	@echo "All tests passed!"

# Install the library and headers
install: all
	@echo "Installing headers to $(INSTALL_INC_DIR)..."
	@mkdir -p $(INSTALL_INC_DIR)
	@cp -r $(INCDIR)/coil $(INSTALL_INC_DIR)
	@cp $(INCDIR)/coil.h $(INSTALL_INC_DIR)
	
	@echo "Installing libraries to $(INSTALL_LIB_DIR)..."
	@mkdir -p $(INSTALL_LIB_DIR)
	@cp $(STATIC_LIB) $(INSTALL_LIB_DIR)
	@cp $(SHARED_LIB) $(INSTALL_LIB_DIR)
	@ln -sf lib$(LIBNAME).so.$(LIBVER) $(INSTALL_LIB_DIR)/lib$(LIBNAME).so.$(LIBVER_MAJOR)
	@ln -sf lib$(LIBNAME).so.$(LIBVER_MAJOR) $(INSTALL_LIB_DIR)/lib$(LIBNAME).so

# Clean up
clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJDIR) $(LIBDIR) $(BINDIR)

# Make sure we rebuild everything when the headers change
$(OBJS): $(wildcard $(INCDIR)/*.h) $(wildcard $(INCDIR)/coil/*.h)

.PHONY: all dirs tests check install clean