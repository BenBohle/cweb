# HttpAPI Library Makefile

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -fPIC -O2 -g
LDFLAGS = -shared
INCLUDES = -Iinclude
LIBS = -lcurl -levent -lcjson

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj
LIBDIR = lib
TESTDIR = tests

# Library name
LIBNAME = libfetch
STATIC_LIB = $(LIBDIR)/$(LIBNAME).a
SHARED_LIB = $(LIBDIR)/$(LIBNAME).so

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Default target
all: directories $(STATIC_LIB) $(SHARED_LIB)

# Create directories
directories:
	@mkdir -p $(OBJDIR) $(LIBDIR)

# Compile object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | directories
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build static library
$(STATIC_LIB): $(OBJECTS)
	ar rcs $@ $^

# Build shared library
$(SHARED_LIB): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Install library
install: all
	sudo cp $(INCDIR)/fetch.h /usr/local/include/
	sudo cp $(STATIC_LIB) /usr/local/lib/
	sudo cp $(SHARED_LIB) /usr/local/lib/
	sudo ldconfig

# Uninstall library
uninstall:
	sudo rm -f /usr/local/include/fetch.h
	sudo rm -f /usr/local/lib/$(LIBNAME).*
	sudo ldconfig

# Clean build files
clean:
	rm -rf $(OBJDIR) $(LIBDIR)

# Clean everything
distclean: clean
	rm -f $(SRCDIR)/*~ $(INCDIR)/*~
# Development build with debug symbols
debug: CFLAGS += -DDEBUG -g3 -O0
debug: all

# Release build with optimizations
release: CFLAGS += -DNDEBUG -O3 -flto
release: LDFLAGS += -flto
release: all

# Format code (requires clang-format)
format:
	find $(SRCDIR) $(INCDIR) -name "*.c" -o -name "*.h" | xargs clang-format -i

# Generate documentation (requires doxygen)
docs:
	doxygen Doxyfile

# Show help
help:
	@echo "Available targets:"
	@echo "  all       - Build static and shared libraries"
	@echo "  install   - Install library system-wide"
	@echo "  uninstall - Remove library from system"
	@echo "  clean     - Remove build files"
	@echo "  distclean - Remove all generated files"
	@echo "  debug     - Build with debug symbols"
	@echo "  release   - Build optimized release version"
	@echo "  memcheck  - Run memory leak check"
	@echo "  format    - Format source code"
	@echo "  docs      - Generate documentation"
	@echo "  help      - Show this help message"

.PHONY: all directories install uninstall clean distclean test debug release memcheck format docs help
