CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -D_GNU_SOURCE -fPIC
LDFLAGS = 
TARGET = findmax
LIBRARY = libfindmax.so.1.0.0
LIBRARY_SONAME = libfindmax.so.1
LIBRARY_LINK = libfindmax.so
SOURCES = main.c file_ops.c format.c
LIB_SOURCES = file_ops.c format.c
TEST_SOURCES = test_findmax.c file_ops.c format.c
OBJECTS = $(SOURCES:.c=.o)
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

# Installation directories
PREFIX ?= /usr/local
DESTDIR ?=
BINDIR = $(PREFIX)/bin
# Architecture-dependent library directory
MULTIARCH ?= $(shell dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || echo "x86_64-linux-gnu")
LIBDIR = $(PREFIX)/lib/$(MULTIARCH)
INCLUDEDIR = $(PREFIX)/include
MANDIR = $(PREFIX)/share/man/man1
COMPLETIONDIR = $(PREFIX)/share/bash-completion/completions

# Default target
all: $(TARGET) $(LIBRARY)

# Build test executable
test_findmax: $(TEST_OBJECTS)
	$(CC) $(TEST_OBJECTS) -o test_findmax $(LDFLAGS)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Build shared library
$(LIBRARY): $(LIB_OBJECTS)
	$(CC) -shared -Wl,-soname,$(LIBRARY_SONAME) -o $(LIBRARY) $(LIB_OBJECTS) $(LDFLAGS)
	ln -sf $(LIBRARY) $(LIBRARY_SONAME)
	ln -sf $(LIBRARY) $(LIBRARY_LINK)

# Build object files
%.o: %.c findmax.h
	$(CC) $(CFLAGS) -c $< -o $@

# Build optimized version with heap
optimized: SOURCES = heap.c file_ops.c format.c
optimized: CFLAGS += -DUSE_OPTIMIZED
optimized: $(TARGET)

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(LIB_OBJECTS) $(TEST_OBJECTS) $(TARGET) $(LIBRARY) $(LIBRARY_SONAME) $(LIBRARY_LINK) test_findmax

# Install to system with DESTDIR and PREFIX support
install: $(TARGET) $(LIBRARY) findmax.1 findmax-completion.bash
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(INCLUDEDIR)
	install -d $(DESTDIR)$(MANDIR)
	install -d $(DESTDIR)$(COMPLETIONDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/
	install -m 755 $(LIBRARY) $(DESTDIR)$(LIBDIR)/
	ln -sf $(LIBRARY) $(DESTDIR)$(LIBDIR)/$(LIBRARY_SONAME)
	ln -sf $(LIBRARY) $(DESTDIR)$(LIBDIR)/$(LIBRARY_LINK)
	install -m 644 findmax.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 findmax.1 $(DESTDIR)$(MANDIR)/
	install -m 644 findmax-completion.bash $(DESTDIR)$(COMPLETIONDIR)/findmax
	ldconfig -n $(DESTDIR)$(LIBDIR) 2>/dev/null || true

# Uninstall from system
uninstall:
	sudo rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	sudo rm -f $(DESTDIR)$(LIBDIR)/$(LIBRARY)
	sudo rm -f $(DESTDIR)$(LIBDIR)/$(LIBRARY_SONAME)
	sudo rm -f $(DESTDIR)$(LIBDIR)/$(LIBRARY_LINK)
	sudo rm -f $(DESTDIR)$(INCLUDEDIR)/findmax.h
	sudo rm -f $(DESTDIR)$(MANDIR)/findmax.1
	sudo rm -f $(DESTDIR)$(COMPLETIONDIR)/findmax
	ldconfig -n $(DESTDIR)$(LIBDIR) 2>/dev/null || true

# Install symlinks to development directory (hardcoded /usr with arch-dependent lib)
install-symlinks:
	sudo install -d /usr/bin
	sudo install -d /usr/lib/$(MULTIARCH)
	sudo install -d /usr/include
	sudo install -d /usr/share/man/man1
	sudo install -d /usr/share/bash-completion/completions
	sudo ln -sf $(PWD)/$(TARGET) /usr/bin/$(TARGET)
	sudo ln -sf $(PWD)/$(LIBRARY) /usr/lib/$(MULTIARCH)/$(LIBRARY)
	sudo ln -sf $(PWD)/$(LIBRARY) /usr/lib/$(MULTIARCH)/$(LIBRARY_SONAME)
	sudo ln -sf $(PWD)/$(LIBRARY) /usr/lib/$(MULTIARCH)/$(LIBRARY_LINK)
	sudo ln -sf $(PWD)/findmax.h /usr/include/findmax.h
	sudo ln -sf $(PWD)/findmax.1 /usr/share/man/man1/findmax.1
	sudo ln -sf $(PWD)/findmax-completion.bash /usr/share/bash-completion/completions/findmax
	ldconfig 2>/dev/null || true

# Uninstall symlinks from development directory
uninstall-symlinks:
	sudo rm -f /usr/bin/$(TARGET)
	sudo rm -f /usr/lib/$(MULTIARCH)/$(LIBRARY)
	sudo rm -f /usr/lib/$(MULTIARCH)/$(LIBRARY_SONAME)
	sudo rm -f /usr/lib/$(MULTIARCH)/$(LIBRARY_LINK)
	sudo rm -f /usr/include/findmax.h
	sudo rm -f /usr/share/man/man1/findmax.1
	sudo rm -f /usr/share/bash-completion/completions/findmax
	ldconfig 2>/dev/null || true

# Run unit tests
test: test_findmax
	@echo "Running unit tests..."
	./test_findmax

# Run integration tests
integration-test: $(TARGET)
	@echo "Running integration tests..."
	@mkdir -p test_dir/subdir
	@touch test_dir/file1.txt test_dir/file2.txt test_dir/subdir/file3.txt
	@echo "Test 1: Find latest file in current directory"
	./$(TARGET) -t test_dir/
	@echo "Test 2: Find latest file recursively"
	./$(TARGET) -t -R test_dir/
	@echo "Test 3: Find largest file"
	./$(TARGET) -S test_dir/
	@echo "Test 4: Custom format"
	./$(TARGET) -t -F "%n %y" test_dir/
	@echo "Test 5: Find top 3 files"
	./$(TARGET) -t -R -3 test_dir/
	@echo "Test 6: Max depth limiting"
	./$(TARGET) -t -R --maxdepth=1 test_dir/
	@echo "Test 7: Dereference test"
	./$(TARGET) -L -t test_dir/
	@rm -rf test_dir

# Run all tests
check: test integration-test

# Run benchmark
benchmark: $(TARGET)
	@echo "Running benchmark..."
	./benchmark.sh --csv --markdown --pdf \
	--output benchmark_report \
	/usr/share/man

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

.PHONY: all clean install uninstall install-symlinks uninstall-symlinks test integration-test check benchmark debug optimized
